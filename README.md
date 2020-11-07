# MshIO

MshIO is a tiny library written with modern C++.  It is created by Qingnan Zhou
as a coding excercise.  It supports reading and writing [MSH format] with both
ASCII and binary encodings.

## Usage

```c++
#include <MshIO/mshio.h>

MshIO::MshSpec spec = MshIO::load_msh("input.msh");

MshIO::save_msh("output.msh", spec);
```

## `MshSpec` data structure

`MshSpec` is a data structure that maps almost verbatim to the information
stored in a MSH file.  Currently, the following sections are supported:

| Section name | Description |
|----:|---|
| [Mesh format] | Format header. |
| [Nodes] | 3D coordinates of nodes and (optionally) their parameterization coordiantes. |
| Elements | A list of elements groupped by blocks. |
| Node data | Scalar/vector/tensor fields defined on nodes. |
| Element data | Scalar/vector/tensor fields defined on elements. |
| Element-node data | Scalar/vector/tensor fields defined over each node of each element. |

The follow sections are supported by MSH format, but not yet supported by MshIO
(contribution welcomed):
* Physical names
* Entities
* Partitioned entities
* Periodic
* Ghost elements
* Parametrizations
* Interpolation scheme

All fields will be populated by `MshIO::load_msh()` method, and all fields
should be set up correctly before calling `MshIO::save_msh()` method. The helper
method `MshIO::validate_spec(spec)` can be used to check if a given `spec` is
valid.

### Mesh format

Mesh format section is the header of MSH file.  It constains information about
the MSH version used, whether the file is binary and data size.  This section is
required.

```c++
auto& format = spec.mesh_format;
format.version = "4.1";            // Only version "2.2" and "4.1" are supported.
format.file_type = 1;              // 0: ASCII, 1: binary.
format.data_size = sizeof(size_t); // Size of data, defined as sizeof(size_t) = 8.
```

### Nodes

Nodes are grouped into node blocks in MSH format.  Each node has a unique "tag",
and the tag is used for referring to this node in other sections.  All tags are
positive.

```c++
auto& nodes = spec.nodes;
nodes.num_entity_blocksl = 1;  // Number of node blocks.
nodes.num_nodes = 3;           // Total number of nodes.
nodes.min_node_tag = 1;
nodes.min_node_tag = 3;
nodes.entity_blocks = ...;     // A std::vector of node blocks.
```

#### Node block

A node block is simply a group of nodes.

```c++
auto& block = nodes.entity_blocks[i];
block.entity_dim = 3;          // The dimension of the entity.
block.entity_tag = 1;          // The entity these nodes belongs to.
block.parametric = 0;          // 0: non-parametric, 1: parametric.
block.num_nodes_in_block = 3;  // The number of nodes in block.
block.tags = ...;              // A std::vector of node tags.
block.data = ...;              // A std::vector of coordinates (x,y,z,<u>,<v>,<w>,...)
```

When `block.parametric` is `1`, `block.data` contains the parametric coordinates
in addition to the XYZ coordinates.  The dimension of the parametric coordinates
is defined by `block.entity_dim` varible.



[MSH format]: https://gmsh.info/doc/texinfo/gmsh.html#MSH-file-format
[Mesh format]: #Mesh-format
[Nodes]: #Nodes
