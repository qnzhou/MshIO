#pragma once
#include <MshIO/MshSpec.h>
#include <ostream>

namespace mshio {

void save_physical_groups(std::ostream& out, const MshSpec& spec);

}
