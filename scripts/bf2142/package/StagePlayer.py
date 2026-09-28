"""Stage the BF2142 VR player payload from explicit, already-built inputs."""
from pathlib import Path
import argparse,hashlib,json,shutil,struct

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def copy_pe(source,target):
    # Preserve all executable sections. Remove only the build machine's PDB
    # filename from CodeView metadata and Opus diagnostic roots below.
    raw=bytearray(source.read_bytes());p=struct.unpack_from('<I',raw,60)[0]
    optional=p+24;magic=struct.unpack_from('<H',raw,optional)[0];directory=optional+(112 if magic==0x20b else 96)
    rva,size=struct.unpack_from('<II',raw,directory+6*8);sections=struct.unpack_from('<H',raw,p+6)[0];section_table=optional+struct.unpack_from('<H',raw,p+20)[0]
    edits=[]
    if rva:
        for i in range(sections):
            at=section_table+i*40;_,va,n,offset=struct.unpack_from('<4I',raw,at+8)
            if va<=rva and rva+size<=va+n:
                debug=offset+rva-va
                for entry in range(debug,debug+size,28):
                    kind,count,_,data=struct.unpack_from('<4I',raw,entry+12)
                    if kind==2 and raw[data:data+4]==b'RSDS' and count>24:
                        original=bytes(raw[data+24:data+count]);label=source.with_suffix('.pdb').name.encode('ascii')+b'\0'
                        assert len(label)<=len(original)
                        raw[data+24:data+count]=label+b'\0'*(len(original)-len(label));edits.append((data+24,data+count))
    # Opus hardening diagnostics embed __FILE__ even in release builds. Keep
    # the diagnostic filename, dropping only this build's absolute root. Never
    # edit executable sections or arbitrary paths in the binary.
    repo=Path(__file__).resolve().parents[3]
    prefixes=[str(repo).encode()+b'\\',repo.as_posix().encode()+b'/']
    for i in range(sections):
        at=section_table+i*40
        name=bytes(raw[at:at+8]).rstrip(b'\0')
        _,_,n,offset=struct.unpack_from('<4I',raw,at+8)
        flags=struct.unpack_from('<I',raw,at+36)[0]
        if name!=b'.rdata' or flags&0x20000000:continue
        for prefix in prefixes:
            cursor=offset
            while True:
                start=raw.find(prefix,cursor,offset+n)
                if start<0:break
                end=raw.find(b'\0',start,offset+n)
                if end<0:break
                cursor=end+1
                relative=bytes(raw[start+len(prefix):end]).replace(b'\\',b'/')
                if not relative.startswith(b'third_party/opus-1.6.1/'):continue
                text=relative.decode('ascii')
                file=repo/text
                if file.suffix not in ('.c','.h') or not file.is_file() or not file.resolve().is_relative_to(repo/'third_party/opus-1.6.1'):continue
                raw[start:end]=relative+b'\0'*(end-start-len(relative))
                edits.append((start,end))
    old=source.read_bytes()
    assert all(any(a<=i<b for a,b in edits) for i,(a,b) in enumerate(zip(old,raw)) if a!=b)
    target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)

