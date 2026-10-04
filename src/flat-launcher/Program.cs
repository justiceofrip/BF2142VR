using System.Diagnostics;
using BF2142.Community;
namespace BF2142.FlatViewer;

internal static class Program {
 [STAThread] static void Main(string[] args) {
  ApplicationConfiguration.Initialize();
  using var mutex=new Mutex(false,@"Local\BF2142-FlatViewer");bool owned=false;
  try{
   try{owned=mutex.WaitOne(args.Contains("--updated")?TimeSpan.FromSeconds(15):TimeSpan.Zero);}catch(AbandonedMutexException){owned=true;}
   if(!owned){MessageBox.Show("BF2142 Flat Viewer is already open. Use its window or taskbar icon.","BF2142 Flat Viewer");return;}
   if(args.Any(a=>a is not("--updated" or "--play")))throw new ArgumentException("Open BF2142FlatViewer.exe normally to play.");
   Application.Run(new FlatForm(args.Contains("--play")));
  }catch(Exception e){MessageBox.Show(FlatPolicy.Friendly(e),"BF2142 Flat Viewer",MessageBoxButtons.OK,MessageBoxIcon.Error);}
  finally{if(owned)mutex.ReleaseMutex();}
 }
}

public sealed partial class FlatForm : Form {
 readonly FlatService service=new();
 readonly TextBox game=new(){ReadOnly=true,Dock=DockStyle.Fill};
 readonly Label state=new(){AutoSize=true,Text="Getting ready..."},server=new(){AutoSize=true,Text="Checking community server..."},version=new(){AutoSize=true,Text="Automatic updates enabled"};
 readonly Button play=new(){Text="PLAY",Enabled=false},browse=new(){Text="Browse..."},check=new(){Text="Check for updates"},repair=new(){Text="Repair"},copy=new(){Text="Copy error report"},save=new(){Text="Save report"},issue=new(){Text="Report on GitHub"};
 readonly CheckBox mic=new(){Text="Enable proximity microphone — nearby players can hear you",AutoSize=true};
 readonly ProgressBar progress=new(){Dock=DockStyle.Top,Height=5,Style=ProgressBarStyle.Blocks};
 readonly TextBox details=new(){Multiline=true,ReadOnly=true,ScrollBars=ScrollBars.Vertical,Dock=DockStyle.Fill};
 readonly System.Text.StringBuilder history=new();
 readonly CancellationTokenSource lifetime=new();
 ReadyBuild? ready;bool busy,running,handoff;readonly bool autoPlay;
 public FlatForm(bool autoPlay=false,bool startOnShown=true) {
  this.autoPlay=autoPlay;BuildInterface();
  game.Text=FlatService.DetectGame();mic.Checked=FlatPolicy.ReadMicrophone(FlatService.MicrophoneFile);
  browse.Click+=(_,_)=>Browse();play.Click+=async(_,_)=>await Play();check.Click+=async(_,_)=>await Update(false);repair.Click+=async(_,_)=>await Update(true);
  copy.Click+=(_,_)=>{try{Clipboard.SetText(Report());state.Text="Report copied. Review it before sharing.";}catch(Exception e){Failure(e);}};
  save.Click+=(_,_)=>SaveReport();issue.Click+=(_,_)=>OpenUrl("https://github.com/justiceofrip/BF2142VR/issues/new?title="+Uri.EscapeDataString("Flat Viewer issue")+"&body="+Uri.EscapeDataString("Flat viewer version: "+(ready?.Package.Version??"unknown")+"\n\nWhat happened?\n\nSteps to reproduce:\n\nPaste or attach the reviewed launcher error report here.\n"));
  FormClosing+=(_,e)=>{
   if(running&&e.CloseReason==CloseReason.UserClosing){e.Cancel=true;WindowState=FormWindowState.Minimized;return;}
   lifetime.Cancel();
  };
  Shown+=async(_,_)=>{if(!startOnShown)return;
   Log("Flat viewer opened.");
   try{FlatService.KeepLauncher(Environment.ProcessPath!);}catch(Exception e){Log("Shortcut: "+e.Message);}
   await Update(false);
   if(autoPlay&&!handoff&&ready is not null)await Play();
  };
 }
 void Browse(){using var picker=new OpenFileDialog{Title="Select your installed Battlefield 2142 game",Filter="Battlefield 2142 game|BF2142.exe",CheckFileExists=true};if(picker.ShowDialog(this)==DialogResult.OK){try{game.Text=Path.GetDirectoryName(picker.FileName)!;FlatService.SaveGame(game.Text);}catch(Exception e){Failure(e);}}}
 void SetBusy(bool value){busy=value;play.Enabled=check.Enabled=repair.Enabled=browse.Enabled=mic.Enabled=!value&&!running;progress.Style=value?ProgressBarStyle.Marquee:ProgressBarStyle.Blocks;}
 void Status(string text){state.Text=text;Log(text);}
 async Task<bool> Update(bool repairCache,bool launchAfter=false) {
  if(busy||running)return false;SetBusy(true);ready=null;
  try{
   ready=await service.Prepare(repairCache,Status,lifetime.Token);version.Text="Installed: "+ready.Package.Version+(ready.Offline?" · update check unavailable":" · up to date");
   bool online=await FlatService.ServerOnline(ready.Manifest,lifetime.Token);
   server.Text=online?ready.Manifest.Name+" · reachable":ready.Manifest.Name+" · offline or unreachable";
   Log(server.Text);
   var exe=Path.Combine(ready.Payload,FlatPolicy.Viewer);
   string target;
   try{target=FlatService.KeepLauncher(exe);}catch(Exception e){Log("Shortcut: "+e.Message);target=exe;}
   if(FlatPolicy.Hash(Environment.ProcessPath!)!=FlatPolicy.Hash(exe)){
    var next=new ProcessStartInfo(target){UseShellExecute=false,WorkingDirectory=FlatService.Home};next.ArgumentList.Add("--updated");if(launchAfter)next.ArgumentList.Add("--play");
    _ = Process.Start(next)??throw new IOException("Could not open the updated launcher.");handoff=true;Close();return false;
   }
   Status(online?"Ready. Click Play, then use your normal Reclamation login.":"The server is currently unreachable. Your addon is installed; try Play again later.");
   return online;
  }catch(Exception e){if(!IsDisposed&&!lifetime.IsCancellationRequested)Failure(e);return false;}
  finally{if(!IsDisposed)SetBusy(false);}
 }
 async Task Play() {
  if(busy||running)return;
  if(!FlatPolicy.IsGame(game.Text)){Browse();if(!FlatPolicy.IsGame(game.Text))return;}
  foreach(var p in Process.GetProcessesByName("BF2142")){using(p){if(!p.HasExited){Status("Close Battlefield 2142 first, then click Play here to enable the viewer.");return;}}}
  if(!await Update(false,true)||handoff)return;
  running=true;SetBusy(false);play.Text="GAME RUNNING";
  try{
   FlatService.SaveGame(game.Text);FlatPolicy.SaveMicrophone(FlatService.MicrophoneFile,mic.Checked);
   var info=FlatPolicy.StartInfo(ready!.Payload,service.ManifestPath,game.Text);
   using var child=new Process{StartInfo=info};
   if(!child.Start())throw new IOException("Could not launch Battlefield 2142.");
   Status("Game running. Log in normally to join. Minimize this launcher while playing.");
   var stdout=ReadLines(child.StandardOutput);var stderr=ReadLines(child.StandardError);
   await child.WaitForExitAsync();await Task.WhenAll(stdout,stderr);
   if(child.ExitCode!=0)throw new IOException("The game couldn't start or closed with an error ("+child.ExitCode+"). See the log below or copy the error report.");
   Status("Game closed. Click Play to join again.");
  }catch(Exception e){Failure(e);}
  finally{running=false;play.Text="PLAY";SetBusy(false);}
 }
 async Task ReadLines(StreamReader reader){while(await reader.ReadLineAsync() is { } line)Log(line);}
 void Failure(Exception e){if(ready is null)server.Text="Server status unavailable";Log(e.GetType().Name+": "+e.Message);state.Text=FlatPolicy.Friendly(e);}
 void Log(string text){string safe=FlatPolicy.Redact(text,game.Text,Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),FlatService.Home,Join.Home);if(history.Length>128*1024)history.Remove(0,history.Length-64*1024);history.AppendLine(DateTime.Now.ToString("HH:mm:ss")+"  "+safe);details.Text=history.ToString();details.SelectionStart=details.TextLength;details.ScrollToCaret();try{File.WriteAllText(Path.Combine(FlatService.Home,"last-session.txt"),Report());}catch(IOException){}catch(UnauthorizedAccessException){}}
 string Report()=>"BF2142 Flat Viewer report\r\nLauncher: "+typeof(FlatForm).Assembly.GetName().Version+"\r\nAddon: "+(ready?.Package.Version??"not loaded")+"\r\nUTC: "+DateTime.UtcNow.ToString("O")+"\r\nWindows: "+Environment.OSVersion.Version+"\r\nNo game files, account profiles, or audio are collected. Review before sharing.\r\n\r\n"+history;
 void SaveReport(){try{using var picker=new SaveFileDialog{Filter="Text report|*.txt",FileName="BF2142-Flat-Viewer-report.txt"};if(picker.ShowDialog(this)==DialogResult.OK){File.WriteAllText(picker.FileName,Report());state.Text="Report saved. Nothing has been uploaded.";}}catch(Exception e){Failure(e);}}
 void OpenUrl(string url){try{Process.Start(new ProcessStartInfo(url){UseShellExecute=true});}catch(Exception e){Failure(e);}}
 protected override void Dispose(bool disposing){if(disposing&&!IsDisposed){lifetime.Cancel();lifetime.Dispose();service.Dispose();}base.Dispose(disposing);}
}
