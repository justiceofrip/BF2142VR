"""Local, reversible stock BF2142 weapon repair. Contains no game assets.
Reads an owned Weapons_client.zip, writes a NEW archive, never edits the input.
Completes absent normal first-person surfaces from matching world models for
28 stock firearm/attachment meshes, then repairs backface visibility in both
first-person LODs. World LODs and all transparent index sets stay intact.
Run TestWeaponMeshes.py and TestCompleteSurfaces.py for fixtures.
"""
import argparse, copy, hashlib, json, math, struct, zipfile
from pathlib import Path
from CompleteWeaponSurfaces import complete
NAMES = {
    'as_aa', 'as_ar_rifle', 'as_ar_rocket', 'as_av', 'as_handgun',
    'as_mg', 'as_smg', 'as_sni', 'bp1_expl_shotgun', 'eu_aa',
    'eu_ar_rifle', 'eu_ar_rocket', 'eu_av', 'eu_handgun', 'eu_mg',
    'eu_smg', 'eu_sni', 'unl_adv_sni', 'unl_av_rifle', 'unl_best_buy_rifle',
    'unl_best_buy_rocket', 'unl_carbine', 'unl_har_rifle', 'unl_har_rocket', 'unl_hmg',
    'unl_lar_rifle', 'unl_lar_rocket', 'unl_shotgun',
}

def pack(fmt,*v): return struct.pack("<"+fmt,*v)
class Mesh:
    def __init__(self,data): self.data=data; self.at=0; self.read()
    def take(self,fmt):
        n=struct.calcsize("<"+fmt)
        if self.at+n>len(self.data): raise ValueError("Truncated mesh")
        v=struct.unpack_from("<"+fmt,self.data,self.at); self.at+=n
        return v[0] if len(v)==1 else v
    def string(self):
        n=self.take("I")
        if n>4096 or self.at+n>len(self.data): raise ValueError("Invalid material string")
        s=self.data[self.at:self.at+n];self.at+=n;return s
    def read(self):
        self.header=self.take("5IB")
        if self.header!=(0,10,0,0,0,0): raise ValueError("Unrecognized bundledmesh version")
        ng=self.take("I")
        if not 1<ng<8: raise ValueError("Unexpected geometry count")
        self.counts=[self.take("I") for _ in range(ng)]
        if any(n<1 or n>8 for n in self.counts):raise ValueError("Invalid LOD count")
        na=self.take("I")
        if not 1<=na<=16:raise ValueError("Invalid declaration")
        self.attrs=[self.take("4H") for _ in range(na)]
        self.vertex_format,self.stride,nv=self.take("3I")
        if self.stride!=48 or not 0<nv<1000000:raise ValueError("Unrecognized stock vertex layout")
        for item in [(0,0,2,0),(0,12,2,3),(0,24,4,2),(0,28,1,5),(0,36,2,6)]:
            if item not in self.attrs:raise ValueError("Unrecognized vertex attributes")
        end=self.at+nv*self.stride
        if end>len(self.data):raise ValueError("Truncated vertex buffer")
        vertices=[self.data[i:i+self.stride] for i in range(self.at,end,self.stride)];self.at=end
        ni=self.take("I")
        if ni>10000000:raise ValueError("Invalid index count")
        indices=list(self.take(str(ni)+"H")) if ni else []
        self.alpha_sort=self.take("I")
        if not 1<=self.alpha_sort<=8:raise ValueError("Unrecognized alpha sort count")
        self.bounds=[self.take("6fI") for _ in range(sum(self.counts))]
        self.lods=[]
        for count in self.counts:
            group=[]
            for _ in range(count):
                mats=[];nm=self.take("I")
                if nm>64:raise ValueError("Invalid material count")
                for _ in range(nm):
                    alpha=self.take("I");shader=self.string();tech=self.string();nt=self.take("I")
                    if nt>16:raise ValueError("Invalid texture count")
                    maps=[self.string() for _ in range(nt)]
                    vs,ix,n,v,u1,u2=self.take("6I")
                    # Blended materials store one full index list PER sort
                    # direction. Keeping only the first silently shifts every
                    # later draw and makes additive effects use other geometry.
                    sets=self.alpha_sort if alpha==1 else 1
                    if alpha>2 or vs+v>nv or ix+n*sets>ni or n%3:raise ValueError("Invalid draw range")
                    faces=[indices[ix+j*n:ix+(j+1)*n] for j in range(sets)]
                    if any(i>=v for face in faces for i in face):raise ValueError("Invalid local index")
                    material=dict(alpha=alpha,shader=shader,tech=tech,maps=maps,vertices=vertices[vs:vs+v],indices=faces[0],unknown=(u1,u2))
                    if alpha==1:material["face_sets"]=faces
                    mats.append(material)
                group.append(mats)
            self.lods.append(group)
        if self.at!=len(self.data):raise ValueError("Unrecognized mesh trailer")
    def encode(self):
        out=bytearray(pack("5IB",*self.header)+pack("I",len(self.counts)))
        for n in self.counts:out+=pack("I",n)
        out+=pack("I",len(self.attrs))
        for attr in self.attrs:out+=pack("4H",*attr)
        vertices=[];indices=[];draws=[]
        for group in self.lods:
            for mats in group:
                for m in mats:
                    draws.append((len(vertices),len(indices),len(m["indices"]),len(m["vertices"]),*m["unknown"]))
                    vertices.extend(m["vertices"])
                    if m["alpha"]==1:
                        faces=m.get("face_sets")
                        if faces is None or len(faces)!=self.alpha_sort or faces[0]!=m["indices"]:raise ValueError("Missing transparent sort sets")
                        if any(len(f)!=len(m["indices"]) or any(i<0 or i>=len(m["vertices"]) for i in f) for f in faces):raise ValueError("Invalid transparent sort set")
                        for face in faces:indices.extend(face)
                    else:indices.extend(m["indices"])
        out+=pack("3I",self.vertex_format,self.stride,len(vertices))+b"".join(vertices)+pack("I",len(indices))
        out+=pack(str(len(indices))+"H",*indices)+pack("I",self.alpha_sort)
        for b in self.bounds:out+=pack("6fI",*b)
        def string(s):return pack("I",len(s))+s
        draw=iter(draws)
        for group in self.lods:
            for mats in group:
                out+=pack("I",len(mats))
                for m in mats:
                    out+=pack("I",m["alpha"])+string(m["shader"])+string(m["tech"])+pack("I",len(m["maps"]))
                    for texture in m["maps"]:out+=string(texture)
                    out+=pack("6I",*next(draw))
        return bytes(out)

