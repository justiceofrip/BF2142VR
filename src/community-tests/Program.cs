using BF2142.Community;
using System.Buffers.Binary;
using System.IO.Compression;
using System.Net;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Text.Json;
static class Tests {
 static int checks;
 static void Check(bool ok,string message){checks++;if(!ok)throw new Exception(message);}
 static void Throws(Action action,string message){bool threw=false;try{action();}catch(Exception e)when(e is InvalidDataException or IOException or CryptographicException or ArgumentException){threw=true;}Check(threw,message);}
 static byte[] Packet(byte channel,byte[] secret,int player=1,ulong session=42){byte[] p=new byte[channel==Wire.Pose?616:60];Wire.Put(p,0,channel==Wire.Pose?0x31524e42u:0x31564e42u);BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(4),channel==Wire.Pose?(ushort)4:(ushort)1);BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(6),channel==Wire.Pose?(ushort)4:(ushort)1);Wire.Put(p,channel==Wire.Pose?40:8,(uint)player);if(channel==Wire.Pose)Wire.Put(p,8,616u);Wire.Put(p,16,session);secret.CopyTo(p,24);return p;}
 static byte[] Roster(byte[] secret,uint epoch,uint generation){byte[] b=new byte[48];Wire.Put(b,0,0x31504642u);secret.CopyTo(b,4);b[36]=2;Wire.Put(b,37,epoch);b[41]=1;b[43]=1;Wire.Put(b,44,generation);return b;}
 static byte[] Proof(byte[] secret,uint epoch,uint generation,byte[] nonce,int player=1){byte[] b=new byte[62];Wire.Put(b,0,0x31504642u);secret.CopyTo(b,4);b[36]=1;Wire.Put(b,37,epoch);b[41]=(byte)player;Wire.Put(b,42,generation);nonce.CopyTo(b,46);return b;}
 static int Port(){using var s=new UdpClient(new IPEndPoint(IPAddress.Loopback,0));return ((IPEndPoint)s.Client.LocalEndPoint!).Port;}
 public static async Task<int> Main(){string temp=Path.Combine(Path.GetTempPath(),"bf2142-community-tests-"+Guid.NewGuid());Directory.CreateDirectory(temp);try{
  Crypto();Paths();LaunchPolicy(temp);await Packages(temp);Offline(temp);await NativeProof();await Bridge(temp);Console.WriteLine($"PASS: {checks} community checks (crypto, replay, ownership, downloads, archive isolation and live TLS/UDP bridge).");return 0;
 }catch(Exception e){Console.Error.WriteLine(e);return 1;}finally{if(Path.GetFileName(temp).StartsWith("bf2142-community-tests-")&&Path.GetDirectoryName(temp)==Path.GetTempPath().TrimEnd(Path.DirectorySeparatorChar))Directory.Delete(temp,true);}}
 static void Offline(string temp){
  string root=Path.Combine(temp,"offline");Directory.CreateDirectory(root);string files=Path.Combine(root,"input");Directory.CreateDirectory(files);File.WriteAllText(Path.Combine(files,"test.txt"),"flat payload");
  string zip=Path.Combine(root,"payload.zip");var package=Publisher.Pack(files,zip,"https://example.com/offline.zip","flat","test-1");
  using var key=ECDsa.Create(ECCurve.NamedCurves.nistP256);string pub=key.ExportSubjectPublicKeyInfoPem();
  var manifest=new ServerManifest{Id="offline",Name="Offline test",Revision=1,Host="127.0.0.1",Build="test",CertificateSha256=new string('0',64),Packages=[package]};
  string descriptor=Path.Combine(root,"signed.json");File.WriteAllBytes(descriptor,PackageStore.Sign(manifest,key.ExportPkcs8PrivateKeyPem()));
  string helper=Path.Combine(root,"helper.exe");File.WriteAllBytes(helper,new byte[128]);string exe=Path.Combine(root,"join.exe");OfflineBundle.Create(helper,zip,descriptor,exe,pub);
  var payload=OfflineBundle.Read(exe)!;Check(payload.Offset==128&&payload.Length==package.Size,"offline footer bounds");
  var installed=OfflineBundle.Install(exe,Path.Combine(root,"cache"),pub);Check(installed.Id=="offline","signed offline install");
  Check(File.ReadAllText(Path.Combine(root,"cache","packages",package.Sha256.ToLowerInvariant(),"test.txt"))=="flat payload","offline installed bytes");
  byte[] original=File.ReadAllBytes(exe),bad=original.ToArray();bad[(int)payload.Offset]^=1;File.WriteAllBytes(exe,bad);Throws(()=>OfflineBundle.Install(exe,Path.Combine(root,"bad-cache"),pub),"offline archive tampering");
  bad=original.ToArray();Array.Fill(bad,(byte)255,bad.Length-8,8);File.WriteAllBytes(exe,bad);Throws(()=>OfflineBundle.Read(exe),"offline bounds tampering");File.WriteAllBytes(exe,original);
  using var other=ECDsa.Create(ECCurve.NamedCurves.nistP256);Throws(()=>OfflineBundle.Install(exe,Path.Combine(root,"foreign"),other.ExportSubjectPublicKeyInfoPem()),"offline publisher mismatch");
 }
 static void Crypto(){var key=RandomNumberGenerator.GetBytes(32);using var sender=new DatagramCipher(key,42,1);using var receiver=new DatagramCipher(key,42,1);byte[] raw=[1,2,3];long now=Wire.Now;
  var first=sender.Seal(Wire.Pose,raw,now);Check(receiver.Open(first,now,out byte channel,out var plain)&&channel==Wire.Pose&&plain.SequenceEqual(raw),"round trip");Check(!receiver.Open(first,now,out _,out _),"replay rejected");
  var a=sender.Seal(Wire.Pose,raw,now);var b=sender.Seal(Wire.Voice,raw,now);Check(receiver.Open(b,now,out _,out _)&&receiver.Open(a,now,out _,out _),"out-of-order unique frames");
  var valid=sender.Seal(Wire.Pose,raw,now);var bad=valid.ToArray();bad[^1]^=1;Check(!receiver.Open(bad,now,out _,out _)&&receiver.Open(valid,now,out _,out _),"bad authentication cannot poison replay state");
  Check(!receiver.Open(sender.Seal(Wire.Pose,raw,now-501),now,out _,out _),"expired frame");Check(!receiver.Open(sender.Seal(Wire.Pose,raw,now+251),now,out _,out _),"future frame");using var reverse=new DatagramCipher(key,42,2);Check(!reverse.Open(sender.Seal(Wire.Pose,raw,now),now,out _,out _),"direction-separated nonce");using var stranger=new DatagramCipher(key,43,1);Check(!stranger.Open(sender.Seal(Wire.Pose,raw,now),now,out _,out _),"different session");
  for(int i=0;i<64;i++){var frame=sender.Seal(Wire.Pose,raw,now);frame[i%frame.Length]^=0x80;Check(!receiver.Open(frame,now,out _,out _),"tampered envelope");}
  var window=new ReplayWindow();Check(window.Accept(10)&&window.Accept(9)&&!window.Accept(9)&&window.Accept(100)&&!window.Accept(10)&&!window.Accept(0),"bounded replay window");
  var local=RandomNumberGenerator.GetBytes(16);foreach(byte kind in new[]{Wire.Pose,Wire.Voice}){var p=Packet(kind,local);Check(Wire.Native(kind,p,local,true,out int player,out ulong session)&&player==1&&session==42,"native identity extraction");p[24]^=1;Check(!Wire.Native(kind,p,local,true,out _,out _),"local token mismatch");for(int length=0;length<60;length++)Check(!Wire.Native(kind,p.AsSpan(0,length),local,true,out _,out _),"truncated local header");}
 }
 static void Paths(){foreach(string path in new[]{"../evil.dll","/evil.dll","C:/evil.dll","x\\y","x:y","dir/../x","a//b","a/CON.dll","Lpt9.txt","com¹.txt","a.","b ","x\u0000x","a/../../other"})Throws(()=>Validation.SafePath(path),"unsafe archive path "+path);Validation.SafePath("runtime/x86/BF2142VRClient.dll");Check(true,"normal package path");
  var link=Join.ParseLink("bf2142vr://join?server=https%3A%2F%2Fexample.org%2Fserver.json&mode=flat");Check(link.Url=="https://example.org/server.json"&&link.Mode=="flat","join link");foreach(var url in new[]{"bf2142vr://join?server=file:///C:/private","bf2142vr://join?server=https://example.org&game=C:/x","bf2142vr://join?server=https://example.org&mode=x","bf2142vr://join?server=https://example.org&server=https://bad.org","bf2142vr://join?server=http://example.org"})Throws(()=>Join.ParseLink(url),"join link cannot supply paths/commands");
 }
 static void LaunchPolicy(string temp){
  var server=new ServerManifest{Host="play.example.org",GamePort=17567};
  string game=Path.Combine(temp,"Game With Spaces"),payload=Path.Combine(temp,"Payload With Spaces"),profile=Path.Combine(temp,"Observer Documents"),settings=Path.Combine(temp,"observer.ini"),network=Path.Combine(temp,"observer-network.ini");
  var info=Join.CreateLaunchInfo(server,payload,game,"flat",settings,profile,false,network);
  Check(!info.UseShellExecute&&info.WorkingDirectory==game&&info.FileName==Path.Combine(payload,"runtime","x86","BF2142VRLauncher.exe"),"observer launches the chosen native runtime without a shell");
  Check(info.ArgumentList.SequenceEqual(new[]{"--game-dir",game,"--mod","bf2142","--windowed","--join-server","play.example.org","--port","17567","--observer-profile",profile,"--flat"}),"cloud observer keeps paths intact, direct join, flat mode and explicit isolation");
  Check(info.Environment["BF2142VR_NETWORK"]==network&&info.Environment["BF2142VR_CONFIG"]==settings,"observer uses its own networking and settings");
  Check(info.Environment["BF2142VR_VOICE_RECEIVE_ONLY"]=="1"&&info.Environment["BFVR_DIAGNOSTICS"]=="off","observer cannot capture a second microphone or enable expensive diagnostics");
  var primary=Join.CreateLaunchInfo(server,payload,game,"vr","primary.ini",null,false,"primary-network.ini");
  Check(!primary.ArgumentList.Contains("--observer-profile")&&!primary.ArgumentList.Contains("--flat")&&primary.ArgumentList.Contains("--presenter"),"primary VR retains its duplicate guard and presenter");
  Check(primary.Environment["BF2142VR_NETWORK"]=="primary-network.ini"&&primary.Environment["BF2142VR_CONFIG"]=="primary.ini"&&primary.Environment["BF2142VR_VOICE_RECEIVE_ONLY"]=="0","observer configuration never leaks into a primary launch");
  var loopback=Join.CreateLaunchInfo(server with{Host="127.0.0.1"},payload,game,"flat",settings,profile,false,network);
  Check(loopback.ArgumentList.Contains("127.0.0.1")&&loopback.ArgumentList.Contains("--observer-profile"),"local observer remains supported");
  var flat=Join.CreateLaunchInfo(server,payload,game,"flat",settings,null,false,network);
  Check(flat.ArgumentList.Contains("--flat")&&!flat.ArgumentList.Contains("--observer-profile")&&!flat.ArgumentList.Contains("--presenter"),"ordinary flat addon does not enable a second instance");
  var desktop=Join.CreateLaunchInfo(server,payload,game,"vr",settings,null,true,network);
  Check(desktop.ArgumentList.Contains("--desktop-vr")&&!desktop.ArgumentList.Contains("--presenter"),"primary desktop simulation retained");
  Throws(()=>Join.CreateLaunchInfo(server,payload,game,"vr",settings,profile,false,network),"isolated profile cannot start VR");
  Throws(()=>Join.CreateLaunchInfo(server,payload,game,"flat",settings,profile,true,network),"isolated profile cannot simulate VR");
  Throws(()=>Join.CreateLaunchInfo(server,payload,game,"bogus",settings,null,false,network),"unknown play mode fails before launch");
  foreach(var invalid in new[]{"", "relative-profile", profile+"\ninjected"})Throws(()=>Join.CreateLaunchInfo(server,payload,game,"flat",settings,invalid,false,network),"invalid observer path rejected");
  foreach(var invalid in new[]{server with{Host=""},server with{Host="play.example.org +foo"},server with{GamePort=0},server with{GamePort=65536},server with{Mod="../escape"}})Throws(()=>Join.CreateLaunchInfo(invalid,payload,game,"flat",settings,profile,false,network),"invalid destination or mod rejected before launch");
  using var first=new BridgeClient(server,RandomNumberGenerator.GetBytes(16));using var second=new BridgeClient(server,RandomNumberGenerator.GetBytes(16));
  Check(new[]{first.PosePort,first.VoicePort,second.PosePort,second.VoicePort}.Distinct().Count()==4,"simultaneous primary and observer bridges own distinct local ports");
 }
 static async Task Packages(string temp){string source=Path.Combine(temp,"payload");Directory.CreateDirectory(Path.Combine(source,"runtime","x86"));File.WriteAllText(Path.Combine(source,"runtime","x86","test.txt"),"signed test fixture");string archive=Path.Combine(temp,"package.zip");var p=Publisher.Pack(source,archive,"https://example.org/package.zip","flat","test-1");var m=new ServerManifest{Id="test",Name="Fixture",Revision=1,Host="127.0.0.1",Build="test",CertificateSha256=new('A',64),Packages=[p]};using var key=ECDsa.Create(ECCurve.NamedCurves.nistP256);var signed=PackageStore.Sign(m,key.ExportPkcs8PrivateKeyPem());Check(PackageStore.Verify(signed,key.ExportSubjectPublicKeyInfoPem()).Id=="test","signed descriptor");using var other=ECDsa.Create(ECCurve.NamedCurves.nistP256);Throws(()=>PackageStore.Verify(signed,other.ExportSubjectPublicKeyInfoPem()),"untrusted publisher");
  var embedded=m with{DescriptorUrl="https://example.org/server.json"};
  async Task<bool> RefreshWorks(ServerManifest update){var h=new FixtureHandler(PackageStore.Sign(update,key.ExportPkcs8PrivateKeyPem()));var updates=new PackageStore(Path.Combine(temp,"updates"),new HttpClient(h));try{var latest=await updates.Refresh(embedded,key.ExportSubjectPublicKeyInfoPem(),CancellationToken.None);return latest.Revision==update.Revision;}catch(InvalidDataException){return false;}}
  Check(await RefreshWorks(m with{Revision=2}),"embedded executable discovers signed server update");
  Check(!await RefreshWorks(m with{Id="different",Revision=2}),"embedded update cannot substitute another server");
  var future=embedded with{Revision=3};var oldUpdates=new PackageStore(Path.Combine(temp,"old-updates"),new HttpClient(new FixtureHandler(signed)));bool oldRejected=false;try{await oldUpdates.Refresh(future,key.ExportSubjectPublicKeyInfoPem(),CancellationToken.None);}catch(InvalidDataException){oldRejected=true;}Check(oldRejected,"embedded update cannot predate executable");
  Throws(()=>Validation.Manifest(m with{DescriptorUrl="http://example.org/server.json"}),"update URL requires HTTPS");
  var store=new PackageStore(Path.Combine(temp,"cache"));string installed=store.InstallArchive(archive,p);Check(File.ReadAllText(Path.Combine(installed,"runtime","x86","test.txt"))=="signed test fixture","extract exact signed files");Check(store.InstallArchive(archive,p)==installed,"repeat installation");store.AcceptRevision(m with{Revision=2});Throws(()=>store.CheckRevision(m),"manifest rollback rejected");
  File.WriteAllText(Path.Combine(installed,"runtime","x86","test.txt"),"modified");Throws(()=>PackageStore.VerifyFiles(installed,p),"modified cache rejected");
  string evil=Path.Combine(temp,"evil.zip");using(var zip=ZipFile.Open(evil,ZipArchiveMode.Create)){using var writer=new StreamWriter(zip.CreateEntry("../outside.txt").Open());writer.Write("escape");}var hostile=p with{Size=new FileInfo(evil).Length,Sha256=Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(evil)))};Throws(()=>store.InstallArchive(evil,hostile),"signed archive may not contain extra or unsafe names");Check(!File.Exists(Path.Combine(temp,"outside.txt")),"archive stays contained");
  var handler=new FixtureHandler(File.ReadAllBytes(archive));var networkStore=new PackageStore(Path.Combine(temp,"network-cache"),new HttpClient(handler));string downloaded=await networkStore.Acquire(p,CancellationToken.None);Check(File.Exists(Path.Combine(downloaded,p.Files[0].Path)),"download, verify and commit");handler.Redirect=true;bool rejected=false;try{await networkStore.DownloadManifest("https://example.org/redirect",CancellationToken.None);}catch(InvalidDataException){rejected=true;}Check(rejected,"HTTPS downgrade blocked");
 }
 sealed class FixtureHandler(byte[] archive):HttpMessageHandler {public bool Redirect;protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request,CancellationToken ct){var r=new HttpResponseMessage(Redirect?HttpStatusCode.Redirect:HttpStatusCode.OK){Content=new ByteArrayContent(archive)};if(Redirect)r.Headers.Location=new Uri("http://example.org/insecure");return Task.FromResult(r);}}
 static async Task NativeProof(){byte[] secret=RandomNumberGenerator.GetBytes(32),nonce=RandomNumberGenerator.GetBytes(16);using var hub=new ProofHub(0,secret,1);Check(hub.Accept(Roster(secret,1,5)),"authoritative roster");using var cts=new CancellationTokenSource(3000);var result=hub.Prove(1,nonce,cts.Token);Check(!hub.Accept(Proof(secret,1,5,nonce,2)),"other native issuer cannot claim a player");Check(!hub.Accept(Proof(secret,1,4,nonce)),"old slot generation");Check(!hub.Accept(Proof(secret,2,5,nonce)),"previous server epoch");Check(hub.Accept(Proof(secret,1,5,nonce))&&await result==5,"native challenge authorized");Check(!hub.Accept(Proof(secret,1,5,nonce)),"challenge one-use");hub.Accept(Roster(secret,1,6));Check(!hub.Current(1,5)&&hub.Current(1,6),"slot reuse revokes old owner");}
 static async Task Bridge(string temp){
  using var certKey=RSA.Create(2048);var request=new CertificateRequest("CN=localhost",certKey,HashAlgorithmName.SHA256,RSASignaturePadding.Pkcs1);using var cert=request.CreateSelfSigned(DateTimeOffset.UtcNow.AddMinutes(-1),DateTimeOffset.UtcNow.AddDays(1));string pfx=Path.Combine(temp,"host.pfx");File.WriteAllBytes(pfx,cert.Export(X509ContentType.Pfx));
  byte[] native=RandomNumberGenerator.GetBytes(16),proofKey=RandomNumberGenerator.GetBytes(32),local=RandomNumberGenerator.GetBytes(16);int tcp=Port(),udp=Port(),proofPort=Port();using var nativePose=new UdpClient(new IPEndPoint(IPAddress.Loopback,0));using var nativeVoice=new UdpClient(new IPEndPoint(IPAddress.Loopback,0));
  var config=new HostConfig{ControlPort=tcp,UdpPort=udp,ProofPort=proofPort,NativePosePort=((IPEndPoint)nativePose.Client.LocalEndPoint!).Port,NativeVoicePort=((IPEndPoint)nativeVoice.Client.LocalEndPoint!).Port,NativeSecret=Convert.ToHexString(native),ProofSecret=Convert.ToHexString(proofKey),ProofEpoch=1,Build="test",Certificate=pfx};
  var m=new ServerManifest{Host="127.0.0.1",ControlPort=tcp,UdpPort=udp,Build="test",CertificateSha256=Convert.ToHexString(SHA256.HashData(cert.RawData))};
  using var stop=new CancellationTokenSource(TimeSpan.FromSeconds(12));using var host=new BridgeServer(config);var hostTask=host.Run(stop.Token);using var bridge=new BridgeClient(m,local);var bridgeTask=bridge.Run(stop.Token);using var game=new UdpClient(new IPEndPoint(IPAddress.Loopback,0));using var voiceGame=new UdpClient(new IPEndPoint(IPAddress.Loopback,0));using var proofSocket=new UdpClient();int relays=0,voices=0;
  async Task RosterLoop(){while(!stop.IsCancellationRequested){await proofSocket.SendAsync(Roster(proofKey,1,1),new IPEndPoint(IPAddress.Loopback,proofPort),stop.Token);await Task.Delay(200,stop.Token);}}
  async Task NativeLoop(UdpClient socket,byte channel){while(!stop.IsCancellationRequested){var p=await socket.ReceiveAsync(stop.Token);Check(Wire.Native(channel,p.Buffer,native,true,out int player,out ulong session)&&player==1&&session==42,"authenticated bridge -> native");BinaryPrimitives.WriteUInt16LittleEndian(p.Buffer.AsSpan(6),channel==Wire.Pose?(ushort)2:(ushort)3);Wire.Put(p.Buffer,channel==Wire.Pose?40:8,2u);await socket.SendAsync(p.Buffer,p.RemoteEndPoint,stop.Token);}}
  async Task GameLoop(){while(!stop.IsCancellationRequested){var p=await game.ReceiveAsync(stop.Token);if(p.Buffer.Length==56){await proofSocket.SendAsync(Proof(proofKey,1,1,p.Buffer.AsSpan(40,16).ToArray()),new IPEndPoint(IPAddress.Loopback,proofPort),stop.Token);}else{Check(Wire.Native(Wire.Pose,p.Buffer,local,false,out int player,out _)&&player==2,"encrypted pose return");Interlocked.Increment(ref relays);}}}
  async Task VoiceLoop(){while(!stop.IsCancellationRequested){var p=await voiceGame.ReceiveAsync(stop.Token);Check(Wire.Native(Wire.Voice,p.Buffer,local,false,out int player,out _)&&player==2,"encrypted voice return");Interlocked.Increment(ref voices);}}
  var tasks=new[]{RosterLoop(),NativeLoop(nativePose,Wire.Pose),NativeLoop(nativeVoice,Wire.Voice),GameLoop(),VoiceLoop()};
  try{for(int i=0;i<80&&(relays<3||voices<3);i++){await game.SendAsync(Packet(Wire.Pose,local),new IPEndPoint(IPAddress.Loopback,bridge.PosePort),stop.Token);await voiceGame.SendAsync(Packet(Wire.Voice,local),new IPEndPoint(IPAddress.Loopback,bridge.VoicePort),stop.Token);await Task.Delay(100,stop.Token);}Check(relays>=3&&voices>=3,"live TLS-authenticated pose and voice exchange");}
  finally{stop.Cancel();try{await Task.WhenAll(tasks.Append(hostTask).Append(bridgeTask));}catch(OperationCanceledException){}}
 }
}
