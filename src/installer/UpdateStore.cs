using System.Text.Json;
using System.Collections.Concurrent;
namespace BF2142.Installer;
public sealed class UpdateStore {
 readonly string root;readonly ReleaseFeed feed;
 public UpdateStore(string root,ReleaseFeed feed) {this.root=Path.GetFullPath(root);this.feed=feed;Directory.CreateDirectory(this.root);ReleaseFeed.NoLinks(this.root,this.root);}
 public long AcceptedRevision {get {string path=Path.Combine(root,"accepted-revision.txt");return File.Exists(path)&&long.TryParse(File.ReadAllText(path),out long n)?n:0;}}
 public void Accept(long revision) {if(revision<AcceptedRevision)throw new InvalidDataException("Cannot downgrade the update channel.");Atomic(Path.Combine(root,"accepted-revision.txt"),revision.ToString());}
 public static void Atomic(string path,string text) {string temp=path+"."+Guid.NewGuid().ToString("N")+".tmp";try {File.WriteAllText(temp,text);File.Move(temp,path,true);}finally{if(File.Exists(temp))File.Delete(temp);}}
 public async Task<string> Acquire(Release release,string? installed,IProgress<(int Percent,string Text)> progress,CancellationToken ct) {
  ReleaseFeed.Validate(release);if(release.Revision<AcceptedRevision)throw new InvalidDataException("Older update refused.");
  string cache=Path.Combine(root,"files");Directory.CreateDirectory(cache);ReleaseFeed.NoLinks(root,cache);
  string staged=Path.Combine(root,"payload-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(staged);
  int done=0;ConcurrentDictionary<string,SemaphoreSlim> locks=new(StringComparer.OrdinalIgnoreCase);
  try {
   await Parallel.ForEachAsync(release.Files,new ParallelOptions{MaxDegreeOfParallelism=4,CancellationToken=ct},async (file,token)=> {
    var gate=locks.GetOrAdd(file.Sha256,_=>new SemaphoreSlim(1,1));await gate.WaitAsync(token);
    try {
    string blob=Path.Combine(cache,file.Sha256.ToLowerInvariant()+".bin");ReleaseFeed.NoLinks(root,blob);
    // Hash cache entries every time; names alone never establish integrity.
    if(!ReleaseFeed.Matches(blob,file)) {
     string temporary=Path.Combine(cache,Guid.NewGuid().ToString("N")+".partial");
     try {
      string? existing=installed is null?null:ReleaseFeed.Inside(installed,file.Path);
      if(existing is not null&&ReleaseFeed.Matches(existing,file))File.Copy(existing,temporary);
      else {await using var output=new FileStream(temporary,FileMode.CreateNew,FileAccess.Write,FileShare.None,65536,true);await feed.Download(file.Url,output,file.Size,file.Size,token);}
      if(!ReleaseFeed.Matches(temporary,file))throw new InvalidDataException("Download verification failed: "+file.Path);
      File.Move(temporary,blob,true);
     } finally {if(File.Exists(temporary))File.Delete(temporary);}
    }
    string dest=ReleaseFeed.Inside(staged,file.Path);Directory.CreateDirectory(Path.GetDirectoryName(dest)!);File.Copy(blob,dest);
    if(!ReleaseFeed.Matches(dest,file))throw new InvalidDataException("Staged file verification failed: "+file.Path);
    int finished=Interlocked.Increment(ref done);progress.Report((finished*100/release.Files.Length,$"Preparing update ({finished}/{release.Files.Length})"));
    }finally{gate.Release();}
   });
   VerifyPayloadManifest(staged,release);
   return staged;
  } catch {RemoveStage(staged);throw;}
 }
 public static void VerifyPayloadManifest(string directory,Release release) {
  using var json=JsonDocument.Parse(File.ReadAllBytes(Path.Combine(directory,"payload.json")));
  if(json.RootElement.GetProperty("version").GetString()!=release.Version)throw new InvalidDataException("Payload version mismatch.");
  var files=json.RootElement.GetProperty("files");var expected=release.Files.Where(f=>f.Path!="payload.json").ToDictionary(f=>f.Path,StringComparer.Ordinal);
  var properties=files.EnumerateObject().ToArray();if(properties.Length!=expected.Count)throw new InvalidDataException("Payload file set mismatch.");
  HashSet<string> seen=[];
  foreach(var property in properties)if(!seen.Add(property.Name)||!expected.TryGetValue(property.Name,out var f)||!string.Equals(property.Value.GetString(),f.Sha256,StringComparison.OrdinalIgnoreCase))throw new InvalidDataException("Payload manifest does not match the signed update.");
 }
 public void RemoveStage(string stage) {
  string full=Path.GetFullPath(stage);
  if(Path.GetDirectoryName(full)!=root||!Path.GetFileName(full).StartsWith("payload-",StringComparison.Ordinal)||!Guid.TryParseExact(Path.GetFileName(full)[8..],"N",out _))throw new IOException("Unsafe stage cleanup refused.");
  if(!Directory.Exists(full))return;
  ReleaseFeed.NoLinks(root,full);
  foreach(string entry in Directory.EnumerateFileSystemEntries(full,"*",new EnumerationOptions{RecurseSubdirectories=true,AttributesToSkip=FileAttributes.ReparsePoint}))ReleaseFeed.NoLinks(full,entry);
  Directory.Delete(full,true);
 }
}