def position(v):return struct.unpack_from("<3f",v)
def inside_vertex(v):
    out=bytearray(v)
    struct.pack_into("<3f",out,12,*(-x for x in struct.unpack_from("<3f",v,12)))
    return bytes(out)
def two_sided(m):
    # Keep transparent/additive optics native: doubling them changes blending.
    if m["alpha"]!=0:return 0
    n=len(m["vertices"])
    if 2*n>65535:raise ValueError("Two-sided material exceeds 16-bit indices")
    original=m["indices"][:];m["vertices"] += [inside_vertex(v) for v in m["vertices"]]
    for i in range(0,len(original),3):
        a,b,c=original[i:i+3];m["indices"] += [c+n,b+n,a+n]
    return len(original)//3

def interpolate(a,b,t):
    # Rigid part IDs are discrete. Tangent handedness uses the nearest
    # endpoint at clipped edges. Clip only triangles
    # from one native part, preserving all original shader attributes.
    if a[24]!=b[24]:raise ValueError("Mixed-part stock triangle")
    out=bytearray(a if t<.5 else b)
    for off,n in [(0,3),(12,3),(28,2),(36,3)]:
        av=struct.unpack_from("<"+str(n)+"f",a,off);bv=struct.unpack_from("<"+str(n)+"f",b,off)
        values=[x+(y-x)*t for x,y in zip(av,bv)]
        if off in (12,36):
            length=math.sqrt(sum(x*x for x in values))
            if length>1e-8:values=[x/length for x in values]
        struct.pack_into("<"+str(n)+"f",out,off,*values)
    return bytes(out)
