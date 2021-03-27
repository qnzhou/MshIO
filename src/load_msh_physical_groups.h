#pragma once
#include <mshio/MshSpec.h>
#include <iostream>

namespace mshio {

void load_physical_groups(std::istream& in, MshSpec& spec);

} // namespace mshio
