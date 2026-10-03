import hashlib,json,struct,tempfile,unittest,zipfile
from pathlib import Path
from unittest.mock import patch
import WaterReflectionRepair as water
import SetupAssets as setup
import TestSetupUpdate as update_tests


def fixture():
    header=[0]*31
    header[0],header[1],header[2],header[3],header[4],header[6]=124,0x2100f,128,128,512,8
    header[18:26]=[32,0x41,0,32,0xff0000,0xff00,0xff,0xff000000]
    header[26]=0x401008
    top=bytes(c for y in range(128) for x in range(128) for c in (x,y,(x+y)%256,255))
    return b'DDS '+struct.pack('<31I',*header)+top+bytes(87508-128-len(top))


def archive(path, raw, duplicate=False):
    path.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(path,'w',compression=zipfile.ZIP_DEFLATED) as z:
        z.comment=b'keep map comment'
        z.writestr('water/EnvMap.dds',raw)
        z.writestr('other/data.bin',b'unrelated map content')
        if duplicate:z.writestr('WATER/ENVMAP.DDS',raw)


class ReflectionTests(unittest.TestCase):
    def test_filtered_cube_retains_detail_and_matches_every_shared_edge(self):
        raw=fixture();result=water.build_cube(raw);h=struct.unpack('<31I',result[4:128])
        self.assertEqual((h[3],h[6],h[27],len(result)),(128,8,0xfe00,524408))
        offset=128;faces=[]
        for face in range(6):
            levels=[]
            for mip in range(8):
                n=128>>mip;levels.append(result[offset:offset+n*n*4]);offset+=n*n*4
            faces.append(levels)
        # The interior of the original scene is retained, not blurred/reprojected.
        self.assertEqual(faces[0][0][(64*128+64)*4:(64*128+64)*4+4],bytes((64,64,128,255)))
        for mip in range(8):
            n=128>>mip;seen={}
            for face in range(6):
                for j in range(n):
                    for i in range(n):
                        if i not in (0,n-1) and j not in (0,n-1):continue
                        key=water.direction(face,i,j,n);pixel=faces[face][mip][(j*n+i)*4:(j*n+i+1)*4]
                        if key in seen:self.assertEqual(pixel,seen[key])
                        seen[key]=pixel
        # A 2x2 interior texel group is averaged in the next level.
        self.assertEqual(faces[0][1][(32*64+32)*4:(32*64+32)*4+4],bytes((64,64,129,255)))

    def test_invalid_and_truncated_inputs_rejected(self):
        for raw in [b'',fixture()[:-1],b'bad!'+fixture()[4:]]:
            with self.assertRaises(ValueError):water.build_cube(raw)

    def test_unknown_missing_duplicate_and_already_correct_reflections_skip(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);game=root/'Game';game.mkdir();stage=root/'stage';stage.mkdir()
            self.assertIsNone(water.stage(game,stage,setup.child,setup.sha))
            for raw,duplicate in [(fixture(),False),(fixture(),True),(water.build_cube(fixture()),False)]:
                archive(game/water.TARGET,raw,duplicate)
                before=setup.sha(game/water.TARGET)
                self.assertIsNone(water.stage(game,stage,setup.child,setup.sha))
                self.assertEqual(setup.sha(game/water.TARGET),before)
                self.assertEqual(list(stage.iterdir()),[])


