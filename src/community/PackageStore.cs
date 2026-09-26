using System.IO.Compression;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
namespace BF2142.Community;

public sealed class PackageStore {
 readonly string root;readonly HttpClient http;
 public PackageStore(string root,HttpClient? http=null){this.root=Path.GetFullPath(root);Directory.CreateDirectory(this.root);Validation.NoLinks(this.root,this.root);this.http=http??new(new HttpClientHandler{AllowAutoRedirect=false}){Timeout=TimeSpan.FromMinutes(10)};}
 public static ServerManifest Verify(ReadOnlySpan<byte> document,string publicKey){
  if(document.Length>2*1024*1024)throw new InvalidDataException("Manifest is too large.");
  var signed=JsonSerializer.Deserialize(document,Json.Default.SignedManifest)??throw new InvalidDataException("Empty manifest.");
  byte[] data=Convert.FromBase64String(signed.Payload),signature=Convert.FromBase64String(signed.Signature);
  using var key=ECDsa.Create();key.ImportFromPem(publicKey);
  if(key.KeySize!=256||!key.VerifyData(data,signature,HashAlgorithmName.SHA256))throw new CryptographicException("The server descriptor is not signed by a trusted BF2142VR publisher.");
  var m=JsonSerializer.Deserialize(data,Json.Default.ServerManifest)??throw new InvalidDataException("Empty manifest.");Validation.Manifest(m);return m;
 }
 public static byte[] Sign(ServerManifest manifest,string privateKey){Validation.Manifest(manifest);using var key=ECDsa.Create();key.ImportFromPem(privateKey);if(key.KeySize!=256)throw new CryptographicException("Use an ECDSA P-256 publishing key.");var data=JsonSerializer.SerializeToUtf8Bytes(manifest,Json.Default.ServerManifest);return JsonSerializer.SerializeToUtf8Bytes(new SignedManifest{Payload=Convert.ToBase64String(data),Signature=Convert.ToBase64String(key.SignData(data,HashAlgorithmName.SHA256))},Json.Default.SignedManifest);}
 public async Task<byte[]> DownloadManifest(string url,CancellationToken ct){using var stream=new MemoryStream();await Download(url,stream,2*1024*1024,null,ct);return stream.ToArray();}
 public async Task<ServerManifest> Refresh(ServerManifest embedded,string publicKey,CancellationToken ct){
  if(embedded.DescriptorUrl.Length==0)throw new InvalidDataException("This join executable does not contain an update address.");
  var current=Verify(await DownloadManifest(embedded.DescriptorUrl,ct),publicKey);
  if(current.Id!=embedded.Id||current.Revision<embedded.Revision)throw new InvalidDataException("The update is for another server or predates this join executable.");
  CheckRevision(current);return current;
 }
 public void CheckRevision(ServerManifest m){string path=Path.Combine(root,"revisions.json");var revisions=File.Exists(path)?JsonSerializer.Deserialize(File.ReadAllBytes(path),Json.Default.DictionaryStringInt64)??[]:[];if(revisions.TryGetValue(m.Id,out long previous)&&m.Revision<previous)throw new InvalidDataException("Server descriptor is older than a previously accepted revision.");}
 public void AcceptRevision(ServerManifest m){CheckRevision(m);string path=Path.Combine(root,"revisions.json");var revisions=File.Exists(path)?JsonSerializer.Deserialize(File.ReadAllBytes(path),Json.Default.DictionaryStringInt64)??[]:[];revisions[m.Id]=m.Revision;Atomic(path,JsonSerializer.SerializeToUtf8Bytes(revisions,Json.Default.DictionaryStringInt64));}
 public async Task<string> Acquire(Package package,CancellationToken ct){
  string packages=Path.Combine(root,"packages");Directory.CreateDirectory(packages);string destination=Path.Combine(packages,package.Sha256.ToLowerInvariant());Validation.NoLinks(root,destination);
  if(Directory.Exists(destination)){VerifyFiles(destination,package);return destination;}
  string archive=Path.Combine(packages,Guid.NewGuid()+".zip.partial");
  try{Console.WriteLine($"Downloading {package.Mode} addon {package.Version} ({package.Size/1048576.0:F1} MB)...");
   await using(var output=new FileStream(archive,FileMode.CreateNew,FileAccess.Write,FileShare.None,65536,true))await Download(package.Url,output,package.Size,package.Size,ct);
   ct.ThrowIfCancellationRequested();return InstallArchive(archive,package);
  }finally{if(File.Exists(archive))File.Delete(archive);}
 }
 public string InstallArchive(string archive,Package package){
  using(var file=File.OpenRead(archive)){if(file.Length!=package.Size||!CryptographicOperations.FixedTimeEquals(SHA256.HashData(file),Convert.FromHexString(package.Sha256)))throw new CryptographicException("Downloaded package checksum failed.");}
  string packages=Path.Combine(root,"packages");Directory.CreateDirectory(packages);Validation.NoLinks(root,packages);
  string destination=Path.Combine(packages,package.Sha256.ToLowerInvariant()),stage=Path.Combine(packages,"stage-"+Guid.NewGuid());
  Directory.CreateDirectory(stage);
  try{
   var expected=package.Files.ToDictionary(f=>f.Path,StringComparer.OrdinalIgnoreCase);var seen=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
   using(var zip=ZipFile.OpenRead(archive)){
    if(zip.Entries.Count>package.Files.Length*2+32)throw new InvalidDataException("Too many archive entries.");
    foreach(var entry in zip.Entries){
     var name=entry.FullName;if(name.EndsWith('/')){Validation.SafePath(name.TrimEnd('/'));continue;}
     Validation.SafePath(name);
     if(((entry.ExternalAttributes>>16)&0xf000)==0xa000||(entry.ExternalAttributes&(int)FileAttributes.ReparsePoint)!=0||!seen.Add(name)||!expected.TryGetValue(name,out var f)||entry.Length!=f.Size)throw new InvalidDataException("Unexpected or unsafe archive entry.");
     var target=Validation.Inside(stage,f.Path);Directory.CreateDirectory(Path.GetDirectoryName(target)!);
     using var input=entry.Open();using var output=new FileStream(target,FileMode.CreateNew,FileAccess.Write,FileShare.None);using var hash=IncrementalHash.CreateHash(HashAlgorithmName.SHA256);
     byte[] buffer=new byte[65536];long total=0;int read;
     while((read=input.Read(buffer))>0){total=checked(total+read);if(total>f.Size)throw new InvalidDataException("Archive file exceeds advertised size.");hash.AppendData(buffer,0,read);output.Write(buffer,0,read);}
     if(total!=f.Size||!CryptographicOperations.FixedTimeEquals(hash.GetHashAndReset(),Convert.FromHexString(f.Sha256)))throw new CryptographicException("Archive file checksum failed.");
    }
   }
   if(seen.Count!=expected.Count)throw new InvalidDataException("Package is missing required files.");
   VerifyFiles(stage,package);if(Directory.Exists(destination)){VerifyFiles(destination,package);return destination;}Directory.Move(stage,destination);return destination;
  }finally{if(Directory.Exists(stage)){Validation.NoLinks(root,stage);Directory.Delete(stage,true);}}
 }
 public static void VerifyFiles(string directory,Package package){foreach(var f in package.Files){string path=Validation.Inside(directory,f.Path);Validation.NoLinks(directory,path);using var file=File.OpenRead(path);if(file.Length!=f.Size||!CryptographicOperations.FixedTimeEquals(SHA256.HashData(file),Convert.FromHexString(f.Sha256)))throw new CryptographicException("Cached addon was modified: "+f.Path);}}
 async Task Download(string url,Stream output,long maximum,long? exact,CancellationToken ct){var uri=Validation.Https(url);
  for(int redirects=0;redirects<6;redirects++){
   using var response=await http.GetAsync(uri,HttpCompletionOption.ResponseHeadersRead,ct);
   if(response.StatusCode is HttpStatusCode.MovedPermanently or HttpStatusCode.Redirect or HttpStatusCode.RedirectMethod or HttpStatusCode.TemporaryRedirect or HttpStatusCode.PermanentRedirect){uri=Validation.Https(new Uri(uri,response.Headers.Location??throw new IOException("Redirect has no location.")).AbsoluteUri);continue;}
   response.EnsureSuccessStatusCode();long? length=response.Content.Headers.ContentLength;if(length>maximum||exact is not null&&length is not null&&length!=exact)throw new InvalidDataException("Download size does not match the signed descriptor.");
   using var input=await response.Content.ReadAsStreamAsync(ct);byte[] buffer=new byte[65536];long count=0;int n;
   while((n=await input.ReadAsync(buffer,ct))>0){count=checked(count+n);if(count>maximum)throw new InvalidDataException("Download exceeds its size limit.");await output.WriteAsync(buffer.AsMemory(0,n),ct);}if(exact is not null&&count!=exact)throw new EndOfStreamException("Incomplete package download.");return;
  }throw new IOException("Too many download redirects.");
 }
 public static void Atomic(string path,byte[] data){string temp=path+"."+Guid.NewGuid()+".tmp";try{File.WriteAllBytes(temp,data);File.Move(temp,path,true);}finally{if(File.Exists(temp))File.Delete(temp);}}
}
