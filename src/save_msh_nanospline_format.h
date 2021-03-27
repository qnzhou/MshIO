#pragma once

#include <mshio/MshSpec.h>

#include <ostream>

namespace mshio
{

inline void save_nanospline_format(std::ostream& out, const MshSpec& spec)
{
    out << "$NanoSplineFormat" << std::endl;
    out << spec.nanospline_format.version << std::endl;
    out << "$EndNanoSplineFormat" << std::endl;
}

}
