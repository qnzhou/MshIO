#pragma once

#include <mshio/MshSpec.h>

#include <ostream>

namespace mshio {

void save_patches(std::ostream& out, const MshSpec& spec);

}
