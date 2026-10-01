"""Developer tool: compile decision-only plans from an owned stock archive.
The generated module contains only reject/append/subdivide decisions. All
geometry and interpolation are still derived from each user's installed game.
Never distribute the source archive, repaired archive, or intermediate meshes.
"""
import argparse,hashlib,time,zipfile,sys,textwrap
from pathlib import Path
import RepairWeaponMeshes as weapons
import RemoveInteriorBackfaces as interiors
from RepairDecisionPlan import DecisionPlan,encode
sys.path.insert(0,str(Path(__file__).parent/'package'))
from WeaponRepairHashes import PROFILES

def build(source,output):
    if output.exists():raise ValueError('Output must be new')
    rows=['"""Repair instructions for hash-identified stock meshes; no game assets."""','PLANS = {']
    with zipfile.ZipFile(source) as archive:
        entries={Path(x).stem.lower():x for x in archive.namelist() if x.lower().endswith('.bundledmesh')}
        for name,profile in sorted(PROFILES.items()):
            start=time.monotonic();data=archive.read(entries[name])
            if hashlib.sha256(data).hexdigest()!=profile['source']:raise ValueError('Source mismatch: '+name)
            surface=[];inside=[]
            repaired,_=weapons.repair(data,name,record=surface)
            repaired,_=interiors.repair(repaired,record=inside)
            if hashlib.sha256(repaired).hexdigest()!=profile['result']:raise ValueError('Result mismatch: '+name)
            compiled=time.monotonic();a,b=encode(surface),encode(inside)
            replay,_=weapons.repair(data,name,surface_plan=DecisionPlan(a))
            replay,_=interiors.repair(replay,plan=DecisionPlan(b))
            if repaired!=replay:raise ValueError('Replay differs: '+name)
            rows.append('    '+repr(name)+': (')
            for code in (a,b):
                rows.append('        (')
                rows.extend('            '+repr(code[i:i+100]) for i in range(0,len(code),100))
                rows.append('        ),')
            rows.append('    ),')
            print(f'{name}: compile={compiled-start:.2f}s replay={time.monotonic()-compiled:.2f}s decisions={len(surface)+len(inside)} bytes={len(a)+len(b)} exact=True',flush=True)
    rows.append('}')
    output.write_text('\n'.join(rows)+'\n',encoding='utf-8')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();build(a.source,a.output)
