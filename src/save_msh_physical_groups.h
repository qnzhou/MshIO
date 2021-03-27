#pragma once
#include <mshio/MshSpec.h>
#include <ostream>

namespace mshio {

void save_physical_groups(std::ostream& out, const MshSpec& spec);

}
