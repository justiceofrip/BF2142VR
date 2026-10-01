"""Remove verified repair-added inside faces of solid weapon shells.
Operates on the owner's completed v20 archive; no game assets are distributed.
Preserve every exterior triangle, all shader data, thin two-sided details,
transparent materials, animation IDs, and all world geometry.
"""
import argparse,copy,hashlib,json,zipfile
from pathlib import Path
from RepairWeaponMeshes import Mesh,NAMES,inside_vertex,position
from CompleteWeaponSurfaces import Triangle,Tree,unit,cross,sub,add,mul,dot

def halves(material):
    if material['alpha']!=0:return None
    vs=material['vertices'];ix=material['indices']
    if len(vs)%2 or len(ix)%6:raise ValueError('Not a completed v20 material')
    n=len(vs)//2;k=len(ix)//2
    if vs[n:]!=[inside_vertex(v) for v in vs[:n]]:raise ValueError('Inner vertex provenance differs')
    reversed_indices=[]
    for i in range(0,k,3):
        a,b,c=ix[i:i+3]
        if max(a,b,c)>=n:raise ValueError('Exterior references inner vertex')
        reversed_indices.extend((c+n,b+n,a+n))
    if ix[k:]!=reversed_indices:raise ValueError('Inner face provenance differs')
    return n,k

def repair(data,plan=None,record=None):
    mesh=Mesh(data);before=copy.deepcopy(mesh.lods);removed=0;retained=0
    for lod in mesh.lods[0]:
        solids=[]
        for m in lod:
            split=halves(m)
            if split is None:continue
            n,k=split
            for i in range(0,k,3):
                vs=[m['vertices'][j] for j in m['indices'][i:i+3]]
                if any(v[24]!=0 for v in vs):continue
                t=Triangle(vs)
                if dot(t.normal,t.normal)>.5:solids.append(t)
        if not solids:continue
        tree=Tree(solids) if plan is None else None
        for m in lod:
            split=halves(m)
            if split is None:continue
            n,k=split;keep=m['indices'][:k]
            for i in range(0,k,3):
                if plan is not None:
                    solid=bool(plan.take(1))
                else:
                    vs=[m['vertices'][j] for j in m['indices'][i:i+3]];ps=[position(v) for v in vs]
                    normal=unit(cross(sub(ps[1],ps[0]),sub(ps[2],ps[0])))
                    center=mul(add(add(ps[0],ps[1]),ps[2]),1/3)
                    # A matching opposing skin within 20 cm establishes a solid
                    # shell. Thin rails, sights and disconnected details retain
                    # their reverse faces; only rigid root part 0 is considered.
                    inward=mul(normal,-1)
                    solid=all(v[24]==0 for v in vs) and tree.hits(add(center,mul(inward,.001)),inward,.20,inward)
                    if record is not None:record.append(int(solid))
                if solid:removed+=1
                else:keep.extend(m['indices'][k+i:k+i+3]);retained+=1
            m['indices']=keep
    encoded=mesh.encode();check=Mesh(encoded)
    if check.lods[1:]!=before[1:]:raise ValueError('World geometry changed')
    for old_lod,new_lod in zip(before[0],check.lods[0]):
        for old,new in zip(old_lod,new_lod):
            split=halves(old)
            if split is None:
                if old!=new:raise ValueError('Transparent material changed')
                continue
            n,k=split
            if new['indices'][:k]!=old['indices'][:k]:raise ValueError('An exterior triangle changed')
            for key in old:
                if key!='indices' and old[key]!=new[key]:raise ValueError('Non-index material data changed')
    if plan is not None:plan.finish()
    return encoded,{'removed_inner_faces':removed,'retained_thin_reverse_faces':retained}

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--input',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--report',type=Path,required=True);a=ap.parse_args()
    if a.output.exists() or a.input.resolve()==a.output.resolve():raise ValueError('Output must be new')
    result={'input_sha256':hashlib.sha256(a.input.read_bytes()).hexdigest(),'weapons':{}}
    with zipfile.ZipFile(a.input) as source,zipfile.ZipFile(a.output,'x',compression=zipfile.ZIP_DEFLATED) as target:
        for item in source.infolist():
            data=source.read(item.filename);name=Path(item.filename).stem.lower()
            if name in NAMES and item.filename.lower().endswith('.bundledmesh'):
                data,stats=repair(data);result['weapons'][name]=stats;print(name,stats,flush=True)
            target.writestr(item,data)
    if set(result['weapons'])!=NAMES:raise ValueError('Reviewed weapon set incomplete')
    with zipfile.ZipFile(a.output) as z:
        if z.testzip():raise ValueError('Archive CRC failed')
    result['output_sha256']=hashlib.sha256(a.output.read_bytes()).hexdigest();a.report.write_text(json.dumps(result,indent=2));print(result['output_sha256'])
if __name__=='__main__':main()
