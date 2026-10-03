namespace BF2142.Installer;

public sealed record InstallationCheck(bool SameVersion,string[] Damaged) {
 public string Message => !SameVersion ? "Update available. Choose Install / Update." : Damaged.Length==0
  ? "Up to date. Installed mod files match the verified release."
  : "Repair needed: "+Damaged.Length+" missing or changed mod file(s). Choose Repair.";
}
public static class InstalledRelease {
 public static InstallationCheck Check(string root,string version,Release release) {
  if(version!=release.Version)return new(false,[]);
  // Compare to the publisher-signed file set, not just the local version label.
  // Player settings and locally generated game assets are outside that set.
  var damaged=new List<string>();
  foreach(var file in release.Files) {
   try {if(!ReleaseFeed.Matches(ReleaseFeed.Inside(root,file.Path),file))damaged.Add(file.Path);}
   catch(IOException){damaged.Add(file.Path);}catch(UnauthorizedAccessException){damaged.Add(file.Path);}
  }
  return new(true,damaged.ToArray());
 }
}
