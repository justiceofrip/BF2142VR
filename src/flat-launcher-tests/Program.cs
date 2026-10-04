using BF2142.Community;
using BF2142.FlatViewer;
using System.Net;
using System.Security.Cryptography;
using System.Text;

static class Tests {
 static int checks;
 static void Check(bool ok,string text){checks++;if(!ok)throw new Exception(text);}
 static async Task Throws(Func<Task> action,string text){try{await action();}catch(Exception e)when(e is InvalidDataException or IOException or CryptographicException or HttpRequestException or OperationCanceledException){Check(true,text);return;}throw new Exception(text);}
 [STAThread] static int Main(string[] args){try{if(args.Length==2&&args[0]=="--render"){
 Application.EnableVisualStyles();using var f=new FlatForm(false,false);f.Opacity=0;f.ShowInTaskbar=false;f.Show();Application.DoEvents();f.PerformLayout();using var bitmap=new System.Drawing.Bitmap(f.Width,f.Height);f.DrawToBitmap(bitmap,new System.Drawing.Rectangle(0,0,f.Width,f.Height));bitmap.Save(args[1]);return 0;
 }Run(args).GetAwaiter().GetResult();Console.WriteLine($"PASS: {checks} flat-viewer checks.");return 0;}catch(Exception e){Console.Error.WriteLine(e);return 1;}}
 static async Task Run(string[] args){
  string root=Path.Combine(Path.GetTempPath(),"bf2142-flat-tests-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
  // Keep fixtures isolated and retain them on failure for inspection.
  Console.WriteLine("Fixtures: "+root);
  string game=Path.Combine(root,"Game With Spaces & Symbols");Directory.CreateDirectory(game);
  Check(!FlatPolicy.IsGame(game),"incomplete installation rejected");File.WriteAllText(Path.Combine(game,"BF2142.exe"),"fixture");File.WriteAllText(Path.Combine(game,"RendDX9.dll"),"fixture");Check(FlatPolicy.IsGame(game),"game recognized");
  var launch=FlatPolicy.StartInfo(root,"manifest with spaces.json",game);Check(launch.ArgumentList.SequenceEqual(new[]{"join-manifest","--manifest","manifest with spaces.json","--mode","flat","--game",game}),"structured flat-only arguments");Check(!launch.UseShellExecute&&launch.CreateNoWindow&&launch.RedirectStandardError,"background helper reports errors");
  string prefs=Path.Combine(root,"flat.ini");Check(!FlatPolicy.ReadMicrophone(prefs),"new microphone defaults off");FlatPolicy.SaveMicrophone(prefs,true);Check(FlatPolicy.ReadMicrophone(prefs),"microphone enabled");FlatPolicy.SaveMicrophone(prefs,false);Check(!FlatPolicy.ReadMicrophone(prefs),"microphone muted");
  File.WriteAllText(prefs,"[VR]\r\nCustom=7\r\n[Else]\r\nKeep=9\r\n");FlatPolicy.SaveMicrophone(prefs,true);Check(File.ReadAllText(prefs).Contains("Custom=7\r\nProximityMicMuted=0\r\n[Else]\r\nKeep=9"),"settings preserved and VR key inserted into correct section");
  var redacted=FlatPolicy.Redact("C:\\Users\\Example\\Games\\BF2142.exe\nsecret=abcd\nuser@example.com\nBearer abcdef\n18.190.153.177");Check(!new[]{"Example","abcd","user@example","abcdef","18.190"}.Any(redacted.Contains),"report redacts local paths and credentials");
  using var key=ECDsa.Create(ECCurve.NamedCurves.nistP256);var input=Path.Combine(root,"input");Directory.CreateDirectory(input);
  foreach(var name in new[]{FlatPolicy.Viewer,FlatPolicy.Helper,"runtime/x86/BF2142VRLauncher.exe","runtime/x86/BF2142VRClient.dll"}){var file=Path.Combine(input,name);Directory.CreateDirectory(Path.GetDirectoryName(file)!);File.WriteAllText(file,"fixture: "+name);}
  var zip=Path.Combine(root,"payload.zip");var package=Publisher.Pack(input,zip,"https://example.org/flat.zip","flat","test-1");
  var manifest=new ServerManifest{Id=FlatPolicy.ServerId,Name="Test",DescriptorUrl=FlatPolicy.Feed,Host="127.0.0.1",CertificateSha256=new string('0',64),Build="v35-community",Revision=20,Packages=[package]};
  var handler=new FakeHttp{Manifest=PackageStore.Sign(manifest,key.ExportPkcs8PrivateKeyPem()),Archive=File.ReadAllBytes(zip)};using var http=new HttpClient(handler);
  using var service=new FlatService(http,Path.Combine(root,"settings"),Path.Combine(root,"cache"),key.ExportSubjectPublicKeyInfoPem());
  var first=await service.Prepare(false,_=>{},CancellationToken.None);Check(!first.Offline&&handler.Downloads==1,"cold install downloads once");PackageStore.VerifyFiles(first.Payload,package);Check(true,"cold install exact hashes");
  await service.Prepare(false,_=>{},CancellationToken.None);Check(handler.Downloads==1,"repeat run reuses verified cache");
  string original=File.ReadAllText(Path.Combine(first.Payload,FlatPolicy.Helper));File.WriteAllText(Path.Combine(first.Payload,FlatPolicy.Helper),"damaged");await Throws(async()=>{await service.Prepare(false,_=>{},CancellationToken.None);},"damaged cache refused");
  var repaired=await service.Prepare(true,_=>{},CancellationToken.None);Check(handler.Downloads==2&&File.ReadAllText(Path.Combine(repaired.Payload,FlatPolicy.Helper))==original,"repair restores signed bytes");
  Check(Directory.EnumerateDirectories(Path.Combine(root,"cache","packages"),"*.previous-*").Any(),"repair retains previous cache");
  handler.Unreachable=true;var offline=await service.Prepare(false,_=>{},CancellationToken.None);Check(offline.Offline,"network failure uses verified installed copy");handler.Unreachable=false;
  handler.Manifest=PackageStore.Sign(manifest with{Revision=19},key.ExportPkcs8PrivateKeyPem());await Throws(async()=>{await service.Prepare(false,_=>{},CancellationToken.None);},"rollback refused");
  handler.Manifest=PackageStore.Sign(manifest with{Id="other-server"},key.ExportPkcs8PrivateKeyPem());await Throws(async()=>{await service.Prepare(false,_=>{},CancellationToken.None);},"wrong update channel refused");
  handler.Manifest=PackageStore.Sign(manifest with{Packages=[package with{Mode="vr"}]},key.ExportPkcs8PrivateKeyPem());await Throws(async()=>{await service.Prepare(false,_=>{},CancellationToken.None);},"VR-only package refused");
  using(var stranger=ECDsa.Create(ECCurve.NamedCurves.nistP256)){handler.Manifest=PackageStore.Sign(manifest,stranger.ExportPkcs8PrivateKeyPem());await Throws(async()=>{await service.Prepare(false,_=>{},CancellationToken.None);},"invalid signature cannot fall back");}
  handler.Manifest=PackageStore.Sign(manifest with{Revision=21},key.ExportPkcs8PrivateKeyPem());await service.Prepare(false,_=>{},CancellationToken.None);Check(true,"new signed revision accepted");
  using(var lease=new FileStream(Path.Combine(root,"cache","joining.lock"),FileMode.Open,FileAccess.ReadWrite,FileShare.None))await Throws(async()=>{await service.Prepare(false,_=>{},CancellationToken.None);},"active game/update cache lock respected");
  using(var cancel=new CancellationTokenSource()){cancel.Cancel();await Throws(async()=>{await service.Prepare(false,_=>{},cancel.Token);},"cancel does not install a partial update");}
  if(args.Length==3&&args[0]=="--package"){
   var exact=PackageStore.Verify(File.ReadAllBytes(args[1]),Join.PublicKey);var p=FlatPolicy.Check(exact);var store=new PackageStore(Path.Combine(root,"release-check"));var path=store.InstallArchive(args[2],p);PackageStore.VerifyFiles(path,p);Check(true,"exact release installs and verifies");
   await store.Acquire(p,CancellationToken.None);Check(true,"exact release repeat cache validates");
   Console.WriteLine("Verified release: "+p.Version+" / "+p.Files.Length+" files / "+path);
  }
 }
 sealed class FakeHttp:HttpMessageHandler {
  public byte[] Manifest=[],Archive=[];public bool Unreachable;public int Downloads;
  protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request,CancellationToken ct){ct.ThrowIfCancellationRequested();if(Unreachable)throw new HttpRequestException("offline");bool manifest=request.RequestUri!.AbsoluteUri==FlatPolicy.Feed;if(!manifest)Downloads++;return Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK){Content=new ByteArrayContent(manifest?Manifest:Archive)});}
 }
}
