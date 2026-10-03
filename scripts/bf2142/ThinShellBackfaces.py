"""Weapon repair: keep translated repair faces inside thin moving parts.

Only verified reverse faces may change. Original/exterior geometry, UVs,
normals, part assignments and world LODs remain byte-for-byte intact.
"""
import collections,math,struct
from RepairWeaponMeshes import Mesh,inside_vertex,position
from CompleteWeaponSurfaces import Triangle,Tree,sub,add,mul,dot,cross,unit
from PlaneSeparatedBackfaces import repair as plane_repair,INSET_METRES

def nearest(tree,origin,direction,limit):
    best=None;pending=[tree]
    while pending:
        node=pending.pop();lo=0.;hi=limit if best is None else best
        for i in range(3):
            if abs(direction[i])<1e-10:
                if origin[i]<node.low[i]-1e-8 or origin[i]>node.high[i]+1e-8:break
            else:
                a=(node.low[i]-origin[i])/direction[i];b=(node.high[i]-origin[i])/direction[i]
                lo=max(lo,min(a,b));hi=min(hi,max(a,b))
                if lo>hi+1e-8:break
        else:
            if node.children:pending.extend(node.children);continue
            for t in node.triangles:
                if dot(t.normal,direction)<.35:continue
                h=cross(direction,t.e2);det=dot(t.e1,h)
                if abs(det)<1e-12:continue
                s=sub(origin,t.a);u=dot(s,h)/det;q=cross(s,t.e1);v=dot(direction,q)/det
                if u<0 or v<0 or u+v>1:continue
                distance=dot(t.e2,q)/det
                if -1e-8<=distance<=limit and (best is None or distance<best):best=max(0.,distance)
    return best

def repair(data):
    # Run the existing provenance and exterior-preservation checks first.
    baseline,stats=plane_repair(data);source=Mesh(data);out=Mesh(baseline)
    limited=removed=0
    quant=max(abs(x) for b in out.bounds[:out.counts[0]] for x in b[:6])/32767
    for lod,fixed_lod in zip(source.lods[0],out.lods[0]):
        parts=collections.defaultdict(list)
        for m in lod:
            if m['alpha']:continue
            half=len(m['vertices'])//2
            for i in range(0,len(m['indices']),3):
                ids=m['indices'][i:i+3]
                if max(ids)>=half:continue
                vs=[m['vertices'][j] for j in ids]
                if len({v[24] for v in vs})==1:parts[vs[0][24]].append(Triangle(vs))
        trees={p:Tree(ts) for p,ts in parts.items()}
        for original,fixed in zip(lod,fixed_lod):
            if original['alpha']:continue
            half=len(original['vertices'])//2
            vertices=original['vertices'][:half];indices=[];lookup={}
            for i in range(0,len(original['indices']),3):
                ids=original['indices'][i:i+3]
                if max(ids)<half:indices.extend(ids);continue
                vs=[original['vertices'][j] for j in ids];ps=[position(v) for v in vs]
                n=unit(cross(sub(ps[1],ps[0]),sub(ps[2],ps[0])))
                center=mul(add(add(ps[0],ps[1]),ps[2]),1/3)
                distance=None
                if len({v[24] for v in vs})==1 and dot(n,n)>.5:
                    tree=trees[vs[0][24]]
                    probes=[center]+[add(mul(p,.99),mul(center,.01)) for p in ps]
                    probes += [add(mul(add(a,b),.495),mul(center,.01)) for a,b in zip(ps,ps[1:]+ps[:1])]
                    hits=[nearest(tree,p,n,INSET_METRES*1.2) for p in probes]
                    distance=min((h for h in hits if h is not None),default=None)
                shift=INSET_METRES
                if distance is not None:
                    # Below two position quantization steps there is no distinct
                    # interior layer to store. The opposing exterior already
                    # supplies that wall; do not create a competing reverse skin.
                    if distance<=quant*2:removed+=1;continue
                    shift=min(shift,distance*.35);limited+=int(shift<INSET_METRES)
                for v,p in zip(vs,ps):
                    b=bytearray(v);struct.pack_into('<3f',b,0,*add(p,mul(n,shift)));b=bytes(b)
                    if b not in lookup:
                        if len(vertices)>=65535:raise ValueError('16-bit vertex limit')
                        lookup[b]=len(vertices);vertices.append(b)
                    indices.append(lookup[b])
            fixed['vertices']=vertices;fixed['indices']=indices
    # Bounds from the full fixed-offset candidate already enclose these shorter
    # shifts and preserve the accepted PAC pistol packed-coordinate repair.
    result=out.encode();check=Mesh(result)
    if check.lods[1:]!=source.lods[1:]:raise ValueError('World LOD changed')
    for a,b in zip(source.lods[0],check.lods[0]):
        for old,new in zip(a,b):
            if old['alpha']:
                if old!=new:raise ValueError('Transparent surface changed')
                continue
            half=len(old['vertices'])//2
            outer=[j for i in range(0,len(old['indices']),3) if max(old['indices'][i:i+3])<half for j in old['indices'][i:i+3]]
            if old['vertices'][:half]!=new['vertices'][:half] or new['indices'][:len(outer)]!=outer:raise ValueError('Exterior changed')
    stats.update(thin_shell_limited=limited,sub_quantization_reverse_faces_removed=removed)
    return result,stats
