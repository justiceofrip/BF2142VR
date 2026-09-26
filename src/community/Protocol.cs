using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.Json.Serialization;
namespace BF2142.Community;

public sealed record Control {
 public int Version {get;init;}=1;
 public string Kind {get;init;}="";
 public string Build {get;init;}="";
 public int Player {get;init;}=-1;
 public ulong NativeSession {get;init;}
 public string Nonce {get;init;}="";
 public ulong Session {get;init;}
 public string SendKey {get;init;}="";
 public string ReceiveKey {get;init;}="";
 public long Time {get;init;}
 public long Echo {get;init;}
 public int UdpPort {get;init;}
 public string Message {get;init;}="";
}
public sealed record PackageFile {public string Path {get;init;}="";public string Sha256 {get;init;}="";public long Size {get;init;}}
public sealed record Package {
 public string Mode {get;init;}="flat";public string Version {get;init;}="";public string Url {get;init;}="";
 public string Sha256 {get;init;}="";public long Size {get;init;}public PackageFile[] Files {get;init;}=[];
}
public sealed record ServerManifest {
 public int Schema {get;init;}=1;public string Id {get;init;}="";public string Name {get;init;}="";public long Revision {get;init;}
 public string DescriptorUrl {get;init;}="";
 public string Host {get;init;}="";public int GamePort {get;init;}=17567;public int ControlPort {get;init;}=17570;public int UdpPort {get;init;}=17571;
 public string CertificateSha256 {get;init;}="";public string Mod {get;init;}="bf2142";public string Build {get;init;}="";public Package[] Packages {get;init;}=[];
}
public sealed record SignedManifest {public string Payload {get;init;}="";public string Signature {get;init;}="";}
public sealed record HostConfig {
 public string Bind {get;init;}="127.0.0.1";public int ControlPort {get;init;}=17570;public int UdpPort {get;init;}=17571;public int ProofPort {get;init;}=17572;
 public int NativePosePort {get;init;}=17568;public int NativeVoicePort {get;init;}=17569;public int MaxClients {get;init;}=32;public string Build {get;init;}="";
 public string Certificate {get;init;}="";public string CertificatePassword {get;init;}="";public string NativeSecret {get;init;}="";public string ProofSecret {get;init;}="";public uint ProofEpoch {get;init;}
 public int RconPort {get;init;}=47142;public string RconPassword {get;init;}="";
 public string GameDirectory {get;init;}="";public string RuntimeDirectory {get;init;}="";public string ServerSettings {get;init;}="";public string MapList {get;init;}="";public string NetworkConfig {get;init;}="";public string ProofConfig {get;init;}="";
}
[JsonSerializable(typeof(Package))][JsonSerializable(typeof(Control))][JsonSerializable(typeof(ServerManifest))][JsonSerializable(typeof(SignedManifest))][JsonSerializable(typeof(HostConfig))]
[JsonSerializable(typeof(Dictionary<string,long>))]
internal partial class Json : JsonSerializerContext {}

