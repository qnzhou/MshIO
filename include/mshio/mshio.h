#pragma once

#include <iostream>
#include <string>

#include <mshio/MshSpec.h>

namespace mshio {

MshSpec load_msh(std::istream& in);
MshSpec load_msh(const std::string& filename);

void save_msh(std::ostream& out, const MshSpec& spec);
void save_msh(const std::string& filename, const MshSpec& spec);

void validate_spec(const MshSpec& spec);

size_t nodes_per_element(std::int32_t element_type);
std::int32_t get_element_dim(std::int32_t element_type);

} // namespace mshio
