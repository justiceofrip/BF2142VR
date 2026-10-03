import copy,math,struct,sys,unittest
from RepairWeaponMeshes import Mesh,two_sided,position
from TestWeaponMeshes import independent_blended_fixture
from PlaneSeparatedBackfaces import repair,INSET_METRES

class PlaneTests(unittest.TestCase):
 def mesh(self,normals=None):
  m=Mesh(independent_blended_fixture()[0]);a=m.lods[0][0][0]
  if normals:
   for i,n in enumerate(normals):
    v=bytearray(a['vertices'][i]);struct.pack_into('<3f',v,12,*n);a['vertices'][i]=bytes(v)
  two_sided(a);return m
 def test_smooth_normals_cannot_move_inside_face_outside_or_sideways(self):
  for normals in [[(0,0,1)]*3,[(1,0,0),(-1,0,0),(0,0,-1)],[(0,0,-1)]*3]:
   with self.subTest(normals=normals):
    m=self.mesh(normals);a=m.lods[0][0][0];fixed,_=repair(m.encode());b=Mesh(fixed).lods[0][0][0]
    for ia,ib in zip(a['indices'][3:],b['indices'][3:]):
     p=position(a['vertices'][ia]);q=position(b['vertices'][ib])
     self.assertEqual(p[:2],q[:2]);self.assertAlmostEqual(q[2]-p[2],-INSET_METRES,places=8)
     self.assertEqual(a['vertices'][ia][12:],b['vertices'][ib][12:])
 def test_preserves_exterior_alpha_world_bounds_and_surface_count(self):
  m=self.mesh();before=copy.deepcopy(m);fixed,_=repair(m.encode());after=Mesh(fixed)
  self.assertEqual(after.bounds[before.counts[0]:],before.bounds[before.counts[0]:]);self.assertEqual(after.lods[1:],before.lods[1:])
  for bounds,lod in zip(after.bounds,after.lods[0]):
   for mat in lod:
    for v in mat['vertices']:self.assertTrue(all(bounds[j]<=p<=bounds[j+3] for j,p in enumerate(position(v))))
  self.assertEqual(after.lods[0][0][1],before.lods[0][0][1])
  a,b=before.lods[0][0][0],after.lods[0][0][0]
  self.assertEqual(a['vertices'][:3],b['vertices'][:3]);self.assertEqual(a['indices'][:3],b['indices'][:3])
  self.assertEqual(len(a['indices']),len(b['indices']))
 def test_trimmed_reverse_faces_do_not_reappear(self):
  m=self.mesh();m.lods[0][0][0]['indices']=m.lods[0][0][0]['indices'][:3]
  data,stats=repair(m.encode());self.assertEqual(stats['separated_inner_faces'],0)
  self.assertEqual(Mesh(data).lods[0][0][0]['indices'],[0,1,2])
 def test_degenerate_face_stays_degenerate(self):
  m=Mesh(independent_blended_fixture()[0]);a=m.lods[0][0][0]
  a['vertices']=[a['vertices'][0]]*3;two_sided(a)
  data,stats=repair(m.encode());self.assertEqual(stats['degenerate_preserved'],1)
  b=Mesh(data).lods[0][0][0]
  self.assertEqual(len({position(b['vertices'][i]) for i in b['indices'][3:]}),1)
 def test_rejects_unverified_reverse_geometry(self):
  with self.assertRaises(ValueError):repair(independent_blended_fixture()[0])
  m=self.mesh();m.lods[0][0][0]['indices'][-1]=0
  with self.assertRaises(ValueError):repair(m.encode())

if __name__=='__main__':unittest.main()