public static class Wire {
 public const byte Pose=1, Voice=2;
 public static long Now=>DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
 public static uint U32(ReadOnlySpan<byte> p,int at)=>BinaryPrimitives.ReadUInt32LittleEndian(p[at..]);
 public static ulong U64(ReadOnlySpan<byte> p,int at)=>BinaryPrimitives.ReadUInt64LittleEndian(p[at..]);
 public static ushort U16(ReadOnlySpan<byte> p,int at)=>BinaryPrimitives.ReadUInt16LittleEndian(p[at..]);
 public static void Put(Span<byte> p,int at,uint v)=>BinaryPrimitives.WriteUInt32LittleEndian(p[at..],v);
 public static void Put(Span<byte> p,int at,ulong v)=>BinaryPrimitives.WriteUInt64LittleEndian(p[at..],v);
 public static bool Native(byte kind,ReadOnlySpan<byte> p,ReadOnlySpan<byte> secret,bool fromClient,out int player,out ulong session){
  player=-1;session=0;if(p.Length<40||secret.Length!=16||!CryptographicOperations.FixedTimeEquals(p.Slice(24,16),secret))return false;
  var k=U16(p,6);
  if(kind==Pose){if(p.Length!=616||U32(p,0)!=0x31524e42||U16(p,4)!=4||U32(p,8)!=616||(fromClient?k is not(1 or 4):k is not(2 or 3)))return false;player=(int)U32(p,40);session=U64(p,16);}
  else if(kind==Voice){if(p.Length<60||p.Length>460||U32(p,0)!=0x31564e42||U16(p,4)!=1||p.Length!=60+U16(p,56)||(fromClient?k is not(1 or 2):k!=3))return false;player=(int)U32(p,8);session=U64(p,16);}
  else return false;
  return player is >=0 and <256 && session!=0;
 }
 public static byte[] Challenge(int player,ulong session,byte[] secret,byte[] nonce){
  if(player is <0 or >255||session==0||secret.Length!=16||nonce.Length!=16)throw new InvalidDataException("Invalid native challenge.");
  byte[] b=new byte[56];Put(b,0,0x31414642u);Put(b,4,1u);Put(b,8,(uint)player);Put(b,16,session);secret.CopyTo(b,24);nonce.CopyTo(b,40);return b;
 }
 public static async Task Write(Stream stream,Control value,CancellationToken ct){
  var data=JsonSerializer.SerializeToUtf8Bytes(value,Json.Default.Control);if(data.Length>8192)throw new InvalidDataException("Control message too large.");
  var length=new byte[4];Put(length,0,(uint)data.Length);await stream.WriteAsync(length,ct);await stream.WriteAsync(data,ct);await stream.FlushAsync(ct);
 }
 public static async Task<Control> Read(Stream stream,CancellationToken ct){
  byte[] length=new byte[4];await stream.ReadExactlyAsync(length,ct);uint n=U32(length,0);if(n==0||n>8192)throw new InvalidDataException("Invalid control frame.");
  byte[] data=new byte[n];await stream.ReadExactlyAsync(data,ct);var v=JsonSerializer.Deserialize(data,Json.Default.Control)??throw new InvalidDataException("Empty control frame.");if(v.Version!=1)throw new InvalidDataException("Bridge protocol mismatch.");return v;
 }
}
public sealed class ReplayWindow {
 ulong largest,seen;
 public bool Accept(ulong sequence){if(sequence==0)return false;if(sequence>largest){var d=sequence-largest;seen=d>=64?1:(seen<<(int)d)|1;largest=sequence;return true;}var old=largest-sequence;if(old>=64||((seen>>(int)old)&1)!=0)return false;seen|=1UL<<(int)old;return true;}
}
public sealed class DatagramCipher : IDisposable {
 public const int Header=32,Tag=16;
 readonly AesGcm aes;readonly ulong session;readonly uint direction;readonly ReplayWindow replay=new();ulong sequence;
 readonly object sync=new();
 public DatagramCipher(byte[] key,ulong session,uint direction){if(key.Length!=32||session==0)throw new ArgumentException("Invalid datagram key/session.");aes=new AesGcm(key,Tag);this.session=session;this.direction=direction;}
 public byte[] Seal(byte kind,byte[] payload,long time){lock(sync){if(payload.Length>1000||kind is not(1 or 2)||sequence==ulong.MaxValue)throw new InvalidDataException("Invalid datagram.");
  byte[] b=new byte[Header+payload.Length+Tag];Wire.Put(b,0,0x31444642u);b[4]=1;b[5]=kind;BinaryPrimitives.WriteUInt16LittleEndian(b.AsSpan(6),checked((ushort)payload.Length));Wire.Put(b,8,session);Wire.Put(b,16,++sequence);BinaryPrimitives.WriteInt64LittleEndian(b.AsSpan(24),time);
  Span<byte> nonce=stackalloc byte[12];Wire.Put(nonce,0,direction);Wire.Put(nonce,4,sequence);aes.Encrypt(nonce,payload,b.AsSpan(Header,payload.Length),b.AsSpan(Header+payload.Length,Tag),b.AsSpan(0,Header));return b;}}
 public bool Open(ReadOnlySpan<byte> b,long now,out byte kind,out byte[] payload){lock(sync){kind=0;payload=[];
  if(b.Length<Header+Tag||b.Length>Header+1000+Tag||Wire.U32(b,0)!=0x31444642||b[4]!=1||b[5] is not(1 or 2)||Wire.U64(b,8)!=session||b.Length!=Header+Wire.U16(b,6)+Tag)return false;
  long stamp=BinaryPrimitives.ReadInt64LittleEndian(b[24..]);if(stamp<now-500||stamp>now+250)return false;
  var seq=Wire.U64(b,16);if(seq==0)return false;var clear=new byte[Wire.U16(b,6)];Span<byte> nonce=stackalloc byte[12];Wire.Put(nonce,0,direction);Wire.Put(nonce,4,seq);
  try{aes.Decrypt(nonce,b.Slice(Header,clear.Length),b.Slice(Header+clear.Length,Tag),clear,b[..Header]);}catch(CryptographicException){return false;}
  if(!replay.Accept(seq))return false;kind=b[5];payload=clear;return true;}}
 public void Dispose()=>aes.Dispose();
}
public sealed class TokenBucket {
 readonly double rate,burst;double tokens;long at=Wire.Now;readonly object sync=new();
 public TokenBucket(double rate,double burst){this.rate=rate;this.burst=burst;tokens=burst;}
 public bool Take(){lock(sync){long now=Wire.Now;tokens=Math.Min(burst,tokens+Math.Max(0,now-at)*rate/1000);at=now;if(tokens<1)return false;tokens--;return true;}}
}
