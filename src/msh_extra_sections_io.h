#pragma once

#include <iosfwd>

namespace mshio {
struct MshSpec;

void load_parametrizations(std::istream& in, MshSpec& spec);
void load_interpolation_scheme(std::istream& in, MshSpec& spec);
void save_parametrizations(std::ostream& out, const MshSpec& spec);
void save_interpolation_schemes(std::ostream& out, const MshSpec& spec);
void validate_extra_sections(const MshSpec& spec);

} // namespace mshio
