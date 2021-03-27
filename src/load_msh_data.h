#pragma once

#include <mshio/MshSpec.h>
#include <iostream>

namespace mshio {

void load_node_data(std::istream& in, MshSpec& spec);

void load_element_data(std::istream& in, MshSpec& spec);

void load_element_node_data(std::istream& in, MshSpec& spec);

} // namespace mshio
