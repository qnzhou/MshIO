# MshIO

MshIO is a tiny library written with modern C++.  It is created by Qingnan Zhou
as a coding exercise.  It supports reading and writing [MSH format] with both
ASCII and binary encodings for both version 2.2 and version 4.1.

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
| [Nodes] | 3D coordinates of nodes and (optionally) their parameterization coordinates. |
| [Elements] | A list of elements grouped by blocks. |
| [Node data] | Scalar/vector/tensor fields defined on nodes. |
| [Element data] | Scalar/vector/tensor fields defined on elements. |
| [Element-node data] | Scalar/vector/tensor fields defined over each node of each element. |

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

Mesh format section is the header of MSH file.  It contains information about
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
positive.  Ideally, tags should be ordered consecutively from 1 to N (the number
of nodes), but it does not have to be so.  It is up to the client application to
maintain a mapping from tag to nodes.

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
auto& block = nodes.entity_blocks[k];
block.entity_dim = 3;          // The dimension of the entity.
block.entity_tag = 1;          // The entity these nodes belongs to.
block.parametric = 0;          // 0: non-parametric, 1: parametric.
block.num_nodes_in_block = 3;  // The number of nodes in block.
block.tags = ...;              // A std::vector of unique, positive node tags.
block.data = ...;              // A std::vector of coordinates (x,y,z,<u>,<v>,<w>,...)
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
elements.entity_blocks = ...;    // A std::vector of element blocks.
```

#### Element block

An element block is a set of elements of the same type:

```c++
auto& block = elements.entity_blocks[k];
block.entity_dim = 2;             // The dimension of the elements.
block.entity_tag = 1;             // The entity these elements belongs to.
block.element_type = 2;           // See element type table below.
block.num_elements_in_block = 12; // The number of elements in this block.
block.data = ...;                 // See more detail below.
```

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

The element data is a flattened `std::vector` of `size_t`.  It follows the
convention below:

```
element_tag node_tag ... node_tag
element_tag node_tag ... node_tag
...
```

i.e. Each element entry consists of an element tag followed by `n` node tags,
where `n` is the number of nodes corresponding determined by the element type.
See the [supported element types](#Supported-element-types) table.

### Post-processing data

One of main advantage of MSH format is its support for storing post-processing
data along with the mesh.  There are 3 types of post-processing data: node data,
element data and element-node data.  Each type of post-processing data consists
of a header and a `std::vector` of entires.

```c++
auto& node_data = spec.node_data[k];
auto& element_data = spec.element_data[k];
auto& element_node_data = spec.element_node_data[k];
```

#### Data header

Data header consists of arrays of `string`, `double` and `int` tags.  Some tags
has pre-defined meaning:

```c++
auto& header = node_data.header;
header.string_tags = {...};  // [field_name, <interpolaiton_scheme>, ...]
header.real_tags = {...};    // [<time value>, ...]
header.int_tags = {...};     // [time step, num fields, num entities, <partition id>, ...]
```

Required special tags:
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
auto& entry = data_entires[k];

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

The following types are supported by MshIO:

| Type ID | Name | Dim | # nodes |
|:--:|----------------------------------|---|----|
| 1  | 2-node line                      | 1 | 2  |
| 2  | 3-node triangle                  | 2 | 3  |
| 3  | 4-node quad                      | 2 | 4  |
| 4  | 4-node tetrahedron               | 3 | 4  |
| 5  | 8-node hexahedron                | 3 | 8  |
| 6  | 6-node prism                     | 3 | 6  |
| 7  | 5-node pyramid                   | 3 | 5  |
| 8  | 3-node second order line         | 1 | 3  |
| 9  | 6-node second order triangle     | 2 | 6  |
| 10 | 9-node second order quad         | 2 | 9  |
| 11 | 10-node second order tetrahedron | 3 | 10 |
| 12 | 27-node second order hexahedron  | 3 | 27 |
| 13 | 18-node second order prism       | 3 | 18 |
| 14 | 14-node second order pyramid     | 3 | 14 |


[MSH format]: https://gmsh.info/doc/texinfo/gmsh.html#MSH-file-format
[Mesh format]: #Mesh-format
[Nodes]: #Nodes
[Elements]: #Elements
[Node data]: #Post-processing-data
[Element data]: #Post-processing-data
[Element-node data]: #Post-processing-data
