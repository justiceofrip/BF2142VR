using System.Diagnostics;
using System.Text.Json;
namespace BF2142.Installer;
public static class InstallService {
 public static string InstalledVersion(string game) {
  string path=Path.Combine(game,"BF2142VR","install.json");if(!File.Exists(path))return "Not installed";
  try {using var doc=JsonDocument.Parse(File.ReadAllBytes(path));return doc.RootElement.GetProperty("status").GetString()=="installed"?doc.RootElement.GetProperty("version").GetString()??"Unknown":"Incomplete installation";}catch {return "Installation needs attention";}
 }
 public static async Task Apply(string payload,string game,Action<string> line) {
  game=Path.GetFullPath(game);ReleaseFeed.NoLinks(game,game);
  if(!File.Exists(Path.Combine(game,"BF2142.exe")))throw new IOException("Choose the folder containing your BF2142.exe.");
  await Worker(payload,["apply","--game",game,"--payload",payload],line);
 }
 public static async Task Worker(string payload,IEnumerable<string> args,Action<string> line) {
  string exe=ReleaseFeed.Inside(payload,"tools/SetupAssets.exe");
  var start=new ProcessStartInfo(exe){UseShellExecute=false,CreateNoWindow=true,RedirectStandardOutput=true,RedirectStandardError=true,WorkingDirectory=payload};
  foreach(string arg in args)start.ArgumentList.Add(arg);
  string logRoot=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"BF2142VR","SetupLogs");Directory.CreateDirectory(logRoot);
  string log=Path.Combine(logRoot,"installer-"+DateTime.Now.ToString("yyyyMMdd-HHmmss-fff")+".log");
  using var writer=new StreamWriter(log){AutoFlush=true};object guard=new();
  void Emit(string? text) {if(text is null)return;lock(guard)writer.WriteLine(text);line(text);}
  using var process=new Process{StartInfo=start};process.OutputDataReceived+=(_,e)=>Emit(e.Data);process.ErrorDataReceived+=(_,e)=>Emit(e.Data);
  Emit("Installer log: "+log);if(!process.Start())throw new IOException("Could not start setup.");
  process.BeginOutputReadLine();process.BeginErrorReadLine();await process.WaitForExitAsync();process.WaitForExit();
  if(process.ExitCode!=0)throw new IOException($"Setup stopped (0x{process.ExitCode:X8}). Your backups and verified repair progress are retained. Log: {log}");
 }
 public static void Launch(string game) {
  string script=Path.Combine(Path.GetFullPath(game),"BF2142VR","tools","Player.ps1");if(!File.Exists(script))throw new IOException("Install BF2142 VR first.");
  var start=new ProcessStartInfo(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System),"WindowsPowerShell","v1.0","powershell.exe")){UseShellExecute=false,CreateNoWindow=true};
  foreach(string arg in new[]{"-NoProfile","-ExecutionPolicy","Bypass","-File",script,"-Action","Play"})start.ArgumentList.Add(arg);
  Process.Start(start);
 }
}
