"""Repair one recognized malformed optional-map reflection from owned assets.

No game textures are distributed. The fallback repeats the supplied scene view,
stitches cube edges and filters mip levels; missing authored views are approximate.
"""
import copy, hashlib, shutil, struct, zipfile

TARGET = 'mods/bf2142/Levels/carbone_island/client.zip'
BACKUP = 'backups/water/carbone-client.zip'
GENERATED = 'generated/CarboneWater-client.zip'
ENTRY = 'water/envmap.dds'
BROKEN = '8456db27d54202051770a871feb73c22fa1b725237270bb9e89539bbb6e2d1b4'
REPAIRED = 'c7422e0b7118af9f1538c8ce0ff8da55a170bacaca917c667c1a1ea280767c0e'

def direction(face, i, j, n):
    a = n - 1
    u, v = 2 * i - a, 2 * j - a
    return [(a, -v, -u), (-a, -v, u), (u, a, v),
            (u, -a, -v), (u, -v, a), (-u, -v, -a)][face]


def stitch(faces, n):
    groups = {}
    for f in range(6):
        for j in range(n):
            for i in range(n):
                if i in (0, n-1) or j in (0, n-1):
                    groups.setdefault(direction(f, i, j, n), []).append((f, i, j))
    # Blend a narrow border into the exact shared edge value, using snapshots
    # so face traversal order cannot change the resulting texture.
    originals = [bytes(f) for f in faces]
    band = max(1, n // 16)
    accum = {}
    for group in groups.values():
        target = [round(sum(originals[f][(j*n+i)*4+c] for f, i, j in group)
                        / len(group)) for c in range(4)]
        for f, i, j in group:
            for t in range(band):
                ii = i+t if i == 0 else i-t if i == n-1 else i
                jj = j+t if j == 0 else j-t if j == n-1 else j
                weight = 1 - t / band
                key = (f, ii, jj)
                accum.setdefault(key, []).append((weight, target))
    for (f, i, j), entries in accum.items():
        offset = (j*n+i)*4
        strength = max(w for w, _ in entries)
        for c in range(4):
            color = sum(w*rgb[c] for w, rgb in entries)/sum(w for w, _ in entries)
            faces[f][offset+c] = round(originals[f][offset+c]*(1-strength) + color*strength)
    # Corners and edges receive the exact group average after blending.
    for group in groups.values():
        target = bytes(round(sum(originals[f][(j*n+i)*4+c] for f, i, j in group)
                             / len(group)) for c in range(4))
        for f, i, j in group:
            faces[f][(j*n+i)*4:(j*n+i+1)*4] = target



def build_cube(raw):
    if len(raw) != 87508 or raw[:4] != b'DDS ':
        raise ValueError('Unsupported water reflection image')
    header = list(struct.unpack('<31I', raw[4:128]))
    if (header[3], header[2], header[21], header[22:26], header[27]) != (
            128, 128, 32, [0xff0000, 0xff00, 0xff, 0xff000000], 0):
        raise ValueError('Unsupported water reflection format')
    faces = [bytearray(raw[128:128+128*128*4]) for _ in range(6)]
    levels = [[] for _ in range(6)]
    n = 128
    while n:
        stitch(faces, n)
        for f in range(6):
            levels[f].append(bytes(faces[f]))
        if n == 1:
            break
        smaller = []
        for face in faces:
            out = bytearray()
            for j in range(n//2):
                for i in range(n//2):
                    for c in range(4):
                        out.append(round(sum(face[((2*j+y)*n+2*i+x)*4+c]
                                             for y in range(2) for x in range(2))/4))
            smaller.append(out)
        faces = smaller
        n //= 2
    
    header[1], header[4], header[5], header[6] = 0x2100f, 512, 0, 8
    header[26], header[27] = 0x401008, 0xfe00
    result = b'DDS ' + struct.pack('<31I', *header) + b''.join(b''.join(f) for f in levels)
    return result


def stage(game, destination, child, sha):
    """Stage a reversible repair. Never change the game here; unknown assets skip."""
    target = child(game, TARGET)
    if not target.is_file():
        return None
    before = sha(target)
    with zipfile.ZipFile(target) as source:
        entries = [i for i in source.infolist()
                   if i.filename.replace('\\', '/').casefold() == ENTRY]
        if len(entries) != 1 or entries[0].file_size != 87508:
            return None
        entry = entries[0]
        raw = source.read(entry)
        if hashlib.sha256(raw).hexdigest() != BROKEN:
            return None
        fixed = build_cube(raw)
        if hashlib.sha256(fixed).hexdigest() != REPAIRED:
            raise ValueError('Water reflection repair verification failed')
        generated = child(destination, GENERATED)
        generated.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(generated, 'w') as output:
            output.comment = source.comment
            for info in source.infolist():
                copied = copy.copy(info)
                if info is entry:
                    output.writestr(copied, fixed)
                else:
                    with source.open(info) as src, output.open(copied, 'w') as dst:
                        shutil.copyfileobj(src, dst, 1024*1024)
    with zipfile.ZipFile(generated) as check:
        if check.testzip():
            raise ValueError('Repaired map archive failed its CRC check')
    backup = child(destination, BACKUP)
    backup.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(target, backup)
    if sha(backup) != before or sha(target) != before:
        raise ValueError('Map changed during water repair staging')
    print('Repaired optional Carbone Island water reflection.', flush=True)
    return dict(target=TARGET, backup=BACKUP, original=before, installed=sha(generated))


def rollback(game, root, state, child, sha, replace_copy):
    change = state.get('water')
    if change is None:
        return
    before, after = change.get('before', ''), change.get('after', '')
    if any(len(h) != 64 or any(c not in '0123456789abcdef' for c in h)
           for h in (before, after)):
        raise ValueError('Invalid water update journal')
    target = child(game, TARGET)
    current = sha(target) if target.exists() else None
    if current == before:
        return
    if current != after:
        raise ValueError('Map changed externally during update; backups retained')
    backup = child(root, BACKUP)
    if sha(backup) != before:
        raise ValueError('Water update rollback backup differs')
    replace_copy(backup, target)
