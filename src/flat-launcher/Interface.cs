namespace BF2142.FlatViewer;
public sealed partial class FlatForm {
 static readonly Color Background=Color.FromArgb(12,21,27),PanelColor=Color.FromArgb(19,33,42),Ice=Color.FromArgb(126,203,222),Amber=Color.FromArgb(244,174,63),Ink=Color.FromArgb(220,231,235),Muted=Color.FromArgb(146,168,181);
 void BuildInterface() {
  Text="BF2142 Flat Viewer — Desktop Launcher";ClientSize=new Size(980,720);MinimumSize=new Size(880,650);StartPosition=FormStartPosition.CenterScreen;
  Font=new Font("Segoe UI",10);AutoScaleMode=AutoScaleMode.Dpi;BackColor=Background;ForeColor=Ink;
  var shell=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=1,RowCount=3,Margin=Padding.Empty};
  shell.RowStyles.Add(new RowStyle(SizeType.Absolute,170));shell.RowStyles.Add(new RowStyle(SizeType.Percent,100));shell.RowStyles.Add(new RowStyle(SizeType.Absolute,32));
  shell.Controls.Add(new BattlefieldBanner{Dock=DockStyle.Fill,Margin=Padding.Empty});
  var body=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=2,Padding=new Padding(24,20,24,0),Margin=Padding.Empty};
  body.ColumnStyles.Clear();body.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,244));body.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));shell.Controls.Add(body,0,1);
  var side=new FlowLayoutPanel{Dock=DockStyle.Fill,FlowDirection=FlowDirection.TopDown,WrapContents=false,AutoScroll=true,Padding=new Padding(0,0,18,0),Margin=Padding.Empty};
  play.Size=new Size(220,62);Style(play,true);play.Font=new Font("Bahnschrift",22,FontStyle.Bold);side.Controls.Add(play);
  side.Controls.Add(Note("See VR players' movements and use proximity voice with keyboard and mouse.",220));
  foreach(var button in new[]{check,repair}){button.Size=new Size(220,38);Style(button);side.Controls.Add(button);}
  version.MaximumSize=new Size(220,60);version.ForeColor=Ice;version.Margin=new Padding(0,14,0,14);side.Controls.Add(version);
  var reclamation=new LinkLabel{Text="Need BF2142 / Reclamation help?",AutoSize=true,MaximumSize=new Size(220,60),LinkColor=Ice,VisitedLinkColor=Ice,Margin=new Padding(0,12,0,8)};
  reclamation.LinkClicked+=(_,_)=>OpenUrl("https://battlefield2142.co/");side.Controls.Add(reclamation);
  side.Controls.Add(Note("Requires your own working BF2142 v1.51 + Reclamation installation.",220));
  var credits=new LinkLabel{Text="Credits & licenses",AutoSize=true,LinkColor=Muted,VisitedLinkColor=Muted,Margin=new Padding(0,18,0,0)};
  credits.LinkClicked+=(_,_)=>ShowCredits();side.Controls.Add(credits);body.Controls.Add(side,0,0);
  var main=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=1,RowCount=10,Margin=Padding.Empty};
  for(int i=0;i<9;i++)main.RowStyles.Add(new RowStyle(SizeType.AutoSize));main.RowStyles.Add(new RowStyle(SizeType.Percent,100));
  main.Controls.Add(Note("BATTLEFIELD 2142 INSTALLATION",620,Ice));
  var path=new TableLayoutPanel{Dock=DockStyle.Top,AutoSize=true,ColumnCount=2,Margin=new Padding(0,4,0,6)};
  path.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));path.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,105));
  game.BackColor=PanelColor;game.ForeColor=Ink;game.BorderStyle=BorderStyle.FixedSingle;game.Margin=new Padding(0,7,10,0);game.AccessibleName="Detected Battlefield 2142 installation";game.PlaceholderText="Choose BF2142.exe once using Browse";
  browse.Size=new Size(100,36);Style(browse);path.Controls.Add(game,0,0);path.Controls.Add(browse,1,0);main.Controls.Add(path);
  server.MaximumSize=new Size(620,50);server.ForeColor=Ice;server.Margin=new Padding(0,6,0,6);main.Controls.Add(server);
  main.Controls.Add(Note("Joins our community crossplay server directly. VR gestures require server support.",620));
  mic.Margin=new Padding(0,14,0,14);mic.MaximumSize=new Size(620,60);main.Controls.Add(mic);
  var statusBox=new TableLayoutPanel{Dock=DockStyle.Top,AutoSize=true,ColumnCount=1,BackColor=PanelColor,Padding=new Padding(14),Margin=new Padding(0,0,0,10)};
  state.MaximumSize=new Size(584,90);state.Margin=new Padding(0,0,0,10);statusBox.Controls.Add(state);statusBox.Controls.Add(progress);main.Controls.Add(statusBox);
  main.Controls.Add(Note("Updates install automatically. Your normal game login stays inside BF2142.",620));
  var reports=new FlowLayoutPanel{Dock=DockStyle.Top,AutoSize=true,Margin=new Padding(0,10,0,6)};
  foreach(var button in new[]{copy,save,issue}){button.AutoSize=true;button.Height=34;Style(button);reports.Controls.Add(button);}main.Controls.Add(reports);
  main.Controls.Add(Note("LAUNCHER STATUS · REPORTS STAY ON YOUR PC",620,Muted));
  details.BackColor=Color.FromArgb(9,17,22);details.ForeColor=Muted;details.Font=new Font("Consolas",9);details.BorderStyle=BorderStyle.FixedSingle;details.Margin=new Padding(0,5,0,8);main.Controls.Add(details);
  body.Controls.Add(main,1,0);
  shell.Controls.Add(new Label{Text="JUSTICEOFRIP + BFVR CONTRIBUTORS  ·  COMMUNITY BETA  ·  NOT AFFILIATED WITH EA / DICE",Dock=DockStyle.Fill,TextAlign=ContentAlignment.MiddleLeft,Padding=new Padding(24,0,0,0),ForeColor=Muted,Font=new Font("Segoe UI",8)},0,2);
  Controls.Add(shell);
 }
 static Label Note(string text,int width,Color? color=null)=>new(){Text=text,AutoSize=true,MaximumSize=new Size(width,0),ForeColor=color??Muted,Margin=new Padding(0,5,0,10)};
 static void Style(Button button,bool primary=false){button.FlatStyle=FlatStyle.Flat;button.BackColor=primary?Amber:PanelColor;button.ForeColor=primary?Background:Ink;button.FlatAppearance.BorderColor=primary?Amber:Color.FromArgb(45,68,80);button.Padding=new Padding(9,0,9,0);button.Cursor=Cursors.Hand;button.Margin=new Padding(0,0,8,8);}
 void ShowCredits(){using var box=new Form{Text="BF2142 Flat Viewer — credits",Size=new Size(780,580),StartPosition=FormStartPosition.CenterParent};var text=new TextBox{Multiline=true,ReadOnly=true,ScrollBars=ScrollBars.Vertical,Dock=DockStyle.Fill};text.Text="BF2142 Flat Viewer by justiceofrip. Based on BFVR by JayBiggsGMG and contributors.\r\nBattlefield 2142 imagery: EA / DICE. Community project; not endorsed by EA.\r\nhttps://github.com/justiceofrip/BF2142VR\r\n\r\n";foreach(var name in new[]{"License","DotnetLicense","DotnetNotices"}){using var stream=typeof(FlatForm).Assembly.GetManifestResourceStream("Flat."+name);if(stream is not null){using var reader=new StreamReader(stream);text.AppendText(reader.ReadToEnd()+"\r\n\r\n");}}box.Controls.Add(text);box.ShowDialog(this);}
}
