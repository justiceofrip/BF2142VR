"""Read-only sight coverage audit against an owned BF2142 installation.
No executable/game asset is copied or modified. An unknown optical weapon,
missing HUD route or mismatched stock zoom factor fails the audit.
"""
import argparse
import json
import re
import zipfile
from pathlib import Path

IRON_SIGHTS = {'eu_handgun', 'as_handgun'}
NUMBER = r'[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?f?'
PROFILE = re.compile(
    r'\{"(?P<name>\w+)",\{(?P<x>'+NUMBER+r'),(?P<y>'+NUMBER+r'),(?P<z>'+NUMBER+r')\},'
    r'(?P<w>'+NUMBER+r'),(?P<h>'+NUMBER+r'),(?P<rect>true|false),'
    r'(?P<factor>'+NUMBER+r'),(?P<mag>'+NUMBER+r')(?P<flags>(?:,(?:true|false))*)\}')

def commands(text):
    comment = False
    for raw in text.splitlines():
        words = raw.strip().split()
        if not words:
            continue
        if words[0].lower() == 'beginrem':
            comment = True
        elif words[0].lower() == 'endrem':
            comment = False
        elif not comment and words[0].lower() != 'rem':
            yield words

def profiles(text):
    result = {}
    text = re.sub(r"//[^\n]*", "", text)
    for match in PROFILE.finditer(text):
        d = match.groupdict()
        name = d.pop('name')
        if name in result:
            raise ValueError('Duplicate optical profile: '+name)
        result[name] = {k: float(v.removesuffix('f')) if k not in ('rect', 'flags') else v for k, v in d.items()}
    if not result:
        raise ValueError('No optical definitions found')
    return result

def weapon_records(archive):
    result = []
    for path in sorted(archive.namelist()):
        if not path.lower().startswith('handheld/') or not path.lower().endswith('.tweak'):
            continue
        lines = list(commands(archive.read(path).decode('latin1')))
        main = next((w[2].lower() for w in lines if len(w)>2 and w[0].lower()=='objecttemplate.activesafe' and w[1].lower()=='genericfirearm'), None)
        if not main:
            continue
        # Stop at the next object/template, before projectile and sound properties.
        props = {}
        started = False
        for w in lines:
            key = w[0].lower()
            if key == 'objecttemplate.activesafe':
                if started:
                    break
                started = True
                continue
            if started and key == 'objecttemplate.create':
                break
            if started:
                props.setdefault(key, []).append(w[1:])
        def values(key):
            return [v[0] for v in props.get('objecttemplate.'+key, []) if v]
        factors = [float(x) for x in values('zoom.addzoomfactor')]
        alt = values('weaponhud.altguiindex')
        result.append(dict(name=main, path=path, factors=factors,
                           zoom_lod=values('zoom.zoomlod'), alt=int(alt[0]) if alt else None,
                           geometry=values('geometry')))
    return result

def hud_routes(archive):
    nodes = {}
    for path in archive.namelist():
        if not path.lower().startswith('hud/hudsetup/weapons/') or not path.lower().endswith('.con'):
            continue
        current = None
        for w in commands(archive.read(path).decode('latin1')):
            key = w[0].lower()
            if key == 'hudbuilder.createsplitnode' and len(w)>2:
                current = w[2]
                nodes.setdefault(current, set())
            elif key.startswith('hudbuilder.create') or key == 'hudbuilder.setactiveobject':
                current = None
            elif current and key == 'hudbuilder.setnodelogicshowvariable' and len(w)>3 and w[2].lower()=='guiindex':
                if w[1].upper() in ('EQUAL', 'OR'):
                    nodes[current].add(int(w[3]))
    return nodes

def audit(records, definitions, huds, routed_nodes, routed_indices):
    errors = []
    report = []
    seen = set()
    for w in records:
        name = w['name']
        if name in seen:
            errors.append('Duplicate weapon template: '+name)
        seen.add(name)
        zooms = w['factors']
        row = dict(name=name, factors=zooms, alt_gui=w['alt'])
        if len(zooms)<2:
            row['coverage'] = 'native/no optical zoom'
            if name in definitions:
                errors.append(name+': profile has no matching native zoom')
        elif name in IRON_SIGHTS and not w['zoom_lod']:
            row['coverage'] = 'native iron sights'
            if name in definitions:
                errors.append(name+': iron-sight pistol unexpectedly gained an overlay')
        else:
            row['coverage'] = 'VR optic'
            d = definitions.get(name)
            if not d:
                errors.append(name+': missing optical profile')
            elif abs(d['factor']-zooms[1])>0.0001:
                errors.append(name+': native zoom factor mismatch')
            if w['zoom_lod'] != ['1']:
                errors.append(name+': unverified native zoom LOD')
            if w['alt'] not in routed_indices:
                errors.append(name+': missing optic HUD selector')
            roots = [node for node, indices in huds.items() if indices=={w['alt']} and node+'CullNode' in routed_nodes]
            row['hud_roots'] = roots
            if not roots:
                errors.append(name+': no exact native zoom HUD root is routed')
        report.append(row)
    for name in definitions.keys()-seen:
        errors.append(name+': optical profile absent from installed templates')
    return report, errors

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir', required=True, type=Path)
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    source = args.source_root/'src'/'bf2142'
    definitions = profiles((source/'GunOptics.cpp').read_text())
    routing = (source/'NativeCrosshair.cpp').read_text()
    routed_nodes = set(re.findall(r'"(\w+CullNode)"', routing))
    index_switch = re.search(r'bool OpticHud\(int index\)\{(.*?)\n', routing).group(1)
    routed_indices = {int(n) for n in re.findall(r'case (\d+):', index_switch)}
    mod = args.game_dir/'mods'/'bf2142'
    with zipfile.ZipFile(mod/'Objects'/'Weapons_server.zip') as z:
        records = weapon_records(z)
    with zipfile.ZipFile(mod/'Menu_server.zip') as z:
        huds = hud_routes(z)
    report, errors = audit(records, definitions, huds, routed_nodes, routed_indices)
    counts = {key: sum(w['coverage']==key for w in report) for key in sorted({w['coverage'] for w in report})}
    print(json.dumps(dict(templates=len(report), coverage=counts, errors=errors), indent=2))
    if args.report:
        args.report.write_text(json.dumps(dict(counts=counts, weapons=report, errors=errors), indent=2)+'\n')
    return bool(errors)

if __name__ == '__main__':
    raise SystemExit(main())