def clip_rear(vertices,z):
    result=[]
    for a,b in zip(vertices,vertices[1:]+vertices[:1]):
        az=position(a)[2];bz=position(b)[2];ain=az<=z;bin=bz<=z
        if ain:result.append(a)
        if ain!=bin:result.append(interpolate(a,b,(z-az)/(bz-az)))
    return result

def repair(data,name):
    if name not in NAMES:raise ValueError("Weapon has no reviewed repair profile")
    mesh=Mesh(data);old=copy.deepcopy(mesh.lods);stats={"backfaces":0,"backfaces_by_lod":[],"stock_faces":0}
    # PAC assault rifle has no first-person rear stock at all. Its complete
    # third-person stock is in the SAME coordinate frame/rigid part 0. Keep
    # original first-person receiver, scope, magazine and all other details.
    if name=="as_ar_rifle":
        if mesh.counts!=[2,2] or mesh.bounds[0][-1]!=5 or mesh.bounds[2][-1]!=5:raise ValueError("Unrecognized PAC rifle parts")
        if abs(mesh.bounds[0][2]+.182334)>.001 or abs(mesh.bounds[2][2]+.370078)>.001:raise ValueError("Stock bounds differ from reviewed mesh")
        cut=mesh.bounds[0][2]+.0005
        for source in old[1][0]:
            if source["alpha"]!=0:continue
            stock=copy.deepcopy(source);stock["vertices"]=[];stock["indices"]=[]
            for i in range(0,len(source["indices"]),3):
                tri=[source["vertices"][j] for j in source["indices"][i:i+3]]
                if any(v[24]!=0 for v in tri):continue
                poly=clip_rear(tri,cut)
                for k in range(1,len(poly)-1):
                    n=len(stock["vertices"]);stock["vertices"] += [poly[0],poly[k],poly[k+1]];stock["indices"] += [n,n+1,n+2];stats["stock_faces"]+=1
            if stock["indices"]:mesh.lods[0][0].append(stock)
        if not stats["stock_faces"]:raise ValueError("No compatible stock surfaces")
        verts=[position(v) for m in mesh.lods[0][0] for v in m["vertices"]]
        mesh.bounds[0]=tuple(min(v[j] for v in verts) for j in range(3))+tuple(max(v[j] for v in verts) for j in range(3))+(5,)
    stats.update(complete(mesh,name,interpolate))
    # Group 0 includes both the ordinary model and its alternate native ADS
    # mesh. Leaving the latter untouched makes missing surfaces return on ADS.
    for lod in mesh.lods[0]:
        faces=sum(two_sided(material) for material in lod)
        stats["backfaces_by_lod"].append(faces);stats["backfaces"]+=faces
    output=mesh.encode();check=Mesh(output)
    if check.lods[1:]!=old[1:]:raise ValueError("Protected world LOD changed")
    if check.lods!=mesh.lods:raise ValueError("Repair round-trip mismatch")
    for old_lod,new_lod in zip(old[0],check.lods[0]):
        for before,after in zip(old_lod,new_lod):
            if before["alpha"]!=0 and before!=after:raise ValueError("Transparent material changed")
    return output,stats

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    if a.input.resolve()==a.output.resolve() or a.output.exists():raise ValueError("Output must be a new, separate archive")
    result={"input_sha256":hashlib.sha256(a.input.read_bytes()).hexdigest(),"weapons":{}}
    with zipfile.ZipFile(a.input) as source,zipfile.ZipFile(a.output,'x',compression=zipfile.ZIP_DEFLATED) as target:
        for item in source.infolist():
            data=source.read(item.filename);name=Path(item.filename).stem.lower()
            if name in NAMES and item.filename.lower().endswith('.bundledmesh'):
                data,stats=repair(data,name);result['weapons'][name]=stats
                print(name, 'completed donor faces:',stats['donor_faces'],flush=True)
            target.writestr(item,data)
    if set(result['weapons'])!=NAMES:raise ValueError("Not all reviewed stock weapons were found")
    with zipfile.ZipFile(a.output) as z:
        if z.testzip():raise ValueError("Output archive CRC failed")
    result['output_sha256']=hashlib.sha256(a.output.read_bytes()).hexdigest()
    a.report.write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
if __name__=='__main__':main()
