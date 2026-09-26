using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using Microsoft.Win32;
namespace BF2142.Community;
public static class Join {
 public static string Home=>Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"BF2142VR","Community");
 public static string PublicKey {get{using var source=typeof(Join).Assembly.GetManifestResourceStream("BF2142.Community.Publisher.pem")??throw new IOException("Publisher key missing.");using var reader=new StreamReader(source);return reader.ReadToEnd();}}
 public static (string Url,string Mode) ParseLink(string link){
  if(link.Length>4096||!Uri.TryCreate(link,UriKind.Absolute,out var uri)||uri.Scheme!="bf2142vr"||uri.Host!="join"||uri.AbsolutePath is not("" or "/")||uri.Port!=-1||uri.UserInfo!=""||uri.Fragment!="")throw new InvalidDataException("Invalid BF2142 join link.");
  Dictionary<string,string> values=[];foreach(var part in uri.Query.TrimStart('?').Split('&')){int at=part.IndexOf('=');if(at<1||!values.TryAdd(Uri.UnescapeDataString(part[..at]),Uri.UnescapeDataString(part[(at+1)..])))throw new InvalidDataException("Invalid join link options.");}
  if(!values.TryGetValue("server",out var url)||values.Keys.Any(x=>x is not("server" or "mode")))throw new InvalidDataException("Join link must contain only server and mode.");Validation.Https(url);string mode=values.GetValueOrDefault("mode","flat");if(mode is not("flat" or "vr"))throw new InvalidDataException("Invalid play mode.");return(url,mode);
 }
 public static void Install(){
  Directory.CreateDirectory(Home);string self=Environment.ProcessPath??throw new IOException("Cannot find helper executable.");if(Path.GetFileNameWithoutExtension(self).Equals("dotnet",StringComparison.OrdinalIgnoreCase))throw new InvalidOperationException("Install the published standalone helper.");
  string destination=Path.Combine(Home,"BF2142Community.exe");if(!Path.GetFullPath(self).Equals(destination,StringComparison.OrdinalIgnoreCase))File.Copy(self,destination,true);
  using var root=Registry.CurrentUser.CreateSubKey(@"Software\Classes\bf2142vr");root.SetValue("","URL:BF2142 community join");root.SetValue("URL Protocol","");using var command=root.CreateSubKey(@"shell\open\command");command.SetValue("",$"\"{destination}\" uri \"%1\"");
  Console.WriteLine("Join links are ready. No administrator access or SteamVR is needed for flat play.");
 }
 public static void Unregister(){using var key=Registry.CurrentUser.OpenSubKey(@"Software\Classes\bf2142vr\shell\open\command");var expected=$"\"{Path.Combine(Home,"BF2142Community.exe")}\" uri \"%1\"";if(key?.GetValue("") as string==expected)Registry.CurrentUser.DeleteSubKeyTree(@"Software\Classes\bf2142vr",false);Console.WriteLine("Join links removed; game files and downloaded builds retained.");}
 public static async Task<int> Run(ServerManifest manifest,string mode,string? gameDir,string? settings,string? observer,bool desktop,CancellationToken ct){
  if(mode is not("flat" or "vr"))throw new InvalidDataException("Choose flat or vr.");Validation.Manifest(manifest);
  Directory.CreateDirectory(Home);
  // A lock file is scoped to this user. A second link cannot race updates or open another primary game.
  using var lease=new FileStream(Path.Combine(Home,"joining.lock"),FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None);
  var store=new PackageStore(Home);store.CheckRevision(manifest);var package=manifest.Packages.SingleOrDefault(p=>p.Mode==mode)??throw new InvalidDataException("This server has no package for that mode.");
  gameDir=FindGame(gameDir);string payload=await store.Acquire(package,ct);store.AcceptRevision(manifest);
  if(mode=="vr")await EnsureVrAssets(gameDir,payload,ct);
  string prefs=Settings(mode,gameDir,settings);return await Launch(manifest,payload,gameDir,mode,prefs,observer,desktop,ct);
 }
 public static ProcessStartInfo CreateLaunchInfo(ServerManifest server,string payload,string game,string mode,string settings,string? observer,bool desktop,string network){
  if(mode is not("flat" or "vr"))throw new InvalidDataException("Choose flat or vr.");
  if(!Validation.HostName(server.Host)||!Validation.Port(server.GamePort)||server.Mod.Length==0||!server.Mod.All(c=>char.IsAsciiLetterOrDigit(c)||c is '_' or '-'))throw new InvalidDataException("Invalid game server destination or mod.");
  if(observer is not null&&(mode!="flat"||desktop||string.IsNullOrWhiteSpace(observer)||!Path.IsPathFullyQualified(observer)||observer.IndexOfAny(['\r','\n'])>=0))throw new InvalidDataException("An isolated observer requires flat mode and an absolute, separate Documents path.");
  var info=new ProcessStartInfo(Path.Combine(payload,"runtime","x86","BF2142VRLauncher.exe")){UseShellExecute=false,WorkingDirectory=game};
  foreach(var arg in new[]{"--game-dir",game,"--mod",server.Mod,"--windowed","--join-server",server.Host,"--port",server.GamePort.ToString(System.Globalization.CultureInfo.InvariantCulture)})info.ArgumentList.Add(arg);
  if(observer is not null){info.ArgumentList.Add("--observer-profile");info.ArgumentList.Add(Path.GetFullPath(observer));}
  if(mode=="flat")info.ArgumentList.Add("--flat");else if(desktop)info.ArgumentList.Add("--desktop-vr");else {info.ArgumentList.Add("--presenter");info.ArgumentList.Add(Path.Combine(payload,"runtime","x64","BFVRPresenter.exe"));}
  info.Environment["BF2142VR_NETWORK"]=network;info.Environment["BF2142VR_CONFIG"]=settings;info.Environment["BF2142VR_VOICE_RECEIVE_ONLY"]=observer is null?"0":"1";info.Environment["BFVR_DIAGNOSTICS"]="off";
  return info;
 }
 public static async Task<int> Launch(ServerManifest server,string payload,string game,string mode,string settings,string? observer,bool desktop,CancellationToken ct){
  byte[] secret=RandomNumberGenerator.GetBytes(16);using var bridge=new BridgeClient(server,secret);using var stop=CancellationTokenSource.CreateLinkedTokenSource(ct);
  string sessions=Path.Combine(Home,"sessions");Directory.CreateDirectory(sessions);string network=Path.Combine(sessions,Guid.NewGuid()+".ini");
  try{
   File.WriteAllText(network,$"[Network]\r\nEnabled=1\r\nPort={bridge.PosePort}\r\nSecret={Convert.ToHexString(secret)}\r\nMirrorToBot=0\r\n[Voice]\r\nEnabled=1\r\nPort={bridge.VoicePort}\r\nRangeMetres=20\r\nEnemyProximity=1\r\n",Encoding.ASCII);
   var info=CreateLaunchInfo(server,payload,game,mode,settings,observer,desktop,network);
   var transport=bridge.Run(stop.Token);using var process=Process.Start(info)??throw new IOException("Could not launch Battlefield 2142.");
   var exit=process.WaitForExitAsync(CancellationToken.None);var first=await Task.WhenAny(exit,transport);
   if(first==transport){try{await transport;}catch(OperationCanceledException)when(ct.IsCancellationRequested){}Console.WriteLine("The addon connection stopped. Close the game normally before joining again.");await exit;}
   stop.Cancel();try{await transport;}catch(OperationCanceledException){}Console.WriteLine("Game closed. Your earlier installed build is unchanged.");return process.ExitCode;
  }finally{stop.Cancel();CryptographicOperations.ZeroMemory(secret);if(File.Exists(network))File.Delete(network);}
 }
 static string Settings(string mode,string game,string? explicitSettings){
  if(explicitSettings is not null)return Path.GetFullPath(explicitSettings);
  string settings=Path.Combine(Home,mode+".ini");if(!File.Exists(settings)){
   string existing=Path.Combine(game,"BF2142VR","BF2142VR.ini");
   if(mode=="vr"&&File.Exists(existing))File.Copy(existing,settings);
   else File.WriteAllText(settings,"[VR]\r\nProximityVoice=1\r\nProximityMicMuted=0\r\nVoiceInputDevice=4294967295\r\nVoiceOutputDevice=4294967295\r\n",Encoding.Unicode);
  }return settings;
 }
 static async Task EnsureVrAssets(string game,string payload,CancellationToken ct){
  string installed=Path.Combine(game,"BF2142VR");if(File.Exists(Path.Combine(installed,"generated","BodyEquipment.bin"))&&File.Exists(Path.Combine(installed,"generated","LobbyScene.bin")))return;
  string setup=Path.Combine(payload,"tools","SetupAssets.exe");if(!File.Exists(setup))throw new FileNotFoundException("VR asset setup is missing from this package.");
  Console.WriteLine("Preparing VR models from your own game installation. This only runs on first setup.");var info=new ProcessStartInfo(setup){UseShellExecute=false,WorkingDirectory=payload};foreach(var a in new[]{"install","--game",game,"--payload",payload})info.ArgumentList.Add(a);using var p=Process.Start(info)??throw new IOException("Could not start VR setup.");await p.WaitForExitAsync(ct);if(p.ExitCode!=0)throw new IOException("VR setup did not finish. Its original-file backups were retained.");
 }
 public static string FindGame(string? explicitPath){
  string saved=Path.Combine(Home,"game-path.txt");string? game=explicitPath;
  if(game is null&&File.Exists(saved))game=File.ReadAllText(saved).Trim();
  if(game is null)foreach(var view in new[]{RegistryView.Registry32,RegistryView.Registry64}){using var hive=RegistryKey.OpenBaseKey(RegistryHive.LocalMachine,view);using var key=hive.OpenSubKey(@"SOFTWARE\Electronic Arts\EA Games\Battlefield 2142");game=key?.GetValue("InstallDir") as string;if(game is not null)break;}
  if(game is null||!File.Exists(Path.Combine(game,"BF2142.exe"))){Console.WriteLine("Select BF2142.exe once; the helper will remember this installation.");game=Picker.Game()??throw new OperationCanceledException("Game selection cancelled.");}
  game=Path.GetFullPath(game);if(!File.Exists(Path.Combine(game,"BF2142.exe"))||!File.Exists(Path.Combine(game,"RendDX9.dll")))throw new IOException("Choose the folder containing BF2142.exe and RendDX9.dll.");Directory.CreateDirectory(Home);PackageStore.Atomic(saved,Encoding.UTF8.GetBytes(game));return game;
 }
}
internal static class Picker {
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)]struct OpenFile {public int size;public IntPtr owner,instance;public string? filter;public IntPtr custom;public int maxCustom,filterIndex;public IntPtr file;public int maxFile;public IntPtr fileTitle;public int maxFileTitle;public string? directory,title;public int flags;public short offset,extension;public string? defaultExtension;public IntPtr data,hook,template,reserved;public int reserved2,flagsEx;}
 [DllImport("comdlg32.dll",CharSet=CharSet.Unicode,SetLastError=true)]static extern bool GetOpenFileNameW(ref OpenFile value);
 public static string? Game(){var buffer=Marshal.AllocHGlobal(32768*2);try{Marshal.WriteInt16(buffer,0);var value=new OpenFile{size=Marshal.SizeOf<OpenFile>(),filter="Battlefield 2142\0BF2142.exe\0\0",file=buffer,maxFile=32768,title="Choose your Battlefield 2142 installation",flags=0x80000|0x1000|0x800|0x8};return GetOpenFileNameW(ref value)?Path.GetDirectoryName(Marshal.PtrToStringUni(buffer)):null;}finally{Marshal.FreeHGlobal(buffer);}}
}
