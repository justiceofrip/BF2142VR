# API reference: https://github.com/ValveSoftware/openvr/blob/master/headers/openvr_capi.h
# Runs as a utility process; never initializes OpenVR inside the OpenXR presenter.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$Manifest,
 [string]$Presenter,
 [uint32]$PresenterProcessId=0,
 [switch]$Watch,
 [string]$Log
)
$ErrorActionPreference='Stop'
try {
 if(-not [Environment]::Is64BitProcess){throw 'SteamVR branding requires 64-bit PowerShell.'}
 $manifestPath=(Resolve-Path -LiteralPath $Manifest).Path
 $entry=(Get-Content -LiteralPath $manifestPath -Raw|ConvertFrom-Json).applications[0]
 if($entry.app_key -ne 'bfvr.battlefield2142'){throw 'Unexpected application key.'}
 if(-not (Test-Path -LiteralPath $entry.image_path -PathType Leaf)){throw '2142 logo is missing.'}
 $paths=Get-Content -LiteralPath (Join-Path $env:LOCALAPPDATA 'openvr\openvrpaths.vrpath') -Raw|ConvertFrom-Json
 $library=Join-Path $paths.runtime[0] 'bin\win64\openvr_api.dll'
 if(-not (Test-Path -LiteralPath $library -PathType Leaf)){throw 'Installed OpenVR utility library unavailable.'}
 Add-Type -TypeDefinition @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class BF2142SteamVRBranding {
 [DllImport("kernel32",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr LoadLibraryEx(string path,IntPtr file,uint flags);
 [DllImport("kernel32",CharSet=CharSet.Ansi)] static extern IntPtr GetProcAddress(IntPtr library,string name);
 [DllImport("kernel32")] static extern bool FreeLibrary(IntPtr library);
 [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate uint Init(ref int error,int type);
 [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void Shutdown();
 [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate IntPtr Interface([MarshalAs(UnmanagedType.LPStr)]string name,ref int error);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate int AddManifest(IntPtr path,[MarshalAs(UnmanagedType.I1)]bool temporary);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate int Identify(uint processId,IntPtr key);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate int KeyByProcess(uint processId,IntPtr output,uint capacity);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate uint Property(IntPtr key,int property,IntPtr output,uint capacity,ref int error);
 static T Bind<T>(IntPtr address){if(address==IntPtr.Zero)throw new InvalidOperationException("OpenVR entry unavailable");return (T)(object)Marshal.GetDelegateForFunctionPointer(address,typeof(T));}
 static IntPtr Utf8(string value){byte[] data=Encoding.UTF8.GetBytes(value+"\0");IntPtr result=Marshal.AllocHGlobal(data.Length);Marshal.Copy(data,0,result,data.Length);return result;}
 static string Text(IntPtr buffer){int n=0;while(n<4096&&Marshal.ReadByte(buffer,n)!=0)++n;byte[] data=new byte[n];Marshal.Copy(buffer,data,0,n);return Encoding.UTF8.GetString(data);}
 public static string Apply(string libraryPath,string manifest,uint processId){
  IntPtr library=LoadLibraryEx(libraryPath,IntPtr.Zero,0x1100);if(library==IntPtr.Zero)throw new InvalidOperationException("Cannot load installed OpenVR library");
  Shutdown shutdown=null;bool started=false;IntPtr path=IntPtr.Zero,key=IntPtr.Zero,buffer=IntPtr.Zero;
  try {
   var init=Bind<Init>(GetProcAddress(library,"VR_InitInternal"));shutdown=Bind<Shutdown>(GetProcAddress(library,"VR_ShutdownInternal"));
   var get=Bind<Interface>(GetProcAddress(library,"VR_GetGenericInterface"));int error=0;init(ref error,4);if(error!=0)throw new InvalidOperationException("OpenVR utility init "+error);started=true;
   IntPtr table=get("FnTable:IVRApplications_008",ref error);
   if(table==IntPtr.Zero||error!=0){error=0;table=get("FnTable:IVRApplications_007",ref error);}
   if(table==IntPtr.Zero||error!=0)throw new InvalidOperationException("OpenVR applications interface "+error);
   // Stable documented first 15 entries of IVRApplications_007/_008.
   var add=Bind<AddManifest>(Marshal.ReadIntPtr(table,0));var identify=Bind<Identify>(Marshal.ReadIntPtr(table,11*IntPtr.Size));
   var byProcess=Bind<KeyByProcess>(Marshal.ReadIntPtr(table,5*IntPtr.Size));var property=Bind<Property>(Marshal.ReadIntPtr(table,14*IntPtr.Size));
   path=Utf8(manifest);key=Utf8("bfvr.battlefield2142");buffer=Marshal.AllocHGlobal(4096);
   error=add(path,false);if(error!=0)throw new InvalidOperationException("AddApplicationManifest "+error);
   if(processId!=0){error=identify(processId,key);if(error!=0)throw new InvalidOperationException("IdentifyApplication "+error);
    error=byProcess(processId,buffer,4096);if(error!=0||Text(buffer)!="bfvr.battlefield2142")throw new InvalidOperationException("Presenter identity verification failed");}
   error=0;property(key,0,buffer,4096,ref error);if(error!=0)throw new InvalidOperationException("Name query "+error);string name=Text(buffer);
   error=0;property(key,52,buffer,4096,ref error);if(error!=0)throw new InvalidOperationException("Image query "+error);
   return "Registered: "+name+"; image="+Text(buffer)+"; presenter="+processId;
  } finally {if(buffer!=IntPtr.Zero)Marshal.FreeHGlobal(buffer);if(key!=IntPtr.Zero)Marshal.FreeHGlobal(key);if(path!=IntPtr.Zero)Marshal.FreeHGlobal(path);if(started)shutdown();FreeLibrary(library);}
 }
}
"@
 if($PresenterProcessId){
  $current=Get-CimInstance Win32_Process -Filter "ProcessId=$PresenterProcessId"
  if(-not $Presenter -or -not $current -or $current.ExecutablePath -ne [IO.Path]::GetFullPath($Presenter)){throw 'Presenter PID does not match the requested executable.'}
 }
 $message=[BF2142SteamVRBranding]::Apply($library,$manifestPath,$PresenterProcessId)
 if($Watch){
  if(-not $Presenter){throw 'A precise presenter path is required for watch mode.'}
  $expected=[IO.Path]::GetFullPath($Presenter);$deadline=[DateTime]::UtcNow.AddSeconds(150)
  $identified=$false
  while([DateTime]::UtcNow -lt $deadline){
   $presenterProcess=Get-CimInstance Win32_Process -Filter "Name='BFVRPresenter.exe'" | Where-Object ExecutablePath -eq $expected | Select-Object -First 1
   if($presenterProcess){
    # The OpenXR application may still be registering during startup; retry
    # only this owned presenter, without focus changes or runtime restarts.
    try {$message=[BF2142SteamVRBranding]::Apply($library,$manifestPath,[uint32]$presenterProcess.ProcessId);$identified=$true;break} catch {}
   }
   Start-Sleep -Milliseconds 750
  }
  if(-not $identified){throw "Presenter identity was not ready before the branding timeout."}
 }
 if($Log){$message|Set-Content -LiteralPath $Log}
 Write-Output $message
} catch {
 $message='SteamVR branding: '+$_.Exception.Message
 if($Log){$message|Set-Content -LiteralPath $Log}
 Write-Warning $message
 exit 1
}
