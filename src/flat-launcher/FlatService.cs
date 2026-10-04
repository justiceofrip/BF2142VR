using BF2142.Community;
using Microsoft.Win32;
using System.Net.Security;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
namespace BF2142.FlatViewer;

public sealed record ReadyBuild(ServerManifest Manifest,Package Package,string Payload,bool Offline);
public sealed class FlatService : IDisposable {
 public static string Home=>Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"BF2142VR","FlatViewer");
 public static string MicrophoneFile=>Path.Combine(Join.Home,"flat.ini");
 public string ManifestPath=>Path.Combine(storage,"current-server.json");
 readonly HttpClient http; readonly bool ownsHttp; readonly string storage,cache,publisherKey;
 readonly PackageStore store;
 public FlatService(HttpClient? client=null,string? storage=null,string? cache=null,string? publisherKey=null){
  this.storage=storage??Home;this.cache=cache??Join.Home;this.publisherKey=publisherKey??Join.PublicKey;
  ownsHttp=client is null;http=client??new(new HttpClientHandler{AllowAutoRedirect=false}){Timeout=TimeSpan.FromMinutes(10)};
  Directory.CreateDirectory(this.storage);Validation.NoLinks(this.storage,this.storage);store=new(this.cache,http);
 }
 public async Task<ReadyBuild> Prepare(bool repair,Action<string> progress,CancellationToken ct) {
  Directory.CreateDirectory(cache);
  using var lease=new FileStream(Path.Combine(cache,"joining.lock"),FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None);
  progress("Checking for flat-viewer updates...");byte[] bytes;bool offline=false;
  try {
   using var timeout=CancellationTokenSource.CreateLinkedTokenSource(ct);timeout.CancelAfter(TimeSpan.FromSeconds(20));
   bytes=await store.DownloadManifest(FlatPolicy.Feed,timeout.Token);
  }catch(Exception e)when((e is HttpRequestException||e is OperationCanceledException&&!ct.IsCancellationRequested)&&File.Exists(ManifestPath)){
   bytes=await File.ReadAllBytesAsync(ManifestPath,ct);offline=true;progress("Update check unavailable. Verifying the previously installed version...");
  }
  var manifest=PackageStore.Verify(bytes,publisherKey);var package=FlatPolicy.Check(manifest);store.CheckRevision(manifest);
  var destination=Path.Combine(cache,"packages",package.Sha256.ToLowerInvariant());
  if(repair&&Directory.Exists(destination)){
   Validation.NoLinks(cache,destination);
   // Retain the old cache for recovery; never delete a running or unrelated folder.
   Directory.Move(destination,destination+".previous-"+Guid.NewGuid().ToString("N"));
  }
  progress("Installing / verifying flat viewer "+package.Version+"...");
  string payload=await store.Acquire(package,ct);ct.ThrowIfCancellationRequested();
  store.AcceptRevision(manifest);PackageStore.Atomic(ManifestPath,bytes);
  return new(manifest,package,payload,offline);
 }
 public static async Task<bool> ServerOnline(ServerManifest server,CancellationToken ct) {
  using var timeout=CancellationTokenSource.CreateLinkedTokenSource(ct);timeout.CancelAfter(TimeSpan.FromSeconds(6));
  try{
   using var client=new TcpClient();await client.ConnectAsync(server.Host,server.ControlPort,timeout.Token);
   using var tls=new SslStream(client.GetStream(),false,(_,cert,_,_)=>cert is not null&&CryptographicOperations.FixedTimeEquals(SHA256.HashData(cert.GetRawCertData()),Convert.FromHexString(server.CertificateSha256)));
   await tls.AuthenticateAsClientAsync(new SslClientAuthenticationOptions{TargetHost=server.Host},timeout.Token);return true;
  }catch(Exception e)when(e is IOException or SocketException or System.Security.Authentication.AuthenticationException or OperationCanceledException){return false;}
 }
 public static string DetectGame() {
  foreach(var name in new[]{Path.Combine(Home,"game-path.txt"),Path.Combine(Join.Home,"game-path.txt")}){
   if(File.Exists(name)){var path=File.ReadAllText(name).Trim();if(FlatPolicy.IsGame(path))return path;}
  }
  foreach(var view in new[]{RegistryView.Registry32,RegistryView.Registry64}){
   using var hive=RegistryKey.OpenBaseKey(RegistryHive.LocalMachine,view);using var key=hive.OpenSubKey(@"SOFTWARE\Electronic Arts\EA Games\Battlefield 2142");
   if(key?.GetValue("InstallDir") is string path&&FlatPolicy.IsGame(path))return path;
  }
  foreach(var parent in new[]{Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86)})
   foreach(var relative in new[]{@"EA GAMES\Battlefield 2142",@"EA Games\Battlefield 2142 Deluxe Edition",@"Origin Games\Battlefield 2142"}){string path=Path.Combine(parent,relative);if(FlatPolicy.IsGame(path))return path;}
  return "";
 }
 public static void SaveGame(string path){if(!FlatPolicy.IsGame(path))throw new IOException("Select the installed BF2142.exe, not the viewer EXE.");PackageStore.Atomic(Path.Combine(Home,"game-path.txt"),Encoding.UTF8.GetBytes(Path.GetFullPath(path)));}
 public static string KeepLauncher(string source) {
  string hash=FlatPolicy.Hash(source);string dir=Path.Combine(Home,"launchers");Directory.CreateDirectory(dir);Validation.NoLinks(Home,dir);
  string target=Path.Combine(dir,"BF2142FlatViewer-"+hash[..16]+".exe");
  if(File.Exists(target)){if(FlatPolicy.Hash(target)!=hash)throw new CryptographicException("Cached launcher changed. Use Repair to restore it.");}
  else {string pending=target+".pending";File.Copy(source,pending,true);File.Move(pending,target);}
  var type=Type.GetTypeFromProgID("WScript.Shell")??throw new IOException("Windows shortcuts unavailable.");dynamic shell=Activator.CreateInstance(type)!;
  dynamic shortcut=shell.CreateShortcut(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory),"BF2142 Flat Viewer.lnk"));
  shortcut.TargetPath=target;shortcut.Arguments="";shortcut.WorkingDirectory=dir;shortcut.Description="BF2142 desktop crossplay — no headset required";shortcut.Save();
  return target;
 }
 public void Dispose(){if(ownsHttp)http.Dispose();}
}
