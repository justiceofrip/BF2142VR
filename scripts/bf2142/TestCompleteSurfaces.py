import copy,struct,unittest
from types import SimpleNamespace
from CompleteWeaponSurfaces import complete,donor_vertex,xyz,Tree,Triangle
from RepairWeaponMeshes import interpolate

def vertex(x,y,z,part=0):return struct.pack('<6f4B5f',x,y,z,0,0,1,part,0,0,0,x,y,1,0,0)
def mat(z=0,part=0,reverse=False):
    vs=[vertex(0,0,z,part),vertex(.1,0,z,part),vertex(0,.1,z,part)]
    if reverse:
        vs=[v[:12]+struct.pack('<3f',0,0,-1)+v[24:] for v in vs]
    return dict(alpha=0,shader=b'BundledMesh.fx',tech=b'Base',maps=[b'owned_donor.dds'],vertices=vs,indices=[2,1,0] if reverse else [0,1,2],unknown=(0,0))
def mesh(fp,donor):return SimpleNamespace(lods=[[fp],[donor]],bounds=[(0,0,0,.1,.1,.1,2),(0,0,0,.1,.1,.1,2)])
class Completion(unittest.TestCase):
    def test_iterative_tree_matches_brute_force(self):
        triangles=[Triangle(mat(z=i*.02)['vertices']) for i in range(64)]
        tree=Tree(triangles)
        for x in (-.01,0,.025,.1,.2):
            for z in (-.1,.01,.7,2):
                for direction in ((0,0,1),(0,0,-1),(.1,0,-1),(0,1,0)):
                    args=((x,.02,z),direction,.1,(0,0,1))
                    self.assertEqual(tree.hits(*args),any(t.hits(*args) for t in triangles))
    def test_query_has_no_recursive_generator_stack(self):
        triangle=Triangle(mat()['vertices']);tree=Tree([triangle])
        for _ in range(2000):
            parent=Tree([triangle]);parent.children=(tree,);parent.triangles=None;tree=parent
        self.assertTrue(tree.hits((.02,.02,.01),(0,0,-1),.1,(0,0,1)))
    def test_adds_real_opposite_wall_not_a_reversed_same_plane(self):
        first=mat();back=mat(-.04,reverse=True);m=mesh([first],[mat(),back]);before=copy.deepcopy(m.lods)
        result=complete(m,'eu_mg',interpolate)
        self.assertEqual(result['donor_faces'],1);self.assertEqual(m.lods[0][0][0],before[0][0][0]);self.assertEqual(m.lods[1],before[1])
        added=m.lods[0][0][1];self.assertEqual(added['maps'],back['maps']);self.assertTrue(all(abs(xyz(v)[2]+.0395)<1e-6 for v in added['vertices']))
        self.assertEqual({v[28:] for v in added['vertices']},{v[28:] for v in back['vertices']})
    def test_complete_first_person_skin_is_not_overpainted(self):
        m=mesh([mat()],[mat()]);self.assertEqual(complete(m,'eu_mg',interpolate)['donor_faces'],0)
        self.assertEqual(len(m.lods[0][0]),1)
    def test_opposing_normal_does_not_hide_missing_side(self):
        m=mesh([mat()],[mat(-.005,reverse=True)])
        self.assertEqual(complete(m,'eu_mg',interpolate)['donor_faces'],1)
    def test_unknown_bone_and_transparency_do_not_get_attached(self):
        alpha=mat(-.04,reverse=True);alpha['alpha']=1
        m=mesh([mat()],[mat(-.04,part=1,reverse=True),alpha]);before=copy.deepcopy(m.lods)
        stats=complete(m,'eu_mg',interpolate);self.assertEqual(stats['donor_faces'],0);self.assertEqual(stats['skipped_unmapped_faces'],1);self.assertEqual(m.lods,before)
    def test_rigid_part_matching_and_preserved_uvs(self):
        m=mesh([mat(part=1)],[mat(-.04,part=1,reverse=True)])
        stats=complete(m,'eu_mg',interpolate);self.assertEqual(stats['donor_parts'],{'1':1})
        self.assertEqual({v[24] for v in m.lods[0][0][-1]['vertices']},{1})
    def test_explicit_coordinate_and_pistol_bolt_profiles(self):
        v=vertex(.1,.2,.3,2);transformed=donor_vertex(v,'unl_har_rocket')
        for a,b in zip(xyz(transformed),(.16,.246,.39)):self.assertAlmostEqual(a,b,places=6)
        self.assertEqual(transformed[24:],v[24:]);self.assertEqual(donor_vertex(v,'as_handgun')[24],3)
        self.assertEqual(donor_vertex(v,'eu_mg'),v)
if __name__=='__main__':unittest.main()