class WaterUpdateTests(unittest.TestCase):
    assets=update_tests.UpdateTests.assets
    assert_old=update_tests.UpdateTests.assert_old
    interrupted=update_tests.UpdateTests.interrupted

    def setUp(self):
        update_tests.UpdateTests.setUp(self)
        self.raw=fixture();self.fixed=water.build_cube(self.raw)
        self.patches.extend([patch.object(water,'BROKEN',hashlib.sha256(self.raw).hexdigest()),
                             patch.object(water,'REPAIRED',hashlib.sha256(self.fixed).hexdigest())])
        for p in self.patches[-2:]:p.start()
        archive(self.game/water.TARGET,self.raw)
        self.map_before=(self.game/water.TARGET).read_bytes()

    tearDown=update_tests.UpdateTests.tearDown

    def test_fresh_install_has_reversible_map_backup(self):
        # Use a fresh app directory, retaining the harmless test game fixtures.
        self.root.rename(self.game/'old-fixture')
        current=setup.sha(self.game/setup.WEAPONS)
        with patch.object(setup,'COMPLETE',current), patch.object(setup,'validate_game',return_value=(current,b'',b'')):
            setup.install(self.game,self.payload)
        state=json.loads((self.root/'install.json').read_text())
        self.assertEqual(len(state['changes']),2)
        setup.restore(self.root,state)
        self.assertEqual((self.game/water.TARGET).read_bytes(),self.map_before)

    def test_update_preserves_other_entries_and_uninstall_restores_exact_archive(self):
        setup.update(self.game,self.payload)
        with zipfile.ZipFile(self.game/water.TARGET) as z:
            self.assertEqual(z.read('water/EnvMap.dds'),self.fixed)
            self.assertEqual(z.read('other/data.bin'),b'unrelated map content')
            self.assertEqual(z.comment,b'keep map comment')
        state=json.loads((self.root/'install.json').read_text())
        self.assertEqual((self.root/water.BACKUP).read_bytes(),self.map_before)
        setup.restore(self.root,state)
        self.assertEqual((self.game/water.TARGET).read_bytes(),self.map_before)

    def test_second_update_keeps_first_backup_without_repairing_again(self):
        setup.update(self.game,self.payload)
        after=(self.game/water.TARGET).read_bytes()
        setup.update(self.game,self.payload)
        self.assertEqual((self.game/water.TARGET).read_bytes(),after)
        self.assertEqual((self.root/water.BACKUP).read_bytes(),self.map_before)
        state=json.loads((self.root/'install.json').read_text())
        self.assertEqual(sum(r['target']==water.TARGET for r in state['changes']),1)

    def test_failed_commit_rolls_back_both_map_and_weapon(self):
        self.next_weapon=b'new weapon repair'
        write=setup.write_json
        def fail(path,value):
            if path.name=='install.json' and value.get('transaction') and value.get('status')=='installed':
                raise OSError('simulated commit failure')
            return write(path,value)
        with patch.object(setup,'write_json',side_effect=fail):
            with self.assertRaises(OSError):setup.update(self.game,self.payload)
        self.assert_old()
        self.assertEqual((self.game/water.TARGET).read_bytes(),self.map_before)

    def interrupted_map(self):
        token=self.interrupted(activate=True)
        backup=self.root/water.BACKUP;backup.parent.mkdir(parents=True);backup.write_bytes(self.map_before)
        archive(self.game/water.TARGET,self.fixed)
        journal={'app':setup.APP,'transaction':token,'water':{'before':hashlib.sha256(self.map_before).hexdigest(),'after':setup.sha(self.game/water.TARGET)}}
        setup.write_json(self.game/'.BF2142VR-update.json',journal)

    def test_interrupted_update_recovers_map_and_previous_runtime(self):
        self.interrupted_map();setup.recover_update(self.game)
        self.assert_old()
        self.assertEqual((self.game/water.TARGET).read_bytes(),self.map_before)

    def test_interrupted_update_preserves_foreign_map_changes(self):
        self.interrupted_map();(self.game/water.TARGET).write_bytes(b'new map from elsewhere')
        with self.assertRaises(ValueError):setup.recover_update(self.game)
        self.assertEqual((self.game/water.TARGET).read_bytes(),b'new map from elsewhere')
        self.assertTrue((self.game/'.BF2142VR-update.json').exists())

    def test_original_backup_corruption_prevents_restore(self):
        setup.update(self.game,self.payload)
        (self.root/water.BACKUP).write_bytes(b'damaged')
        after=(self.game/water.TARGET).read_bytes()
        with self.assertRaises(ValueError):setup.restore(self.root,json.loads((self.root/'install.json').read_text()))
        self.assertEqual((self.game/water.TARGET).read_bytes(),after)


if __name__=='__main__':unittest.main()
