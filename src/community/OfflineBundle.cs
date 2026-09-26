using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text;
namespace BF2142.Community;
// Offline flat playtest delivery shares the signed manifest and exact package verifier.
// Nothing is loaded directly from unchecked appended bytes.
public static class OfflineBundle {
 static readonly byte[] Magic=Encoding.ASCII.GetBytes("BFJOIN02");
 public sealed record Payload(byte[] Manifest,long Offset,long Length);
 public static Payload? Read(string path){
  using var file=File.OpenRead(path);if(file.Length<20)return null;
  file.Position=file.Length-20;byte[] tail=new byte[20];file.ReadExactly(tail);
  if(!tail.AsSpan(0,8).SequenceEqual(Magic))return null;
  uint manifest=BinaryPrimitives.ReadUInt32LittleEndian(tail.AsSpan(8));ulong archive=BinaryPrimitives.ReadUInt64LittleEndian(tail.AsSpan(12));
  if(manifest is 0 or >2097152||archive is 0 or >1073741824||(ulong)file.Length<20UL+manifest+archive)throw new InvalidDataException("Invalid bundled payload bounds.");
  long offset=file.Length-20-manifest-(long)archive;
  if(offset<64)throw new InvalidDataException("Bundled executable is incomplete.");
  file.Position=offset+(long)archive;byte[] data=new byte[manifest];file.ReadExactly(data);return new(data,offset,(long)archive);
 }
 public static void Create(string helper,string archive,string descriptor,string output,string publicKey){
  var document=File.ReadAllBytes(descriptor);var m=PackageStore.Verify(document,publicKey);
  if(m.Packages.Length!=1||m.Packages[0].Mode!="flat")throw new InvalidDataException("The offline playtest contains one flat package.");
  var package=m.Packages[0];using(var input=File.OpenRead(archive))if(input.Length!=package.Size||!CryptographicOperations.FixedTimeEquals(SHA256.HashData(input),Convert.FromHexString(package.Sha256)))throw new CryptographicException("Bundled archive checksum failed.");
  using var file=new FileStream(output,FileMode.CreateNew,FileAccess.Write,FileShare.None);
  using(var input=File.OpenRead(helper))input.CopyTo(file);
  using(var input=File.OpenRead(archive))input.CopyTo(file);
  file.Write(document);byte[] footer=new byte[20];Magic.CopyTo(footer,0);
  BinaryPrimitives.WriteUInt32LittleEndian(footer.AsSpan(8),(uint)document.Length);BinaryPrimitives.WriteUInt64LittleEndian(footer.AsSpan(12),(ulong)package.Size);file.Write(footer);
 }
 public static ServerManifest Install(string executable,string root,string publicKey){
  var payload=Read(executable)??throw new InvalidDataException("Bundled payload missing.");
  var m=PackageStore.Verify(payload.Manifest,publicKey);
  if(m.Packages.Length!=1||m.Packages[0].Mode!="flat"||m.Packages[0].Size!=payload.Length)throw new InvalidDataException("Bundled package does not match its signed descriptor.");
  var store=new PackageStore(root);store.CheckRevision(m);
  string archive=Path.Combine(root,Guid.NewGuid()+".zip.partial");
  try{
   using(var file=File.OpenRead(executable))using(var output=new FileStream(archive,FileMode.CreateNew,FileAccess.Write,FileShare.None)){
    file.Position=payload.Offset;byte[] bytes=new byte[65536];long remaining=payload.Length;
    while(remaining>0){int count=file.Read(bytes,0,(int)Math.Min(bytes.Length,remaining));if(count==0)throw new EndOfStreamException("Bundled archive truncated.");output.Write(bytes,0,count);remaining-=count;}
   }
   store.InstallArchive(archive,m.Packages[0]);store.AcceptRevision(m);return m;
  }finally{if(File.Exists(archive))File.Delete(archive);}
 }
}
