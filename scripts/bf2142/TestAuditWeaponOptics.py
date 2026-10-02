import io
import unittest
import zipfile
from AuditWeaponOptics import audit, commands, hud_routes, profiles, weapon_records

class CoverageTests(unittest.TestCase):
    def record(self, name='unl_test_rifle'):
        return dict(name=name, factors=[0,.484], zoom_lod=['1'], alt=84)
    def check(self, records=None, definitions=None, huds=None, nodes=None, indices=None):
        return audit(records if records is not None else [self.record()],
                     definitions if definitions is not None else {'unl_test_rifle':dict(factor=.484)},
                     huds if huds is not None else {'EuAssaultZoom':{84}, 'EuAssaultHud':{83,84}},
                     nodes if nodes is not None else {'EuAssaultZoomCullNode'},
                     indices if indices is not None else {84})
    def test_complete_and_missing_unlock(self):
        self.assertFalse(self.check()[1])
        self.assertIn('missing optical profile', self.check(definitions={})[1][0])
        self.assertTrue(self.check(records=[self.record('future_unlock')])[1])
    def test_template_factor_mismatch(self):
        self.assertIn('factor mismatch',self.check(definitions={'unl_test_rifle':dict(factor=.59)})[1][0])
    def test_missing_selector_or_exact_zoom_root(self):
        self.assertTrue(self.check(indices=set())[1])
        self.assertTrue(self.check(nodes={'EuAssaultZoomCullNodeCustom'})[1])
        self.assertTrue(self.check(nodes={'EuAssaultHudCullNode'})[1])
    def test_native_exceptions_are_explicit(self):
        p=self.record('eu_handgun');p['zoom_lod']=[]
        rows, errors=self.check(records=[p],definitions={})
        self.assertFalse(errors);self.assertEqual(rows[0]['coverage'],'native iron sights')
        p['zoom_lod']=['1'];self.assertTrue(self.check(records=[p],definitions={})[1])
        p=self.record('tool');p['factors']=[0]
        self.assertEqual(self.check(records=[p],definitions={})[0][0]['coverage'],'native/no optical zoom')
    def test_stale_and_duplicate_profiles(self):
        self.assertTrue(self.check(records=[])[1])
        text='{"unl_test_rifle",{0,.1f,-.2f},.01f,.01f,false,.484f,2.f}'
        self.assertEqual(profiles(text)['unl_test_rifle']['factor'],.484)
        with self.assertRaises(ValueError):profiles(text+text)
    def test_archive_parsing_and_commented_properties(self):
        memory=io.BytesIO()
        with zipfile.ZipFile(memory,'w') as z:
            z.writestr('Handheld/test/test.tweak','''ObjectTemplate.activeSafe GenericFireArm Test
ObjectTemplate.zoom.addZoomFactor 0
beginrem
ObjectTemplate.zoom.addZoomFactor 99
endrem
ObjectTemplate.zoom.addZoomFactor .484
ObjectTemplate.zoom.zoomLod 1
ObjectTemplate.weaponHud.altGuiIndex 84
ObjectTemplate.create GenericProjectile bullet
ObjectTemplate.zoom.addZoomFactor .9
''')
            z.writestr('HUD/HudSetup/Weapons/test.con','''hudBuilder.createSplitNode WeaponHuds EuAssaultHud
hudBuilder.setNodeLogicShowVariable EQUAL GuiIndex 83
hudBuilder.setNodeLogicShowVariable OR GuiIndex 84
hudBuilder.createSplitNode EuAssaultHud EuAssaultZoom
hudBuilder.setNodeLogicShowVariable EQUAL GuiIndex 84
hudBuilder.createPictureNode EuAssaultZoom Dot 0 0 1 1
hudBuilder.setNodeLogicShowVariable EQUAL GuiIndex 55
''')
        with zipfile.ZipFile(memory) as z:
            records=weapon_records(z);huds=hud_routes(z)
        self.assertEqual(records[0]['factors'],[0,.484])
        self.assertEqual(huds,{'EuAssaultHud':{83,84},'EuAssaultZoom':{84}})
        self.assertEqual(list(commands('rem hidden\nbeginrem\nignored\nendrem\nvisible')), [['visible']])

if __name__=='__main__':unittest.main()
