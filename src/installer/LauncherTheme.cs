using System.Drawing.Drawing2D;
using System.Runtime.InteropServices;
namespace BF2142.Installer;

public sealed partial class SetupForm {
 static readonly Color Background=Color.FromArgb(12,21,27),PanelColor=Color.FromArgb(19,33,42),
  Border=Color.FromArgb(45,68,80),Ice=Color.FromArgb(126,203,222),Amber=Color.FromArgb(244,174,63),
  Ink=Color.FromArgb(220,231,235),Muted=Color.FromArgb(146,168,181);
 void BuildInterface() {
  Text="BF2142 VR — Launcher";ClientSize=new Size(1000,740);MinimumSize=new Size(940,720);
  StartPosition=FormStartPosition.CenterScreen;AutoScaleMode=AutoScaleMode.Dpi;
  Font=new Font("Segoe UI",10);BackColor=Background;ForeColor=Ink;
  var shell=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=1,RowCount=3,Margin=Padding.Empty};
  shell.RowStyles.Add(new RowStyle(SizeType.Absolute,180));shell.RowStyles.Add(new RowStyle(SizeType.Percent,100));shell.RowStyles.Add(new RowStyle(SizeType.Absolute,36));
  shell.Controls.Add(new BattlefieldBanner{Dock=DockStyle.Fill,Margin=Padding.Empty},0,0);
  var body=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=2,RowCount=1,Padding=new Padding(28,22,28,0),Margin=Padding.Empty};
  body.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,264));body.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));shell.Controls.Add(body,0,1);
  var sidebar=Stack();sidebar.Padding=new Padding(0,0,24,0);body.Controls.Add(sidebar,0,0);
  sidebar.Controls.Add(Caption("ENTER THE BATTLEFIELD",Ice));
  StyleButton(play,true);play.Text="PLAY VR";play.Height=60;play.Font=new Font("Bahnschrift",19,FontStyle.Bold);sidebar.Controls.Add(play);
  sidebar.Controls.Add(Note("Connect your headset and start your OpenXR runtime before launching.",224,43));
  installed.ForeColor=Ink;available.ForeColor=Muted;installed.Font=available.Font=new Font("Segoe UI",9);installed.MaximumSize=available.MaximumSize=new Size(236,42);
  installed.Margin=new Padding(0,12,0,5);available.Margin=new Padding(0,0,0,16);sidebar.Controls.Add(installed);sidebar.Controls.Add(available);
  foreach(var button in new[]{check,install,repair}){StyleButton(button);sidebar.Controls.Add(button);}
  var reclamation=new LinkLabel{Text="BF2142 RECLAMATION\nbattlefield2142.co  ↗",AutoSize=false,Height=57,Dock=DockStyle.Top,LinkColor=Ice,ActiveLinkColor=Amber,VisitedLinkColor=Ice,LinkBehavior=LinkBehavior.HoverUnderline,Font=new Font("Bahnschrift",11),Margin=new Padding(0,20,0,0)};
  reclamation.LinkClicked+=(_,_)=>{try{System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo("https://battlefield2142.co/"){UseShellExecute=true});}catch(Exception e){Report(e);}};
  sidebar.Controls.Add(reclamation);sidebar.Controls.Add(Note("Hub, setup help & multiplayer",230,22));
  var main=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=1,RowCount=8,Margin=Padding.Empty};
  for(int i=0;i<7;i++)main.RowStyles.Add(new RowStyle(SizeType.AutoSize));main.RowStyles.Add(new RowStyle(SizeType.Percent,100));body.Controls.Add(main,1,0);
  main.Controls.Add(Caption("GAME INSTALLATION",Ice));
  var path=new TableLayoutPanel{Dock=DockStyle.Top,AutoSize=true,ColumnCount=2,Margin=new Padding(0,0,0,8)};
  path.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));path.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,103));
  game.BackColor=PanelColor;game.ForeColor=Ink;game.BorderStyle=BorderStyle.FixedSingle;game.Font=new Font("Segoe UI",11);game.Margin=new Padding(0,7,10,0);game.AccessibleName="Battlefield 2142 installation folder";
  StyleButton(browse);browse.Height=38;browse.Margin=Padding.Empty;path.Controls.Add(game,0,0);path.Controls.Add(browse,1,0);main.Controls.Add(path);
  main.Controls.Add(Note("Select the folder containing BF2142.exe. Requires BF2142 v1.51 + Reclamation.",625,40));
  var state=new TableLayoutPanel{Dock=DockStyle.Top,AutoSize=true,ColumnCount=1,BackColor=PanelColor,Padding=new Padding(16,12,16,14),Margin=new Padding(0,5,0,14)};
  state.Controls.Add(Caption("UPDATE STATUS",Amber));status.ForeColor=Ink;status.MaximumSize=new Size(578,72);status.Margin=new Padding(0,0,0,12);state.Controls.Add(status);
  progress.Height=5;progress.Margin=Padding.Empty;state.Controls.Add(progress);main.Controls.Add(state);
  main.Controls.Add(Note("Updates keep your settings and original backups. Close the game before installing.",625,34));
  var actions=new FlowLayoutPanel{Dock=DockStyle.Top,AutoSize=true,Margin=new Padding(0,4,0,12),WrapContents=true};
  foreach(var b in new[]{report,issue,cancel}){StyleButton(b);b.Dock=DockStyle.None;b.AutoSize=true;b.MinimumSize=new Size(0,34);b.Height=34;b.Margin=new Padding(0,0,8,4);actions.Controls.Add(b);}main.Controls.Add(actions);
  main.Controls.Add(Caption("INSTALLATION LOG",Muted));
  log.BackColor=Color.FromArgb(9,17,22);log.ForeColor=Muted;log.Font=new Font("Consolas",9);log.BorderStyle=BorderStyle.FixedSingle;log.Margin=Padding.Empty;main.Controls.Add(log);
  var footer=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=2,Padding=new Padding(28,0,28,0),Margin=Padding.Empty};
  footer.ColumnStyles.Clear();footer.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));footer.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,130));
  footer.Controls.Add(new Label{Text="COMMUNITY VR PORT   /   JUSTICEOFRIP + BFVR CONTRIBUTORS",ForeColor=Muted,Font=new Font("Bahnschrift",8),AutoSize=true,Anchor=AnchorStyles.Left,Margin=Padding.Empty},0,0);
  StyleButton(credits);credits.FlatAppearance.BorderSize=0;credits.BackColor=Background;credits.Font=new Font("Segoe UI",8);credits.Height=28;credits.Margin=Padding.Empty;footer.Controls.Add(credits,1,0);shell.Controls.Add(footer,0,2);Controls.Add(shell);
  Shown+=(_,_)=>{if(OperatingSystem.IsWindowsVersionAtLeast(10,0,17763)){int dark=1;DwmSetWindowAttribute(Handle,20,ref dark,sizeof(int));}};
 }
 static TableLayoutPanel Stack()=>new(){Dock=DockStyle.Fill,ColumnCount=1,AutoScroll=true,Margin=Padding.Empty};
 static Label Caption(string text,Color color)=>new(){Text=text,Font=new Font("Bahnschrift",10,FontStyle.Bold),ForeColor=color,AutoSize=true,Margin=new Padding(0,0,0,12)};
 static Label Note(string text,int width,int height)=>new(){Text=text,ForeColor=Muted,Font=new Font("Segoe UI",9),AutoSize=true,MaximumSize=new Size(width,height),Margin=new Padding(0,6,0,0)};
 static void StyleButton(Button b,bool primary=false) {
  b.AutoSize=false;b.Dock=DockStyle.Top;b.Height=39;b.Margin=new Padding(0,0,0,8);b.FlatStyle=FlatStyle.Flat;
  b.UseVisualStyleBackColor=false;b.BackColor=primary?Amber:PanelColor;b.ForeColor=primary?Background:Ink;
  b.FlatAppearance.BorderColor=primary?Amber:Border;b.FlatAppearance.BorderSize=1;
  b.FlatAppearance.MouseOverBackColor=primary?Color.FromArgb(255,196,92):Color.FromArgb(35,58,70);
  b.FlatAppearance.MouseDownBackColor=primary?Color.FromArgb(220,145,30):Color.FromArgb(28,76,92);
  b.Cursor=Cursors.Hand;b.Padding=new Padding(10,0,10,0);
 }
 [DllImport("dwmapi.dll")] static extern int DwmSetWindowAttribute(IntPtr hwnd,int attribute,ref int value,int size);
}

