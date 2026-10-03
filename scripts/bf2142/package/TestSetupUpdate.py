import copy,hashlib,json,tempfile,unittest,uuid
from pathlib import Path
from unittest.mock import patch
import SetupAssets as setup

class UpdateTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.game=Path(self.temp.name).resolve()/'Game';self.game.mkdir()
        self.root=self.game/'BF2142VR';self.root.mkdir();self.payload=Path(self.temp.name).resolve()/'payload';self.payload.mkdir()
        self.original=b'original stock';self.weapon=b'accepted repair'
        target=self.game/setup.WEAPONS;target.parent.mkdir(parents=True);target.write_bytes(self.weapon)
        (self.root/'backups').mkdir();(self.root/'backups/Weapons_client.zip').write_bytes(self.original)
        self.row={'target':setup.WEAPONS,'backup':'backups/Weapons_client.zip','original':setup.sha(self.root/'backups/Weapons_client.zip'),'installed':setup.sha(target)}
        self.state={'app':setup.APP,'version':'old','game':str(self.game),'status':'installed','changes':[self.row]}
        setup.write_json(self.root/'install.json',self.state)
        (self.root/'BF2142VR.ini').write_text('[VR]\nSnapAngle=45\nMovementDirection=controller\n',encoding='utf-16')
        (self.root/'runtime.txt').write_text('old runtime');(self.payload/'runtime.txt').write_text('new runtime')
        self.manifest={'version':setup.VERSION,'files':{'runtime.txt':setup.sha(self.payload/'runtime.txt')}}
        setup.write_json(self.payload/'payload.json',self.manifest)
        self.patches=[patch.object(setup,'running'),patch.object(setup,'validate_game',return_value=(setup.COMPLETE,b'',b'')),patch.object(setup,'payload_files',return_value=self.manifest),patch.object(setup,'build_assets',side_effect=self.assets)]
        for p in self.patches:p.start()
    def tearDown(self):
        for p in reversed(self.patches):p.stop()
        self.temp.cleanup()
    def assets(self,game,stage,original):
        (stage/'generated').mkdir();(stage/'generated/Weapons_client.zip').write_bytes(getattr(self,'next_weapon',self.weapon))
    def assert_old(self):
        self.assertEqual((self.root/'runtime.txt').read_text(),'old runtime')
        self.assertEqual((self.root/'backups/Weapons_client.zip').read_bytes(),self.original)
        self.assertEqual((self.game/setup.WEAPONS).read_bytes(),self.weapon)
    def test_update_retains_settings_original_backup_and_game(self):
        setup.update(self.game,self.payload)
        self.assertEqual((self.root/'runtime.txt').read_text(),'new runtime')
        self.assertIn('SnapAngle=45',(self.root/'BF2142VR.ini').read_text(encoding='utf-16'))
        self.assertIn('MovementDirection=controller',(self.root/'BF2142VR.ini').read_text(encoding='utf-16'))
        self.assertEqual((self.root/'backups/Weapons_client.zip').read_bytes(),self.original)
        self.assertEqual((self.game/setup.WEAPONS).read_bytes(),self.weapon)
        self.assertEqual(json.loads((self.root/'install.json').read_text())['status'],'installed')
        previous=list(self.game.glob('.BF2142VR-previous-*'));self.assertEqual(len(previous),1)
        self.assertEqual((previous[0]/'runtime.txt').read_text(),'old runtime')
        self.assertFalse((self.game/'.BF2142VR-update.json').exists())
    def test_generation_failure_leaves_previous_intact(self):
        with patch.object(setup,'build_assets',side_effect=RuntimeError('failed worker')):
            with self.assertRaises(RuntimeError):setup.update(self.game,self.payload)
        self.assert_old();self.assertEqual(list(self.game.glob('.BF2142VR-setup-*')),[])
    def test_activation_failure_restores_previous(self):
        with patch.object(setup,'refresh_settings',side_effect=OSError('failure')):
            with self.assertRaises(OSError):setup.update(self.game,self.payload)
        self.assert_old();self.assertFalse((self.game/'.BF2142VR-update.json').exists())
    def test_changed_game_refused(self):
        (self.game/setup.WEAPONS).write_bytes(b'another mod')
        with self.assertRaises(ValueError):setup.update(self.game,self.payload)
        self.assertEqual((self.game/setup.WEAPONS).read_bytes(),b'another mod')
    def test_corrupt_original_backup_refused(self):
        (self.root/'backups/Weapons_client.zip').write_bytes(b'damaged')
        with self.assertRaises(ValueError):setup.update(self.game,self.payload)
        self.assertEqual((self.root/'runtime.txt').read_text(),'old runtime')
    def interrupted(self,activate=False,complete=False):
        token=uuid.uuid4().hex;setup.write_json(self.game/'.BF2142VR-update.json',{'app':setup.APP,'transaction':token})
        self.root.rename(self.game/('.BF2142VR-previous-'+token))
        if activate:
            self.root.mkdir();state=dict(self.state,transaction=token,status='installed' if complete else 'prepared')
            setup.write_json(self.root/'install.json',state)
            if complete:
                (self.root/'backups').mkdir();(self.root/'backups/Weapons_client.zip').write_bytes(self.original)
        return token
    def test_power_loss_between_directory_renames_restores_old(self):
        self.interrupted();setup.recover_update(self.game);self.assert_old()
    def test_power_loss_before_commit_retains_failed_runtime_and_restores_old(self):
        token=self.interrupted(activate=True);setup.recover_update(self.game);self.assert_old()
        self.assertTrue((self.game/('.BF2142VR-interrupted-'+token)).exists())
    def test_completed_update_journal_keeps_new_install(self):
        self.interrupted(activate=True,complete=True);setup.recover_update(self.game)
        self.assertFalse((self.game/'.BF2142VR-update.json').exists())
        self.assertFalse((self.root/'runtime.txt').exists())
    def test_invalid_journal_never_moves_files(self):
        setup.write_json(self.game/'.BF2142VR-update.json',{'app':setup.APP,'transaction':'../elsewhere'})
        with self.assertRaises(ValueError):setup.recover_update(self.game)
        self.assert_old()
    def test_updated_weapon_archive_keeps_stock_uninstall_backup(self):
        self.next_weapon=b'separated repaired skins'
        setup.update(self.game,self.payload)
        self.assertEqual((self.game/setup.WEAPONS).read_bytes(),self.next_weapon)
        state=json.loads((self.root/'install.json').read_text())
        self.assertEqual(state['changes'][0]['installed'],setup.sha(self.game/setup.WEAPONS))
        self.assertEqual((self.root/'backups/Weapons_client.zip').read_bytes(),self.original)
        setup.restore(self.root,state)
        self.assertEqual((self.game/setup.WEAPONS).read_bytes(),self.original)
    def test_failure_after_archive_replacement_rolls_back_runtime_and_archive(self):
        self.next_weapon=b'separated repaired skins'
        write=setup.write_json
        def fail_commit(path,value):
            if path.name=='install.json' and value.get('transaction') and value.get('status')=='installed':
                raise OSError('commit failure')
            return write(path,value)
        with patch.object(setup,'write_json',side_effect=fail_commit):
            with self.assertRaises(OSError):setup.update(self.game,self.payload)
        self.assert_old()
        self.assertFalse((self.game/'.BF2142VR-update.json').exists())
    def prepare_interrupted_weapon(self):
        token=self.interrupted(activate=True)
        (self.root/'update-rollback').mkdir()
        (self.root/'update-rollback/Weapons_client.zip').write_bytes(self.weapon)
        (self.game/setup.WEAPONS).write_bytes(b'new separated skins')
        setup.write_json(self.game/'.BF2142VR-update.json',{'app':setup.APP,'transaction':token,
            'weapon':{'before':self.row['installed'],'after':setup.sha(self.game/setup.WEAPONS)}})
        return token
    def test_interrupted_archive_replacement_restores_previous(self):
        token=self.prepare_interrupted_weapon()
        setup.recover_update(self.game)
        self.assert_old()
        self.assertTrue((self.game/('.BF2142VR-interrupted-'+token)).exists())
    def test_interrupted_archive_foreign_change_is_preserved(self):
        self.prepare_interrupted_weapon()
        (self.game/setup.WEAPONS).write_bytes(b'foreign change')
        with self.assertRaises(ValueError):setup.recover_update(self.game)
        self.assertEqual((self.game/setup.WEAPONS).read_bytes(),b'foreign change')
        self.assertTrue((self.game/'.BF2142VR-update.json').exists())
class RunningProcessTests(unittest.TestCase):
    def state(self,value,code=259,query_ok=True):
        class Kernel:
            def WaitForSingleObject(inner,handle,timeout):
                self.assertEqual((handle,timeout),(123,0));return value
            def GetExitCodeProcess(inner,handle,output):
                self.assertEqual(handle,123);output._obj.value=code;return query_ok
        return setup.process_is_active(Kernel(),123)
    def test_exited_retained_object_does_not_block_update(self):
        self.assertFalse(self.state(0))
    def test_live_process_still_blocks_update(self):
        self.assertTrue(self.state(258))
    def test_final_exit_code_with_unsignaled_handle_is_exited(self):
        self.assertFalse(self.state(258,0))
    def test_failed_exit_query_is_not_ignored(self):
        with self.assertRaises(OSError):self.state(258,0,False)
    def test_failed_probe_cannot_authorize_update(self):
        with self.assertRaises(OSError):self.state(0xffffffff)

if __name__=='__main__':unittest.main()
