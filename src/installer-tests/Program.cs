using BF2142.Installer;
using System.Net;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text;
int checks=0;void Check(bool value,string name){if(!value)throw new Exception(name);checks++;}
void Refuse(Action action,string name){try{action();}catch{checks++;return;}throw new Exception("Accepted "+name);}
async Task RefuseAsync(Func<Task> action,string name){try{await action();}catch{checks++;return;}throw new Exception("Accepted "+name);}
using var key=ECDsa.Create(ECCurve.NamedCurves.nistP256);
string Url(string id)=>"https://github.com/justiceofrip/BF2142VR/releases/download/test/"+id;
string Hash(byte[] b)=>Convert.ToHexString(SHA256.HashData(b));
Dictionary<string,byte[]> content=[];
ReleaseFile Make(string path,string text){byte[] data=Encoding.UTF8.GetBytes(text);string url=Url(Hash(data));content[url]=data;return new(path,data.Length,Hash(data),url);}
List<ReleaseFile> files=[];foreach(string path in new[]{"tools/SetupAssets.exe","tools/Player.ps1","runtime/x86/BF2142VRLauncher.exe","runtime/x86/BF2142VRClient.dll","runtime/x64/BFVRPresenter.exe"})files.Add(Make(path,"synthetic "+path));
var payload=JsonSerializer.Serialize(new{version="0.2.0-test",files=files.ToDictionary(f=>f.Path,f=>f.Sha256)});files.Add(Make("payload.json",payload));
var release=new Release(1,6,"0.2.0-test","test",1,files.ToArray());
byte[] Sign(Release r){var data=JsonSerializer.SerializeToUtf8Bytes(r);return JsonSerializer.SerializeToUtf8Bytes(new SignedRelease(Convert.ToBase64String(data),Convert.ToBase64String(key.SignData(data,HashAlgorithmName.SHA256))));}
var signed=Sign(release);Check(ReleaseFeed.Verify(signed,key.ExportSubjectPublicKeyInfoPem()).Revision==6,"signature");
using(var stranger=ECDsa.Create(ECCurve.NamedCurves.nistP256))Refuse(()=>ReleaseFeed.Verify(signed,stranger.ExportSubjectPublicKeyInfoPem()),"wrong publisher");
Refuse(()=>ReleaseFeed.Verify(signed,key.ExportSubjectPublicKeyInfoPem(),7),"downgrade");
Refuse(()=>ReleaseFeed.Verify(Sign(release with{MinimumUpdater=2}),key.ExportSubjectPublicKeyInfoPem()),"future updater");
foreach(var path in new[]{"../escape","/absolute","a\\b","C:/escape","x:stream","CON.txt","sub/../escape","file.","file ","a//b","backups/file","generated/file","BF2142.exe","install.json","BF2142VR.ini"})Refuse(()=>ReleaseFeed.SafePath(path),path);
Refuse(()=>ReleaseFeed.Validate(release with{Files=[..files,files[0] with{Path=files[0].Path.ToUpperInvariant()}]}),"duplicate");
Refuse(()=>ReleaseFeed.Validate(release with{Files=[files[0]]}),"missing files");
Refuse(()=>ReleaseFeed.Validate(release with{Files=[..files.Take(5),files[5] with{Size=long.MaxValue}]}),"size");
foreach(var url in new[]{"http://github.com/justiceofrip","https://evil.test/file","https://github.com:444/file","https://user:pass@github.com/file","https://github.com/file#fragment"})Refuse(()=>ReleaseFeed.AllowedUrl(url,false),url);
Refuse(()=>ReleaseFeed.Validate(release with{Files=[files[0] with{Url="https://github.com/another/repo/releases/download/tag/a"},..files.Skip(1)]}),"other repository");
string root=Path.Combine(Path.GetTempPath(),"bfvr-updater-tests-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
try {
 var handler=new FakeHttp(content);using var feed=new ReleaseFeed(handler);var store=new UpdateStore(Path.Combine(root,"cache"),feed);var progress=new Progress<(int,string)>();
 string first=await store.Acquire(release,null,progress,CancellationToken.None);Check(handler.Requests==files.Count,"initial downloads");UpdateStore.VerifyPayloadManifest(first,release);store.RemoveStage(first);
 handler.Requests=0;string second=await store.Acquire(release,null,progress,CancellationToken.None);Check(handler.Requests==0,"all reused");store.RemoveStage(second);
 File.WriteAllText(Path.Combine(root,"cache","files",files[0].Sha256.ToLowerInvariant()+".bin"),"corrupt");string repaired=await store.Acquire(release,null,progress,CancellationToken.None);Check(handler.Requests==1,"only corrupt download");store.RemoveStage(repaired);
 var changed=Make("tools/Player.ps1","changed script");var newFiles=files.Select(f=>f.Path==changed.Path?changed:f).Where(f=>f.Path!="payload.json").ToList();newFiles.Add(Make("payload.json",JsonSerializer.Serialize(new{version="0.2.0-test2",files=newFiles.ToDictionary(f=>f.Path,f=>f.Sha256)})));var updated=release with{Version="0.2.0-test2",Revision=7,Files=newFiles.ToArray()};
 handler.Requests=0;string third=await store.Acquire(updated,null,progress,CancellationToken.None);Check(handler.Requests==2,"only changed script and manifest");store.Accept(7);store.RemoveStage(third);await RefuseAsync(()=>store.Acquire(release,null,progress,CancellationToken.None),"cached downgrade");
 Refuse(()=>store.RemoveStage(root),"unsafe cleanup");
 var freshHandler=new FakeHttp(content);using var freshFeed=new ReleaseFeed(freshHandler);var freshStore=new UpdateStore(Path.Combine(root,"reuse"),freshFeed);string installed=Path.Combine(root,"installed");Directory.CreateDirectory(installed);
 foreach(var file in updated.Files){string dest=ReleaseFeed.Inside(installed,file.Path);Directory.CreateDirectory(Path.GetDirectoryName(dest)!);File.WriteAllBytes(dest,content[file.Url]);}
 Check(InstalledRelease.Check(installed,updated.Version,updated).Damaged.Length==0,"signed installation intact");
 File.WriteAllText(Path.Combine(installed,"tools","Player.ps1"),"broken");
 var damaged=InstalledRelease.Check(installed,updated.Version,updated);Check(damaged.SameVersion&&damaged.Damaged.SequenceEqual(new[]{"tools/Player.ps1"}),"same-version tampering detected");
 Check(!InstalledRelease.Check(installed,"old",updated).SameVersion,"new release distinguished from repair");
 File.WriteAllBytes(Path.Combine(installed,"tools","Player.ps1"),content[changed.Url]);
 string reused=await freshStore.Acquire(updated,installed,progress,CancellationToken.None);Check(freshHandler.Requests==0,"reuse verified installed files");freshStore.RemoveStage(reused);
 content[files[0].Url]=Encoding.UTF8.GetBytes("truncated");var badStore=new UpdateStore(Path.Combine(root,"bad"),freshFeed);await RefuseAsync(()=>badStore.Acquire(release,null,progress,CancellationToken.None),"truncated download");
 Check(Directory.GetDirectories(Path.Combine(root,"bad"),"payload-*").Length==0,"failed stage removed");
 using var cancelled=new CancellationTokenSource();cancelled.Cancel();await RefuseAsync(()=>freshStore.Acquire(updated,null,progress,cancelled.Token),"cancel");
}finally{Directory.Delete(root,true);}
var report=Diagnostics.CreateReport("v1","v2","password=secret123\nBearer abc.def\nname@example.com\nC:\\Users\\Somebody\\Game\\file.py\nexit 0xC0000005", "Verified mesh abcdef1234", @"D:\PrivateGame");
foreach(string secret in new[]{"secret123","abc.def","name@example.com","Somebody"})Check(!report.Contains(secret),"report removes "+secret);
Check(report.Contains("0xC0000005")&&report.Contains("Verified mesh abcdef1234"),"diagnostics retain actionable error/hash");
Check(Diagnostics.IssueUrl("v1 & injected","report.txt").StartsWith("https://github.com/justiceofrip/BF2142VR/issues/new?title="),"issue destination fixed");
Check(!Diagnostics.IssueUrl("v1 & injected","report.txt").Contains(" & "),"issue fields escaped");
Console.WriteLine($"PASS: {checks} installer feed/cache/update checks");
sealed class FakeHttp(Dictionary<string,byte[]> files):HttpMessageHandler {
 public int Requests;
 protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request,CancellationToken ct){Interlocked.Increment(ref Requests);ct.ThrowIfCancellationRequested();return Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK){Content=new ByteArrayContent(files[request.RequestUri!.AbsoluteUri])});}
}
