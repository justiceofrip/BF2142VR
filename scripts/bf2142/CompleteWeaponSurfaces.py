"""Complete absent first-person surfaces using the owned matching world mesh.
No assets included. Match outward-facing coverage by rigid part, retain original
FP triangles, and borrow donor UV/material data only for uncovered regions.
"""
import copy, math, struct

def xyz(v):return struct.unpack_from('<3f',v)
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def add(a,b):return tuple(x+y for x,y in zip(a,b))
def mul(a,s):return tuple(x*s for x in a)
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def unit(a):
    length=math.sqrt(dot(a,a));return mul(a,1/length) if length>1e-10 else (0.,0.,0.)
def normal(v):return struct.unpack_from('<3f',v,12)

class Triangle:
    def __init__(self,vertices):
        self.a,self.b,self.c=[xyz(v) for v in vertices]
        self.e1=sub(self.b,self.a);self.e2=sub(self.c,self.a)
        self.normal=unit(cross(self.e1,self.e2))
        self.low=tuple(min(p[i] for p in (self.a,self.b,self.c)) for i in range(3))
        self.high=tuple(max(p[i] for p in (self.a,self.b,self.c)) for i in range(3))
    def hits(self,origin,direction,limit,outward):
        if dot(self.normal,outward)<.35:return False
        h=cross(direction,self.e2);det=dot(self.e1,h)
        if abs(det)<1e-11:return False
        inv=1/det;s=sub(origin,self.a);u=dot(s,h)*inv
        if u<-.00001 or u>1.00001:return False
        q=cross(s,self.e1);v=dot(direction,q)*inv
        if v<-.00001 or u+v>1.00001:return False
        t=dot(self.e2,q)*inv
        return -.00001<=t<=limit+.00001

class Tree:
    def __init__(self,triangles):
        self.low=tuple(min(t.low[i] for t in triangles) for i in range(3))
        self.high=tuple(max(t.high[i] for t in triangles) for i in range(3))
        self.children=None;self.triangles=triangles
        if len(triangles)>12:
            axis=max(range(3),key=lambda i:self.high[i]-self.low[i])
            ordered=sorted(triangles,key=lambda t:t.low[axis]+t.high[axis]);half=len(ordered)//2
            self.children=(Tree(ordered[:half]),Tree(ordered[half:]));self.triangles=None
    def hits(self,origin,direction,limit,outward):
        # Avoid nested recursive any()/generator frames in the installer hot
        # loop. Preserve left-before-right traversal and all geometric tests.
        pending=[self]
        while pending:
            node=pending.pop();low=0.;high=limit;overlaps=True
            for i in range(3):
                if abs(direction[i])<1e-10:
                    if origin[i]<node.low[i]-1e-7 or origin[i]>node.high[i]+1e-7:
                        overlaps=False;break
                else:
                    a=(node.low[i]-origin[i])/direction[i];b=(node.high[i]-origin[i])/direction[i]
                    low=max(low,min(a,b));high=min(high,max(a,b))
                    if low>high+1e-7:overlaps=False;break
            if not overlaps:continue
            if node.children:
                pending.extend(reversed(node.children))
            else:
                for triangle in node.triangles:
                    if triangle.hits(origin,direction,limit,outward):return True
        return False

def donor_vertex(v,name):
    out=bytearray(v)
    # These Voss variants use an enlarged, nonuniformly scaled FP gun. The
    # shared moving-part landmarks establish the matching coordinate frame.
    if name in ('unl_har_rifle','unl_har_rocket'):
        scale=(1.6,1.2,1.2);offset=(0.,.006,.03)
        struct.pack_into('<3f',out,0,*(a*s+b for a,s,b in zip(xyz(v),scale,offset)))
        struct.pack_into('<3f',out,12,*unit(tuple(a/s for a,s in zip(normal(v),scale))))
        tangent=struct.unpack_from('<3f',v,36)
        struct.pack_into('<3f',out,36,*unit(tuple(a*s for a,s in zip(tangent,scale))))
    # PAC pistol's world bolt uses part 2; its FP bolt is part 3.
    if name=='as_handgun' and out[24]==2:out[24]=3
    return bytes(out)

