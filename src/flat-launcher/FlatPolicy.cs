using System.Diagnostics;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using BF2142.Community;
namespace BF2142.FlatViewer;

public static class FlatPolicy {
 public const string Feed="https://raw.githubusercontent.com/justiceofrip/BF2142VR/updates/flat.json";
 public const string ServerId="bf2142-flat-viewer";
 public const string Viewer="launcher/BF2142FlatViewer.exe",Helper="BF2142Community.exe";
 public static Package Check(ServerManifest manifest) {
  Validation.Manifest(manifest);
  if(manifest.Id!=ServerId||manifest.DescriptorUrl!=Feed||manifest.Packages.Length!=1||manifest.Packages[0].Mode!="flat")
   throw new InvalidDataException("This update is not for the BF2142 Flat Viewer.");
  var p=manifest.Packages[0];
  foreach(var name in new[]{Viewer,Helper,"runtime/x86/BF2142VRLauncher.exe","runtime/x86/BF2142VRClient.dll"})
   if(!p.Files.Any(f=>f.Path==name))throw new InvalidDataException("The flat update is missing a required launcher file.");
  return p;
 }
 public static bool IsGame(string? path)=>!string.IsNullOrWhiteSpace(path)&&File.Exists(Path.Combine(path,"BF2142.exe"))&&File.Exists(Path.Combine(path,"RendDX9.dll"));
 public static string Redact(string text,params string[] paths) {
  foreach(var path in paths.Where(p=>!string.IsNullOrWhiteSpace(p)).OrderByDescending(p=>p.Length))text=text.Replace(path,"[local path]",StringComparison.OrdinalIgnoreCase);
  text=Regex.Replace(text,@"(?im)\b(password|passwd|token|secret|authorization|api[_-]?key)\s*[:=]\s*[^\r\n]+","$1=[redacted]");
  text=Regex.Replace(text,@"(?i)Bearer\s+[A-Za-z0-9._~+/-]+=*","Bearer [redacted]");
  text=Regex.Replace(text,@"\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b","[email]");
  text=Regex.Replace(text,@"\b(?:\d{1,3}\.){3}\d{1,3}\b","[address]");
  return Regex.Replace(text,@"(?i)[A-Z]:[\\/][^\r\n\""<>|]*","[local path]");
 }
 public static string Friendly(Exception e)=>e switch {
  OperationCanceledException=>"Cancelled. You can try again whenever you're ready.",
  HttpRequestException=>"The update service couldn't be reached. Check your connection, then try again.",
  CryptographicException=>"The download or cached addon failed verification. Click Repair to download a clean copy.",
  UnauthorizedAccessException=>"Windows blocked access to the launcher files. Check the folder permissions and try again.",
  _=>e.Message
 };
 public static string Hash(string path){using var f=File.OpenRead(path);return Convert.ToHexString(SHA256.HashData(f));}
 public static ProcessStartInfo StartInfo(string payload,string manifest,string game) {
  if(!IsGame(game))throw new IOException("Choose your installed BF2142.exe using Browse.");
  var p=new ProcessStartInfo(Path.Combine(payload,Helper)){UseShellExecute=false,CreateNoWindow=true,WorkingDirectory=game,RedirectStandardOutput=true,RedirectStandardError=true};
  foreach(var s in new[]{"join-manifest","--manifest",manifest,"--mode","flat","--game",game})p.ArgumentList.Add(s);
  return p;
 }
 public static bool ReadMicrophone(string file) {
  if(!File.Exists(file))return false;
  var match=Regex.Matches(File.ReadAllText(file),@"(?im)^\s*ProximityMicMuted\s*=\s*([01])\s*$");
  return match.Count>0&&match[^1].Groups[1].Value=="0";
 }
 public static void SaveMicrophone(string file,bool enabled) {
  string text=File.Exists(file)?File.ReadAllText(file):"[VR]\r\nProximityVoice=1\r\n";
  // Only the VR section owns this preference. Keep unrelated settings verbatim.
  var lines=text.Replace("\r\n","\n").Split('\n').ToList();bool vr=false,found=false,section=false;
  for(int i=0;i<lines.Count;i++){
   if(Regex.IsMatch(lines[i],@"^\s*\[.*\]\s*$")){
    if(vr&&!found){lines.Insert(i,"ProximityMicMuted="+(enabled?"0":"1"));i++;found=true;}
    vr=lines[i].Trim().Equals("[VR]",StringComparison.OrdinalIgnoreCase);section|=vr;
   }else if(vr&&Regex.IsMatch(lines[i],@"^\s*ProximityMicMuted\s*=",RegexOptions.IgnoreCase)){
    lines[i]="ProximityMicMuted="+(enabled?"0":"1");found=true;
   }
  }
  if(!found){if(!section)lines.Add("[VR]");lines.Add("ProximityMicMuted="+(enabled?"0":"1"));}
  PackageStore.Atomic(file,Encoding.Unicode.GetPreamble().Concat(Encoding.Unicode.GetBytes(string.Join("\r\n",lines))).ToArray());
 }
}
