using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using System.Security.Cryptography;
namespace BF2142.Community;

// The native server reports the issuer of a private ClientCommand challenge.
// Neither the TLS player's claim nor a local UDP pose is sufficient to authorize it.
public sealed class ProofHub : IDisposable {
 readonly UdpClient udp;readonly byte[] secret;readonly uint epoch;
 readonly object sync=new();Dictionary<int,uint> roster=[];long heartbeat;
 readonly ConcurrentDictionary<string,Pending> pending=new();
 sealed record Pending(int Player,TaskCompletionSource<uint> Answer);
 public ProofHub(int port,byte[] secret,uint epoch){if(secret.Length!=32||epoch==0)throw new ArgumentException("Invalid proof configuration.");this.secret=secret;this.epoch=epoch;udp=new(new IPEndPoint(IPAddress.Loopback,port));}
 public bool Current(int player,uint generation){lock(sync)return Wire.Now-heartbeat<2500&&roster.TryGetValue(player,out var g)&&g==generation;}
 public async Task<uint> Prove(int player,byte[] nonce,CancellationToken ct){
  if(player is <0 or >255||nonce.Length!=16)throw new InvalidDataException("Invalid proof request.");
  string key=Convert.ToHexString(nonce);var request=new Pending(player,new(TaskCreationOptions.RunContinuationsAsynchronously));
  if(!pending.TryAdd(key,request))throw new InvalidOperationException("Duplicate challenge.");
  try{return await request.Answer.Task.WaitAsync(TimeSpan.FromSeconds(15),ct);}finally{pending.TryRemove(key,out _);}
 }
 public async Task Run(CancellationToken ct){while(!ct.IsCancellationRequested){var packet=await udp.ReceiveAsync(ct);if(IPAddress.IsLoopback(packet.RemoteEndPoint.Address))Accept(packet.Buffer);}}
 public bool Accept(ReadOnlySpan<byte> p){
  if(p.Length<41||Wire.U32(p,0)!=0x31504642||!CryptographicOperations.FixedTimeEquals(p.Slice(4,32),secret)||Wire.U32(p,37)!=epoch)return false;
  if(p[36]==2){
   if(p.Length<43)return false;int count=Wire.U16(p,41);if(count>256||p.Length!=43+count*5)return false;
   Dictionary<int,uint> next=[];for(int i=0;i<count;i++){int at=43+i*5;uint g=Wire.U32(p,at+1);if(g==0||!next.TryAdd(p[at],g))return false;}
   lock(sync){roster=next;heartbeat=Wire.Now;}return true;
  }
  if(p[36]!=1||p.Length!=62)return false;
  int player=p[41];uint generation=Wire.U32(p,42);
  if(!Current(player,generation))return false;
  string key=Convert.ToHexString(p.Slice(46,16));
  return pending.TryGetValue(key,out var request)&&request.Player==player&&request.Answer.TrySetResult(generation);
 }
 public void Dispose(){udp.Dispose();foreach(var p in pending.Values)p.Answer.TrySetCanceled();pending.Clear();}
}
