#!/usr/bin/env python3
"""Generate/check portability fixtures using the independent Gmsh 4.13.1 API.

Generation: python -m pip install gmsh==4.13.1; python data/generate_portability_fixtures.py
Validation: python data/generate_portability_fixtures.py --check OUTPUT_DIRECTORY
Fixtures are Gmsh output, not MshIO round trips. Gmsh 4.13.1 truncates sparse
element tags internally, so its fixtures use 11 and 13; MshIO tests replace 11
with 1099511627787 in sparse regressions. Postprocessing references stay 32-bit.
"""

import argparse
from pathlib import Path

import gmsh

ROOT = Path(__file__).resolve().parent
NODE_TAGS = [1, 2, 3, 4, 4294967297, 1099511627783, 281474976710665]
ELEMENT_TAGS = [13, 11]
COORDINATES = [0., 0., 0., 1., 0., 0., 0., 1., 0., 0., 0., 1.,
               2., 0., 0., 2., 1., 0., 2., 0., 1.]


def generate():
    gmsh.model.add("mshio-portability")
    geo = gmsh.model.geo
    for tag, xyz in enumerate([(0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1)], 1):
        geo.addPoint(*xyz, 1., tag)
    for tag, ends in enumerate([(1, 2), (2, 3), (3, 1), (1, 4), (2, 4), (3, 4)], 1):
        geo.addLine(*ends, tag)
    for tag, edges in enumerate([[1, 2, 3], [1, 5, -4], [2, 6, -5], [3, 4, -6]], 1):
        geo.addCurveLoop(edges, tag)
        geo.addPlaneSurface([tag], tag)
    geo.addSurfaceLoop([-1, 2, 3, 4], 1)
    geo.addVolume([1], 1)
    geo.synchronize()
    for dim, tag, name in [(0, 101, "corner"), (1, 102, "edge"),
                           (2, 103, "face"), (3, 104, "body")]:
        gmsh.model.addPhysicalGroup(dim, [1], tag, name)
    gmsh.model.mesh.addNodes(3, 1, NODE_TAGS, COORDINATES)
    gmsh.model.mesh.addElementsByType(
        1, 4, ELEMENT_TAGS, [1, 2, 3, 4, 2, *NODE_TAGS[4:]])
    views = []
    for name, kind, tags, values, components in [
        ("node-vector", "NodeData", [1, 2], [[1., 2., 3.], [4., 5., 6.]], 3),
        ("element-scalar", "ElementData", [13], [[7.]], 1),
        ("element-node", "ElementNodeData", [13], [[1., 2., 3., 4., 5., 6., 7., 8.]], 2),
    ]:
        view = gmsh.view.add(name)
        gmsh.view.addModelData(view, 0, "mshio-portability", kind, tags, values, 0.5, components)
        # Exercise the supported core MSH sections without an interpolation extension.
        gmsh.view.setInterpolationMatrices(view, "Tetrahedron", -1, [], [])
        views.append(view)
    gmsh.option.setNumber("Mesh.MshFileVersion", 4.1)
    gmsh.option.setNumber("PostProcessing.SaveMesh", 0)
    gmsh.option.setNumber("Mesh.SaveAll", 1)
    for binary, suffix in [(0, "ascii"), (1, "binary")]:
        gmsh.option.setNumber("Mesh.Binary", binary)
        path = str(ROOT / f"portability_4.1_{suffix}.msh")
        gmsh.write(path)
        for view in views:
            gmsh.view.write(view, path, True)


def snapshot(path):
    gmsh.clear()
    gmsh.open(str(path))
    tags, coordinates, _ = gmsh.model.mesh.getNodes()
    nodes = sorted((int(tag), tuple(coordinates[3*i:3*i+3])) for i, tag in enumerate(tags))
    types, element_tags, connectivity = gmsh.model.mesh.getElements()
    elements = []
    for kind, tags, data in zip(types, element_tags, connectivity):
        width = gmsh.model.mesh.getElementProperties(int(kind))[3]
        elements.extend((int(kind), int(tag), tuple(int(x) for x in data[width*i:width*(i+1)]))
                        for i, tag in enumerate(tags))
    entities = []
    # Loaded discrete entities need not have a geometric bounding box in Gmsh.
    # Stored bounds are checked against fixture bytes by the native C++ tests.
    for dim, tag in gmsh.model.getEntities():
        physicals = tuple(int(x) for x in gmsh.model.getPhysicalGroupsForEntity(dim, tag))
        boundary = tuple(gmsh.model.getBoundary([(dim, tag)], oriented=True)) if dim else ()
        entities.append((dim, tag, physicals, boundary))
    groups = [(dim, tag, gmsh.model.getPhysicalName(dim, tag))
              for dim, tag in gmsh.model.getPhysicalGroups()]
    views = []
    for view in gmsh.view.getTags():
        kind, tags, values, time, components = gmsh.view.getModelData(int(view), 0)
        entries = sorted((int(tag), tuple(value)) for tag, value in zip(tags, values))
        views.append((gmsh.view.option.getString(int(view), "Name"), kind, entries, time, components))
    return nodes, sorted(elements), entities, groups, sorted(views)


def check(directory):
    expected = snapshot(ROOT / "portability_4.1_binary.msh")
    for encoding in ["ascii", "binary"]:
        path = directory / f"portability_{encoding}.msh"
        actual = snapshot(path)
        if actual != expected:
            raise RuntimeError(f"Independent Gmsh validation differs for {path}")
        print(f"Gmsh validated {path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", type=Path)
    args = parser.parse_args()
    gmsh.initialize()
    try:
        check(args.check) if args.check else generate()
    finally:
        gmsh.finalize()
