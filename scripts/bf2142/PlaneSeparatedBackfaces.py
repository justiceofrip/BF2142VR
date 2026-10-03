"""Weapon repair: translate each added inner face along its geometric normal.

Preserve original exterior vertices/indices, materials and world LODs. Lighting
normals are not displacement directions: sharp-edge smoothing can point them
outside a thin shell or invert a small triangle when offset per vertex.
"""
import copy, math, struct
from RepairWeaponMeshes import Mesh, inside_vertex, position, expand_first_person_bounds
from CompleteWeaponSurfaces import cross, sub, dot

INSET_METRES = .0005

def repair(data):
    mesh=Mesh(data); before=copy.deepcopy(mesh.lods); moved=0; faces=0; degenerate=0
    for lod in mesh.lods[0]:
        for mat in lod:
            if mat['alpha'] != 0: continue
            vertices=mat['vertices']; indices=mat['indices']; half=len(vertices)//2
            if len(vertices)%2 or vertices[half:] != [inside_vertex(v) for v in vertices[:half]]:
                raise ValueError('Repair backface provenance differs')
            front=[]; back=[]; seen_back=False; outer=set()
            for at in range(0,len(indices),3):
                ids=tuple(indices[at:at+3])
                if len(ids)!=3: raise ValueError('Invalid triangle')
                if max(ids)<half:
                    if seen_back: raise ValueError('Exterior follows reverse faces')
                    front.extend(ids)
                    outer.add(ids);outer.add(ids[1:]+ids[:1]);outer.add(ids[2:]+ids[:2])
                elif min(ids)>=half:
                    seen_back=True;back.append(ids)
                else: raise ValueError('Mixed original/reverse triangle')
            result=vertices[:half]; output=front[:]; lookup={}
            for ids in back:
                if tuple(i-half for i in reversed(ids)) not in outer:
                    raise ValueError('Reverse triangle has no original counterpart')
                vs=[vertices[i] for i in ids]; p=list(map(position,vs))
                n=cross(sub(p[1],p[0]),sub(p[2],p[0])); length=math.sqrt(dot(n,n))
                if not math.isfinite(length) or not all(math.isfinite(x) for pt in p for x in pt):
                    raise ValueError('Invalid repair-added face')
                # Degenerate faces produce no pixels; do not turn them into new
                # geometry by moving their corners along unrelated smooth normals.
                shift=tuple(x*INSET_METRES/length for x in n) if length>1e-12 else (0.,0.,0.)
                if length<=1e-12:degenerate+=1
                shifted=[]
                for v,pt in zip(vs,p):
                    out=bytearray(v);struct.pack_into('<3f',out,0,*(x+d for x,d in zip(pt,shift)))
                    out=bytes(out)
                    if out not in lookup:
                        if len(result)>=65535:raise ValueError('Separated material exceeds 16-bit vertex limit')
                        lookup[out]=len(result);result.append(out);moved+=1
                    output.append(lookup[out]);shifted.append(position(out))
                after=cross(sub(shifted[1],shifted[0]),sub(shifted[2],shifted[0]))
                if length>1e-12 and dot(n,after)<=0:raise ValueError('Face orientation changed after float packing')
                faces+=1
            mat['vertices']=result;mat['indices']=output
    expanded=expand_first_person_bounds(mesh)
    encoded=mesh.encode(); check=Mesh(encoded)
    if check.lods[1:]!=before[1:] or check.bounds!=mesh.bounds:raise ValueError('Protected world geometry changed')
    for oldlod,newlod in zip(before[0],check.lods[0]):
        for a,b in zip(oldlod,newlod):
            if a['alpha']:
                if a!=b:raise ValueError('Transparent material changed')
                continue
            half=len(a['vertices'])//2
            if a['vertices'][:half]!=b['vertices'][:half]:raise ValueError('Exterior vertices changed')
            for key in a:
                if key not in ('vertices','indices') and a[key]!=b[key]:raise ValueError('Material data changed')
            if len(a['indices'])!=len(b['indices']):raise ValueError('Surface removed')
            for at in range(0,len(a['indices']),3):
                ia=a['indices'][at:at+3];ib=b['indices'][at:at+3]
                if max(ia)<half:
                    if ia!=ib:raise ValueError('Exterior indices changed')
                elif any(a['vertices'][x][12:]!=b['vertices'][y][12:] for x,y in zip(ia,ib)):
                    raise ValueError('Reverse shader attributes changed')
    return encoded,dict(separated_inner_vertices=moved,separated_inner_faces=faces,
                        degenerate_preserved=degenerate,inset_metres=INSET_METRES,
                        expanded_first_person_bounds=expanded)
