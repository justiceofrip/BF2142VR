using System.Collections.Concurrent;
using System.Net;
using System.Net.Security;
using System.Net.Sockets;
using System.Security.Authentication;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
namespace BF2142.Community;

public sealed class BridgeServer : IDisposable {
 readonly HostConfig config;readonly byte[] nativeSecret;readonly X509Certificate2 certificate;
 readonly TcpListener listener;readonly UdpClient udp;readonly ProofHub proof;
 readonly ConcurrentDictionary<ulong,Peer> peers=new();readonly ConcurrentDictionary<int,Peer> players=new();
 readonly SemaphoreSlim capacity;readonly object playerLock=new();readonly ConcurrentDictionary<int,Task> workers=new();int workerId;
 long accepted,received,forwarded,rejected;
 public BridgeServer(HostConfig config){
  this.config=config;Validation.Host(config);nativeSecret=Convert.FromHexString(config.NativeSecret);
  certificate=X509CertificateLoader.LoadPkcs12FromFile(config.Certificate,config.CertificatePassword);
  if(!certificate.HasPrivateKey)throw new InvalidDataException("Host certificate has no private key.");
  var bind=IPAddress.Parse(config.Bind);listener=new(bind,config.ControlPort);udp=new(new IPEndPoint(bind,config.UdpPort));
  proof=new(config.ProofPort,Convert.FromHexString(config.ProofSecret),config.ProofEpoch);capacity=new(config.MaxClients+8);
 }
 public async Task Run(CancellationToken ct){
  listener.Start(64);Console.WriteLine($"Community bridge: TLS {config.ControlPort}, encrypted UDP {config.UdpPort}; native services remain loopback.");
  using var stop=CancellationTokenSource.CreateLinkedTokenSource(ct);
  var tasks=new[]{Accept(stop.Token),Receive(stop.Token),proof.Run(stop.Token),Monitor(stop.Token)};
  try{await Task.WhenAny(tasks);stop.Cancel();await Task.WhenAll(tasks);}finally{stop.Cancel();listener.Stop();foreach(var p in peers.Values)p.Stop();await Task.WhenAll(workers.Values.ToArray());}
 }
 async Task Accept(CancellationToken ct){while(!ct.IsCancellationRequested){
  var client=await listener.AcceptTcpClientAsync(ct);
  if(!capacity.Wait(0)){client.Dispose();continue;}
  int id=Interlocked.Increment(ref workerId);var task=Handle(client,ct);workers[id]=task;
  _=task.ContinueWith(_=>{workers.TryRemove(id,out var ignored);capacity.Release();},CancellationToken.None,TaskContinuationOptions.ExecuteSynchronously,TaskScheduler.Default);
 }}
 async Task Handle(TcpClient client,CancellationToken ct){
  Peer? peer=null;
  try{
   using(client)using(var tls=new SslStream(client.GetStream()))using(var timeout=CancellationTokenSource.CreateLinkedTokenSource(ct)){
    timeout.CancelAfter(TimeSpan.FromSeconds(25));client.NoDelay=true;
    await tls.AuthenticateAsServerAsync(new SslServerAuthenticationOptions{ServerCertificate=certificate,EnabledSslProtocols=SslProtocols.Tls12|SslProtocols.Tls13},timeout.Token);
    var hello=await Wire.Read(tls,timeout.Token);
    if(hello.Kind!="hello"||hello.Build!=config.Build||hello.Player is <0 or >255||hello.NativeSession==0)throw new InvalidDataException("Client version or native identity is invalid.");
    byte[] nonce=RandomNumberGenerator.GetBytes(16);nonce[15]|=1; // native command trims trailing zero words
    var result=proof.Prove(hello.Player,nonce,timeout.Token);
    await Wire.Write(tls,new(){Kind="challenge",Nonce=Convert.ToHexString(nonce),Time=Wire.Now},timeout.Token);
    uint generation=await result;
    if(!proof.Current(hello.Player,generation))throw new InvalidDataException("Native player disconnected during authentication.");
    ulong id;do{id=Wire.U64(RandomNumberGenerator.GetBytes(8),0);}while(id==0||peers.ContainsKey(id));
    byte[] up=RandomNumberGenerator.GetBytes(32),down=RandomNumberGenerator.GetBytes(32);
    peer=new(hello.Player,generation,hello.NativeSession,id,((IPEndPoint)client.Client.RemoteEndPoint!).Address,up,down,ct);
    lock(playerLock){if(players.TryGetValue(peer.Player,out var old))old.Stop();else if(players.Count>=config.MaxClients)throw new IOException("Server addon capacity reached.");players[peer.Player]=peer;peers[id]=peer;}
    await Wire.Write(tls,new(){Kind="ready",Session=id,SendKey=Convert.ToBase64String(up),ReceiveKey=Convert.ToBase64String(down),Time=Wire.Now,UdpPort=config.UdpPort},timeout.Token);
    CryptographicOperations.ZeroMemory(up);CryptographicOperations.ZeroMemory(down);
    timeout.CancelAfter(Timeout.InfiniteTimeSpan);Interlocked.Increment(ref accepted);
    Console.WriteLine($"Player {peer.Player}: native connection verified; encrypted addon connected.");
    var control=ControlLoop(tls,peer);var poses=NativeReplies(peer,Wire.Pose);var voice=NativeReplies(peer,Wire.Voice);
    try{await Task.WhenAny(control,poses,voice);peer.Stop();await Task.WhenAll(control,poses,voice);}catch(OperationCanceledException){}
   }
  }catch(Exception e)when(e is InvalidDataException or IOException or SocketException or AuthenticationException or OperationCanceledException or TimeoutException or CryptographicException or FormatException){Interlocked.Increment(ref rejected);}
  finally{if(peer is not null){peer.Stop();peers.TryRemove(peer.Id,out _);lock(playerLock){if(players.TryGetValue(peer.Player,out var current)&&ReferenceEquals(peer,current))players.TryRemove(peer.Player,out _);}peer.Dispose();}client.Dispose();}
 }
 async Task ControlLoop(SslStream tls,Peer peer){while(!peer.Token.IsCancellationRequested){
  using var wait=CancellationTokenSource.CreateLinkedTokenSource(peer.Token);wait.CancelAfter(TimeSpan.FromSeconds(8));
  var p=await Wire.Read(tls,wait.Token);
  if(p.Kind!="ping"||!proof.Current(peer.Player,peer.Generation))throw new InvalidDataException("Expired native connection.");
  await Wire.Write(tls,new(){Kind="pong",Echo=p.Time,Time=Wire.Now},peer.Token);
 }}
 async Task Receive(CancellationToken ct){while(!ct.IsCancellationRequested){
  var p=await udp.ReceiveAsync(ct);var b=p.Buffer;if(b.Length<DatagramCipher.Header)continue;
  if(!peers.TryGetValue(Wire.U64(b,8),out var peer)||!peer.Address.Equals(p.RemoteEndPoint.Address)||peer.Token.IsCancellationRequested||!proof.Current(peer.Player,peer.Generation)){Interlocked.Increment(ref rejected);continue;}
  if(!peer.Decode(b,out byte channel,out var raw)||!Wire.Native(channel,raw,ZeroSecret,true,out int player,out ulong session)||player!=peer.Player||session!=peer.NativeSession){Interlocked.Increment(ref rejected);continue;}
  peer.Endpoint=p.RemoteEndPoint;nativeSecret.CopyTo(raw,24);Interlocked.Increment(ref received);
  try{await peer.Local(channel).SendAsync(raw,new IPEndPoint(IPAddress.Loopback,channel==Wire.Pose?config.NativePosePort:config.NativeVoicePort),ct);}
  catch(Exception e)when(e is ObjectDisposedException or SocketException){Interlocked.Increment(ref rejected);peer.Stop();} // One departing peer must not stop the public relay.
 }}
 async Task NativeReplies(Peer peer,byte channel){
  var port=channel==Wire.Pose?config.NativePosePort:config.NativeVoicePort;
  while(!peer.Token.IsCancellationRequested){var p=await peer.Local(channel).ReceiveAsync(peer.Token);
   if(!IPAddress.IsLoopback(p.RemoteEndPoint.Address)||p.RemoteEndPoint.Port!=port||peer.Endpoint is not{} endpoint||!proof.Current(peer.Player,peer.Generation)||!Wire.Native(channel,p.Buffer,nativeSecret,false,out _,out _))continue;
   Array.Clear(p.Buffer,24,16);var b=peer.Down.Seal(channel,p.Buffer,Wire.Now);await udp.SendAsync(b,endpoint,peer.Token);Interlocked.Increment(ref forwarded);
  }
 }
 async Task Monitor(CancellationToken ct){using var timer=new PeriodicTimer(TimeSpan.FromSeconds(5));while(await timer.WaitForNextTickAsync(ct)){
  foreach(var p in peers.Values)if(!proof.Current(p.Player,p.Generation))p.Stop();
  Console.WriteLine($"Bridge: {peers.Count} connected; authenticated {Interlocked.Read(ref accepted)}, received {Interlocked.Read(ref received)}, relayed {Interlocked.Read(ref forwarded)}, rejected {Interlocked.Read(ref rejected)}.");
 }}
 public static readonly byte[] ZeroSecret=new byte[16];
 public void Dispose(){listener.Stop();udp.Dispose();proof.Dispose();certificate.Dispose();capacity.Dispose();}
 sealed class Peer : IDisposable {
  public readonly int Player;public readonly uint Generation;public readonly ulong NativeSession,Id;public readonly IPAddress Address;
  public readonly DatagramCipher Up,Down;public readonly TokenBucket Budget=new(130,260);public IPEndPoint? Endpoint;
  readonly UdpClient pose=new(new IPEndPoint(IPAddress.Loopback,0)),voice=new(new IPEndPoint(IPAddress.Loopback,0));readonly CancellationTokenSource stop;readonly object cipherLock=new();bool disposed;
  public Peer(int player,uint generation,ulong native,ulong id,IPAddress address,byte[] up,byte[] down,CancellationToken ct){Player=player;Generation=generation;NativeSession=native;Id=id;Address=address;Up=new(up,id,1);Down=new(down,id,2);stop=CancellationTokenSource.CreateLinkedTokenSource(ct);}
  public bool Decode(byte[] b,out byte channel,out byte[] raw){lock(cipherLock){channel=0;raw=[];return !disposed&&!stop.IsCancellationRequested&&Budget.Take()&&Up.Open(b,Wire.Now,out channel,out raw);}}
  public CancellationToken Token=>stop.Token;public UdpClient Local(byte channel)=>channel==Wire.Pose?pose:voice;public void Stop()=>stop.Cancel();
  public void Dispose(){lock(cipherLock){disposed=true;stop.Cancel();pose.Dispose();voice.Dispose();Up.Dispose();Down.Dispose();}/* Keep cancellation source readable for in-flight routing lookups. */}
 }
}
