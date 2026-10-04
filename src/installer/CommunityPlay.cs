using BF2142.Community;
using System.Diagnostics;
namespace BF2142.Installer;

// Only the signed server connection metadata is shared with Flat Viewer.
// Never acquire its package (or the older community VR package).
public static class CommunityPlay {
 public const string Feed="https://raw.githubusercontent.com/justiceofrip/BF2142VR/updates/flat.json";
 public static void CheckServer(ServerManifest server) {
  Validation.Manifest(server);
  if(server.Id!="bf2142-flat-viewer"||server.DescriptorUrl!=Feed)
   throw new InvalidDataException("Unexpected community server descriptor.");
 }
 public static async Task<int> Run(string game) {
  game=Path.GetFullPath(game);
  foreach(var process in Process.GetProcessesByName("BF2142"))using(process)
   if(!process.HasExited)throw new IOException("Close BF2142 before starting another game.");
  string payload=Path.Combine(game,"BF2142VR");
  if(!File.Exists(Path.Combine(game,"BF2142.exe")))throw new IOException("Select your Battlefield 2142 installation first.");
  Directory.CreateDirectory(Join.Home);
  using var lease=new FileStream(Path.Combine(Join.Home,"joining.lock"),FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None);
  await InstallService.Worker(payload,["check","--root",payload],Console.WriteLine);
  using var http=new HttpClient(new HttpClientHandler{AllowAutoRedirect=false}){Timeout=TimeSpan.FromSeconds(20)};
  var store=new PackageStore(Join.Home,http);
  string cache=Path.Combine(Join.Home,"vr-server.json");byte[] bytes;
  try{bytes=await store.DownloadManifest(Feed,CancellationToken.None);}
  catch(Exception e)when((e is HttpRequestException or OperationCanceledException)&&File.Exists(cache)){
   Console.WriteLine("Server directory unavailable; verifying saved server details.");bytes=File.ReadAllBytes(cache);
  }
  var server=PackageStore.Verify(bytes,Join.PublicKey);CheckServer(server);store.CheckRevision(server);
  store.AcceptRevision(server);PackageStore.Atomic(cache,bytes);
  // Match the user's already-running SteamVR session, without modifying the
  // system's registered OpenXR runtime or another application's environment.
  foreach(var process in Process.GetProcessesByName("vrserver"))using(process){
   string? path;
   try{path=process.MainModule?.FileName;}
   catch(Exception e)when(e is System.ComponentModel.Win32Exception or InvalidOperationException){continue;}
   string? root=path is null?null:Directory.GetParent(path)?.Parent?.Parent?.FullName;
   string runtime=Path.Combine(root??"", "steamxr_win64.json");
   if(root is not null&&File.Exists(runtime)){Environment.SetEnvironmentVariable("XR_RUNTIME_JSON",runtime);break;}
  }
  Console.WriteLine("Starting VR. Log in and join "+server.Name+" in the multiplayer browser.");
  return await Join.Launch(server,payload,game,"vr",Path.Combine(payload,"BF2142VR.ini"),null,false,CancellationToken.None,true);
 }
}
