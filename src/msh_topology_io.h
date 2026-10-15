#pragma once

#include <iosfwd>

namespace mshio {
struct MshSpec;

void load_partitioned_entities(std::istream& in, MshSpec& spec);
void load_periodic(std::istream& in, MshSpec& spec);
void load_ghost_elements(std::istream& in, MshSpec& spec);
void save_partitioned_entities(std::ostream& out, const MshSpec& spec);
void save_periodic(std::ostream& out, const MshSpec& spec);
void save_ghost_elements(std::ostream& out, const MshSpec& spec);
void validate_topology_sections(const MshSpec& spec);

} // namespace mshio
