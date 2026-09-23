import copy, struct, unittest
from RepairWeaponMeshes import Mesh, position, two_sided, clip_rear, interpolate, pack, repair

def vertex(x,y,z):return pack('6f4B5f',x,y,z,0,0,1,0,0,0,0,x,y,1,0,0)
def material(alpha=0):
    m=dict(alpha=alpha,shader=b'BundledMesh.fx',tech=b'Base',maps=[b'fixture.dds'],vertices=[vertex(0,0,0),vertex(1,0,0),vertex(0,1,0)],indices=[0,1,2],unknown=(0,0))
    if alpha==1:m['face_sets']=[[0,1,2] for _ in range(8)]
    return m


def independent_blended_fixture():
    # Hand-pack an external-format fixture, independent of Mesh.encode. Eight
    # distinct sort orders sit between opaque draws. v17 incorrectly drops 7.
    orders=[[0,1,2],[1,2,0],[2,0,1],[0,2,3],[2,3,0],[3,0,2],[1,3,2],[3,2,1]]
    attrs=[(0,0,2,0),(0,12,2,3),(0,24,4,2),(0,28,1,5),(0,36,2,6),(255,0,17,0)]
    vertices=material()['vertices']+[vertex(0,0,0),vertex(1,0,0),vertex(0,1,0),vertex(1,1,0)]+material()['vertices']
    indices=[0,1,2]+[i for face in orders for i in face]+[0,1,2]
    out=bytearray(pack('5IB',0,10,0,0,0,0)+pack('3I',2,1,1)+pack('I',len(attrs)))
    for a in attrs:out+=pack('4H',*a)
    out+=pack('3I',4,48,len(vertices))+b''.join(vertices)+pack('I',len(indices))
    index_offset=len(out);out+=pack(str(len(indices))+'H',*indices)+pack('I',8)
    for _ in range(2):out+=pack('6fI',0,0,0,1,1,1,1)
    def string(value):return pack('I',len(value))+value
    def mat(alpha,vs,ix,nv):
        return pack('I',alpha)+string(b'BundledMesh.fx')+string(b'Alpha' if alpha else b'Base')+pack('I',1)+string(b'fixture.dds')+pack('6I',vs,ix,3,nv,0,0)
    out+=pack('I',2)+mat(0,0,0,3)+mat(1,3,3,4)+pack('I',1)+mat(0,7,27,3)
    return bytes(out),orders,index_offset
class Repairs(unittest.TestCase):
    def test_backface_winding_normals_and_bindings(self):
        m=material();original=copy.deepcopy(m);self.assertEqual(two_sided(m),1)
        self.assertEqual(m['indices'],[0,1,2,5,4,3])
        self.assertEqual(m['vertices'][:3],original['vertices'])
        for a,b in zip(original['vertices'],m['vertices'][3:]):
            self.assertEqual(position(a),position(b));self.assertEqual(a[24:],b[24:])
            self.assertEqual(struct.unpack_from('<3f',b,12),(0,0,-1))
    def test_transparent_scope_unchanged(self):
        m=material(1);before=copy.deepcopy(m);self.assertEqual(two_sided(m),0);self.assertEqual(m,before)
    def test_clip_stock_keeps_rear_and_interpolates_uv(self):
        triangle=[vertex(0,0,-2),vertex(1,0,0),vertex(0,1,0)];out=clip_rear(triangle,-1)
        self.assertEqual(len(out),3);self.assertTrue(all(position(v)[2]<=-1 for v in out))
        self.assertIn((.5,0,-1),[position(v) for v in out])
        edge=next(v for v in out if position(v)[0]==.5);self.assertEqual(struct.unpack_from('<2f',edge,28),(.5,0))
        self.assertEqual(clip_rear(triangle,-3),[]);self.assertEqual(clip_rear(triangle,1),triangle)
    def test_reject_blended_parts_and_index_overflow(self):
        a=vertex(0,0,-1);b=bytearray(vertex(1,1,1));b[24]=1
        with self.assertRaises(ValueError):interpolate(a,b,.5)
        m=material();m['vertices']=[a]*32768
        with self.assertRaises(ValueError):two_sided(m)
    def test_full_format_roundtrip_and_other_lods(self):
        m=Mesh.__new__(Mesh);m.header=(0,10,0,0,0,0);m.counts=[2,2];m.attrs=[(0,0,2,0),(0,12,2,3),(0,24,4,2),(0,28,1,5),(0,36,2,6),(255,0,17,0)]
        m.vertex_format=4;m.stride=48;m.alpha_sort=8;m.bounds=[(0,0,0,1,1,1,1)]*4;m.lods=[[ [material()], [material(1)] ],[ [material()], [material()] ]]
        encoded=m.encode();parsed=Mesh(encoded);self.assertEqual(parsed.lods,m.lods);self.assertEqual(parsed.encode(),encoded)
        before=copy.deepcopy(parsed.lods);two_sided(parsed.lods[0][0][0]);reparsed=Mesh(parsed.encode());self.assertEqual(reparsed.lods[1:],before[1:]);self.assertEqual(reparsed.lods[0][1],before[0][1])
        with self.assertRaises(ValueError):Mesh(encoded[:-1])
    def test_external_eight_direction_alpha_fixture(self):
        data,orders,_=independent_blended_fixture();m=Mesh(data)
        self.assertEqual(m.lods[0][0][1]['face_sets'],orders)
        self.assertEqual(m.encode(),data)
        output,_=repair(data,'eu_ar_rifle');fixed=Mesh(output)
        self.assertEqual(fixed.lods[0][0][1],m.lods[0][0][1])
        self.assertEqual(fixed.lods[1],m.lods[1])
        self.assertEqual(len(fixed.lods[0][0][0]['indices']),6)
    def test_reject_invalid_later_alpha_set(self):
        data,_,offset=independent_blended_fixture();bad=bytearray(data)
        struct.pack_into('<H',bad,offset+(3+7*3)*2,4)
        with self.assertRaises(ValueError):Mesh(bad)
    def test_refuse_missing_sort_sets_on_write(self):
        data,_,_=independent_blended_fixture();m=Mesh(data)
        m.lods[0][0][1]['face_sets'].pop()
        with self.assertRaises(ValueError):m.encode()
    def test_machinegun_normal_and_aiming_lods(self):
        raw,_,_=independent_blended_fixture();m=Mesh(raw)
        m.counts=[2,1];m.lods[0].append(copy.deepcopy(m.lods[0][0]));m.bounds.insert(1,m.bounds[0])
        before=copy.deepcopy(m.lods)
        fixed,stats=repair(m.encode(),'eu_mg');after=Mesh(fixed)
        self.assertEqual(stats['backfaces_by_lod'],[1,1])
        for index in [0,1]:
            self.assertEqual(len(after.lods[0][index][0]['indices']),6)
            self.assertEqual(after.lods[0][index][1],before[0][index][1])
        self.assertEqual(after.lods[1:],before[1:])
if __name__=='__main__':unittest.main()
