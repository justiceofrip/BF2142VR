"""BF2142 VR player setup. Builds owned assets locally and records reversible changes."""
from pathlib import Path
import argparse,ctypes,hashlib,io,json,os,shutil,struct,sys,uuid,zipfile
from ctypes import wintypes as w
import RepairWeaponMeshes as weapons
import RemoveInteriorBackfaces as interiors
import ExportBodyEquipment as equipment
import ExportLobbyScene as lobby
from PIL import Image

APP='BF2142VR'
VERSION='0.2.0-beta.2'
STOCK='1a9903113df3fa5b24282ce8d2adbf54ddb58160155b28dea09f26fe85b782f9'
COMPLETE='e5d605ed915adac29c57840835d900bbc68a3c3ea4a2c7f7077000f6db8c144d'
WEAPONS='mods/bf2142/Objects/Weapons_client.zip'

def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
    return h.hexdigest()

def write_json(path,value):
    temp=path.with_name(path.name+'.pending')
    with temp.open('w',encoding='utf-8') as f:json.dump(value,f,indent=2);f.flush();os.fsync(f.fileno())
    os.replace(temp,path)

def child(root,relative):
    root=Path(root).resolve();p=Path(relative)
    if p.is_absolute() or '..' in p.parts or ':' in relative:raise ValueError('Unsafe manifest path')
    target=root/p
    if not target.resolve().is_relative_to(root):raise ValueError('Path leaves installation folder')
    for part in [target,*target.parents]:
        if part==root:break
        if part.exists() and (part.is_symlink() or part.is_junction()):raise ValueError('Linked installation paths are unsupported')
    return target

def running(game):
    k=ctypes.WinDLL('kernel32',use_last_error=True)
    class Entry(ctypes.Structure):
        _fields_=[('dwSize',w.DWORD),('cntUsage',w.DWORD),('th32ProcessID',w.DWORD),('th32DefaultHeapID',ctypes.c_size_t),('th32ModuleID',w.DWORD),('cntThreads',w.DWORD),('th32ParentProcessID',w.DWORD),('pcPriClassBase',w.LONG),('dwFlags',w.DWORD),('szExeFile',w.WCHAR*260)]
    k.CreateToolhelp32Snapshot.argtypes=[w.DWORD,w.DWORD];k.CreateToolhelp32Snapshot.restype=w.HANDLE
    k.Process32FirstW.argtypes=[w.HANDLE,ctypes.POINTER(Entry)];k.Process32NextW.argtypes=[w.HANDLE,ctypes.POINTER(Entry)]
    k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
    k.CloseHandle.argtypes=[w.HANDLE];k.QueryFullProcessImageNameW.argtypes=[w.HANDLE,w.DWORD,w.LPWSTR,ctypes.POINTER(w.DWORD)]
    snap=k.CreateToolhelp32Snapshot(2,0)
    if snap==w.HANDLE(-1).value:raise OSError('Cannot check running games')
    e=Entry();e.dwSize=ctypes.sizeof(e);ok=k.Process32FirstW(snap,ctypes.byref(e))
    try:
        while ok:
            if e.szExeFile.lower()=='bf2142.exe':
                h=k.OpenProcess(0x1000,False,e.th32ProcessID)
                if not h:raise RuntimeError('Close BF2142 before setup or uninstall')
                try:
                    buf=ctypes.create_unicode_buffer(32768);n=w.DWORD(len(buf))
                    if not k.QueryFullProcessImageNameW(h,0,buf,ctypes.byref(n)):raise OSError('Cannot identify running game')
                    if Path(buf.value).resolve()==(game/'BF2142.exe').resolve():raise RuntimeError('Close this BF2142 session before setup or uninstall')
                finally:k.CloseHandle(h)
            ok=k.Process32NextW(snap,ctypes.byref(e))
    finally:k.CloseHandle(snap)

def pe(data,machine):
    if data[:2]!=b'MZ' or len(data)<64:raise ValueError('Not a Windows executable')
    p=struct.unpack_from('<I',data,60)[0]
    if data[p:p+4]!=b'PE\0\0' or struct.unpack_from('<H',data,p+4)[0]!=machine:raise ValueError('Wrong executable architecture')
    return p

