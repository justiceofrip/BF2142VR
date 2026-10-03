"""Separate verified repair-added inner skins from the original weapon surface.

Consumes completed, interior-trimmed first-person meshes. No original exterior,
transparent material, rig attribute, triangle index or world LOD is changed.
"""
import copy, math, struct
from RepairWeaponMeshes import Mesh, inside_vertex, expand_first_person_bounds

INSET_METRES = .0005

def repair(data):
    mesh = Mesh(data)
    before = copy.deepcopy(mesh.lods)
    moved = 0
    for lod in mesh.lods[0]:
        for material in lod:
            if material['alpha'] != 0:
                continue
            vertices = material['vertices']
            if len(vertices) % 2:
                raise ValueError('Expected paired repaired vertices')
            half = len(vertices) // 2
            if vertices[half:] != [inside_vertex(v) for v in vertices[:half]]:
                raise ValueError('Repair backface provenance differs')
            for index in range(half, len(vertices)):
                vertex = vertices[index]
                position = struct.unpack_from('<3f', vertex)
                normal = struct.unpack_from('<3f', vertex, 12)
                length = math.sqrt(sum(x*x for x in normal))
                if not all(math.isfinite(x) for x in position) or not math.isfinite(length) or length < .01:
                    raise ValueError('Invalid repair-added inner vertex')
                result = bytearray(vertex)
                # The paired normal already points inward. A physical offset
                # survives native vertex packing without relying on draw order
                # or depth bias, and preserves the reverse side of thin details.
                struct.pack_into('<3f', result, 0,
                    *(p + INSET_METRES*n/length for p, n in zip(position, normal)))
                vertices[index] = bytes(result)
                moved += 1
    expanded = expand_first_person_bounds(mesh)
    encoded = mesh.encode()
    check = Mesh(encoded)
    if check.lods[1:] != before[1:]:
        raise ValueError('World geometry changed')
    for old_lod, new_lod in zip(before[0], check.lods[0]):
        for old, new in zip(old_lod, new_lod):
            if old['alpha'] != 0:
                if old != new: raise ValueError('Transparent material changed')
                continue
            half = len(old['vertices']) // 2
            if old['vertices'][:half] != new['vertices'][:half]:
                raise ValueError('Exterior vertex changed')
            if any(a[12:] != b[12:] for a,b in zip(old['vertices'][half:],new['vertices'][half:])):
                raise ValueError('Inner vertex attributes changed')
            if any(old[key] != new[key] for key in old if key != 'vertices'):
                raise ValueError('Material or indices changed')
    return encoded, {'separated_inner_vertices': moved, 'inset_metres': INSET_METRES,
                     'expanded_first_person_bounds': expanded}
