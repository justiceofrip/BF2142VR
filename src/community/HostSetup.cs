using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Text.Json;
namespace BF2142.Community;
public static class HostSetup {
 public static void Create(string root,string game,string runtime,string address,string build,bool local){
  root=Path.GetFullPath(root);game=Path.GetFullPath(game);runtime=Path.GetFullPath(runtime);
  if(Directory.Exists(root))throw new IOException("Choose a new host configuration folder; existing settings are never replaced.");
  if(!Validation.HostName(address)||!Validation.Label(build)||!File.Exists(Path.Combine(game,"BF2142_w32ded.exe"))||!File.Exists(Path.Combine(runtime,"BF2142VRServerLauncher.exe")))throw new IOException("A BF2142 v1.51 Windows dedicated server and the matching addon runtime are required.");
  Directory.CreateDirectory(root);
  using var key=RSA.Create(3072);var request=new CertificateRequest("CN=BF2142 Community Server",key,HashAlgorithmName.SHA256,RSASignaturePadding.Pkcs1);
  request.CertificateExtensions.Add(new X509KeyUsageExtension(X509KeyUsageFlags.DigitalSignature,false));request.CertificateExtensions.Add(new X509EnhancedKeyUsageExtension(new OidCollection{new("1.3.6.1.5.5.7.3.1")},false));
  using var cert=request.CreateSelfSigned(DateTimeOffset.UtcNow.AddMinutes(-5),DateTimeOffset.UtcNow.AddYears(2));string password=Convert.ToBase64String(RandomNumberGenerator.GetBytes(32));
  string certificate=Path.Combine(root,"host.pfx");File.WriteAllBytes(certificate,cert.Export(X509ContentType.Pfx,password));
  uint epoch;do{epoch=Wire.U32(RandomNumberGenerator.GetBytes(4),0);}while(epoch==0);
  var config=new HostConfig{Bind=local?"127.0.0.1":"0.0.0.0",Build=build,Certificate=certificate,CertificatePassword=password,NativeSecret=Convert.ToHexString(RandomNumberGenerator.GetBytes(16)),ProofSecret=Convert.ToHexString(RandomNumberGenerator.GetBytes(32)),ProofEpoch=epoch,GameDirectory=game,RuntimeDirectory=runtime,ServerSettings=Path.Combine(root,"ServerSettings.con"),MapList=Path.Combine(root,"maplist.con"),NetworkConfig=Path.Combine(root,"network.ini"),ProofConfig=Path.Combine(root,"proof.key"),RconPassword=Convert.ToHexString(RandomNumberGenerator.GetBytes(24))};
  File.WriteAllText(config.NetworkConfig,$"[Network]\r\nEnabled=1\r\nPort={config.NativePosePort}\r\nSecret={config.NativeSecret}\r\nMirrorToBot=0\r\n[Voice]\r\nEnabled=1\r\nPort={config.NativeVoicePort}\r\nRangeMetres=20\r\nEnemyProximity=1\r\n",Encoding.ASCII);
  File.WriteAllText(config.ProofConfig,$"{config.ProofSecret}:{config.ProofPort}:{config.ProofEpoch}",Encoding.ASCII);
  File.WriteAllBytes(Path.Combine(root,"host.json"),JsonSerializer.SerializeToUtf8Bytes(config,Json.Default.HostConfig));
  string bind=local?"127.0.0.1":"0.0.0.0";
  File.WriteAllText(config.ServerSettings,$"sv.serverName \"BF2142 VR Crossplay Test\"\r\nsv.internet {(local?0:1)}\r\nsv.serverIP \"{bind}\"\r\nsv.interfaceIP \"{bind}\"\r\nsv.serverPort 17567\r\nsv.gameSpyPort 29900\r\nsv.maxPlayers 16\r\nsv.numPlayersNeededToStart 1\r\nsv.punkBuster 0\r\nsv.adminScript \"bfvr_community_admin\"\r\nsv.allowNATNegotiation 0\r\nsv.autoRecord 0\r\nsv.useGlobalRank 0\r\nsv.useGlobalUnlocks 0\r\nsv.spawnTime 5\r\nsv.startDelay 5\r\nsv.ticketRatio 300\r\nsv.roundsPerMap 3\r\nsv.timeLimit 0\r\nsv.autoBalanceTeam 0\r\nsv.tkPunishEnabled 0\r\nsv.voipEnabled 1\r\nsv.voipQuality 3\r\nsv.voipServerRemote 0\r\nsv.voipServerRemoteIP \"\"\r\nsv.voipServerPort 55125\r\nsv.voipBFClientPort 55123\r\nsv.voipBFServerPort 55124\r\nsv.allowTitanMovement 1\r\n",Encoding.ASCII);
  File.WriteAllText(config.MapList,"mapList.append Suez_Canal gpm_ti 48\r\n",Encoding.ASCII);
  var descriptor=new ServerManifest{Id="bf2142-crossplay-test",Name="BF2142 VR Crossplay Test",Revision=1,Host=address,Build=build,CertificateSha256=Convert.ToHexString(SHA256.HashData(cert.RawData))};
  File.WriteAllBytes(Path.Combine(root,"server.unsigned.json"),JsonSerializer.SerializeToUtf8Bytes(descriptor,Json.Default.ServerManifest));
  Console.WriteLine("Private host configuration created. Keep host.json, network.ini, proof.key and host.pfx on the server; only the signed server descriptor is public.");
 }
 public static async Task Run(HostConfig config,CancellationToken ct){
  Validation.Host(config);
  // The embedded game's Python exposes file I/O but no environment dictionary.
  PackageStore.Atomic(Path.Combine(config.GameDirectory,"admin","bfvr_community.key"),Encoding.ASCII.GetBytes($"{config.ProofSecret}:{config.ProofPort}:{config.ProofEpoch}"));
  using var stop=CancellationTokenSource.CreateLinkedTokenSource(ct);using var bridge=new BridgeServer(config);
  var start=new ProcessStartInfo(Path.Combine(config.RuntimeDirectory,"BF2142VRServerLauncher.exe")){UseShellExecute=false,CreateNoWindow=true,RedirectStandardOutput=true,RedirectStandardError=true,WorkingDirectory=config.GameDirectory};
  foreach(var a in new[]{config.GameDirectory,config.ServerSettings,config.MapList})start.ArgumentList.Add(a);start.Environment["BF2142VR_NETWORK"]=config.NetworkConfig;start.Environment["BF2142VR_COMMUNITY_PROOF"]=config.ProofConfig;
  var bridgeTask=bridge.Run(stop.Token);using var native=Process.Start(start)??throw new IOException("Could not start native server.");
  async Task Forward(StreamReader source,TextWriter target){while(await source.ReadLineAsync() is { } line)await target.WriteLineAsync(line);}
  var output=Forward(native.StandardOutput,Console.Out);var errors=Forward(native.StandardError,Console.Error);
  var nativeExit=native.WaitForExitAsync(CancellationToken.None);
  try{await Task.WhenAny(bridgeTask,nativeExit);if(nativeExit.IsCompleted){await Task.WhenAll(output,errors);if(!ct.IsCancellationRequested&&native.ExitCode!=0)throw new IOException($"Native game server exited unexpectedly (code 0x{native.ExitCode:X8}). Check the native extension log and the server user's Battlefield 2142/dmp folder.");Console.WriteLine($"Native game server exited (code {native.ExitCode}).");}else await bridgeTask;}
  finally{stop.Cancel();try{await bridgeTask;}catch(OperationCanceledException){}if(!native.HasExited){try{await Shutdown(config,CancellationToken.None);await nativeExit.WaitAsync(TimeSpan.FromSeconds(20));}catch(Exception e)when(e is IOException or SocketException or TimeoutException or OperationCanceledException){Console.Error.WriteLine("Native server did not confirm shutdown. Use the host's stop command before restarting.");}}}
 }
 public static async Task Shutdown(HostConfig config,CancellationToken ct){
  using var timeout=CancellationTokenSource.CreateLinkedTokenSource(ct);timeout.CancelAfter(TimeSpan.FromSeconds(5));using var client=new TcpClient();await client.ConnectAsync(IPAddress.Loopback,config.RconPort,timeout.Token);using var stream=client.GetStream();
  async Task<string> Read(string end){var text=new StringBuilder();byte[] b=new byte[1];while(text.Length<8192){int n=await stream.ReadAsync(b,timeout.Token);if(n==0)break;text.Append((char)b[0]);if(text.ToString().EndsWith(end,StringComparison.Ordinal))return text.ToString();}throw new IOException("Invalid local RCON response.");}
  string greeting=await Read("\n\n");const string marker="Digest seed: ";int at=greeting.IndexOf(marker,StringComparison.Ordinal);if(at<0)throw new IOException("Native RCON is unavailable.");string seed=greeting[(at+marker.Length)..].Split('\n')[0].Trim();if(seed.Length>200||seed.Any(c=>!char.IsAsciiLetterOrDigit(c)))throw new IOException("Invalid RCON greeting.");
  // MD5 is the legacy game's localhost-only RCON protocol, never network authentication.
  string digest=Convert.ToHexString(MD5.HashData(Encoding.ASCII.GetBytes(seed+config.RconPassword))).ToLowerInvariant();await stream.WriteAsync(Encoding.ASCII.GetBytes("\x02login "+digest+"\n"),timeout.Token);if(!(await Read("\x04")).Contains("Authentication successful"))throw new IOException("Local RCON authentication failed.");await stream.WriteAsync(Encoding.ASCII.GetBytes("\x02exec exit\n"),timeout.Token);
  // Keep the socket open until the native admin has processed the command.
  _=await Read("\x04");
 }
}
