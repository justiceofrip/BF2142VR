"""BF2142 VR player setup. Builds owned assets locally and records reversible changes."""
from pathlib import Path
import argparse,contextlib,ctypes,hashlib,io,json,os,shutil,struct,sys,uuid,zipfile
from ctypes import wintypes as w
import RepairWeaponMeshes as weapons
import WeaponRepairWorker as repair_worker
import WaterReflectionRepair as water_repair
import RemoveInteriorBackfaces as interiors
import ThinShellBackfaces as separation
import ExportBodyEquipment as equipment
import ExportLobbyScene as lobby
from PIL import Image

APP='BF2142VR'
VERSION='0.2.0-beta.4-hotfix.3'
INTRO_MOVIES=tuple('mods/bf2142/Movies/'+name+'.bik' for name in ('Dice','EA','Intro','Legal','Legal_na'))
STOCK='1a9903113df3fa5b24282ce8d2adbf54ddb58160155b28dea09f26fe85b782f9'
LEGACY_COMPLETE='e5d605ed915adac29c57840835d900bbc68a3c3ea4a2c7f7077000f6db8c144d'
UNBOUNDED_COMPLETE='008dddbf4b9e87f1d108b66d0626bde6468d9a5e7a3d7b04329ac6c71b0ed2b3'
SMOOTH_COMPLETE='00af68ba109d7a814b89ea0e720a01c575bd6e4319e8e73b32b91dcfaf4ecb8b'
COMPLETE='d17d482faed629a31872c3a0e9df7698ced9204fd3242e4fab51bc4ec9d81235'
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

def process_is_active(kernel, handle):
    # A retained Windows process object may outlive the game. A signaled
    # process handle is exited even if its cached image name still resolves.
    state=kernel.WaitForSingleObject(handle,0)
    if state==0:return False
    if state==258:
        # Some graphics-driver teardown leaves an unsignaled process object
        # after its exit code is final. Match .NET Process.HasExited here.
        code=w.DWORD()
        if not kernel.GetExitCodeProcess(handle,ctypes.byref(code)):
            raise OSError('Cannot query the game exit status')
        return code.value==259  # STILL_ACTIVE

    raise OSError('Cannot determine whether the game has exited')


