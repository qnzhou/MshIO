#pragma once

#include <MshIO/MshSpec.h>

#include <iostream>

namespace mshio {

void load_elements(std::istream& in, MshSpec& spec);

}
