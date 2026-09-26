using System.Net;
using System.Security.Cryptography;
namespace BF2142.Community;
public static class Validation {
 public static bool Hex(string value,int bytes)=>value.Length==bytes*2&&value.All(Uri.IsHexDigit);
 public static bool Label(string value)=>value.Length is >0 and <=100&&value.All(c=>char.IsAsciiLetterOrDigit(c)||c is '-' or '_' or '.');
 public static bool Port(int p)=>p is >0 and <=65535;
 public static bool HostName(string value)=>value.Length is >0 and <=253&&value.All(c=>char.IsAsciiLetterOrDigit(c)||c is '.' or '-')&&Uri.CheckHostName(value) is UriHostNameType.Dns or UriHostNameType.IPv4;
 public static void Host(HostConfig c){if(!IPAddress.TryParse(c.Bind,out var bind)||bind.AddressFamily!=System.Net.Sockets.AddressFamily.InterNetwork||!new[]{c.ControlPort,c.UdpPort,c.ProofPort,c.NativePosePort,c.NativeVoicePort}.All(Port)||!Hex(c.NativeSecret,16)||!Hex(c.ProofSecret,32)||c.ProofEpoch==0||c.MaxClients is <1 or >256||!Label(c.Build))throw new InvalidDataException("Invalid host configuration.");}
 public static void Manifest(ServerManifest m){
  if(m.Schema!=1||!Label(m.Id)||!Label(m.Build)||m.Revision<1||!HostName(m.Host)||!Port(m.GamePort)||!Port(m.ControlPort)||!Port(m.UdpPort)||!Hex(m.CertificateSha256,32)||!Label(m.Mod)||m.Name.Length>200||m.Packages.Length is <1 or >2)throw new InvalidDataException("Invalid server manifest.");
  if(m.DescriptorUrl.Length!=0)Https(m.DescriptorUrl);
  var modes=new HashSet<string>();foreach(var p in m.Packages){if(p.Mode is not("flat" or "vr")||!modes.Add(p.Mode)||!Label(p.Version)||!Hex(p.Sha256,32)||p.Size is <1 or >1073741824||p.Files.Length is <1 or >8192)throw new InvalidDataException("Invalid package metadata.");Https(p.Url);long total=0;var paths=new HashSet<string>(StringComparer.OrdinalIgnoreCase);foreach(var f in p.Files){SafePath(f.Path);if(!paths.Add(f.Path)||!Hex(f.Sha256,32)||f.Size is <0 or >1073741824)throw new InvalidDataException("Invalid package file.");total=checked(total+f.Size);}if(total>2147483648)throw new InvalidDataException("Package expands beyond the allowed size.");}
 }
 public static Uri Https(string url){if(!Uri.TryCreate(url,UriKind.Absolute,out var uri)||uri.Scheme!="https"||!string.IsNullOrEmpty(uri.UserInfo)||!string.IsNullOrEmpty(uri.Fragment))throw new InvalidDataException("A secure HTTPS URL is required.");return uri;}
 public static void SafePath(string path){
  if(path.Length is <1 or >220||path.Contains('\\')||path.StartsWith('/')||path.EndsWith('/'))throw new InvalidDataException("Unsafe package path.");
  foreach(string part in path.Split('/')){if(part.Length==0||part is "." or ".."||part.EndsWith('.')||part.EndsWith(' ')||part.Any(c=>c<32||c is ':' or '*' or '?' or '"' or '<' or '>' or '|'))throw new InvalidDataException("Unsafe package path.");string stem=part.Split('.')[0].ToUpperInvariant();if(stem is "CON" or "PRN" or "AUX" or "NUL"||stem.Length==4&&(stem.StartsWith("COM")||stem.StartsWith("LPT"))&&(stem[3] is >= '0' and <= '9'||stem[3] is '¹' or '²' or '³'))throw new InvalidDataException("Reserved Windows path.");}
 }
 public static string Inside(string root,string relative){SafePath(relative);string full=Path.GetFullPath(Path.Combine(root,relative));if(!full.StartsWith(Path.GetFullPath(root)+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))throw new InvalidDataException("Path escapes its package.");return full;}
 public static void NoLinks(string root,string path){var current=Path.GetFullPath(path);var parent=Path.GetFullPath(root);while(current.StartsWith(parent,StringComparison.OrdinalIgnoreCase)){if((File.Exists(current)||Directory.Exists(current))&&(File.GetAttributes(current)&FileAttributes.ReparsePoint)!=0)throw new InvalidDataException("Linked package paths are not allowed.");if(current.Equals(parent,StringComparison.OrdinalIgnoreCase))return;current=Path.GetDirectoryName(current)??throw new InvalidDataException("Unsafe package root.");}throw new InvalidDataException("Package path escapes root.");}
}
