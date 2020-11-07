#pragma once

#include <MshIO/MshSpec.h>

#include <iostream>

namespace mshio {

void load_nodes(std::istream& in, MshSpec& spec);

}
