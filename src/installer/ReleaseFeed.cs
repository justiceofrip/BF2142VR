using System.Net;
using System.Reflection;
using System.Security.Cryptography;
using System.Text.Json;
namespace BF2142.Installer;

public sealed record ReleaseFile(string Path, long Size, string Sha256, string Url);
public sealed record Release(int Schema, long Revision, string Version, string Channel, int MinimumUpdater, ReleaseFile[] Files);
public sealed record SignedRelease(string Payload, string Signature);
public sealed class ReleaseFeed : IDisposable {
 public const string FeedUrl="https://raw.githubusercontent.com/justiceofrip/BF2142VR/updates/test.json";
 public const string ReleasesUrl="https://github.com/justiceofrip/BF2142VR/releases";
 public const int UpdaterVersion=1;
 readonly HttpClient http;
 public ReleaseFeed(HttpMessageHandler? handler=null) {
  http=new(handler??new HttpClientHandler{AllowAutoRedirect=false}){Timeout=TimeSpan.FromMinutes(5)};
  http.DefaultRequestHeaders.UserAgent.ParseAdd("BF2142VRSetup/1.0");
 }
 public void Dispose()=>http.Dispose();
 public static string PublicKey() {
  using var stream=Assembly.GetExecutingAssembly().GetManifestResourceStream("BF2142.Installer.Publisher.pem")??throw new IOException("Publisher key missing.");
  using var reader=new StreamReader(stream);return reader.ReadToEnd();
 }
 public static Release Verify(byte[] document,string key,long minimumRevision=0) {
  if(document.Length>2*1024*1024)throw new InvalidDataException("Update descriptor is too large.");
  var signed=JsonSerializer.Deserialize<SignedRelease>(document)??throw new InvalidDataException("Missing update signature.");
  var data=Convert.FromBase64String(signed.Payload);var signature=Convert.FromBase64String(signed.Signature);
  using var verifier=ECDsa.Create();verifier.ImportFromPem(key);
  if(verifier.KeySize!=256||!verifier.VerifyData(data,signature,HashAlgorithmName.SHA256))throw new CryptographicException("Update signature is invalid. Nothing was installed.");
  var release=JsonSerializer.Deserialize<Release>(data)??throw new InvalidDataException("Empty update descriptor.");
  Validate(release);
  if(release.Revision<minimumRevision)throw new InvalidDataException("This update is older than the last successfully installed release.");
  if(release.MinimumUpdater>UpdaterVersion)throw new InvalidDataException("Download the newer BF2142VRSetup.exe from GitHub to use this update.");
  return release;
 }
 public static void Validate(Release release) {
  if(release.Schema!=1||release.Revision<1||release.Channel!="test"||release.MinimumUpdater<1||string.IsNullOrWhiteSpace(release.Version)||release.Version.Length>80||release.Version.Any(c=>!char.IsAsciiLetterOrDigit(c)&&c!='.'&&c!='-')||release.Files is null||release.Files.Length is <1 or >512)
   throw new InvalidDataException("Invalid update descriptor.");
  HashSet<string> names=new(StringComparer.OrdinalIgnoreCase);long total=0;
  foreach(var file in release.Files) {
   if(file is null)throw new InvalidDataException("Missing file entry.");
   SafePath(file.Path);if(!names.Add(file.Path))throw new InvalidDataException("Duplicate update file.");
   if(file.Size is <0 or >134217728||file.Sha256 is null||file.Sha256.Length!=64||!file.Sha256.All(Uri.IsHexDigit))throw new InvalidDataException("Invalid file identity.");
   var uri=AllowedUrl(file.Url,false);
   if(uri.Host!="github.com"||!uri.AbsolutePath.StartsWith("/justiceofrip/BF2142VR/releases/download/",StringComparison.Ordinal))throw new InvalidDataException("Update file is not hosted by the BF2142VR release repository.");
   total=checked(total+file.Size);
  }
  if(total>512L*1024*1024)throw new InvalidDataException("Update exceeds the size limit.");
  foreach(string required in new[]{"payload.json","tools/SetupAssets.exe","tools/Player.ps1","runtime/x86/BF2142VRLauncher.exe","runtime/x86/BF2142VRClient.dll","runtime/x64/BFVRPresenter.exe"})
   if(!names.Contains(required))throw new InvalidDataException("Incomplete update: "+required);
 }
 public static void SafePath(string path) {
  if(string.IsNullOrEmpty(path)||path.Length>220||path.Contains('\\')||path.Contains(':')||path.StartsWith('/')||path.Any(char.IsControl))throw new InvalidDataException("Unsafe update path.");
  foreach(string part in path.Split('/')) {
   string stem=part.Split('.')[0].ToUpperInvariant();
   if(part.Length==0||part is "." or ".."||part.EndsWith('.')||part.EndsWith(' ')||part.IndexOfAny(Path.GetInvalidFileNameChars())>=0||stem is "CON" or "PRN" or "AUX" or "NUL"||((stem.StartsWith("COM")||stem.StartsWith("LPT"))&&stem.Length==4&&char.IsDigit(stem[3])))throw new InvalidDataException("Unsafe update path.");
  }
  if(new[]{"BF2142.exe","RendDX9.dll","Weapons_client.zip","BodyEquipment.bin","LobbyScene.bin","install.json","BF2142VR.ini"}.Contains(Path.GetFileName(path),StringComparer.OrdinalIgnoreCase)||path.StartsWith("backups/",StringComparison.OrdinalIgnoreCase)||path.StartsWith("generated/",StringComparison.OrdinalIgnoreCase))throw new InvalidDataException("An update cannot distribute game files, backups or player settings.");
 }
 public static string Inside(string root,string relative) {
  SafePath(relative);root=Path.GetFullPath(root);string result=Path.GetFullPath(Path.Combine(root,relative));
  if(!result.StartsWith(root+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))throw new InvalidDataException("Path leaves update directory.");
  NoLinks(root,result);return result;
 }
 public static void NoLinks(string root,string target) {
  root=Path.GetFullPath(root);target=Path.GetFullPath(target);
  if(target!=root&&!target.StartsWith(root+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))throw new InvalidDataException("Path leaves update directory.");
  for(string? current=target;current!=null;current=Path.GetDirectoryName(current)) {
   if((File.Exists(current)||Directory.Exists(current))&&(File.GetAttributes(current)&FileAttributes.ReparsePoint)!=0)throw new IOException("Linked update/install directories are unsupported: "+current);
   // Check root ancestors too so a linked install root cannot redirect writes.
  }
 }
 public static bool Matches(string path,ReleaseFile file) {
  if(!File.Exists(path)||new FileInfo(path).Length!=file.Size)return false;
  using var stream=File.OpenRead(path);return Convert.ToHexString(SHA256.HashData(stream)).Equals(file.Sha256,StringComparison.OrdinalIgnoreCase);
 }
 public static Uri AllowedUrl(string url,bool redirects) {
  if(!Uri.TryCreate(url,UriKind.Absolute,out var uri)||uri.Scheme!="https"||!uri.IsDefaultPort||uri.UserInfo.Length!=0||uri.Fragment.Length!=0)throw new InvalidDataException("Updates require HTTPS.");
  if(uri.Host is not ("github.com" or "raw.githubusercontent.com")&&!(redirects&&uri.Host is "release-assets.githubusercontent.com" or "objects.githubusercontent.com"))throw new InvalidDataException("Untrusted update host.");
  return uri;
 }
 public async Task<byte[]> Descriptor(CancellationToken ct) {
  using var output=new MemoryStream();await Download(FeedUrl,output,2*1024*1024,null,ct);return output.ToArray();
 }
 public async Task Download(string url,Stream output,long maximum,long? exact,CancellationToken ct) {
  Uri uri=AllowedUrl(url,false);
  for(int i=0;i<6;i++) {
   using var response=await http.GetAsync(uri,HttpCompletionOption.ResponseHeadersRead,ct);
   if(response.StatusCode is HttpStatusCode.MovedPermanently or HttpStatusCode.Redirect or HttpStatusCode.RedirectMethod or HttpStatusCode.TemporaryRedirect or HttpStatusCode.PermanentRedirect) {uri=AllowedUrl(new Uri(uri,response.Headers.Location??throw new IOException("Missing redirect.")).AbsoluteUri,true);continue;}
   response.EnsureSuccessStatusCode();long? length=response.Content.Headers.ContentLength;
   if(length>maximum||(exact.HasValue&&length.HasValue&&length!=exact))throw new InvalidDataException("Unexpected download size.");
   using var input=await response.Content.ReadAsStreamAsync(ct);byte[] buffer=new byte[65536];long count=0;int n;
   while((n=await input.ReadAsync(buffer,ct))>0) {count=checked(count+n);if(count>maximum)throw new InvalidDataException("Download exceeds declared size.");await output.WriteAsync(buffer.AsMemory(0,n),ct);}
   if(exact.HasValue&&count!=exact)throw new EndOfStreamException("Download was interrupted. Run Update again to resume verified progress.");return;
  }
  throw new IOException("Too many redirects.");
 }
}
