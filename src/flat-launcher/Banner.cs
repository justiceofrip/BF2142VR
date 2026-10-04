using System.Drawing.Drawing2D;
namespace BF2142.FlatViewer;
sealed class BattlefieldBanner : Control {
 readonly Image walker,logo;
 public BattlefieldBanner(){walker=LoadArtwork("Walker");logo=LoadArtwork("Logo");SetStyle(ControlStyles.UserPaint|ControlStyles.AllPaintingInWmPaint|ControlStyles.OptimizedDoubleBuffer,true);}
 static Bitmap LoadArtwork(string name){using var stream=typeof(BattlefieldBanner).Assembly.GetManifestResourceStream("Flat."+name+".png")??throw new IOException(name+" artwork missing.");using var image=Image.FromStream(stream);return new Bitmap(image);}
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
  g.DrawString("WELCOME BACK, SOLDIER",small,ice,28,18);g.DrawImage(logo,new Rectangle(28,38,300,90));
  g.FillRectangle(orange,358,64,194,29);using var dark=new SolidBrush(Color.FromArgb(12,21,27));g.DrawString("FLAT VIEWER",vr,dark,366,64);
  using var line=new Pen(Color.FromArgb(91,153,173));g.DrawLine(line,28,157,Width-28,157);
  g.DrawString("DESKTOP PLAYERS  /  NO HEADSET NEEDED",small,ice,358,106);
 }
}
