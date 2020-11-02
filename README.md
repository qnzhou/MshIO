# MshIO

MshIO is a header-only library written with modern C++.  It is created by
Qingnan Zhou as a coding excercise.  It supports reading and writing [MSH
format] with both ASCII and binary encodings.

## Usage

```c++
#include <MshIO/mshio.h>

MshIO::MshSpec spec = MshIO::load_msh("input.msh");

MshIO::save_msh("output.msh", spec);

```

## `MshSpec` data structure

TODO.


[MSH format]: https://gmsh.info/doc/texinfo/gmsh.html#MSH-file-format
