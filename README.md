# MshIO

MshIO is a tiny library written with modern C++.  It is created by Qingnan Zhou
as a coding exercise.  It supports reading and writing [MSH format] with both
ASCII and binary encodings for both version 2.2 and version 4.1.

![build and test](https://github.com/qnzhou/MshIO/workflows/build%20and%20test/badge.svg)

## Build

```sh
# Build C++ library
mkdir build
cd build
cmake ..
make
```

To build and run the C++ tests from the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMSHIO_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

To build and test the Python package:

```sh
python -m pip install .
```

To run python unit tests:

```sh
python -m pip install '.[test]'
python -m pytest tests
```

## Usage

In C++:
```c++
#include <MshIO/mshio.h>

mshio::MshSpec spec = mshio::load_msh("input.msh");

mshio::save_msh("output.msh", spec);
```

or in Python (requires Python 3.10 or newer)
```python
import mshio

spec = mshio.load_msh("input.msh")
mshio.save_msh("output.msh", spec)
```

Python object collections (`entity_blocks`, `entries`, entity collections,
`physical_groups`, and the data collections) are live vector wrappers, not
Python lists. Accessing a collection or its length does not copy its elements.
Indexing and iteration return independent element copies to avoid dangling
references when the vector changes. Modify an element and assign only that item
back:

```python
blocks = spec.nodes.entity_blocks
block = blocks[0]
block.entity_tag = 2
coordinates = block.data
coordinates[0] += 1.0
block.data = coordinates
blocks[0] = block
```

Collection operations such as `append`, `pop`, and `clear` modify the stored
mesh directly. Whole-collection assignment still accepts Python sequences.
Keep mesh counts and tag bounds consistent when changing collection or payload
sizes; `validate_spec()` checks these invariants.

Large numeric fields return fresh, flat, writable NumPy snapshots:

| Field | NumPy dtype |
| --- | --- |
| `NodeBlock.tags`, `ElementBlock.data` | `uint64` |
| `NodeBlock.data`, `DataEntry.data` | `float64` |

Editing a snapshot does not change the mesh until it is assigned back. Snapshots
are copied under the GIL into independently owned buffers before NumPy sees
them, and remain valid after replacing their source field or destroying the
mesh. Setters accept one-dimensional CPU arrays and Python sequences: matching,
aligned contiguous arrays are copied directly, while unaligned, object-dtype,
non-native-endian, and other supported inputs use checked scalar conversion.
Integer identifiers are not silently narrowed or wrapped. Small metadata
vectors (header tags, physical-group tags, and boundary tags) retain their
Python-list copy semantics.

Nested object collections also require assignment back at each item boundary:

```python
dataset = spec.node_data[0]
entry = dataset.entries[0]
entry.data = [1.0, 2.0, 3.0]
dataset.entries[0] = entry
spec.node_data[0] = dataset
```

Mesh and dataset representations summarize C++ fields directly, without copying
numeric payloads.

## `MshSpec` data structure

`MshSpec` ([code](include/mshio/MshSpec.h)) is a data structure
that maps almost verbatim to the information stored in a MSH file.  Currently,
the following sections are supported:

|        Section name | Description                                                                  |
| ------------------: | ---------------------------------------------------------------------------- |
|       [Mesh format] | Format header.                                                               |
|             [Nodes] | 3D coordinates of nodes and (optionally) their parameterization coordinates. |
|          [Elements] | A list of elements grouped by blocks.                                        |
|          [Entities] | The boundary representation (BRep) of the model.                             |
|   [Physical groups] | Metadata defined on elements.                                                |
|         [Node data] | Scalar/vector/tensor fields defined on nodes.                                |
|      [Element data] | Scalar/vector/tensor fields defined on elements.                             |
| [Element-node data] | Scalar/vector/tensor fields defined over each node of each element.          |
| [Partitioned entities] | Partition memberships, parent entities, and ghost-entity ownership. |
| [Periodic links] | Affine transforms and corresponding slave/master node pairs. |
| [Ghost elements] | Element ownership and receiving ghost partitions. |
| [Discrete parametrizations] | Curve/surface samples, local triangulations, and curvature vectors. |
| [Interpolation schemes] | Named interpolation topologies and their matrices. |


All fields will be populated by `mshio::load_msh()` method, and all fields
should be set up correctly before calling `mshio::save_msh()` method. The helper
method `mshio::validate_spec(spec)` checks that counts and data sizes are
consistent, node and element tags are unique, and every element references an
existing node. `load_msh()` does not perform these checks; call `validate_spec()`
before relying on tags from untrusted files.

### Portability and public integer types

MSH 4.1 uses an eight-byte format contract independent of the host ABI:
specification `size_t` fields are `std::uint64_t`, specification `int` fields are
`std::int32_t`, and coordinates and values are eight-byte `double`s. Both ASCII
and binary MSH 4.1 files require `MeshFormat::data_size == 8`; four-byte MSH 4.1
files are not supported. The existing binary endianness policy is unchanged.

Node and element counts, block counts, tag bounds, node tags, element tags, and
connectivity now use `std::uint64_t` instead of host `size_t`. Entity dimensions,
entity tags, physical-group tags, signed boundary tags, and element types use
`std::int32_t`. Post-processing entry tags and integer header tags remain
32-bit, as required by the current MSH specification; they cannot refer to mesh
identifiers above `INT32_MAX`. MSH 2.2 retains its separate 32-bit layouts and
rejects identifiers that cannot be represented when saving.

The optional NanoSpline `Curve` and `Patch` integer fields also use
`std::uint64_t`; their custom-section headers remain ASCII.

C++ consumers must migrate tag vectors and tag-to-index maps to the fixed-width
types; resolve identifiers before converting them to local mesh indices. Python
integers retain the same API but must fit the corresponding fixed-width field.
Container sizes and indices remain host `size_t`. Sparse mesh identifiers above
`UINT32_MAX` do not require correspondingly large allocations.

Loading checks file counts against host allocation and container limits, and
checks stride and byte-size products before allocation or binary IO. Declared
counts are not trusted for allocation: containers grow as data is actually read,
so memory use is proportional to the input size. Out-of-range fields,
inconsistent section counts, overflowing allocation/stride products, and
truncated supported-section payloads raise exceptions in Release as well as
Debug builds. This is not complete file-format validation: empty/headerless
inputs can produce an empty spec, and `validate_spec()` checks mesh data
invariants rather than required-header presence. A mesh must still fit the
host's available memory.


### Mesh format

Mesh format section is the header of MSH file.  It contains information about
the MSH version used, whether the file is binary and data size.  This section is
required.

```c++
auto& format = spec.mesh_format;
format.version = "4.1";            // Only version "2.2" and "4.1" are supported.
format.file_type = 1;              // 0: ASCII, 1: binary.
format.data_size = 8;              // Fixed MSH 4.1 format width.
```

### Nodes

Nodes are grouped into node blocks in MSH format.  Each node has a unique "tag",
and the tag is used for referring to this node in other sections.  All tags are
positive.  Ideally, tags should be ordered consecutively from 1 to N (the number
of nodes), but it does not have to be so.  It is up to the client application to
maintain a mapping from tag to nodes.

```c++
auto& nodes = spec.nodes;
nodes.num_entity_blocksl = 1;  // Number of node blocks.
nodes.num_nodes = 3;           // Total number of nodes.
nodes.min_node_tag = 1;
nodes.max_node_tag = 3;
nodes.entity_blocks = {...};   // A std::vector of node blocks.
```

#### Node block

A node block is simply a group of nodes.

```c++
auto& block = nodes.entity_blocks[k];
block.entity_dim = 2;          // The dimension of the entity.
block.entity_tag = 1;          // The entity these nodes belongs to.
block.parametric = 0;          // 0: non-parametric, 1: parametric.
block.num_nodes_in_block = 3;  // The number of nodes in block.
block.tags = {...};            // A std::vector of unique, positive node tags.
block.data = {...};            // A std::vector of coordinates (x,y,z,<u>,<v>,<w>,...)
```

When `block.parametric` is `1`, `block.data` contains the parametric coordinates
in addition to the XYZ coordinates.  The dimension of the parametric coordinates
is defined by `block.entity_dim` variable.

### Elements

Elements are grouped into element blocks.  Each element has a unique positive
"tag".  Ideally, tags should be ordered consecutively from 1 to M (the number of
elements), but it does not have not be so.  It is up to the client application
to maintain a mapping from tag to elements.

```c++
auto& elements = spec.elements;
elements.num_entity_blocks = 1;  // Number of element blocks.
elements.num_elements = 12;      // Total number of elmeents.
elements.min_element_tag = 1;
elements.max_element_tag = 12;
elements.entity_blocks = {...};  // A std::vector of element blocks.
```

#### Element block

An element block is a set of elements of the same type:

```c++
auto& block = elements.entity_blocks[k];
block.entity_dim = 2;             // The dimension of the elements.
block.entity_tag = 1;             // The entity these elements belongs to.
block.element_type = 2;           // See element type table below.
block.num_elements_in_block = 12; // The number of elements in this block.
block.data = {...};               // See more detail below.
```

MSH 2.2 stores integer metadata tags on each element. `ElementBlock::legacy_tags`
(`block.legacy_tags` in Python) retains these tags for every element in the block:
the physical-group tag, elementary-entity tag, and any following partition,
ghost-partition, or application-defined tags. Elements with different metadata
remain in separate blocks. Loaded records with fewer than two tags are padded
with zeros, which mean no physical group or no elementary entity.

When saving MSH 2.2:

- Nonempty `legacy_tags` supplies the physical-group tag and all tags after the
  elementary-entity tag. `block.entity_tag` supplies the elementary-entity tag,
  including after changing a block's entity.
- With empty `legacy_tags`, the physical-group tag comes from the matching
  entity's `physical_group_tags`, or zero if there is no membership. Entity
  dimension and tag both participate in this lookup; physical names alone do
  not assign membership.
- Multiple physical groups on an entity cannot fit in one MSH 2.2 element
  record. Saving raises `UnsupportedFeature` unless `legacy_tags` explicitly
  selects the element's physical group.

Clear `legacy_tags` to derive membership from entities after editing them.
MSH 4.1 output does not serialize `legacy_tags`; its physical groups are stored
on entities. Partition and ghost metadata use `partitioned_entities` and
`ghost_elements`, rather than the legacy element-tag array.

#### Element types

MSH format supports a large set of finite element types.  Element types
determines the dimension of the element as well as the number of nodes that make
up the element.

```c++
int element_type = ...;  // Element type is encoded as an int.

// To look up the number of nodes for this element type:
size_t n = nodes_per_element(element_type);

// To look up the dimension of the element:
int dim = get_element_dim(element_type);
```

See all supported element types [here](#Supported-element-types).


#### Element data

The element data is a flattened `std::vector<std::uint64_t>`. It follows the
convention below:

```
element_tag node_tag ... node_tag
element_tag node_tag ... node_tag
...
```

i.e. Each element entry consists of an element tag followed by `n` node tags,
where `n` is the number of nodes corresponding determined by the element type.
See the [supported element types](#Supported-element-types) table.

### Entities

Entities make up the boundary representation of the mesh model. Nodes and
elements belong to specific entities. The four types of entities are points,
lines, surfaces and volumes. Higher order entities are bounded by lower ones
(e.g. volumes are bounded by surfaces). Entities also contain bounding box and
physical group information.

```c++
auto& entities = spec.entities;
auto& point_entities = entities.points;
auto& curve_entities = entities.curves;
auto& surface_entities = entities.surfaces;
auto& volume_entities = entities.volumes;

// look up the physical groups of an element block
auto& block = elements.entity_blocks[i];
assert(block.entity_dim == 2); // surface entity (0 for point, 1 for curve...)
auto surface_entity = std::find_if(surface_entities.begin(),
    surface_entities.end(), [&](const mshio::SurfaceEntity& s) {
    return s.tag == block.entity_tag;
});
assert(surface_entity != surface_entities.end());
auto& physical_groups = surface_entity->physical_group_tags;
```

### Partitioned entities and ghost elements

`spec.partitioned_entities` stores `num_partitions`, a `ghost_entities` vector,
and separate `points`, `curves`, `surfaces`, and `volumes` vectors. The
`PartitionedPointEntity`, `PartitionedCurveEntity`, `PartitionedSurfaceEntity`,
and `PartitionedVolumeEntity` types retain the corresponding ordinary entity's
geometry, physical-group and boundary fields, plus `parent_dim`, `parent_tag`,
and `partitions`. A zero parent tag means no parent. A `GhostEntity` contains
its entity `tag` and owning `partition`.

`spec.ghost_elements` contains `GhostElement` records: a 64-bit `element_tag`,
a 32-bit owning `partition`, and a list of 32-bit `ghost_partitions` receiving
the element. Entity and partition tags remain signed 32-bit fields, unlike mesh
identifiers and section counts.

Both sections support MSH 4.1 ASCII and binary. Saving either nonempty section
as MSH 2.2 raises an exception instead of silently dropping it. Legacy 2.2
per-element partition tags remain available through `ElementBlock::legacy_tags`;
there is no implicit conversion between these representations.

### Periodic links

`spec.periodic_links` contains `PeriodicLink` records with `entity_dim`,
`slave_tag`, `master_tag`, `affine_transform`, and `node_pairs`. The affine
transform is empty or contains a row-major 4-by-4 matrix (16 doubles).
Each `PeriodicNodePair` stores 64-bit slave and master node identifiers as
`slave_tag` and `master_tag`.

MSH 4.1 uses its native ASCII/binary section layout. MSH 2.2 keeps this section
ASCII even in binary files, with the optional `Affine` marker; node identifiers
must fit signed 32-bit integers when saving that version. To independently
load legacy periodicity with Gmsh, set `Mesh.IgnorePeriodicity = 0`: Gmsh skips
MSH 2.2 periodic sections by default.

### Discrete parametrizations

`spec.parametrizations` contains `CurveParametrization` and
`SurfaceParametrization` records in `curves` and `surfaces`. Their samples are
local discrete geometry data, not references to the mesh's node tags:

- Curves store `tag`, `num_nodes`, and flat `data` records `[x, y, z, u]`.
- Surfaces store `tag`, `num_nodes`, `num_triangles`, and 11-double `data`
  records `[x, y, z, u, v, curvMaxX, curvMaxY, curvMaxZ, curvMinX, curvMinY,
  curvMinZ]`. `triangle_indices` contains three signed 32-bit, **zero-based**
  local sample indices per triangle.

Keep counts consistent when editing payloads; malformed sizes and out-of-range
triangle indices are rejected. This section supports MSH 4.1 ASCII/binary and
rejects nonempty MSH 2.2 export. It is distinct from parametric coordinates in
`NodeBlock::data`.

### Interpolation schemes

`spec.interpolation_schemes` contains named `InterpolationScheme` records.
Each scheme has `name` and a `topologies` vector. An `InterpolationTopology`
contains `element_topology` and `matrices`; topology IDs 1–10 designate points,
lines, triangles, quadrangles, tetrahedra, pyramids, prisms, hexahedra, polygons,
and polyhedra respectively. These IDs are not MSH element-type IDs.

Each `InterpolationMatrix` has signed 32-bit `num_rows`, `num_columns`, and a
row-major double `data` vector. All matrices and multiple named schemes are
preserved. The section is ASCII even in binary MSH 2.2/4.1 files and is written
before datasets that refer to it through `DataHeader::string_tags[1]`.

All of these models are exposed in Python under the same names and fields.
Object-vector indexing returns independent copies: edit a record and assign it
back at every object-vector boundary, as for existing mesh blocks. New numeric
metadata vectors use Python-list snapshots. `validate_spec()` checks their local
sizes and identifier ranges, but does not resolve optional metadata references
against a complete mesh; standalone metadata/post-processing files are allowed.


### Physical groups

Physical groups are used to tag elements with arbitrary metadata (e.g.
"boundary", "steel", etc.). Physical groups have a `std::int32_t` tag and
dimension, and a `string` name. The name may be empty.

```c++
auto& physical_groups = spec.physical_groups;
auto& pg = physical_groups[k];

pg.dim = 0;
pg.tag = 1;
pg.name = "My Physical Group";
```

### Post-processing data

One of main advantage of MSH format is its support for storing post-processing
data along with the mesh.  There are 3 types of post-processing data: node data,
element data and element-node data.  Each type of post-processing data consists
of a header and a `std::vector` of entries.

```c++
auto& node_data = spec.node_data[k];
auto& element_data = spec.element_data[k];
auto& element_node_data = spec.element_node_data[k];
```

#### Data header

Data header consists of arrays of `string`, `double` and `std::int32_t` tags.
Some tags have pre-defined meaning:

```c++
auto& header = node_data.header;
header.string_tags = {...};  // [field_name, <interpolaiton_scheme>, ...]
header.real_tags = {...};    // [<time value>, ...]
header.int_tags = {...};     // [time step, num fields, num entities, <partition id>, ...]
```

Conventional header tags:
* `string_tags[0]`: The name of the data.
* `int_tags[0]`: Time step index, starting from 0.
* `int_tags[1]`: The number of fields per data entry. 1 for scalar field, 3 for
  vector field, and 9 for tensor field.
* `int_tags[2]`: The number of data entries.

#### Data entry

Data entry describe the data associated with a target node or element or
element-node.

```c++
auto& data_entries = node_data.entries;
auto& entry = data_entries[k];

entry.tag = 1; // The target node (for node data) or element tag (for element data).
entry.data = {...}; // A std::vector of double data.
```

The `entry.data`'s size is determined in the header (i.e. `header.int_tags[1]`)
for node and element data.

For element-node data, `entry.tag` is the target element tag, and it has an
extra field `entry.num_nodes_per_element` that determines the number of nodes in
each element (this can be computed from element type using `nodes_per_element`
method). The `entry.data` is of size `num_nodes_per_element` times the number of
fields.

### Supported element types

MshIO supports **132 fixed-width element types** from the released Gmsh 4.15.2
wire definitions: IDs **1–33, 36–68, 70–75, 79–137, and 140**, in both MSH 2.2
and 4.1 ASCII/binary. `nodes_per_element()` and `get_element_dim()` provide the
record width and dimension for every supported ID.

Variable-width polygon/polyhedron types 34, 35, and 69 are rejected, as are
undefined IDs 76–78 and MINI types 138–139, which lack fixed-width MSH record
definitions in the released reader. Some defined fixed-width IDs also lack a
working factory in Gmsh 4.15.2 itself; MshIO preserves their wire records without
requiring Gmsh to instantiate a basis. Gmsh-backed tests cover the native-loader
subset, while C++ round-trip tests cover all 132 types.

The original low-order and order-3–5 types are listed below:

| Type ID | Name                                     | Dim | # nodes |
| :-----: | ---------------------------------------- | --- | ------- |
|    1    | 2-node line                              | 1   | 2       |
|    2    | 3-node triangle                          | 2   | 3       |
|    3    | 4-node quad                              | 2   | 4       |
|    4    | 4-node tetrahedron                       | 3   | 4       |
|    5    | 8-node hexahedron                        | 3   | 8       |
|    6    | 6-node prism                             | 3   | 6       |
|    7    | 5-node pyramid                           | 3   | 5       |
|    8    | 3-node second order line                 | 1   | 3       |
|    9    | 6-node second order triangle             | 2   | 6       |
|   10    | 9-node second order quad                 | 2   | 9       |
|   11    | 10-node second order tetrahedron         | 3   | 10      |
|   12    | 27-node second order hexahedron          | 3   | 27      |
|   13    | 18-node second order prism               | 3   | 18      |
|   14    | 14-node second order pyramid             | 3   | 14      |
|   15    | 1-node point                             | 0   | 1       |
|   16    | 8-node second order quadrangle           | 2   | 8       |
|   17    | 20-node second order hexahedron          | 3   | 20      |
|   18    | 15-node second order prism               | 3   | 15      |
|   19    | 13-node second order pyramid             | 3   | 13      |
|   20    | 9-node third order incomplete triangle   | 2   | 9       |
|   21    | 10-node third order triangle             | 2   | 10      |
|   22    | 12-node fourth order incomplete triangle | 2   | 12      |
|   23    | 15-node fourth order triangle            | 2   | 15      |
|   24    | 15-node fifth order incomplete triangle  | 2   | 15      |
|   25    | 21-node fifth order complete triangle    | 2   | 21      |
|   26    | 4-node third order edge                  | 1   | 4       |
|   27    | 5-node fourth order edge                 | 1   | 5       |
|   28    | 6-node fifth order edge                  | 1   | 6       |
|   29    | 20-node third order tetrahedron          | 3   | 20      |
|   30    | 35-node fourth order tetrahedron         | 3   | 35      |
|   31    | 56-node fifth order tetrahedron          | 3   | 56      |

Additional supported families:

| Type IDs | Family |
| --- | --- |
| 32–33 | 22/28-node incomplete tetrahedra |
| 36–61 | Higher-order complete/incomplete quadrangles and triangles |
| 62–66 | Order-6–10 lines |
| 67–68, 70 | Border line/triangle and child line |
| 71–75, 79–83 | Order-6–10 complete/incomplete tetrahedra |
| 84–89, 132 | Order-zero line, triangle, quadrangle, tetrahedron, hexahedron, prism, pyramid |
| 90–91, 106–110 | Order-3–9 complete prisms |
| 92–98 | Order-3–9 complete hexahedra, including 64- and 125-node types 92/93 |
| 99–105 | Order-3–9 incomplete hexahedra |
| 111–117 | Order-3–9 incomplete prisms |
| 118–124, 125–131 | Order-3–9 complete/incomplete pyramids |
| 133–136 | Fixed-width XFEM point, line, triangle, tetrahedron |
| 137 | 16-node incomplete tetrahedron |
| 140 | 4-node trihedron |


[MSH format]: https://gmsh.info/doc/texinfo/gmsh.html#MSH-file-format
[Mesh format]: #Mesh-format
[Nodes]: #Nodes
[Elements]: #Elements
[Node data]: #Post-processing-data
[Element data]: #Post-processing-data
[Element-node data]: #Post-processing-data
[Entities]: #Entities
[Physical groups]: #Physical-groups
[Partitioned entities]: #Partitioned-entities-and-ghost-elements
[Ghost elements]: #Partitioned-entities-and-ghost-elements
[Periodic links]: #Periodic-links
[Discrete parametrizations]: #Discrete-parametrizations
[Interpolation schemes]: #Interpolation-schemes
