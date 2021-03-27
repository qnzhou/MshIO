#pragma once

#include <mshio/MshSpec.h>
#include <iostream>

namespace mshio {

void save_node_data(std::ostream& out, const MshSpec& spec);

void save_element_data(std::ostream& out, const MshSpec& spec);

void save_element_node_data(std::ostream& out, const MshSpec& spec);

} // namespace mshio