def running(game):
    k=ctypes.WinDLL('kernel32',use_last_error=True)
    class Entry(ctypes.Structure):
        _fields_=[('dwSize',w.DWORD),('cntUsage',w.DWORD),('th32ProcessID',w.DWORD),('th32DefaultHeapID',ctypes.c_size_t),('th32ModuleID',w.DWORD),('cntThreads',w.DWORD),('th32ParentProcessID',w.DWORD),('pcPriClassBase',w.LONG),('dwFlags',w.DWORD),('szExeFile',w.WCHAR*260)]
    k.CreateToolhelp32Snapshot.argtypes=[w.DWORD,w.DWORD];k.CreateToolhelp32Snapshot.restype=w.HANDLE
    k.Process32FirstW.argtypes=[w.HANDLE,ctypes.POINTER(Entry)];k.Process32NextW.argtypes=[w.HANDLE,ctypes.POINTER(Entry)]
    k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
    k.GetExitCodeProcess.argtypes=[w.HANDLE,ctypes.POINTER(w.DWORD)];k.GetExitCodeProcess.restype=w.BOOL
    k.WaitForSingleObject.argtypes=[w.HANDLE,w.DWORD];k.WaitForSingleObject.restype=w.DWORD
    k.CloseHandle.argtypes=[w.HANDLE];k.QueryFullProcessImageNameW.argtypes=[w.HANDLE,w.DWORD,w.LPWSTR,ctypes.POINTER(w.DWORD)]
    snap=k.CreateToolhelp32Snapshot(2,0)
    if snap==w.HANDLE(-1).value:raise OSError('Cannot check running games')
    e=Entry();e.dwSize=ctypes.sizeof(e);ok=k.Process32FirstW(snap,ctypes.byref(e))
    try:
        while ok:
            if e.szExeFile.lower()=='bf2142.exe':
                h=k.OpenProcess(0x101000,False,e.th32ProcessID)
                if not h:raise RuntimeError('Close BF2142 before setup or uninstall')
                try:
                    if not process_is_active(k,h):
                        ok=k.Process32NextW(snap,ctypes.byref(e));continue
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
    if original not in (STOCK,LEGACY_COMPLETE,UNBOUNDED_COMPLETE,SMOOTH_COMPLETE,COMPLETE):raise ValueError('This beta needs the stock BF2142 weapon archive. Use a clean v1.51 installation; Remaster and other weapon packs are not supported yet.')
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
    if original in (UNBOUNDED_COMPLETE,SMOOTH_COMPLETE):
        manifest=json.loads(child(game,APP+'/install.json').read_text(encoding='utf-8'))
        rows=mutations(child(game,APP),manifest)
        row=next((r for r in rows if r[0]['target']==WEAPONS),None)
        if row is None or row[0]['original'] not in (STOCK,LEGACY_COMPLETE):
            raise ValueError('This older private repair needs its verified original weapon backup. Keep all backups and restore the original installation first.')
        source=row[2];original=row[0]['original']

    if original==COMPLETE:shutil.copy2(source,target)
    else:
        found=set()
        with zipfile.ZipFile(source) as src,zipfile.ZipFile(target,'x',compression=zipfile.ZIP_DEFLATED) as out:
            for item in src.infolist():
                data=src.read(item);name=Path(item.filename).stem.lower()
                if name in weapons.NAMES and item.filename.lower().endswith('.bundledmesh'):
                    print('Preparing weapon',len(found)+1,'/',len(weapons.NAMES),name,flush=True)
                    if original==LEGACY_COMPLETE:
                        data,_=separation.repair(data)
                        profile=repair_worker.PROFILES[name]
                        if len(data)!=profile['size'] or hashlib.sha256(data).hexdigest()!=profile['result']:
                            raise ValueError('Weapon separation differs from accepted model: '+name)
                    else:
                        cache_root=Path(os.environ.get('LOCALAPPDATA',Path.home()/'AppData/Local'))/'BF2142VR/WeaponCache'
                        cache_root.mkdir(parents=True,exist_ok=True)
                        cache=child(cache_root,COMPLETE)
                        data=repair_worker.repair_cached(data,name,cache,repair_worker.worker_command())
                    found.add(name)
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
        if row['target'] not in [WEAPONS,water_repair.TARGET,'BF2142.exe',*INTRO_MOVIES]:raise ValueError('Unrecognized game-file change')
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
        water=water_repair.stage(game,stage,child,sha)
        if water:changes.append(water)
        if exe!=patched:
            (stage/'backups/BF2142.exe').write_bytes(exe);(stage/'generated/BF2142.exe').write_bytes(patched)
            changes.append({'target':'BF2142.exe','backup':'backups/BF2142.exe','original':hashlib.sha256(exe).hexdigest(),'installed':hashlib.sha256(patched).hexdigest()})
        for movie in INTRO_MOVIES:
            source=child(game,movie)
            if not source.is_file():continue
            backup='backups/intro/'+Path(movie).name
            child(stage,backup).parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(source,child(stage,backup))
            changes.append({'target':movie,'backup':backup,'original':sha(source),'installed':None})
        m={'app':APP,'version':VERSION,'build':'renderer-beta4-hotfix2','game':str(game),'status':'prepared','changes':changes}
        write_json(stage/'install.json',m);running(game)
        for row in changes:
            if sha(child(game,row['target']))!=row['original']:raise ValueError('Game files changed during setup')
        stage.rename(final);committed=True
        try:
            refresh_settings(final)
            for row in changes:
                if row['target'] in INTRO_MOVIES:child(game,row['target']).unlink()
                else:replace_copy(child(final,water_repair.GENERATED) if row['target']==water_repair.TARGET else final/'generated'/Path(row['target']).name,child(game,row['target']))
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

@contextlib.contextmanager
def install_lock(game):
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.CreateMutexW.argtypes=[ctypes.c_void_p,w.BOOL,w.LPCWSTR];kernel.CreateMutexW.restype=w.HANDLE
    kernel.WaitForSingleObject.argtypes=[w.HANDLE,w.DWORD];kernel.WaitForSingleObject.restype=w.DWORD
    kernel.ReleaseMutex.argtypes=[w.HANDLE];kernel.CloseHandle.argtypes=[w.HANDLE]
    name='Local\\BF2142VR-GameSetup-'+hashlib.sha256(str(game.resolve()).casefold().encode()).hexdigest()
    handle=kernel.CreateMutexW(None,False,name)
    if not handle:raise OSError('Cannot acquire installer lock')
    acquired=False
    try:
        acquired=kernel.WaitForSingleObject(handle,0) in (0,0x80)
        if not acquired:raise RuntimeError('Another setup/update is already using this game folder')
        yield
    finally:
        if acquired:kernel.ReleaseMutex(handle)
        kernel.CloseHandle(handle)