// BF2142 walker reference image supplied by the project owner for this launcher.
sealed class BattlefieldBanner : Control {
 readonly Image walker,logo;
 public BattlefieldBanner(){walker=LoadArtwork("Walker");logo=LoadArtwork("Logo");SetStyle(ControlStyles.UserPaint|ControlStyles.AllPaintingInWmPaint|ControlStyles.OptimizedDoubleBuffer,true);}
 static Bitmap LoadArtwork(string name){using var stream=typeof(BattlefieldBanner).Assembly.GetManifestResourceStream("BF2142.Installer."+name+".png")??throw new IOException(name+" artwork missing.");using var image=Image.FromStream(stream);return new Bitmap(image);}
 protected override void Dispose(bool disposing){if(disposing){walker.Dispose();logo.Dispose();}base.Dispose(disposing);}
 protected override void OnPaint(PaintEventArgs e) {
  var g=e.Graphics;g.SmoothingMode=SmoothingMode.AntiAlias;
  using var fill=new LinearGradientBrush(ClientRectangle,Color.FromArgb(33,57,70),Color.FromArgb(12,24,31),0f);g.FillRectangle(fill,ClientRectangle);
  using var grid=new Pen(Color.FromArgb(20,140,190,205));
  for(int x=0;x<Width;x+=40)g.DrawLine(grid,x,0,x,Height);for(int y=0;y<Height;y+=40)g.DrawLine(grid,0,y,Width,y);
  // Preserve the supplied artwork and blend it into the launcher header.
  // This is UI composition; the embedded source image stays unchanged.
  var art=new Rectangle(Width-352,-18,280,210);
  g.InterpolationMode=InterpolationMode.HighQualityBicubic;g.DrawImage(walker,art);
  using var veil=new SolidBrush(Color.FromArgb(65,9,28,39));g.FillRectangle(veil,new Rectangle(Width-420,0,420,Height));
  using var fade=new LinearGradientBrush(new Rectangle(Width-422,0,235,Height),Color.FromArgb(255,16,31,40),Color.FromArgb(0,16,31,40),0f);g.FillRectangle(fade,Width-422,0,235,Height);
  using var bottom=new LinearGradientBrush(new Rectangle(Width-420,Height-48,420,48),Color.Transparent,Color.FromArgb(230,12,24,31),90f);g.FillRectangle(bottom,Width-420,Height-48,420,48);
  using var white=new SolidBrush(Color.FromArgb(225,236,238));using var ice=new SolidBrush(Color.FromArgb(135,200,214));using var orange=new SolidBrush(Color.FromArgb(244,174,63));
  using var small=new Font("Bahnschrift",9,FontStyle.Bold);using var vr=new Font("Bahnschrift",15,FontStyle.Bold);
  g.DrawString("WELCOME BACK, SOLDIER",small,ice,28,18);g.DrawImage(logo,new Rectangle(28,44,330,100));
  g.FillRectangle(orange,382,74,52,27);using var dark=new SolidBrush(Color.FromArgb(12,21,27));g.DrawString("VR",vr,dark,390,74);
  using var line=new Pen(Color.FromArgb(91,153,173));g.DrawLine(line,28,157,Width-28,157);
  g.DrawString("VIRTUAL REALITY  /  BETA",small,ice,382,116);
 }
}

