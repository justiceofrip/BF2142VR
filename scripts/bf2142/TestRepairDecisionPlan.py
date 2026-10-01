import copy, unittest, base64, zlib
from unittest.mock import patch
from RepairDecisionPlan import DecisionPlan,encode,MAX_DECISIONS
from CompleteWeaponSurfaces import complete
from RepairWeaponMeshes import interpolate,repair,Mesh
from RemoveInteriorBackfaces import repair as remove_inside
from TestCompleteSurfaces import mat,mesh
from TestWeaponMeshes import independent_blended_fixture

class Plans(unittest.TestCase):
    def test_bounds_and_consumption(self):
        plan=DecisionPlan(encode([0,1,2]));self.assertEqual([plan.take(2) for _ in range(3)],[0,1,2]);plan.finish()
        with self.assertRaises(ValueError):plan.take(2)
        with self.assertRaises(ValueError):DecisionPlan(encode([2])).take(1)
        with self.assertRaises(ValueError):DecisionPlan(encode([0])).finish()
        for bad in (base64.b85encode(zlib.compress(bytes(MAX_DECISIONS+1))).decode(),encode([0])[:-2],base64.b85encode(zlib.compress(b'')+b'trailing').decode()):
            with self.assertRaises((ValueError,zlib.error)):DecisionPlan(bad)
    def test_surface_replay_avoids_rays_and_preserves_geometry(self):
        for donor in ([mat()], [mat(-.04,reverse=True)], [mat(-.005),mat(-.04,reverse=True)]):
            original=mesh([mat()],donor);replay=copy.deepcopy(original);record=[]
            stats=complete(original,'eu_mg',interpolate,record=record)
            with patch('CompleteWeaponSurfaces.Tree',side_effect=AssertionError('No tree during replay')):
                self.assertEqual(complete(replay,'eu_mg',interpolate,DecisionPlan(encode(record))),stats)
            self.assertEqual(replay.lods,original.lods);self.assertEqual(replay.bounds,original.bounds)
    def test_binary_repair_preserves_transparency_and_world_lods(self):
        source,_,_=independent_blended_fixture();surface=[];inside=[]
        completed,_=repair(source,'eu_mg',record=surface)
        expected,stats=remove_inside(completed,record=inside)
        with patch('CompleteWeaponSurfaces.Tree.hits',side_effect=AssertionError('No ray search during replay')):
            replay,_=repair(source,'eu_mg',surface_plan=DecisionPlan(encode(surface)))
            replay,replay_stats=remove_inside(replay,plan=DecisionPlan(encode(inside)))
        self.assertEqual(replay,expected);self.assertEqual(stats,replay_stats)
        self.assertEqual(Mesh(replay).lods[1:],Mesh(source).lods[1:])
        with self.assertRaises(ValueError):remove_inside(completed,plan=DecisionPlan(encode([])))
    def test_surface_rejects_truncated_or_extra_plan(self):
        for values in ([],[1,1],[3],[2]*7):
            with self.assertRaises(ValueError):
                complete(mesh([mat()],[mat(-.04,reverse=True)]),'eu_mg',interpolate,DecisionPlan(encode(values)))

if __name__=='__main__':unittest.main()
