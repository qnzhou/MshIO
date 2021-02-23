#pragma once
#include <MshIO/MshSpec.h>
#include <iostream>

namespace mshio {

void load_entities(std::istream& in, MshSpec& spec);

}