def rollback_weapon_update(game,root,state):
    water_repair.rollback(game,root,state,child,sha,replace_copy)
    change=state.get('weapon')
    if change is None:return
    before,after=change.get('before',''),change.get('after','')
    if any(len(h)!=64 or any(c not in '0123456789abcdef' for c in h) for h in (before,after)):
        raise ValueError('Invalid weapon update journal')
    target=child(game,WEAPONS)
    current=sha(target) if target.exists() else None
    if current==before:return
    if current!=after:raise ValueError('Weapon archive changed externally during update; backups retained')
    backup=child(root,'update-rollback/Weapons_client.zip')
    if sha(backup)!=before:raise ValueError('Weapon update rollback backup differs')
    replace_copy(backup,target)


def recover_update(game):
    journal=child(game,'.BF2142VR-update.json')
    if not journal.exists():return
    state=json.loads(journal.read_text(encoding='utf-8'));token=state.get('transaction','')
    if state.get('app')!=APP or len(token)!=32 or uuid.UUID(hex=token).hex!=token:raise ValueError('Invalid interrupted-update journal; keep it and the backups')
    final=child(game,APP);previous=child(game,'.BF2142VR-previous-'+token)
    # The previous directory name is derived locally, never read as a free path.
    if final.exists():
        m=json.loads((final/'install.json').read_text(encoding='utf-8'))
        if m.get('transaction')==token:
            if m.get('status')=='installed':
                mutations(final,m);journal.unlink();return
            if not previous.exists():raise ValueError('Interrupted update is missing its rollback runtime')
            rollback_weapon_update(game,final,state)
            failed=child(game,'.BF2142VR-interrupted-'+token)
            final.rename(failed)
        elif previous.exists():raise ValueError('Conflicting interrupted update; preserve both installation folders')
    if previous.exists():
        if final.exists():raise ValueError('Cannot recover over an existing installation')
        previous.rename(final)
        print('Recovered the previous installation after an interrupted update.',flush=True)
    elif not final.exists():raise ValueError('Interrupted update has no recoverable installation; retain backups')
    journal.unlink()


def update(game,payload):
    game=game.resolve();payload=payload.resolve();final=child(game,APP)
    if not final.exists():return install(game,payload)
    manifest=payload_files(payload);original,_,_=validate_game(game)
    m=json.loads((final/'install.json').read_text(encoding='utf-8'))
    if m.get('status')!='installed':raise ValueError('The previous installation is not complete. Use its Uninstall.cmd to restore it first; keep its backups.')
    rows=mutations(final,m)
    if not all(current==row['installed'] for row,_,_,current in rows):raise ValueError('Game files changed since installation. Restore/uninstall the previous version first; backups are retained.')
    stage=child(game,'.BF2142VR-setup-'+uuid.uuid4().hex);stage.mkdir()
    token=uuid.uuid4().hex
    previous=child(game,'.BF2142VR-previous-'+token)
    journal=child(game,'.BF2142VR-update.json')
    moved=False;activated=False
    transaction={'app':APP,'transaction':token}
    try:
        for rel in manifest['files']:
            dest=child(stage,rel);dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(child(payload,rel),dest)
        shutil.copy2(payload/'payload.json',stage/'payload.json')
        build_assets(game,stage,original)
        # Keep the first stock backup, not a backup of an already patched game.
        for row,_,backup,_ in rows:
            dest=child(stage,row['backup']);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(backup,dest)
        settings=child(final,'BF2142VR.ini')
        if settings.exists():shutil.copy2(settings,stage/'BF2142VR.ini')
        updated=dict(m,version=VERSION,build='renderer-beta4-hotfix2',status='prepared',transaction=token)
        updated['changes']=[dict(row) for row in m['changes']]
        weapon_row=next(row for row in updated['changes'] if row['target']==WEAPONS)
        generated=stage/'generated/Weapons_client.zip'
        generated_hash=sha(generated)
        if generated_hash!=weapon_row['installed']:
            rollback=child(stage,'update-rollback/Weapons_client.zip');rollback.parent.mkdir()
            shutil.copy2(child(game,WEAPONS),rollback)
            if sha(rollback)!=weapon_row['installed']:raise ValueError('Weapon changed during update staging')
            transaction['weapon']={'before':weapon_row['installed'],'after':generated_hash}
            weapon_row['installed']=generated_hash
        water=water_repair.stage(game,stage,child,sha)
        if water:
            if any(row['target']==water_repair.TARGET for row in updated['changes']):
                raise ValueError('Recorded water repair differs; original backup retained')
            updated['changes'].append(water)
            transaction['water']={'before':water['original'],'after':water['installed']}
        write_json(stage/'install.json',updated)
        running(game)
        if not all(current==row['installed'] for row,_,_,current in mutations(final,m)):
            raise ValueError('Game files changed during update')
        if 'water' in transaction and sha(child(game,water_repair.TARGET))!=transaction['water']['before']:
            raise ValueError('Map changed during update')
        write_json(journal,transaction)
        final.rename(previous);moved=True
        stage.rename(final);activated=True
        refresh_settings(final);payload_files(final)
        if 'weapon' in transaction:
            replace_copy(final/'generated/Weapons_client.zip',child(game,WEAPONS))
        if 'water' in transaction:
            replace_copy(child(final,water_repair.GENERATED),child(game,water_repair.TARGET))
        mutations(final,updated)
        updated['status']='installed';write_json(final/'install.json',updated)
        journal.unlink()
        print('Updated Battlefield 2142 VR: '+str(final),flush=True)
        print('Previous runtime/settings retained at: '+str(previous),flush=True)
        return final
    except BaseException:
        if moved:
            if activated:
                rollback_weapon_update(game,final,transaction)
                final.rename(stage)
            previous.rename(final)
        if journal.exists():journal.unlink()
        raise
    finally:
        if stage.exists():
            if stage.resolve().parent!=game or stage.is_symlink() or stage.is_junction():raise ValueError('Unsafe update stage')
            shutil.rmtree(stage)