def rva(data,address,size):
    p=pe(data,0x14c);count=struct.unpack_from('<H',data,p+6)[0];optional=struct.unpack_from('<H',data,p+20)[0]
    for i in range(count):
        off=p+24+optional+i*40;virtual,va,raw,at=struct.unpack_from('<4I',data,off+8)
        if va<=address and address+size<=va+raw:return data[at+address-va:at+address-va+size]
    raise ValueError('Unsupported executable layout')

def validate_game(game):
    game=game.resolve()
    if not game.is_dir():raise ValueError('Select the folder containing BF2142.exe')
    running(game)
    data=child(game,'BF2142.exe').read_bytes();p=pe(data,0x14c)
    if rva(data,0x1cd180,12)!=bytes.fromhex('55 8b ec 53 56 57 8b 7d 08 57 8b f1'):raise ValueError('Unsupported game build. Install Battlefield 2142 v1.51 first.')
    pe(child(game,'RendDX9.dll').read_bytes(),0x14c)
    objects=child(game,'mods/bf2142/Objects')
    for name in ['Weapons_client.zip','Vehicles_client.zip','Vehicles_server.zip','Common_client.zip']:
        if not (objects/name).is_file():raise ValueError('Missing stock game archive: '+name)
    original=sha(objects/'Weapons_client.zip')
    if original not in (STOCK,COMPLETE):raise ValueError('This beta needs the stock BF2142 weapon archive. Use a clean v1.51 installation; Remaster and other weapon packs are not supported yet.')
    patched=bytearray(data);flags=struct.unpack_from('<H',data,p+22)[0];struct.pack_into('<H',patched,p+22,flags|0x20)
    return original,data,bytes(patched)

def payload_files(payload):
    m=json.loads((payload/'payload.json').read_text(encoding='utf-8'))
    if m.get('version')!=VERSION or not isinstance(m.get('files'),dict):raise ValueError('Unsupported package manifest')
    for rel,h in m['files'].items():
        path=child(payload,rel)
        if not path.is_file() or sha(path)!=h:raise ValueError('Package verification failed: '+rel)
    for rel in ['tools/SetupAssets.exe','tools/Player.ps1','Play VR.cmd','Uninstall.cmd','runtime/x86/BF2142VRLauncher.exe','runtime/x86/BF2142VRClient.dll','runtime/x64/BFVRPresenter.exe','runtime/x64/runtime/openxr/win64/openxr_loader.dll']:
        if rel not in m['files']:raise ValueError('Incomplete package: '+rel)
    return m

def decode(raw,size):
    with Image.open(io.BytesIO(raw),formats=['DDS']) as im:
        if im.width>8192 or im.height>8192:raise ValueError('Oversized DDS')
        image=im.convert('RGBA').resize((size,size),Image.Resampling.BICUBIC)
        return image.tobytes('raw','BGRA')