def stage(checkpoint,tools,python_home,python_env,dest):
    repo=Path(__file__).resolve().parents[3];package=Path(__file__).parent;assets=package.parent
    if dest.exists():raise ValueError('Destination must be new')
    dest.mkdir(parents=True)
    for name in ['BF2142VRLauncher.exe','BF2142VRClient.dll']:copy_pe(checkpoint/'x86'/name,dest/'runtime/x86'/name)
    copy_pe(checkpoint/'x64/BFVRPresenter.exe',dest/'runtime/x64/BFVRPresenter.exe')
    for name in ['assets','runtime']:shutil.copytree(checkpoint/'x64'/name,dest/'runtime/x64'/name)
    shutil.copytree(tools,dest/'tools')
    for name in ['Player.ps1']:
        shutil.copy2(package/name,dest/'tools'/name)
    for name in ['Start-SteamVRBranding.ps1','Register-SteamVR.ps1']:shutil.copy2(assets/name,dest/'tools'/name)
    for name in ['Setup.cmd','Play VR.cmd','Desktop Preview.cmd','Uninstall.cmd','START HERE.txt','CONTROLS.txt','TROUBLESHOOTING.txt','RELEASE NOTES.txt']:shutil.copy2(package/name,dest/name)
    shutil.copy2(repo/'LICENSE',dest/'LICENSE.txt')
    license_dir=dest/'licenses';license_dir.mkdir()
    mapping=[(repo/'third_party/opus-1.6.1/COPYING','Opus-1.6.1.txt'),(repo/'licenses/OpenXR-Loader-1.1.61.txt','OpenXR-Loader-1.1.61.txt'),(repo/'third_party/minhook-1.3.4/LICENSE.txt','MinHook-LICENSE.txt'),(python_home/'LICENSE.txt','Python-LICENSE.txt'),(python_env/'Lib/site-packages/pillow-12.3.0.dist-info/licenses/LICENSE','Pillow-LICENSE.txt'),(python_env/'Lib/site-packages/pyinstaller-6.22.3.dist-info/licenses/COPYING.txt','PyInstaller-COPYING.txt'),(repo/'licenses/OpenSSL-3.0.11.txt','OpenSSL-3.0.11.txt'),(repo/'licenses/libffi.txt','libffi.txt')]
    for src,name in mapping:shutil.copy2(src,license_dir/name)
    (dest/'THIRD PARTY NOTICES.txt').write_text('''BF2142 VR is based on BFVR by JayBiggsGMG and the BFVR contributors.
https://github.com/JayBiggsGMG/BFVR-Battlefield-1942-VR-Mod
The mod and its setup source are under the included MIT license.

OpenXR Loader 1.1.61 (Khronos Group): Apache 2.0; see licenses.
MinHook 1.3.4 (Tsuda Kageyu and contributors): BSD; see licenses.
Opus 1.6.1 (Xiph.Org Foundation and contributors): BSD; see licenses/Opus-1.6.1.txt.
Python 3.12.0 (Python Software Foundation): PSF license; see licenses.
Pillow 12.3.0 (Pillow/PIL contributors): MIT-CMU and bundled dependency
notices, all reproduced in licenses/Pillow-LICENSE.txt.
PyInstaller 6.22.3 (PyInstaller Development Team): its unmodified compiled
bootloader uses the distribution exception; runtime hooks are Apache 2.0.
Full terms are in licenses/PyInstaller-COPYING.txt.
OpenSSL 3.0.11 (OpenSSL Project, Python runtime): Apache 2.0; see licenses.
libffi (Anthony Green and contributors, Python runtime): MIT; see licenses.
Microsoft Visual C++ runtime DLL: redistributed with the Python/Pillow runtime
for use by these Windows applications; Microsoft retains its rights.

The setup utility's source scripts are included in source-tools. The built
client and presenter derive from the BFVR repository above with the BF2142
port additions. Full source and build instructions:
https://github.com/justiceofrip/BF2142VR
No original game executable, renderer, archives, decoded game meshes, textures,
account data or maps are distributed. Local setup-generated game assets remain
subject to the original game's ownership and are not public mod source.
''')
    source_dir=dest/'source-tools';source_dir.mkdir()
    for name in ['RepairWeaponMeshes.py','CompleteWeaponSurfaces.py','RemoveInteriorBackfaces.py','ExportBodyEquipment.py','ExportLobbyScene.py']:shutil.copy2(assets/name,source_dir/name)
    shutil.copy2(package/'SetupAssets.py',source_dir/'SetupAssets.py')
    (dest/'PACKAGE CHECKS.txt').write_text('BF2142 VR 0.2.0-beta.4-test.1\nSee release CHECKS.txt for exact payload validation.\nIPC 26 / pose v4. No new headset acceptance claim.\n')
    # Reject this build's private input locations in either path spelling.
    private_roots=[repo.parent,checkpoint,tools,python_home,python_env,Path.home()]
    forbidden=sorted({str(p.resolve()).lower().replace('\\','/') for p in private_roots})
    forbidden += [p.replace('/','\\') for p in forbidden]
    files={}
    for path in sorted(dest.rglob('*')):
        if not path.is_file():continue
        rel=path.relative_to(dest).as_posix();data=path.read_bytes();lower=data.lower()
        for text in forbidden:
            if text.encode() in lower or text.encode('utf-16le') in lower:raise ValueError('Private build path in '+rel)
        if path.suffix.lower() in ['.pdb','.dmp','.bik','.bundledmesh'] or path.name in ['Weapons_client.zip','BodyEquipment.bin','LobbyScene.bin','install.json']:raise ValueError('Private or developer artifact: '+rel)
        files[rel]=sha(path)
    (dest/'payload.json').write_text(json.dumps({'version':'0.2.0-beta.4-test.1','build':'wrist-rendering-beta4-test1','files':files},indent=2))
    print('Staged',len(files),'files;',sum(x.stat().st_size for x in dest.rglob('*') if x.is_file()),'bytes:',dest)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['checkpoint','tools','python-home','python-env','destination']:p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();stage(a.checkpoint,a.tools,a.python_home,a.python_env,a.destination)
