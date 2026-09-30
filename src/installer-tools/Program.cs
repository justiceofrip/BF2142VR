using BF2142.Installer;
using System.Security.Cryptography;
using System.Text.Json;
if(args.Length==2&&args[0]=="verify-online") {
 using var feed=new ReleaseFeed();var store=new UpdateStore(args[1],feed);var doc=await feed.Descriptor(CancellationToken.None);var onlineRelease=ReleaseFeed.Verify(doc,ReleaseFeed.PublicKey(),store.AcceptedRevision);
 string path=await store.Acquire(onlineRelease,null,new Progress<(int,string)>(p=>{if(p.Item1%10==0)Console.WriteLine(p.Item2);}),CancellationToken.None);Console.WriteLine("VERIFIED "+onlineRelease.Version+" "+path);return;
}
if(args.Length!=7||args[0]!="prepare")throw new ArgumentException("prepare PAYLOAD KEY_FILE REVISION TAG OUTPUT PREVIOUS_MANIFEST_OR_DASH; verify-online CACHE");
string payload=Path.GetFullPath(args[1]),outDir=Path.GetFullPath(args[5]),tag=args[4];long revision=long.Parse(args[3]);
if(Directory.Exists(outDir))throw new IOException("Use a new output directory.");
if(tag.Any(c=>!char.IsAsciiLetterOrDigit(c)&&c!='.'&&c!='-'))throw new ArgumentException("Unsafe tag.");
using var key=ECDsa.Create();key.ImportFromPem(File.ReadAllText(args[2]));
if(key.ExportSubjectPublicKeyInfoPem()!=ReleaseFeed.PublicKey().Trim())throw new CryptographicException("Publishing key does not match the installer's pinned key.");
var previous=args[6]=="-"?null:ReleaseFeed.Verify(File.ReadAllBytes(args[6]),ReleaseFeed.PublicKey());
if(previous is not null&&revision<=previous.Revision)throw new InvalidDataException("Revision must increase.");
using var document=JsonDocument.Parse(File.ReadAllBytes(Path.Combine(payload,"payload.json")));string version=document.RootElement.GetProperty("version").GetString()!;
var fileMap=document.RootElement.GetProperty("files").EnumerateObject().ToDictionary(p=>p.Name,p=>p.Value.GetString()!);
if(Directory.EnumerateFiles(payload,"*",SearchOption.AllDirectories).Count()!=fileMap.Count+1)throw new InvalidDataException("Unlisted files in payload.");
List<ReleaseFile> files=[];List<(string Source,string Name)> uploads=[];
foreach(string path in fileMap.Keys.Append("payload.json").Order()) {
 string source=ReleaseFeed.Inside(payload,path);using var input=File.OpenRead(source);string hash=Convert.ToHexString(SHA256.HashData(input)).ToLowerInvariant();
 if(path!="payload.json"&&!hash.Equals(fileMap[path],StringComparison.OrdinalIgnoreCase))throw new InvalidDataException("Payload verification failed: "+path);
 string asset="file-"+hash;var prior=previous?.Files.FirstOrDefault(f=>f.Sha256.Equals(hash,StringComparison.OrdinalIgnoreCase)&&f.Size==input.Length);
 string url=prior?.Url??"https://github.com/justiceofrip/BF2142VR/releases/download/"+tag+"/"+asset;
 files.Add(new(path,input.Length,hash,url));if(prior is null)uploads.Add((source,asset));
}
var release=new Release(1,revision,version,"test",1,files.ToArray());ReleaseFeed.Validate(release);UpdateStore.VerifyPayloadManifest(payload,release);
byte[] data=JsonSerializer.SerializeToUtf8Bytes(release);byte[] signed=JsonSerializer.SerializeToUtf8Bytes(new SignedRelease(Convert.ToBase64String(data),Convert.ToBase64String(key.SignData(data,HashAlgorithmName.SHA256))));ReleaseFeed.Verify(signed,ReleaseFeed.PublicKey());
Directory.CreateDirectory(outDir);string assets=Path.Combine(outDir,"files");Directory.CreateDirectory(assets);
foreach(var file in uploads.DistinctBy(f=>f.Name))File.Copy(file.Source,Path.Combine(assets,file.Name));
File.WriteAllBytes(Path.Combine(outDir,"update.json"),signed);File.WriteAllBytes(Path.Combine(outDir,"manifest-readable.json"),JsonSerializer.SerializeToUtf8Bytes(release,new JsonSerializerOptions{WriteIndented=true}));
Console.WriteLine($"Prepared signed {version}: {files.Count} files, {uploads.Select(f=>f.Name).Distinct().Count()} new content assets. No upload performed.");
