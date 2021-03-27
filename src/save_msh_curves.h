#pragma once

#include <mshio/MshSpec.h>

#include <ostream>

namespace mshio {

void save_curves(std::ostream& out, const MshSpec& spec);

}
