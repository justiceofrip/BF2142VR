import sys,struct,unittest,copy
from RepairWeaponMeshes import Mesh,two_sided,position
from TestWeaponMeshes import independent_blended_fixture
from ThinShellBackfaces import repair

class ThinTests(unittest.TestCase):
 def shell(self,thickness,part=0,otherpart=None):
  m=Mesh(independent_blended_fixture()[0]);a=m.lods[0][0][0]
  vs=[]
  for i,z in enumerate((0,-thickness)):
   for x,y in ((0,0),(.03,0),(0,.03)):
    v=bytearray(a['vertices'][0]);struct.pack_into('<3f',v,0,x,y,z);v[24]=part if i==0 or otherpart is None else otherpart;vs.append(bytes(v))
  a['vertices']=vs;a['indices']=[0,1,2,5,4,3];two_sided(a);return m
 def test_opposing_moving_part_wall_limits_inset_without_changing_exterior(self):
  for part in (0,1,4,5):
   m=self.shell(.0003,part);before=copy.deepcopy(m);data,stats=repair(m.encode());after=Mesh(data);a=after.lods[0][0][0]
   self.assertEqual(stats['thin_shell_limited'],2);self.assertEqual(stats['sub_quantization_reverse_faces_removed'],0)
   self.assertEqual(a['vertices'][:6],before.lods[0][0][0]['vertices'][:6]);self.assertEqual(a['indices'][:6],[0,1,2,5,4,3])
   self.assertEqual(len(a['indices']),12)
   for j in a['indices'][6:]:self.assertGreater(position(a['vertices'][j])[2],-.0003);self.assertLess(position(a['vertices'][j])[2],0)
   self.assertEqual(after.lods[1:],before.lods[1:]);self.assertEqual(after.lods[0][0][1],before.lods[0][0][1])
 def test_unrepresentable_inner_layer_removed_but_both_exteriors_preserved(self):
  m=self.shell(.000001);data,stats=repair(m.encode());a=Mesh(data).lods[0][0][0]
  self.assertEqual(stats['sub_quantization_reverse_faces_removed'],2);self.assertEqual(a['indices'],[0,1,2,5,4,3])
 def test_unrelated_animated_part_cannot_clip_face(self):
  data,stats=repair(self.shell(.0001,1,4).encode());self.assertEqual(stats['thin_shell_limited'],0);self.assertEqual(stats['sub_quantization_reverse_faces_removed'],0)
 def test_open_sheet_keeps_its_reverse_surface(self):
  m=self.shell(.0003);a=m.lods[0][0][0];a['indices']=[0,1,2,8,7,6]
  data,stats=repair(m.encode());self.assertEqual(stats['thin_shell_limited'],0);self.assertEqual(len(Mesh(data).lods[0][0][0]['indices']),6)
if __name__=='__main__':unittest.main()
