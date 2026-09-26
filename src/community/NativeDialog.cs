using System.Runtime.InteropServices;
namespace BF2142.Community;
internal static class NativeDialog {
 [StructLayout(LayoutKind.Sequential,Pack=1)]struct Button {public int id;public IntPtr text;}
 // commctrl.h puts TASKDIALOGCONFIG and TASKDIALOG_BUTTON under pshpack1.
 [StructLayout(LayoutKind.Sequential,Pack=1)]struct Config {
  public uint size;public IntPtr owner,instance;public uint flags,commonButtons;
  public IntPtr title,mainIcon,instruction,content;public uint buttonCount;public IntPtr buttons;public int defaultButton;
  public uint radioCount;public IntPtr radioButtons;public int defaultRadio;public IntPtr verification,expanded,expandedControl,collapsedControl,footerIcon,footer,callback,callbackData;public uint width;
 }
 [DllImport("comctl32.dll",ExactSpelling=true)]static extern int TaskDialogIndirect(ref Config config,out int button,out int radio,out int check);
 public static string? PlayMode(string server,bool vrAvailable=true,bool offline=false){List<IntPtr> strings=[];IntPtr Text(string s){var p=Marshal.StringToHGlobalUni(s);strings.Add(p);return p;}IntPtr buttons=Marshal.AllocHGlobal(Marshal.SizeOf<Button>()*2);
  try{
   Marshal.StructureToPtr(new Button{id=101,text=Text("Play on desktop\nSee tracked VR players. Nearby players can hear your microphone during gameplay. No headset needed.")},buttons,false);
   Marshal.StructureToPtr(new Button{id=102,text=Text("Play in VR\nConnect your headset and start SteamVR before joining.")},buttons+Marshal.SizeOf<Button>(),false);
   var config=new Config{size=(uint)Marshal.SizeOf<Config>(),flags=0x10|0x8,commonButtons=8,title=Text("Battlefield 2142 Community"),instruction=Text("Join "+server),content=Text(offline?"This playtest installs its bundled flat addon. Select your BF2142.exe once, then log in normally. Updates to this offline test are manual.":"The matching addon will download and update automatically. Your normal game login is used."),buttonCount=vrAvailable?2u:1u,buttons=buttons,defaultButton=101,width=340};
   int hr=TaskDialogIndirect(ref config,out int result,out _,out _);if(hr<0)Marshal.ThrowExceptionForHR(hr);return result==101?"flat":result==102?"vr":null;
  }finally{foreach(var s in strings)Marshal.FreeHGlobal(s);Marshal.FreeHGlobal(buttons);}
 }
}
