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

`MshSpec` is a data structure that maps almost verbatim to the information stored in a MSH file.  Currently, the following sections are supported:

| Section name | Description |
|----:|---|
| Mesh format | Format header. |
| Nodes | 3D coordinates of nodes and (optionally) their parameterization coordiantes. |
| Elements | A list of elements groupped by blocks. |
| Node data | Scalar/vector/tensor fields defined on nodes. |
| Element data | Scalar/vector/tensor fields defined on elements. |
| Element-node data | Scalar/vector/tensor fields defined over each node of each element. |

The follow sections are supported by MSH format, but not yet supported by MshIO (contribution welcomed):
* Physical names
* Entities
* Partitioned entities
* Periodic
* Ghost elements
* Parametrizations
* Interpolation scheme

All fields will be populated by `MshIO::load_msh()` method, and all fields should be set up correctly before calling `MshIO::save_msh()` method.
The helper method `MshIO::validate_spec(spec)` can be used to check if a given `spec` is valid.

### Mesh format
Mesh format section is the header of MSH file.  It constains information about the MSH version used, whether the file is binary and data size.

```c++
auto& format = spec.mesh_format;
format.version = "4.1";            // Only version "2.2" and "4.1" are supported.
format.file_type = 1;              // 0: ASCII, 1: binary.
format.data_size = sizeof(size_t); // Size of data, defined as sizeof(size_t) = 8.
```



[MSH format]: https://gmsh.info/doc/texinfo/gmsh.html#MSH-file-format
