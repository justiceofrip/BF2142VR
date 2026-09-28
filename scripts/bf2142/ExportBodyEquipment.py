"""Build local holster visuals from an owned stock Weapons_client.zip.
The output contains game assets and must stay outside the source repository.
No input archive or game file is modified. Requires ffmpeg for DDS decoding.
"""
import argparse, struct, subprocess, tempfile, zipfile, json
from pathlib import Path
from RepairWeaponMeshes import Mesh

def export(source, output, ffmpeg, extra, decode=None):
    z=zipfile.ZipFile(source);lookup={n.lower().replace('\\','/'):n for n in z.namelist()}
    texture_lookup={key:(z,name) for key,name in lookup.items()}
    for path in extra:
        archive=zipfile.ZipFile(path)
        texture_lookup.update({n.lower().replace('\\','/'):(archive,n) for n in archive.namelist()})
    textures={};models=[];report={}
    def texture(path):
        key=path.decode('latin1').lower().replace('\\','/').removeprefix('objects/weapons/')
        if key in textures:return textures[key]
        if key not in texture_lookup:raise ValueError('Missing texture: '+key)
        raw=texture_lookup[key][0].read(texture_lookup[key][1])
        if decode is not None:pixels=decode(raw,256)
        else:
            with tempfile.TemporaryDirectory() as folder:
                dds=Path(folder)/'source.dds';dds.write_bytes(raw)
                result=subprocess.run([str(ffmpeg),'-hide_banner','-loglevel','error','-i',str(dds),'-vf','scale=256:256','-frames:v','1','-f','rawvideo','-pix_fmt','bgra','pipe:1'],capture_output=True,check=True)
            pixels=result.stdout
        if len(pixels)!=256*256*4:raise ValueError('DDS decode size')
        textures[key]=pixels;return pixels
    for key,n in lookup.items():
        if not key.startswith('handheld/') or not key.endswith('.bundledmesh'):continue
        m=Mesh(z.read(n));name=Path(key).stem.encode('ascii')
        mats=[mat for mat in m.lods[1][0] if mat['alpha']==0 and mat['indices']]
        missing=[mat['maps'][0].decode('latin1').lower().replace('\\','/').removeprefix('objects/weapons/') for mat in mats if mat['maps'][0].decode('latin1').lower().replace('\\','/').removeprefix('objects/weapons/') not in texture_lookup]
        if missing:
            report[name.decode()]={'skipped_missing_texture':missing};continue
        data=bytearray();faces=0
        for mat in mats:
            tex=texture(mat['maps'][0]);v=mat['vertices'];ix=mat['indices'];faces+=len(ix)//3
            if len(v)>65535 or len(ix)>60000:raise ValueError('Oversized draw')
            data+=struct.pack('<4I',len(v),len(ix),256,256)
            for vertex in v:data+=struct.pack('<5f',*struct.unpack_from('<3f',vertex),*struct.unpack_from('<2f',vertex,28))
            data+=struct.pack('<'+str(len(ix))+'H',*ix)+tex
        models.append(struct.pack('<I',len(name))+name+struct.pack('<I',len(mats))+data)
        report[name.decode()]=faces
    output.parent.mkdir(parents=True,exist_ok=True)
    if output.exists():raise ValueError('Refusing to replace an existing asset pack')
    output.write_bytes(b'BFHP0001'+struct.pack('<I',len(models))+b''.join(models))
    output.with_suffix('.json').write_text(json.dumps(report,indent=2))
    print('Exported',len(models),'models;',output.stat().st_size,'bytes:',output)
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--ffmpeg',type=Path,required=True);p.add_argument('--textures',type=Path,action='append',default=[]);a=p.parse_args();export(a.input,a.output,a.ffmpeg,a.textures)