sealed class TacticalProgress : Control {
 readonly System.Windows.Forms.Timer timer=new(){Interval=50};int value,phase;ProgressBarStyle style;
 [System.ComponentModel.DefaultValue(0)] public int Minimum{get;set;}
 [System.ComponentModel.DefaultValue(100)] public int Maximum{get;set;}=100;
 [System.ComponentModel.DefaultValue(0)] public int Value{get=>value;set{this.value=Math.Clamp(value,Minimum,Maximum);Invalidate();}}
 [System.ComponentModel.DefaultValue(ProgressBarStyle.Blocks)] public ProgressBarStyle Style{get=>style;set{style=value;timer.Enabled=style==ProgressBarStyle.Marquee&&Visible;Invalidate();}}
 public TacticalProgress(){DoubleBuffered=true;timer.Tick+=(_,_)=>{phase=(phase+3)%120;Invalidate();};}
 protected override void OnVisibleChanged(EventArgs e){base.OnVisibleChanged(e);timer.Enabled=style==ProgressBarStyle.Marquee&&Visible;}
 protected override void OnPaint(PaintEventArgs e){e.Graphics.Clear(Color.FromArgb(39,59,70));using var fill=new SolidBrush(Color.FromArgb(117,201,222));if(style==ProgressBarStyle.Marquee)e.Graphics.FillRectangle(fill,(phase-20)*Width/100,0,Width/5,Height);else e.Graphics.FillRectangle(fill,0,0,(float)(value-Minimum)/Math.Max(1,Maximum-Minimum)*Width,Height);}
 protected override void Dispose(bool disposing){if(disposing)timer.Dispose();base.Dispose(disposing);}
}

sealed class TacticalButton : Button {
 protected override void OnPaint(PaintEventArgs e) {
  if(Enabled){base.OnPaint(e);return;}
  var bg=BackColor.R>150?Color.FromArgb(103,78,41):BackColor;
  e.Graphics.Clear(bg);using var pen=new Pen(FlatAppearance.BorderColor);
  if(FlatAppearance.BorderSize>0)e.Graphics.DrawRectangle(pen,0,0,Width-1,Height-1);
  TextRenderer.DrawText(e.Graphics,Text,Font,ClientRectangle,Color.FromArgb(141,158,164),TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter);
 }
}
