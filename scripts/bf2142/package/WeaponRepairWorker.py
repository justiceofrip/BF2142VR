"""Isolated, resumable repair of owned meshes; no generated content is shipped."""
from pathlib import Path
import hashlib, os, subprocess, sys, tempfile, uuid
from WeaponRepairHashes import PROFILES


def digest(data):
    return hashlib.sha256(data).hexdigest()


def accepted(path, profile):
    if path.is_symlink() or not path.is_file() or path.stat().st_size != profile['size']:
        return False
    return digest(path.read_bytes()) == profile['result']


def repair_one(name, source, output):
    import RepairWeaponMeshes as weapons
    import RemoveInteriorBackfaces as interiors
    from RepairDecisionPlan import DecisionPlan
    from WeaponRepairPlans import PLANS
    profile = PROFILES[name]
    data = source.read_bytes()
    if digest(data) != profile['source']:
        raise ValueError('Unrecognized source mesh: ' + name)
    # Only decision opcodes are shipped. Every coordinate, index, material and
    # texture still comes from the hash-identified owned source. Missing or
    # invalid plans fail closed; do not silently run the slow geometric search.
    surface, inside = PLANS[name]
    print('Applying verified repair plan: ' + name, flush=True)
    data, _ = weapons.repair(data, name, surface_plan=DecisionPlan(surface))
    data, _ = interiors.repair(data, plan=DecisionPlan(inside))
    if len(data) != profile['size'] or digest(data) != profile['result']:
        raise ValueError('Weapon repair differs from accepted model: ' + name)
    with output.open('xb') as f:
        f.write(data)
        f.flush()
        os.fsync(f.fileno())


def repair_cached(data, name, cache, command, runner=subprocess.run):
    profile = PROFILES[name]
    if digest(data) != profile['source']:
        raise ValueError('Unrecognized source mesh: ' + name)
    cache.mkdir(parents=True, exist_ok=True)
    for part in (cache, *cache.parents):
        if part.is_symlink() or part.is_junction():
            raise ValueError('Linked repair cache is unsupported')
    cached = cache / (profile['result'] + '.bin')
    if accepted(cached, profile):
        print('Using verified repair: ' + name, flush=True)
        return cached.read_bytes()
    # Each process starts with a clean interpreter. A failed worker can never
    # commit a partial mesh. The complete archive hash is also checked by setup.
    for attempt in range(2):
        with tempfile.TemporaryDirectory(prefix='repair-', dir=cache) as temporary:
            source = Path(temporary) / 'source.bin'
            output = Path(temporary) / 'result.bin'
            source.write_bytes(data)
            args = [*command, 'repair-weapon', '--name', name, '--input', str(source), '--output', str(output)]
            try:
                result = runner(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True, encoding='utf-8', errors='replace', timeout=180,
                                creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
                if result.returncode == 0 and accepted(output, profile):
                    # Publish a parent-owned sibling file (also works with
                    # virtualized child-process temp directories).
                    ready = cache / (uuid.uuid4().hex + '.pending')
                    try:
                        repaired = output.read_bytes()
                        with ready.open('xb') as f:
                            f.write(repaired); f.flush(); os.fsync(f.fileno())
                        if not accepted(ready, profile):
                            raise ValueError('Cache write verification failed: ' + name)
                        os.replace(ready, cached)
                    finally:
                        if ready.exists():ready.unlink()
                    return repaired
                detail = f'exit 0x{result.returncode & 0xffffffff:08X}: ' + (result.stdout or '')[-6000:]
            except subprocess.TimeoutExpired:
                detail = 'worker exceeded its 180-second limit'
            print('Repair worker failed for ' + name + ' (' + detail + ')', flush=True)
        if attempt == 0:
            print('Retrying this weapon once in a fresh worker; completed repairs are retained.', flush=True)
    raise RuntimeError('Repair failed twice for ' + name + '. Original game files are unchanged. '
                       'Verified progress is saved for the next setup attempt. See the setup log.')


def worker_command():
    if getattr(sys, 'frozen', False):
        return [sys.executable]
    return [sys.executable, str(Path(__file__).with_name('SetupAssets.py'))]
