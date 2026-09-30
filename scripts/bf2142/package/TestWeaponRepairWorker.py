import hashlib, subprocess, tempfile, unittest
from pathlib import Path
from unittest.mock import patch
import WeaponRepairWorker as worker

class WorkerTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.cache=Path(self.tmp.name)/'cache'
        self.source=b'owned synthetic input';self.output=b'verified synthetic output'
        self.profile={'source':worker.digest(self.source),'result':worker.digest(self.output),'size':len(self.output)}
        self.patch=patch.dict(worker.PROFILES,{'fixture':self.profile});self.patch.start();self.calls=0
    def tearDown(self):self.patch.stop();self.tmp.cleanup()
    def run_worker(self,args,**kwargs):
        self.calls+=1;Path(args[-1]).write_bytes(self.output)
        return subprocess.CompletedProcess(args,0,'')
    def repair(self,runner=None):return worker.repair_cached(self.source,'fixture',self.cache,['worker'],runner or self.run_worker)
    def test_success_then_verified_cache_needs_no_worker(self):
        self.assertEqual(self.repair(),self.output);self.assertEqual(self.repair(),self.output);self.assertEqual(self.calls,1)
    def test_source_mismatch_never_executes(self):
        with self.assertRaises(ValueError):worker.repair_cached(b'wrong','fixture',self.cache,['worker'],self.run_worker)
        self.assertEqual(self.calls,0)
    def test_native_crash_retries_only_failed_weapon(self):
        def run(args,**kwargs):
            if not self.calls:self.calls+=1;return subprocess.CompletedProcess(args,0xC0000005,'fatal access violation')
            return self.run_worker(args,**kwargs)
        self.assertEqual(self.repair(run),self.output);self.assertEqual(self.calls,2)
    def test_persistent_crash_bounded_and_no_partial_cache(self):
        def run(args,**kwargs):
            self.calls+=1;Path(args[-1]).write_bytes(b'partial');return subprocess.CompletedProcess(args,0xC0000409,'fatal')
        with self.assertRaises(RuntimeError):self.repair(run)
        self.assertEqual(self.calls,2);self.assertEqual(list(self.cache.iterdir()),[])
    def test_success_exit_with_bad_bytes_is_rejected(self):
        def run(args,**kwargs):
            self.calls+=1;Path(args[-1]).write_bytes(b'bad bytes');return subprocess.CompletedProcess(args,0,'')
        with self.assertRaises(RuntimeError):self.repair(run)
        self.assertEqual(self.calls,2);self.assertEqual(list(self.cache.iterdir()),[])
    def test_modified_cache_rebuilt(self):
        self.repair();(self.cache/(self.profile['result']+'.bin')).write_bytes(b'changed')
        self.assertEqual(self.repair(),self.output);self.assertEqual(self.calls,2)
    def test_timeout_bounded_and_cleaned(self):
        def run(args,**kwargs):self.calls+=1;raise subprocess.TimeoutExpired(args,180)
        with self.assertRaises(RuntimeError):self.repair(run)
        self.assertEqual(self.calls,2);self.assertEqual(list(self.cache.iterdir()),[])
    def test_partial_failed_worker_does_not_poison_next_attempt(self):
        def run(args,**kwargs):
            if not self.calls:self.calls+=1;Path(args[-1]).write_bytes(b'partial');return subprocess.CompletedProcess(args,1,'TypeError')
            return self.run_worker(args,**kwargs)
        self.assertEqual(self.repair(run),self.output);self.assertEqual(self.repair(),self.output);self.assertEqual(self.calls,2)
if __name__=='__main__':unittest.main()