def complete(mesh,name,interpolate,plan=None,record=None):
    native=mesh.lods[0][0];parts={};allowed=set()
    for mat in native:
        if mat['alpha']!=0:continue
        allowed.update(v[24] for v in mat['vertices'])
        if plan is not None:continue  # Decisions already verified for this exact source hash.
        for i in range(0,len(mat['indices']),3):
            vs=[mat['vertices'][j] for j in mat['indices'][i:i+3]]
            if len({v[24] for v in vs})!=1:continue
            t=Triangle(vs)
            if dot(t.normal,t.normal)>.5:parts.setdefault(vs[0][24],[]).append(t)
    trees={part:Tree(ts) for part,ts in parts.items()}
    result={'donor_faces':0,'donor_parts':{},'skipped_unmapped_faces':0,'donor_materials':0}
    # The tree always describes the original detailed FP surface, never prior
    # donations or reversed inner faces. World geometry stays untouched.
    for source in mesh.lods[1][0]:
        if source['alpha']!=0:continue
        materials=[];material=copy.deepcopy(source);material['vertices']=[];material['indices']=[];lookup={}
        def append_triangle(vs):
            nonlocal material,lookup
            # Inset the new skin half a millimetre so original detailed faces
            # retain depth priority at the boundary, with no coplanar fighting.
            inset=[]
            for v in vs:
                v=bytearray(v);struct.pack_into('<3f',v,0,*sub(xyz(v),mul(unit(normal(v)),.0005)));inset.append(bytes(v))
            if len(material['vertices'])+3>30000:
                materials.append(material);material=copy.deepcopy(source);material['vertices']=[];material['indices']=[];lookup={}
            for v in inset:
                if v not in lookup:lookup[v]=len(material['vertices']);material['vertices'].append(v)
                material['indices'].append(lookup[v])
            result['donor_faces']+=1;part=vs[0][24];result['donor_parts'][str(part)]=result['donor_parts'].get(str(part),0)+1
        cache={}
        def covered(v,face_normal,part):
            # Distinguish an outward skin from the opposite wall visible
            # through a hollow mesh. Opposing normals cannot cover this side.
            p=xyz(v);n=face_normal;key=(p,n,part)
            if key not in cache:
                tree=trees.get(part);radius=.018
                cache[key]=bool(tree and tree.hits(add(p,mul(n,radius)),mul(n,-1),radius*2,n))
            return cache[key]
        def fill(vs,n,part,depth=0):
            if plan is not None:
                decision=plan.take(2)
            else:
                center=interpolate(interpolate(vs[0],vs[1],.5),vs[2],1/3)
                flags=[covered(v,n,part) for v in (*vs,center)]
                if all(flags):decision=0
                elif not any(flags):decision=1
                elif depth>=4 or max(dot(sub(xyz(a),xyz(b)),sub(xyz(a),xyz(b))) for a,b in zip(vs,vs[1:]+vs[:1]))<.006**2:
                    decision=0 if flags[-1] else 1
                else:decision=2
                if record is not None:record.append(decision)
            if decision==0:return
            if decision==1:append_triangle(vs);return
            if depth>=4:raise ValueError('Invalid surface plan depth')
            ab=interpolate(vs[0],vs[1],.5);bc=interpolate(vs[1],vs[2],.5);ca=interpolate(vs[2],vs[0],.5)
            for child in ([vs[0],ab,ca],[ab,vs[1],bc],[ca,bc,vs[2]],[ab,bc,ca]):fill(child,n,part,depth+1)
        for i in range(0,len(source['indices']),3):
            vs=[donor_vertex(source['vertices'][j],name) for j in source['indices'][i:i+3]]
            ids={v[24] for v in vs}
            if len(ids)!=1 or vs[0][24] not in allowed:result['skipped_unmapped_faces']+=1;continue
            n=unit(cross(sub(xyz(vs[1]),xyz(vs[0])),sub(xyz(vs[2]),xyz(vs[0]))))
            if dot(n,n)<.5:continue
            fill(vs,n,vs[0][24])
        if material['indices']:materials.append(material)
        result['donor_materials']+=len(materials);native.extend(materials)
    positions=[xyz(v) for material in native for v in material['vertices']]
    prior=mesh.bounds[0]
    mesh.bounds[0]=tuple(min(v[i] for v in positions) for i in range(3))+tuple(max(v[i] for v in positions) for i in range(3))+(prior[-1],)
    if plan is not None:plan.finish()
    return result
