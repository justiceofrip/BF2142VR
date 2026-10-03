using System.Text;
using System.Text.RegularExpressions;
namespace BF2142.Installer;

// Installation diagnostics only. Never collect game profiles, account files,
// microphone data, generated assets or arbitrary files from the install root.
public static class Diagnostics {
 public static string Redact(string text, params string[] privatePaths) {
  foreach(string path in privatePaths.Where(p=>!string.IsNullOrWhiteSpace(p)).OrderByDescending(p=>p.Length))
   text=text.Replace(path,"[local path]",StringComparison.OrdinalIgnoreCase).Replace(path.Replace('\\','/'),"[local path]",StringComparison.OrdinalIgnoreCase);
  text=Regex.Replace(text,@"(?im)\b(password|passwd|token|secret|authorization|api[_-]?key)\s*[:=]\s*[^\r\n]+","$1=[redacted]");
  text=Regex.Replace(text,@"(?i)Bearer\s+[A-Za-z0-9._~+/-]+=*","Bearer [redacted]");
  text=Regex.Replace(text,@"\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b","[email]");
  text=Regex.Replace(text,@"\b(?:\d{1,3}\.){3}\d{1,3}\b","[address]");
  // Stack traces may contain build paths or a different Windows username.
  text=Regex.Replace(text,@"(?i)[A-Z]:[\\/][^\r\n\""<>|]*","[local path]");
  return text;
 }
 public static string CreateReport(string installed,string available,string session,string? setupLog,params string[] privatePaths) {
  var b=new StringBuilder();
  b.AppendLine("BF2142 VR installation report");
  b.AppendLine("UTC: "+DateTime.UtcNow.ToString("O"));
  b.AppendLine("Launcher: "+typeof(Diagnostics).Assembly.GetName().Version);
  b.AppendLine("Installed: "+installed);b.AppendLine("Available: "+available);
  b.AppendLine("Windows: "+Environment.OSVersion.Version+" / "+System.Runtime.InteropServices.RuntimeInformation.OSArchitecture);
  b.AppendLine("Paths and common credentials redacted. Review before sharing.");
  b.AppendLine("No game files, accounts, generated models or backups are collected.");
  b.AppendLine("\n--- Launcher ---");b.AppendLine(session);
  if(setupLog is not null){b.AppendLine("\n--- Most recent setup attempt ---");b.AppendLine(setupLog);}
  return Redact(b.ToString(),privatePaths);
 }
 public static string IssueUrl(string version,string reportName) =>
  "https://github.com/justiceofrip/BF2142VR/issues/new?title="+Uri.EscapeDataString("Installation / launcher issue ("+version+")")+
  "&body="+Uri.EscapeDataString("Build: "+version+"\n\nWhat happened?\n\nSteps to reproduce:\n\nExpected result:\n\nAttach the reviewed installation report: "+Path.GetFileName(reportName)+"\nDo not attach game files, account profiles, backups or the WeaponCache.\n");
}
