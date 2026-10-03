import copy, math, struct, unittest
from RepairWeaponMeshes import Mesh, two_sided, position, expand_first_person_bounds
from TestWeaponMeshes import independent_blended_fixture
from SeparateWeaponBackfaces import repair, INSET_METRES

def completed():
    mesh = Mesh(independent_blended_fixture()[0])
    two_sided(mesh.lods[0][0][0])
    return mesh

class SeparationTests(unittest.TestCase):
    def test_preserves_exterior_alpha_world_and_shader_attributes(self):
        mesh = completed(); old = copy.deepcopy(mesh)
        output, stats = repair(mesh.encode()); result = Mesh(output)
        self.assertEqual(result.lods[1:], old.lods[1:])
        self.assertEqual(result.lods[0][0][1], old.lods[0][0][1])
        self.assertEqual(result.bounds[old.counts[0]:], old.bounds[old.counts[0]:])
        self.assertEqual([b[-1] for b in result.bounds], [b[-1] for b in old.bounds])
        for bound,lod in zip(result.bounds,result.lods[0]):
            for mat in lod:
                for v in mat['vertices']:
                    self.assertTrue(all(bound[i]<=p<=bound[i+3] for i,p in enumerate(position(v))))
        a,b = old.lods[0][0][0],result.lods[0][0][0]
        self.assertEqual(a['vertices'][:3],b['vertices'][:3])
        for key in a:
            if key!='vertices': self.assertEqual(a[key],b[key])
        for x,y in zip(a['vertices'][3:],b['vertices'][3:]):
            self.assertEqual(x[12:], y[12:])
            self.assertAlmostEqual(position(y)[2],position(x)[2]-INSET_METRES,places=8)
        self.assertEqual(stats['separated_inner_vertices'],3)

    def test_rejects_stock_modified_and_already_separated_input(self):
        with self.assertRaises(ValueError): repair(independent_blended_fixture()[0])
        mesh=completed();v=bytearray(mesh.lods[0][0][0]['vertices'][3]);struct.pack_into('<f',v,0,9)
        mesh.lods[0][0][0]['vertices'][3]=bytes(v)
        with self.assertRaises(ValueError): repair(mesh.encode())
        fixed,_=repair(completed().encode())
        with self.assertRaises(ValueError): repair(fixed)

    def test_retains_trimmed_reverse_index_list(self):
        mesh=completed();mesh.lods[0][0][0]['indices']=mesh.lods[0][0][0]['indices'][:3]
        result,_=repair(mesh.encode())
        self.assertEqual(Mesh(result).lods[0][0][0]['indices'],[0,1,2])

    def test_normal_length_does_not_scale_separation(self):
        mesh=completed();mat=mesh.lods[0][0][0]
        for i,v in enumerate(mat['vertices']):
            out=bytearray(v);struct.pack_into('<f',out,20,2 if i<3 else -2);mat['vertices'][i]=bytes(out)
        result,_=repair(mesh.encode())
        self.assertAlmostEqual(position(Mesh(result).lods[0][0][0]['vertices'][3])[2],-INSET_METRES,places=8)

    def test_negative_extreme_cannot_wrap_above_pistol(self):
        mesh=completed();old_world=copy.deepcopy(mesh.lods[1:]);old_bounds=mesh.bounds[:]
        mat=mesh.lods[0][0][0];v=bytearray(mat['vertices'][-1])
        # Include an unused added vertex: native packing also converts these.
        negative_limit=max(abs(b[j]) for b in mesh.bounds for j in (1,4))
        struct.pack_into('<f',v,4,-negative_limit-.0005);mat['vertices'].append(bytes(v))
        y=position(v)[1]
        code=round(y*32767/negative_limit)
        self.assertLess(code,-32768)
        self.assertGreater(((code+32768)%65536-32768)*negative_limit/32767,0)
        self.assertEqual(expand_first_person_bounds(mesh),1)
        result=Mesh(mesh.encode());limit=max(abs(b[j]) for b in result.bounds for j in (1,4))
        code=round(y*32767/limit)
        self.assertGreaterEqual(code,-32768);self.assertLessEqual(code,32767)
        self.assertAlmostEqual(code*limit/32767,y,places=6)
        self.assertEqual(result.lods[1:],old_world)
        self.assertEqual(result.bounds[mesh.counts[0]:],old_bounds[mesh.counts[0]:])
        self.assertEqual(expand_first_person_bounds(result),0)

    def test_bounds_never_shrink_and_reject_nonfinite_position(self):
        mesh=completed();mesh.bounds[0]=(-10.,-10.,-10.,10.,10.,10.,mesh.bounds[0][-1])
        before=mesh.bounds[:];self.assertEqual(expand_first_person_bounds(mesh),0)
        self.assertEqual(mesh.bounds,before)
        v=bytearray(mesh.lods[0][0][0]['vertices'][0]);struct.pack_into('<f',v,0,float('nan'))
        mesh.lods[0][0][0]['vertices'][0]=bytes(v)
        with self.assertRaises(ValueError):expand_first_person_bounds(mesh)

if __name__=='__main__':unittest.main()
