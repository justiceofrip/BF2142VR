using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
namespace BF2142.Community;
internal static class Program {
 static string Required(Dictionary<string,string> a,string key)=>a.TryGetValue(key,out var v)?v:throw new ArgumentException("Missing --"+key);
 static Dictionary<string,string> Options(string[] args){Dictionary<string,string> result=[];for(int i=1;i<args.Length;i++){if(!args[i].StartsWith("--")||i+1>=args.Length||!result.TryAdd(args[i][2..],args[++i]))throw new ArgumentException("Use --option value pairs.");}return result;}
 public static async Task<int> Main(string[] args){using var stop=new CancellationTokenSource();Console.CancelKeyPress+=(_,e)=>{e.Cancel=true;stop.Cancel();};
  try{
   if(args.Length==0){var offline=OfflineBundle.Read(Environment.ProcessPath??throw new IOException("Executable path missing."));var embedded=offline?.Manifest??Publisher.Embedded();if(embedded is null){Console.WriteLine("Battlefield 2142 Community\n1. Enable one-click server links\n2. Join a server link\n3. Remove server-link registration\n");switch(Console.ReadLine()){case "1":Join.Install();break;case "2":Console.Write("Server link: ");var link=Join.ParseLink(Console.ReadLine()??"");return await FromUrl(link.Url,link.Mode,[],stop.Token);case "3":Join.Unregister();break;}return 0;}
    var manifest=PackageStore.Verify(embedded,Join.PublicKey);var mode=NativeDialog.PlayMode(manifest.Name,manifest.Packages.Any(p=>p.Mode=="vr"),offline is not null);if(mode is null)return 0;manifest=offline is null?await new PackageStore(Join.Home).Refresh(manifest,Join.PublicKey,stop.Token):OfflineBundle.Install(Environment.ProcessPath!,Join.Home,Join.PublicKey);Join.Install();return await Join.Run(manifest,mode,null,null,null,false,stop.Token);
   }
   if(args[0]=="uri"){if(args.Length!=2)throw new ArgumentException("Invalid join link invocation.");var link=Join.ParseLink(args[1]);return await FromUrl(link.Url,link.Mode,[],stop.Token);}
   var a=Options(args);
   switch(args[0]){
    case "install":Join.Install();return 0;
    case "keygen":{string path=Required(a,"out");if(File.Exists(path))throw new IOException("Publishing key already exists.");using var key=ECDsa.Create(ECCurve.NamedCurves.nistP256);File.WriteAllText(path,key.ExportPkcs8PrivateKeyPem());File.WriteAllText(path+".public.pem",key.ExportSubjectPublicKeyInfoPem());Console.WriteLine("Publishing key created. Keep the private file offline and outside distribution folders.");return 0;}
    case "unregister":Join.Unregister();return 0;
    case "init-host":HostSetup.Create(Required(a,"root"),Required(a,"game"),Required(a,"runtime"),Required(a,"address"),Required(a,"build"),a.GetValueOrDefault("local")=="1");return 0;
    case "host-run":{var c=JsonSerializer.Deserialize(File.ReadAllBytes(Required(a,"config")),Json.Default.HostConfig)??throw new InvalidDataException("Host configuration missing.");await HostSetup.Run(c,stop.Token);return 0;}
    case "host-stop":{var c=JsonSerializer.Deserialize(File.ReadAllBytes(Required(a,"config")),Json.Default.HostConfig)??throw new InvalidDataException("Host configuration missing.");await HostSetup.Shutdown(c,stop.Token);return 0;}
    case "host":{var c=JsonSerializer.Deserialize(await File.ReadAllBytesAsync(Required(a,"config"),stop.Token),Json.Default.HostConfig)??throw new InvalidDataException("Host configuration missing.");using var host=new BridgeServer(c);await host.Run(stop.Token);return 0;}
    case "join":return await FromUrl(Required(a,"server"),a.GetValueOrDefault("mode","flat"),a,stop.Token);
    case "join-manifest":{var m=PackageStore.Verify(await File.ReadAllBytesAsync(Required(a,"manifest"),stop.Token),Join.PublicKey);return await Join.Run(m,a.GetValueOrDefault("mode","flat"),a.GetValueOrDefault("game"),null,null,false,stop.Token);}
    case "lab-client":{
     var m=JsonSerializer.Deserialize(await File.ReadAllBytesAsync(Required(a,"manifest"),stop.Token),Json.Default.ServerManifest)??throw new InvalidDataException("Missing lab descriptor.");
     if(m.Host!="127.0.0.1"||!Validation.Hex(m.CertificateSha256,32)||!Validation.Label(m.Build))throw new InvalidDataException("Lab mode is restricted to this PC.");
     return await Join.Launch(m,Path.GetFullPath(Required(a,"payload")),Path.GetFullPath(Required(a,"game")),a.GetValueOrDefault("mode","flat"),Path.GetFullPath(Required(a,"settings")),a.GetValueOrDefault("observer"),a.GetValueOrDefault("desktop")=="1",stop.Token);
    }
    case "sign":{var m=JsonSerializer.Deserialize(File.ReadAllBytes(Required(a,"manifest")),Json.Default.ServerManifest)??throw new InvalidDataException("Missing descriptor.");File.WriteAllBytes(Required(a,"out"),PackageStore.Sign(m,File.ReadAllText(Required(a,"key"))));Console.WriteLine("Signed server descriptor written.");return 0;}
    case "pack":{var p=Publisher.Pack(Required(a,"root"),Required(a,"out"),Required(a,"url"),Required(a,"mode"),Required(a,"version"));File.WriteAllBytes(Required(a,"out")+".json",JsonSerializer.SerializeToUtf8Bytes(p,Json.Default.Package));Console.WriteLine($"Packaged {p.Files.Length} files; SHA256 {p.Sha256}");return 0;}
    case "bundle-flat":OfflineBundle.Create(Required(a,"helper"),Required(a,"archive"),Required(a,"manifest"),Required(a,"out"),Join.PublicKey);return 0;
    case "bundle-join":Publisher.Bundle(Required(a,"helper"),Required(a,"manifest"),Required(a,"out"));return 0;
    default:Console.WriteLine("Commands: install, join --server HTTPS_URL --mode flat|vr, host --config PATH, sign, pack, bundle-join.\nThe published server-specific executable also offers desktop/VR buttons on launch.");return 2;
   }
  }catch(OperationCanceledException){return 0;}catch(Exception e){Console.Error.WriteLine("BF2142 Community: "+e.Message);return 1;}
 }
 static async Task<int> FromUrl(string url,string mode,Dictionary<string,string> options,CancellationToken ct){var store=new PackageStore(Join.Home);var m=PackageStore.Verify(await store.DownloadManifest(url,ct),Join.PublicKey);return await Join.Run(m,mode,options.GetValueOrDefault("game"),null,null,false,ct);}
}
public static class Publisher {
 static readonly byte[] Magic=Encoding.ASCII.GetBytes("BFJOIN01");
 public static Package Pack(string root,string output,string url,string mode,string version){
  root=Path.GetFullPath(root);output=Path.GetFullPath(output);Validation.Https(url);if(!Validation.Label(version)||mode is not("flat" or "vr")||output.StartsWith(root+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))throw new ArgumentException("Invalid package settings.");
  List<PackageFile> files=[];foreach(string f in Directory.EnumerateFiles(root,"*",SearchOption.AllDirectories).Order()){
   Validation.NoLinks(root,f);string relative=Path.GetRelativePath(root,f).Replace('\\','/');Validation.SafePath(relative);
   if(new[]{".pdb",".dmp",".key",".pfx",".pem",".con",".bundledmesh",".bik"}.Contains(Path.GetExtension(f).ToLowerInvariant())||new[]{"BF2142.exe","RendDX9.dll","BF2142_w32ded.exe","Weapons_client.zip","BodyEquipment.bin","LobbyScene.bin","install.json","network.ini"}.Contains(Path.GetFileName(f),StringComparer.OrdinalIgnoreCase))throw new InvalidDataException("Private, game or development file in package: "+relative);
   using var input=File.OpenRead(f);files.Add(new(){Path=relative,Size=input.Length,Sha256=Convert.ToHexString(SHA256.HashData(input))});
  }
  if(files.Count==0)throw new InvalidDataException("Empty package.");
  using(var zip=ZipFile.Open(output,ZipArchiveMode.Create))foreach(var f in files)zip.CreateEntryFromFile(Path.Combine(root,f.Path),f.Path,CompressionLevel.Optimal);
  using var archive=File.OpenRead(output);return new(){Mode=mode,Version=version,Url=url,Size=archive.Length,Sha256=Convert.ToHexString(SHA256.HashData(archive)),Files=files.ToArray()};
 }
 public static void Bundle(string helper,string descriptor,string output){var data=File.ReadAllBytes(descriptor);if(PackageStore.Verify(data,Join.PublicKey).DescriptorUrl.Length==0)throw new InvalidDataException("A server-specific executable requires its signed HTTPS update address.");if(data.Length>2*1024*1024)throw new InvalidDataException("Descriptor too large.");if(File.Exists(output))throw new IOException("Join executable already exists.");File.Copy(helper,output);using var file=new FileStream(output,FileMode.Append);file.Write(data);byte[] footer=new byte[12];Magic.CopyTo(footer,0);Wire.Put(footer,8,(uint)data.Length);file.Write(footer);Console.WriteLine("Server-specific join executable created.");}
 public static byte[]? Embedded(){string? path=Environment.ProcessPath;if(path is null)return null;using var file=File.OpenRead(path);if(file.Length<12)return null;file.Position=file.Length-12;byte[] tail=new byte[12];file.ReadExactly(tail);if(!tail.AsSpan(0,8).SequenceEqual(Magic))return null;uint count=Wire.U32(tail,8);if(count==0||count>2*1024*1024||count>file.Length-12)throw new InvalidDataException("Invalid embedded server descriptor.");file.Position=file.Length-12-count;byte[] data=new byte[count];file.ReadExactly(data);return data;}
}
