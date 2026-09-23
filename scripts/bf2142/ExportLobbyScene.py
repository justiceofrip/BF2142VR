"""Export two display walkers from an owned stock BF2142 installation.
Writes a new private asset pack; never modifies archives. Do not distribute it.
"""
import argparse,zipfile,struct,math,subprocess,tempfile,json
from pathlib import Path
from RepairWeaponMeshes import Mesh

def multiply(a,b):return [[sum(a[i][k]*b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]
def matrix(pos=(0,0,0),rot=(0,0,0)):
 y,p,r=[math.radians(v) for v in rot];cy,sy,cp,sp,cr,sr=math.cos(y),math.sin(y),math.cos(p),math.sin(p),math.cos(r),math.sin(r)
 a=multiply(multiply([[cr,sr,0,0],[-sr,cr,0,0],[0,0,1,0],[0,0,0,1]],[[1,0,0,0],[0,cp,sp,0],[0,-sp,cp,0],[0,0,0,1]]),[[cy,0,-sy,0],[0,1,0,0],[sy,0,cy,0],[0,0,0,1]])
 a[3]=[*pos,1];return a

def export(folder,output,ffmpeg,decode=None):
 if output.exists():raise ValueError('Refusing to overwrite an asset pack')
 client=zipfile.ZipFile(folder/'Vehicles_client.zip');server=zipfile.ZipFile(folder/'Vehicles_server.zip')
 lookup={n.lower():n for n in client.namelist()};servernames={n.lower():n for n in server.namelist()}
 draws=[];report={}
 for name,placement in [('us_heavy_mech',(-5.1,0,9.5,155)),('as_heavy_mech',(6.4,0,13.0,-153))]:
  mesh=Mesh(client.read(lookup[f'{name}/meshes/{name}.bundledmesh']))
  nodes={};current=None;child=None
  for suffix in ['con','tweak']:
   for line in server.read(servernames[f'{name}/{name}.{suffix}']).decode('latin1').splitlines():
    tokens=line.strip().split();
    if not tokens:continue
    cmd=tokens[0].lower()
    if cmd in ['objecttemplate.create','objecttemplate.activesafe'] and len(tokens)>=3:
     current=nodes.setdefault(tokens[2].lower(),{'children':[]});child=None
    elif current is not None:
     if cmd=='objecttemplate.addtemplate' and len(tokens)==2:
      child={'name':tokens[1].lower(),'pos':(0,0,0),'rot':(0,0,0)};current['children'].append(child)
     elif cmd=='objecttemplate.geometrypart':current['part']=int(tokens[1])
     elif child is not None and cmd in ['objecttemplate.setposition','objecttemplate.setrotation']:
      child['pos' if cmd.endswith('position') else 'rot']=tuple(float(x) for x in tokens[1].split('/'))
  nodes[name]['part']=0;parts={}
  def visit(key,transform,depth=0):
   if depth>12:raise ValueError('Cyclic object graph')
   node=nodes.get(key,{})
   if 'part' in node:
    part=node['part']
    if part in parts and parts[part]!=transform:raise ValueError('Ambiguous part transform')
    parts[part]=transform
   for c in node.get('children',[]):visit(c['name'],multiply(matrix(c['pos'],c['rot']),transform),depth+1)
  visit(name,matrix())
  materials=mesh.lods[1][1];positions=[];data=[]
  for mat in materials:
   if mat['alpha']!=0:continue
   verts=[]
   for v in mat['vertices']:
    part=v[24]
    if part not in parts:raise ValueError(f'{name}: unmapped part {part}')
    t=parts[part];p=struct.unpack_from('<3f',v);n=struct.unpack_from('<3f',v,12);uv=struct.unpack_from('<2f',v,28)
    p=tuple(sum(p[k]*t[k][j] for k in range(3))+t[3][j] for j in range(3));n=tuple(sum(n[k]*t[k][j] for k in range(3)) for j in range(3))
    positions.append(p);verts.append((p,n,uv))
   data.append((mat,verts))
  low=[min(p[i] for p in positions) for i in range(3)];high=[max(p[i] for p in positions) for i in range(3)]
  transform=matrix((placement[0],-1.6-low[1],placement[2]),(placement[3],0,0))
  for mat,verts in data:
   vertices=bytearray()
   for p,n,uv in verts:
    p=tuple(sum(p[k]*transform[k][j] for k in range(3))+transform[3][j] for j in range(3));n=tuple(sum(n[k]*transform[k][j] for k in range(3)) for j in range(3))
    vertices+=struct.pack('<8f',*p,*n,*uv)
   key=mat['maps'][0].decode('latin1').lower().replace('\\','/').removeprefix('objects/vehicles/')
   if decode is not None:texture=decode(client.read(lookup[key]),1024)
   else:
    with tempfile.TemporaryDirectory() as d:
     path=Path(d)/'texture.dds';path.write_bytes(client.read(lookup[key]))
     texture=subprocess.run([str(ffmpeg),'-loglevel','error','-i',str(path),'-vf','scale=1024:1024','-frames:v','1','-f','rawvideo','-pix_fmt','bgra','pipe:1'],capture_output=True,check=True).stdout
   assert len(texture)==1024*1024*4
   indices=mat['indices'];draws.append(struct.pack('<4I',len(verts),len(indices),1024,1024)+vertices+struct.pack('<'+str(len(indices))+'H',*indices)+texture)
  report[name]={'bounds':[low,high],'parts':len(parts),'triangles':sum(len(m['indices'])//3 for m,_ in data),'placement':placement}
 output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(b'BFLS0001'+struct.pack('<I',len(draws))+b''.join(draws));output.with_suffix('.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2));print('Private lobby pack:',output,output.stat().st_size)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--objects',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--ffmpeg',type=Path,required=True);a=p.parse_args();export(a.objects,a.output,a.ffmpeg)