def build_assets(game,stage,original):
    objects=child(game,'mods/bf2142/Objects');source=objects/'Weapons_client.zip';target=stage/'generated/Weapons_client.zip';target.parent.mkdir()
    if original==COMPLETE:shutil.copy2(source,target)
    else:
        found=set()
        with zipfile.ZipFile(source) as src,zipfile.ZipFile(target,'x',compression=zipfile.ZIP_DEFLATED) as out:
            for item in src.infolist():
                data=src.read(item);name=Path(item.filename).stem.lower()
                if name in weapons.NAMES and item.filename.lower().endswith('.bundledmesh'):
                    data,_=weapons.repair(data,name);data,_=interiors.repair(data);found.add(name)
                    print('Repairing weapon',len(found),'/',len(weapons.NAMES),name,flush=True)
                out.writestr(item,data)
        if found!=weapons.NAMES:raise ValueError('Incomplete weapon repair')
        if sha(target)!=COMPLETE:raise ValueError('Weapon repair differs from the accepted build; original files were left untouched')
    with zipfile.ZipFile(target) as z:
        if z.testzip():raise ValueError('Generated weapon archive failed its CRC check')
    assets=stage/'generated'
    print('Building body equipment...',flush=True)
    equipment.export(source,assets/'BodyEquipment.bin',None,[objects/'Common_client.zip'],decode=decode)
    print('Building walker hangar...',flush=True)
    lobby.export(objects,assets/'LobbyScene.bin',None,decode=decode)
    # The optional SteamVR image is extracted from the owner's menu archive.
    menu=game/'mods/bf2142/menu_client.zip'
    if menu.is_file():
        with zipfile.ZipFile(menu) as z:
            names={n.lower():n for n in z.namelist()}
            key='external/flashmenu/images/2142_logo_inv.png'
            if key in names:
                with Image.open(io.BytesIO(z.read(names[key])),formats=['PNG']) as im:
                    logo=im.convert('RGBA');logo.save(assets/'BF2142-logo.png')
                    icon=Image.new('RGBA',(256,256));small=logo.copy();small.thumbnail((256,256));icon.alpha_composite(small,((256-small.width)//2,(256-small.height)//2));icon.save(assets/'BF2142VR.ico',sizes=[(32,32),(48,48),(256,256)])

def refresh_settings(root):
    import configparser
    path=root/'BF2142VR.ini';c=configparser.ConfigParser();c.optionxform=str
    if path.exists():
        # Native Windows INI writes may add a UTF-16 BOM.
        raw=path.read_bytes();text=raw.decode('utf-16') if raw[:2] in (b'\xff\xfe',b'\xfe\xff') else raw.decode('utf-8-sig');c.read_string(text)
    if not c.has_section('VR'):c.add_section('VR')
    defaults={'WorldMSAASamples':'8','WorldScale':'1.0','HeightOffset':'0','TurnSpeed':'600','Controllers':'1','TrackedWeapon':'0','MotionHands':'1','WeaponOptics':'1','AutomaticADS':'1','BodyInventory':'1','FingerPoses':'1','MotionActions':'1','WeaponFaceFade':'0','SnapTurning':'1','SnapAngle':'30','HideCrosshair':'1','PhysicalStance':'1','StandingHeight':'0','MenuRoom':'1','ToggleWeaponGrip':'1','GrenadeArc':'1','MovementDirection':'head'}
    for key,value in defaults.items():
        if key not in c['VR']:c['VR'][key]=value
    c['VR']['BodyEquipmentFile']=str(root/'generated/BodyEquipment.bin');c['VR']['LobbySceneFile']=str(root/'generated/LobbyScene.bin')
    with path.open('w',encoding='utf-16') as f:c.write(f,space_around_delimiters=False)
    image=root/'generated/BF2142-logo.png'
    if image.is_file():
        manifest={'source':'builtin','applications':[{'app_key':'bfvr.battlefield2142','launch_type':'binary','binary_path_windows':str(Path(os.environ['SystemRoot'])/'System32/WindowsPowerShell/v1.0/powershell.exe'),'arguments':'-NoProfile -ExecutionPolicy Bypass -File "'+str(root/'tools/Player.ps1')+'" -Action Play','working_directory':str(root),'image_path':str(image),'strings':{'en_us':{'name':'Battlefield 2142 VR','description':'Battlefield 2142 VR '+VERSION}}}]}
        write_json(root/'BF2142VR.vrmanifest',manifest)

def replace_copy(source,target):
    temp=target.with_name(target.name+'.bfvr-'+uuid.uuid4().hex+'.pending')
    try:
        shutil.copy2(source,temp)
        if sha(temp)!=sha(source):raise ValueError('Staging verification failed')
        os.replace(temp,target)
    finally:
        if temp.exists():temp.unlink()

def mutations(root,m):
    game=root.parent.resolve()
    if Path(m['game']).resolve()!=game or m.get('app')!=APP:raise ValueError('Install state belongs to a different game folder')
    rows=[]
    for row in m['changes']:
        if row['target'] not in [WEAPONS,'BF2142.exe']:raise ValueError('Unrecognized game-file change')
        target=child(game,row['target']);backup=child(root,row['backup']);current=sha(target) if target.exists() else None
        if sha(backup)!=row['original']:raise ValueError('Backup verification failed; no files restored')
        if current is not None and current not in (row['original'],row['installed']):raise ValueError('Another mod changed '+row['target']+'. Automatic restore stopped; original backup is preserved.')
        rows.append((row,target,backup,current))
    return rows

def restore(root,m):
    rows=mutations(root,m);running(root.parent)
    for row,target,backup,current in rows:
        if current!=row['original']:replace_copy(backup,target)
    m['status']='uninstalled';write_json(root/'install.json',m)

def install(game,payload):
    game=game.resolve();payload=payload.resolve();manifest=payload_files(payload)
    original,exe,patched=validate_game(game);final=game/APP
    if final.exists():
        marker=final/'install.json'
        if marker.is_file():
            m=json.loads(marker.read_text(encoding='utf-8'))
            if m.get('status')=='installed' and m.get('version')==VERSION:
                rows=mutations(final,m)
                if all(current==row['installed'] for row,_,_,current in rows):
                    print('This beta is already installed:',final);return final
        raise ValueError('BF2142VR already exists. Run its Uninstall.cmd first, then move that folder aside before installing again.')
    stage=game/('.BF2142VR-setup-'+uuid.uuid4().hex);stage.mkdir();committed=False
    try:
        for rel in manifest['files']:
            src=child(payload,rel);dest=child(stage,rel);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dest)
        shutil.copy2(payload/'payload.json',stage/'payload.json')
        build_assets(game,stage,original)
        (stage/'backups').mkdir();shutil.copy2(child(game,WEAPONS),stage/'backups/Weapons_client.zip')
        changes=[{'target':WEAPONS,'backup':'backups/Weapons_client.zip','original':original,'installed':COMPLETE}]
        if exe!=patched:
            (stage/'backups/BF2142.exe').write_bytes(exe);(stage/'generated/BF2142.exe').write_bytes(patched)
            changes.append({'target':'BF2142.exe','backup':'backups/BF2142.exe','original':hashlib.sha256(exe).hexdigest(),'installed':hashlib.sha256(patched).hexdigest()})
        m={'app':APP,'version':VERSION,'build':'quality-hotfix-beta2','game':str(game),'status':'prepared','changes':changes}
        write_json(stage/'install.json',m);running(game)
        for row in changes:
            if sha(child(game,row['target']))!=row['original']:raise ValueError('Game files changed during setup')
        stage.rename(final);committed=True
        try:
            refresh_settings(final)
            for row in changes:replace_copy(final/'generated'/Path(row['target']).name,child(game,row['target']))
            m['status']='installed';write_json(final/'install.json',m)
        except BaseException:
            restore(final,m);raise
        print('Installed Battlefield 2142 VR:',final,flush=True);return final
    finally:
        if not committed and stage.exists():
            # This exact temporary directory was created above; never remove a
            # computed game root or an arbitrary user-selected directory.
            if stage.resolve().parent!=game or not stage.name.startswith('.BF2142VR-setup-') or stage.is_symlink() or stage.is_junction():raise ValueError('Unsafe temporary directory')
            shutil.rmtree(stage)

def check(root):
    root=root.resolve();m=json.loads((root/'install.json').read_text(encoding='utf-8'))
    if m.get('status')!='installed':raise ValueError('VR is not installed. Run Setup.cmd.')
    rows=mutations(root,m)
    if not all(current==row['installed'] for row,_,_,current in rows):raise ValueError('Installed game files changed. Run setup again or restore the game.')
    payload_files(root);refresh_settings(root)
    print('Installation verified:',VERSION)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('action',choices=['install','check','uninstall','assets']);p.add_argument('--game',type=Path);p.add_argument('--payload',type=Path);p.add_argument('--root',type=Path);p.add_argument('--output',type=Path);a=p.parse_args()
    if a.action=='install':
        if not a.game or not a.payload:p.error('install requires --game and --payload')
        install(a.game,a.payload)
    elif a.action=='assets':
        if not a.game or not a.output:p.error('assets requires --game and --output')
        original,_,_=validate_game(a.game);a.output.mkdir(exist_ok=False);build_assets(a.game,a.output,original)
    else:
        if not a.root:p.error('requires --root')
        root=a.root.resolve()
        if a.action=='check':check(root)
        else:
            m=json.loads((root/'install.json').read_text(encoding='utf-8'));restore(root,m)
            print('Original game files restored. VR is disabled. This BF2142VR folder retains your backups and settings; you may remove it after closing setup.')
if __name__=='__main__':
    import faulthandler,traceback
    faulthandler.enable()
    try:main()
    except Exception as error:
        print('\nSETUP STOPPED:',error,flush=True)
        traceback.print_exc()
        sys.exit(1)
