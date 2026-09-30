using System.Diagnostics;
using System.Text.Json;
namespace BF2142.Installer;
internal static class Program {
 [STAThread] static void Main(string[] args) {
  using var mutex=new Mutex(true,@"Local\BF2142VR-Installer",out bool owned);
  if(!owned){MessageBox.Show("The BF2142 VR installer is already running.","Battlefield 2142 VR");return;}
  try {
   ApplicationConfiguration.Initialize();
   if(args.Length==4&&args[0]=="--apply-offline"&&args[2]=="--game") {
    InstallService.Apply(args[1],args[3],Console.WriteLine).GetAwaiter().GetResult();return;
   }
   if(args.Length==2&&args[0]=="--render-preview") {
    using var form=new SetupForm(true);form.StartPosition=FormStartPosition.Manual;form.Location=new Point(-20000,-20000);form.Show();Application.DoEvents();using var bitmap=new Bitmap(form.Width,form.Height);form.DrawToBitmap(bitmap,new Rectangle(Point.Empty,form.Size));bitmap.Save(Path.GetFullPath(args[1]));form.Close();return;
   }
   if(args.Length==1&&args[0]=="--preview"){Application.Run(new SetupForm(true));return;}
   if(args.Length!=0)throw new ArgumentException("Launch the installer normally, or use --apply-offline PAYLOAD --game GAME for a local developer check.");
   Application.Run(new SetupForm());
  } catch(Exception error) {Environment.ExitCode=1;if(args.Length>0){Console.Error.WriteLine(error);return;}MessageBox.Show(error.Message,"Battlefield 2142 VR",MessageBoxButtons.OK,MessageBoxIcon.Error);}
 }
}
public sealed class SetupForm : Form {
 readonly string dataRoot=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"BF2142VR","Updater");
 readonly TextBox game=new(){Dock=DockStyle.Fill};
 readonly Label installed=new(){AutoSize=true};
 readonly Label available=new(){AutoSize=true,Text="Checking for updates..."};
 readonly Label status=new(){AutoSize=true,MaximumSize=new Size(720,70),Text="Select your installed Battlefield 2142 folder."};
 readonly ProgressBar progress=new(){Dock=DockStyle.Fill,Minimum=0,Maximum=100};
 readonly TextBox log=new(){Multiline=true,ReadOnly=true,ScrollBars=ScrollBars.Vertical,Dock=DockStyle.Fill,WordWrap=true};
 readonly Button browse=new(){Text="Browse...",AutoSize=true};
 readonly Button install=new(){Text="Install / Update",AutoSize=true,Enabled=false};
 readonly Button repair=new(){Text="Repair",AutoSize=true,Enabled=false};
 readonly Button check=new(){Text="Check for updates",AutoSize=true};
 readonly Button play=new(){Text="Play VR",AutoSize=true};
 readonly Button credits=new(){Text="Credits / licenses",AutoSize=true};
 readonly Button cancel=new(){Text="Cancel download",AutoSize=true,Enabled=false};
 readonly ReleaseFeed feed=new();
 readonly UpdateStore store;
 Release? release;
 bool busy,applying;
 CancellationTokenSource? cancellation;
 public SetupForm(bool preview=false) {
  Text="Battlefield 2142 VR — Installer & Updater";ClientSize=new Size(780,580);MinimumSize=new Size(690,530);StartPosition=FormStartPosition.CenterScreen;
  Font=new Font("Segoe UI",10);BackColor=Color.FromArgb(242,244,247);
  Directory.CreateDirectory(dataRoot);store=new UpdateStore(dataRoot,feed);
  var layout=new TableLayoutPanel{Dock=DockStyle.Fill,Padding=new Padding(22),ColumnCount=1,RowCount=10};
  for(int i=0;i<9;i++)layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));layout.RowStyles.Add(new RowStyle(SizeType.Percent,100));
  layout.Controls.Add(new Label{Text="BATTLEFIELD 2142 VR",Font=new Font("Segoe UI",20,FontStyle.Bold),AutoSize=true,Margin=new Padding(0,0,0,8)});
  layout.Controls.Add(new Label{Text="Install once. Keep this app for updates and repairs.\nRequires your own working BF2142 v1.51 + Reclamation installation.",AutoSize=true,Margin=new Padding(0,0,0,14)});
  var path=new TableLayoutPanel{Dock=DockStyle.Fill,AutoSize=true,ColumnCount=2};path.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));path.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));path.Controls.Add(game,0,0);path.Controls.Add(browse,1,0);layout.Controls.Add(path);
  layout.Controls.Add(installed);layout.Controls.Add(available);
  layout.Controls.Add(new Label{Text="Preview channel · Settings and original backups are preserved.\nClose BF2142 before installing. Connect your headset before Play VR.",AutoSize=true,Margin=new Padding(0,8,0,8)});
  var buttons=new FlowLayoutPanel{Dock=DockStyle.Fill,AutoSize=true,WrapContents=true};foreach(var b in new[]{install,repair,check,play,cancel,credits})buttons.Controls.Add(b);layout.Controls.Add(buttons);
  layout.Controls.Add(progress);layout.Controls.Add(status);layout.Controls.Add(log);Controls.Add(layout);
  browse.Click+=(_,_)=>Browse();game.TextChanged+=(_,_)=>RefreshInstalled();check.Click+=async(_,_)=>await Check();install.Click+=async(_,_)=>await Apply();repair.Click+=async(_,_)=>await Apply();cancel.Click+=(_,_)=>cancellation?.Cancel();
  credits.Click+=(_,_)=>{using var box=new Form{Text="Credits and licenses",Size=new Size(780,580),StartPosition=FormStartPosition.CenterParent};var text=new TextBox{Multiline=true,ReadOnly=true,ScrollBars=ScrollBars.Vertical,Dock=DockStyle.Fill};text.Text="Battlefield 2142 VR by justiceofrip. Based on BFVR by JayBiggsGMG and contributors.\r\nhttps://github.com/justiceofrip/BF2142VR\r\n\r\n";foreach(var name in new[]{"License","DotnetLicense","DotnetNotices"}){using var stream=System.Reflection.Assembly.GetExecutingAssembly().GetManifestResourceStream("BF2142.Installer."+name);if(stream is not null){using var reader=new StreamReader(stream);text.AppendText(reader.ReadToEnd()+"\r\n\r\n");}}box.Controls.Add(text);box.ShowDialog(this);};
  play.Click+=(_,_)=>{try{InstallService.Launch(game.Text);}catch(Exception e){Report(e);}};
  FormClosing+=(_,e)=>{if(busy){e.Cancel=true;status.Text=applying?"Finishing installation safely. Please keep this window open.":"Cancel the download before closing.";}};
  FormClosed+=(_,_)=>feed.Dispose();
  string settings=Path.Combine(dataRoot,"game-path.txt");if(File.Exists(settings))game.Text=File.ReadAllText(settings).Trim();
  RefreshInstalled();Shown+=async(_,_)=>{if(preview){available.Text="Available: 0.2.0-beta.4-test.6 (preview)";status.Text="Interface preview — no changes will be made.";return;}KeepUpdater();await Check();};
 }
 void Browse() {
  using var picker=new OpenFileDialog{Title="Select your installed BF2142.exe",Filter="Battlefield 2142|BF2142.exe",CheckFileExists=true};
  if(picker.ShowDialog(this)==DialogResult.OK){game.Text=Path.GetDirectoryName(picker.FileName)!;UpdateStore.Atomic(Path.Combine(dataRoot,"game-path.txt"),game.Text);}
 }
 void RefreshInstalled() {installed.Text="Installed: "+InstallService.InstalledVersion(game.Text);play.Enabled=!busy&&File.Exists(Path.Combine(game.Text,"BF2142VR","tools","Player.ps1"));}
 void SetBusy(bool value) {busy=value;browse.Enabled=game.Enabled=check.Enabled=!value;install.Enabled=repair.Enabled=!value&&release is not null;cancel.Enabled=value&&!applying;RefreshInstalled();}
 void Append(string text) {if(IsDisposed)return;if(InvokeRequired){BeginInvoke(()=>Append(text));return;}if(log.TextLength>80000)log.Text=log.Text[^40000..];log.AppendText(text+Environment.NewLine);if(text.StartsWith("Preparing weapon")||text.StartsWith("Repairing weapon")||text.StartsWith("Building "))status.Text=text;}
 void Report(Exception error) {status.Text="Could not finish: "+error.Message;Append(error.ToString());MessageBox.Show(this,error.Message,"BF2142 VR",MessageBoxButtons.OK,MessageBoxIcon.Warning);}
 async Task Check() {
  if(busy)return;SetBusy(true);cancel.Enabled=false;status.Text="Checking the BF2142 VR update channel...";
  try {var document=await feed.Descriptor(CancellationToken.None);release=ReleaseFeed.Verify(document,ReleaseFeed.PublicKey(),store.AcceptedRevision);available.Text="Available: "+release.Version;status.Text="Ready. Choose Install / Update, or Repair to verify and restore mod files.";}
  catch(Exception e) {available.Text="Update service unavailable";status.Text="You can still play your installed version. Check for updates again later.";Append(e.Message);}
  finally {SetBusy(false);}
 }
 async Task Apply() {
  if(busy||release is null)return;
  if(!File.Exists(Path.Combine(game.Text,"BF2142.exe"))){Browse();if(!File.Exists(Path.Combine(game.Text,"BF2142.exe")))return;}
  foreach(var p in Process.GetProcessesByName("BF2142")){p.Dispose();MessageBox.Show(this,"Close BF2142, then choose Install / Update again.","BF2142 VR");return;}
  string folder=Path.GetFullPath(game.Text),selectedVersion=release.Version;UpdateStore.Atomic(Path.Combine(dataRoot,"game-path.txt"),folder);
  using var stop=new CancellationTokenSource();cancellation=stop;applying=false;SetBusy(true);string? payload=null;
  try {
   var report=new Progress<(int Percent,string Text)>(p=>{progress.Value=p.Percent;status.Text=p.Text;});
   payload=await store.Acquire(release,Path.Combine(folder,"BF2142VR"),report,stop.Token);stop.Token.ThrowIfCancellationRequested();
   applying=true;cancel.Enabled=false;status.Text="Installing — your original game backups will be retained.";progress.Style=ProgressBarStyle.Marquee;
   await InstallService.Apply(payload,folder,Append);store.Accept(release.Revision);
   try {CreateShortcut("Battlefield 2142 VR",Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System),"WindowsPowerShell","v1.0","powershell.exe"),"-NoProfile -ExecutionPolicy Bypass -File \""+Path.Combine(folder,"BF2142VR","tools","Player.ps1")+"\" -Action Play",Path.Combine(folder,"BF2142VR"));}
   catch(Exception e){Append("Shortcut could not be created. Use Play VR here. "+e.Message);}
   status.Text="Ready — "+selectedVersion+" installed. Connect your headset, then Play VR.";progress.Value=100;
  } catch(OperationCanceledException){status.Text="Download cancelled. Verified files are saved; your game is unchanged.";}
  catch(Exception e){Report(e);}
  finally {if(payload is not null)try{store.RemoveStage(payload);}catch(Exception e){Append("Temporary payload retained: "+e.Message);}progress.Style=ProgressBarStyle.Blocks;applying=false;cancellation=null;SetBusy(false);}
 }
 void KeepUpdater() {
  try {string own=Environment.ProcessPath??throw new IOException("Missing executable path.");string copy=Path.Combine(dataRoot,"BF2142VRSetup.exe");if(!Path.GetFullPath(own).Equals(copy,StringComparison.OrdinalIgnoreCase))File.Copy(own,copy,true);CreateShortcut("BF2142 VR Installer",copy,"",dataRoot);}
  catch(Exception e){Append("Keep the downloaded EXE for future updates. "+e.Message);}
 }
 static void CreateShortcut(string name,string target,string arguments,string directory) {
  // Structured COM arguments, no generated shell script or game credentials.
  var type=Type.GetTypeFromProgID("WScript.Shell")??throw new IOException("Windows shortcuts unavailable.");dynamic shell=Activator.CreateInstance(type)!;
  dynamic shortcut=shell.CreateShortcut(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory),name+".lnk"));shortcut.TargetPath=target;shortcut.Arguments=arguments;shortcut.WorkingDirectory=directory;shortcut.Description="Battlefield 2142 VR";shortcut.Save();
 }
}