def check(root):
    root=root.resolve();m=json.loads((root/'install.json').read_text(encoding='utf-8'))
    if m.get('status')!='installed':raise ValueError('VR is not installed. Run Setup.cmd.')
    rows=mutations(root,m)
    if not all(current==row['installed'] for row,_,_,current in rows):raise ValueError('Installed game files changed. Run setup again or restore the game.')
    payload_files(root);refresh_settings(root)
    print('Installation verified:',VERSION)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('action',choices=['install','check','uninstall','assets','repair-weapon','apply']);p.add_argument('--game',type=Path);p.add_argument('--payload',type=Path);p.add_argument('--root',type=Path);p.add_argument('--output',type=Path);p.add_argument('--name',choices=sorted(weapons.NAMES));p.add_argument('--input',type=Path);a=p.parse_args()
    if a.action=='repair-weapon':
        if not a.name or not a.input or not a.output:p.error('repair-weapon requires --name, --input and --output')
        repair_worker.repair_one(a.name,a.input,a.output)
    elif a.action=='apply':
        if not a.game or not a.payload:p.error('apply requires --game and --payload')
        with install_lock(a.game):
            recover_update(a.game.resolve());update(a.game,a.payload)
    elif a.action=='install':
        if not a.game or not a.payload:p.error('install requires --game and --payload')
        with install_lock(a.game):
            recover_update(a.game.resolve());install(a.game,a.payload)
    elif a.action=='assets':
        if not a.game or not a.output:p.error('assets requires --game and --output')
        original,_,_=validate_game(a.game);a.output.mkdir(exist_ok=False);build_assets(a.game,a.output,original)
    else:
        if not a.root:p.error('requires --root')
        root=a.root.resolve()
        with install_lock(root.parent):
            recover_update(root.parent)
            if a.action=='check':check(root)
            else:
                m=json.loads((root/'install.json').read_text(encoding='utf-8'));restore(root,m)
                print('Original game files restored. VR is disabled. This BF2142VR folder retains your backups and settings; you may remove it after closing setup.')
if __name__=='__main__':
    import faulthandler,traceback
    faulthandler.enable()
    print('BF2142 VR setup '+VERSION+' / Python '+sys.version.split()[0]+' / '+str(struct.calcsize('P')*8)+'-bit',flush=True)
    try:main()
    except Exception as error:
        print('\nSETUP STOPPED:',error,flush=True)
        traceback.print_exc()
        sys.exit(1)
