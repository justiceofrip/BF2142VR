using System.Net;
using System.Net.Security;
using System.Net.Sockets;
using System.Security.Authentication;
using System.Security.Cryptography;
namespace BF2142.Community;

public sealed class BridgeClient : IDisposable {
 readonly ServerManifest server;readonly byte[] secret;readonly UdpClient pose,voice,remote=new(AddressFamily.InterNetwork);
 readonly object sync=new();IPEndPoint? localPose,localVoice;int player=-1;ulong nativeSession;long lastPose;
 Connection? connection;long clockOffset;IPAddress? address;long sent,received;
 sealed record Connection(ulong Id,int Player,ulong Native,DatagramCipher Up,DatagramCipher Down);
 public int PosePort=>((IPEndPoint)pose.Client.LocalEndPoint!).Port;
 public int VoicePort=>((IPEndPoint)voice.Client.LocalEndPoint!).Port;
 public BridgeClient(ServerManifest server,byte[] secret){this.server=server;this.secret=secret;pose=new(new IPEndPoint(IPAddress.Loopback,0));voice=new(new IPEndPoint(IPAddress.Loopback,0));remote.Client.Bind(new IPEndPoint(IPAddress.Any,0));}
 public async Task Run(CancellationToken ct){
  address=(await Dns.GetHostAddressesAsync(server.Host,ct)).FirstOrDefault(x=>x.AddressFamily==AddressFamily.InterNetwork)??throw new IOException("No IPv4 address for game host.");
  using var stop=CancellationTokenSource.CreateLinkedTokenSource(ct);
  var tasks=new[]{Local(Wire.Pose,stop.Token),Local(Wire.Voice,stop.Token),Remote(stop.Token),Connect(stop.Token)};
  try{await Task.WhenAny(tasks);stop.Cancel();await Task.WhenAll(tasks);}finally{stop.Cancel();Clear();}
 }
 void Clear(){lock(sync){var c=connection;connection=null;c?.Up.Dispose();c?.Down.Dispose();}}
 async Task Local(byte channel,CancellationToken ct){var socket=channel==Wire.Pose?pose:voice;
  while(!ct.IsCancellationRequested){var packet=await socket.ReceiveAsync(ct);var b=packet.Buffer;
   if(!IPAddress.IsLoopback(packet.RemoteEndPoint.Address)||!Wire.Native(channel,b,secret,true,out int p,out ulong session))continue;
   byte[]? encrypted=null;
   lock(sync){
    if(channel==Wire.Pose){if(localPose is not null&&!localPose.Equals(packet.RemoteEndPoint))continue;localPose=packet.RemoteEndPoint;player=p;nativeSession=session;lastPose=Wire.Now;}
    else{if(localVoice is not null&&!localVoice.Equals(packet.RemoteEndPoint))continue;localVoice=packet.RemoteEndPoint;}
    var c=connection;if(c is not null&&c.Player==p&&c.Native==session){Array.Clear(b,24,16);encrypted=c.Up.Seal(channel,b,Wire.Now+clockOffset);}
   }
   if(encrypted is not null&&address is not null){await remote.SendAsync(encrypted,new IPEndPoint(address,server.UdpPort),ct);Interlocked.Increment(ref sent);}
  }
 }
 async Task Remote(CancellationToken ct){while(!ct.IsCancellationRequested){var p=await remote.ReceiveAsync(ct);
  if(!p.RemoteEndPoint.Address.Equals(address)||p.RemoteEndPoint.Port!=server.UdpPort)continue;
  byte channel;byte[] b;IPEndPoint? target;
  lock(sync){var c=connection;if(c is null||!c.Down.Open(p.Buffer,Wire.Now+clockOffset,out channel,out b)||!Wire.Native(channel,b,BridgeServer.ZeroSecret,false,out _,out _))continue;
   target=channel==Wire.Pose?localPose:localVoice;secret.CopyTo(b,24);
  }
  if(target is not null){await(channel==Wire.Pose?pose:voice).SendAsync(b,target,ct);Interlocked.Increment(ref received);}
 }}
 async Task Connect(CancellationToken ct){int retry=0;
  while(!ct.IsCancellationRequested){int id;ulong session;IPEndPoint? target;
   lock(sync){id=player;session=nativeSession;target=localPose;if(Wire.Now-lastPose>2500)id=-1;}
   if(id<0||target is null){await Task.Delay(250,ct);continue;}
   try{await Authenticate(id,session,target,ct);retry=0;}
   catch(Exception e)when(e is InvalidDataException or IOException or SocketException or AuthenticationException or CryptographicException or TimeoutException or OperationCanceledException or FormatException){if(ct.IsCancellationRequested)break;if(retry==0)Console.WriteLine("Addon connection pending; reconnecting automatically while the game stays open.");}
   finally{Clear();}
   await Task.Delay(Math.Min(5000,500*(++retry)),ct);
  }
 }
 async Task Authenticate(int id,ulong session,IPEndPoint target,CancellationToken ct){
  using var client=new TcpClient(AddressFamily.InterNetwork){NoDelay=true};
  using var timeout=CancellationTokenSource.CreateLinkedTokenSource(ct);timeout.CancelAfter(TimeSpan.FromSeconds(25));
  await client.ConnectAsync(address!,server.ControlPort,timeout.Token);
  var expected=Convert.FromHexString(server.CertificateSha256);
  using var tls=new SslStream(client.GetStream(),false,(_,certificate,_,_)=>certificate is not null&&CryptographicOperations.FixedTimeEquals(SHA256.HashData(certificate.GetRawCertData()),expected));
  await tls.AuthenticateAsClientAsync(new SslClientAuthenticationOptions{TargetHost=server.Host,EnabledSslProtocols=SslProtocols.Tls12|SslProtocols.Tls13},timeout.Token);
  await Wire.Write(tls,new(){Kind="hello",Build=server.Build,Player=id,NativeSession=session},timeout.Token);
  var challenge=await Wire.Read(tls,timeout.Token);if(challenge.Kind!="challenge")throw new InvalidDataException("Expected native challenge.");
  byte[] nonce=Convert.FromHexString(challenge.Nonce);var proof=Wire.Challenge(id,session,secret,nonce);
  var ready=Wire.Read(tls,timeout.Token);
  while(!ready.IsCompleted){await pose.SendAsync(proof,target,timeout.Token);await Task.WhenAny(ready,Task.Delay(900,timeout.Token));timeout.Token.ThrowIfCancellationRequested();}
  var response=await ready;if(response.Kind!="ready"||response.Session==0||response.UdpPort!=server.UdpPort)throw new InvalidDataException("Invalid bridge session.");
  // Midpoint ping avoids trusting the latency of the native challenge round trip.
  await Ping(tls,timeout.Token);
  var c=new Connection(response.Session,id,session,new(Convert.FromBase64String(response.SendKey),response.Session,1),new(Convert.FromBase64String(response.ReceiveKey),response.Session,2));
  lock(sync){connection=c;}
  timeout.CancelAfter(Timeout.InfiniteTimeSpan);Console.WriteLine("Connected: native player verified; encrypted arms, aiming and voice ready.");
  while(!ct.IsCancellationRequested){await Task.Delay(2000,ct);
   lock(sync){if(player!=id||nativeSession!=session||Wire.Now-lastPose>5000)throw new IOException("Native game connection changed.");}
   using var ping=CancellationTokenSource.CreateLinkedTokenSource(ct);ping.CancelAfter(TimeSpan.FromSeconds(6));await Ping(tls,ping.Token);
  }
 }
 async Task Ping(SslStream tls,CancellationToken ct){long start=Wire.Now;await Wire.Write(tls,new(){Kind="ping",Time=start},ct);var p=await Wire.Read(tls,ct);long end=Wire.Now;
  if(p.Kind!="pong"||p.Echo!=start||end-start>4000)throw new InvalidDataException("Invalid bridge heartbeat.");lock(sync){clockOffset=p.Time-(start+(end-start)/2);}
 }
 public void Dispose(){Clear();pose.Dispose();voice.Dispose();remote.Dispose();}
}
