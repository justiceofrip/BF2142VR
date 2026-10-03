using System.Diagnostics;
using System.Text.Json;
namespace BF2142.Installer;
internal static class Program {
 [STAThread] static void Main(string[] args) {
  using var mutex=new Mutex(true,@"Local\BF2142VR-Installer",out bool owned);
  if(!owned&&args.Length>0){Environment.ExitCode=2;Console.Error.WriteLine("The launcher is already running.");return;}
  if(!owned){MessageBox.Show("The BF2142 VR installer is already running.","Battlefield 2142 VR");return;}
  try {
   ApplicationConfiguration.Initialize();
   if(args.Length==4&&args[0]=="--apply-offline"&&args[2]=="--game") {
    InstallService.Apply(args[1],args[3],Console.WriteLine).GetAwaiter().GetResult();return;
   }
   if(args.Length==2&&args[0]=="--render-preview") {
    using var form=new SetupForm(true);form.StartPosition=FormStartPosition.Manual;form.Location=new Point(-20000,-20000);form.Show();Application.DoEvents();using var bitmap=new Bitmap(form.Width,form.Height);form.DrawToBitmap(bitmap,new Rectangle(Point.Empty,form.Size));bitmap.Save(Path.GetFullPath(args[1]));form.Close();return;
   }
   if(args.Length==2&&args[0]=="--game") {Application.Run(new SetupForm(false,Path.GetFullPath(args[1])));return;}
   if(args.Length==1&&args[0]=="--preview"){Application.Run(new SetupForm(true));return;}
   if(args.Length!=0)throw new ArgumentException("Launch the installer normally, or use --apply-offline PAYLOAD --game GAME for a local developer check.");
   Application.Run(new SetupForm());
  } catch(Exception error) {Environment.ExitCode=1;if(args.Length>0){Console.Error.WriteLine(error);return;}MessageBox.Show(error.Message,"Battlefield 2142 VR",MessageBoxButtons.OK,MessageBoxIcon.Error);}
 }
}
public sealed partial class SetupForm : Form {
 readonly string dataRoot=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"BF2142VR","Updater");
 readonly TextBox game=new(){Dock=DockStyle.Fill};
 readonly Label installed=new(){AutoSize=true};
 readonly Label available=new(){AutoSize=true,Text="Checking for updates..."};
 readonly Label status=new(){AutoSize=true,MaximumSize=new Size(720,70),Text="Select your installed Battlefield 2142 folder."};
 readonly TacticalProgress progress=new(){Dock=DockStyle.Fill,Minimum=0,Maximum=100};
 readonly TextBox log=new(){Multiline=true,ReadOnly=true,ScrollBars=ScrollBars.Vertical,Dock=DockStyle.Fill,WordWrap=true};
 readonly TacticalButton browse=new(){Text="Browse...",AutoSize=true};
 readonly TacticalButton install=new(){Text="Install / Update",AutoSize=true,Enabled=false};
 readonly TacticalButton repair=new(){Text="Repair",AutoSize=true,Enabled=false};
 readonly TacticalButton check=new(){Text="Check for updates",AutoSize=true};
 readonly TacticalButton play=new(){Text="Play VR",AutoSize=true};
 readonly TacticalButton credits=new(){Text="Credits / licenses",AutoSize=true};
 readonly TacticalButton cancel=new(){Text="Cancel download",AutoSize=true,Enabled=false};
 readonly TacticalButton report=new(){Text="Save error report",AutoSize=true};
 readonly TacticalButton issue=new(){Text="Report on GitHub",AutoSize=true};
 readonly System.Text.StringBuilder session=new();
 string? reportPath;
 readonly ReleaseFeed feed=new();
 readonly UpdateStore store;
 Release? release;
 bool busy,applying;
 CancellationTokenSource? cancellation;
 public SetupForm(bool preview=false,string? initialGame=null) {
  Directory.CreateDirectory(dataRoot);store=new UpdateStore(dataRoot,feed);
  BuildInterface();
  browse.Click+=(_,_)=>Browse();game.TextChanged+=(_,_)=>RefreshInstalled();check.Click+=async(_,_)=>await Check();install.Click+=async(_,_)=>await Apply();repair.Click+=async(_,_)=>await Apply();cancel.Click+=(_,_)=>cancellation?.Cancel();
  report.Click+=(_,_)=>SaveReport();
  issue.Click+=(_,_)=>{if(SaveReport())try{Process.Start(new ProcessStartInfo(Diagnostics.IssueUrl(release?.Version??InstallService.InstalledVersion(game.Text),reportPath!)){UseShellExecute=true});}catch(Exception e){Report(e);}};
  credits.Click+=(_,_)=>{using var box=new Form{Text="Credits and licenses",Size=new Size(780,580),StartPosition=FormStartPosition.CenterParent};var text=new TextBox{Multiline=true,ReadOnly=true,ScrollBars=ScrollBars.Vertical,Dock=DockStyle.Fill};text.Text="Battlefield 2142 VR by justiceofrip. Based on BFVR by JayBiggsGMG and contributors.\r\nhttps://github.com/justiceofrip/BF2142VR\r\n\r\n";foreach(var name in new[]{"License","DotnetLicense","DotnetNotices"}){using var stream=System.Reflection.Assembly.GetExecutingAssembly().GetManifestResourceStream("BF2142.Installer."+name);if(stream is not null){using var reader=new StreamReader(stream);text.AppendText(reader.ReadToEnd()+"\r\n\r\n");}}box.Controls.Add(text);box.ShowDialog(this);};
  play.Click+=(_,_)=>{try{InstallService.Launch(game.Text);}catch(Exception e){Report(e);}};
  FormClosing+=(_,e)=>{if(busy){e.Cancel=true;status.Text=applying?"Finishing installation safely. Please keep this window open.":"Cancel the download before closing.";}};
  FormClosed+=(_,_)=>feed.Dispose();
  string settings=Path.Combine(dataRoot,"game-path.txt");if(File.Exists(settings))game.Text=File.ReadAllText(settings).Trim();
  if(initialGame is not null){game.Text=initialGame;UpdateStore.Atomic(Path.Combine(dataRoot,"game-path.txt"),initialGame);}
  session.AppendLine("Launcher opened "+DateTime.UtcNow.ToString("O"));
  RefreshInstalled();Shown+=async(_,_)=>{if(preview){available.Text="Available: 0.2.0-beta.4";status.Text="Interface preview — no changes will be made.";return;}KeepUpdater();await Check();};
 }
 void Browse() {
  using var picker=new OpenFileDialog{Title="Select your installed BF2142.exe",Filter="Battlefield 2142|BF2142.exe",CheckFileExists=true};
  if(picker.ShowDialog(this)==DialogResult.OK){game.Text=Path.GetDirectoryName(picker.FileName)!;UpdateStore.Atomic(Path.Combine(dataRoot,"game-path.txt"),game.Text);}
 }
 void RefreshInstalled() {installed.Text="Installed: "+InstallService.InstalledVersion(game.Text);play.Enabled=!applying&&File.Exists(Path.Combine(game.Text,"BF2142VR","tools","Player.ps1"));}
 void SetBusy(bool value) {busy=value;browse.Enabled=game.Enabled=check.Enabled=!value;install.Enabled=repair.Enabled=!value&&release is not null;cancel.Enabled=value&&!applying;RefreshInstalled();}
 void Append(string text) {if(IsDisposed)return;if(InvokeRequired){BeginInvoke(()=>Append(text));return;}session.AppendLine(text);
  try {File.AppendAllText(Path.Combine(dataRoot,"launcher-session.log"),Diagnostics.Redact(text,game.Text,Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),dataRoot)+Environment.NewLine);} catch(IOException){} catch(UnauthorizedAccessException){}
  if(log.TextLength>80000)log.Text=log.Text[^40000..];log.AppendText(text+Environment.NewLine);if(text.StartsWith("Preparing weapon")||text.StartsWith("Repairing weapon")||text.StartsWith("Building "))status.Text=text;}
 void Report(Exception error) {status.Text="Could not finish: "+error.Message;Append(error.ToString());MessageBox.Show(this,error.Message,"BF2142 VR",MessageBoxButtons.OK,MessageBoxIcon.Warning);}
 async Task Check() {
  if(busy)return;SetBusy(true);cancel.Enabled=false;status.Text="Checking the BF2142 VR update channel...";
  try {var document=await feed.Descriptor(CancellationToken.None);release=ReleaseFeed.Verify(document,ReleaseFeed.PublicKey(),store.AcceptedRevision);available.Text="Available: "+release.Version;
   string folder=game.Text,current=InstallService.InstalledVersion(folder);
   var verification=await Task.Run(()=>InstalledRelease.Check(Path.Combine(folder,"BF2142VR"),current,release));
   status.Text=verification.Message;Append(verification.Message);
   foreach(string path in verification.Damaged)Append("Needs repair: "+path);}
  catch(Exception e) {available.Text="Update service unavailable";status.Text="You can still play your installed version. Check for updates again later.";Append(e.Message);}
  finally {SetBusy(false);}
 }
 async Task Apply() {
  if(busy||release is null)return;
  if(!File.Exists(Path.Combine(game.Text,"BF2142.exe"))){Browse();if(!File.Exists(Path.Combine(game.Text,"BF2142.exe")))return;}
  foreach(var p in Process.GetProcessesByName("BF2142")){using(p){if(p.HasExited)continue;}MessageBox.Show(this,"Close BF2142, then choose Install / Update again.","BF2142 VR");return;}
  string folder=Path.GetFullPath(game.Text),selectedVersion=release.Version;UpdateStore.Atomic(Path.Combine(dataRoot,"game-path.txt"),folder);
  using var stop=new CancellationTokenSource();cancellation=stop;applying=false;SetBusy(true);string? payload=null;
  try {
   var report=new Progress<(int Percent,string Text)>(p=>{progress.Value=p.Percent;status.Text=p.Text;});
   payload=await store.Acquire(release,Path.Combine(folder,"BF2142VR"),report,stop.Token);stop.Token.ThrowIfCancellationRequested();
   applying=true;cancel.Enabled=false;status.Text="Installing — your original game backups will be retained.";progress.Style=ProgressBarStyle.Marquee;
   await InstallService.Apply(payload,folder,Append);store.Accept(release.Revision);
   KeepUpdater();
   status.Text="Ready — "+selectedVersion+" installed. Connect your headset, then Play VR.";progress.Value=100;
  } catch(OperationCanceledException){status.Text="Download cancelled. Verified files are saved; your game is unchanged.";}
  catch(Exception e){Report(e);}
  finally {if(payload is not null)try{store.RemoveStage(payload);}catch(Exception e){Append("Temporary payload retained: "+e.Message);}progress.Style=ProgressBarStyle.Blocks;applying=false;cancellation=null;SetBusy(false);}
 }
 void KeepUpdater() {
  try {
   string own=Environment.ProcessPath??throw new IOException("Missing executable path.");
   string source=own;
   // Adopt a newer launcher only after its installed payload passes the worker's
   // signed-file checks. Immutable cache filenames avoid replacing a running EXE.
   string installedLauncher=Path.Combine(game.Text,"BF2142VR","BF2142VRSetup.exe");
   if(applying&&File.Exists(installedLauncher))source=installedLauncher;
   using var input=File.OpenRead(source);string hash=Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(input));
   string copy=Path.Combine(dataRoot,"BF2142VR-"+hash[..16]+".exe");
   if(!Path.GetFullPath(source).Equals(copy,StringComparison.OrdinalIgnoreCase)){
    string pending=copy+".pending";File.Copy(source,pending,true);
    if(File.Exists(copy)){using var previous=File.OpenRead(copy);if(Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(previous))!=hash)throw new IOException("Cached launcher was modified. Keep the downloaded launcher and remove the damaged cache file.");File.Delete(pending);}
    else File.Move(pending,copy);
   }
   CreateShortcut("Battlefield 2142 VR",copy,"",dataRoot);
  }
  catch(Exception e){Append("Keep the downloaded EXE for future updates. "+e.Message);}
 }
 bool SaveReport() {
  try {
   using var picker=new SaveFileDialog{Title="Save installation report — review before sharing",Filter="Text report|*.txt",FileName="BF2142VR-install-report-"+DateTime.Now.ToString("yyyyMMdd-HHmmss")+".txt"};
   if(picker.ShowDialog(this)!=DialogResult.OK)return false;
   string logs=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"BF2142VR","SetupLogs");
   string? latest=Directory.Exists(logs)?Directory.EnumerateFiles(logs,"*.log").Where(p=>(File.GetAttributes(p)&FileAttributes.ReparsePoint)==0).OrderByDescending(File.GetLastWriteTimeUtc).FirstOrDefault():null;
   string? setup=null;if(latest is not null){using var reader=new StreamReader(latest);char[] buffer=new char[1024*1024];int n=reader.ReadBlock(buffer,0,buffer.Length);setup=new string(buffer,0,n);if(!reader.EndOfStream)setup+="\n[Log truncated at 1 MiB]";}
   string text=Diagnostics.CreateReport(InstallService.InstalledVersion(game.Text),release?.Version??"Unavailable",session.ToString(),setup,game.Text,Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData));
   File.WriteAllText(picker.FileName,text);reportPath=picker.FileName;
   status.Text="Report saved. Review it before attaching it to a GitHub issue. Nothing was uploaded.";return true;
  }catch(Exception e){Report(e);return false;}
 }
 static void CreateShortcut(string name,string target,string arguments,string directory) {
  // Structured COM arguments, no generated shell script or game credentials.
  var type=Type.GetTypeFromProgID("WScript.Shell")??throw new IOException("Windows shortcuts unavailable.");dynamic shell=Activator.CreateInstance(type)!;
  dynamic shortcut=shell.CreateShortcut(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory),name+".lnk"));shortcut.TargetPath=target;shortcut.Arguments=arguments;shortcut.WorkingDirectory=directory;shortcut.Description="Battlefield 2142 VR";shortcut.Save();
 }
}
