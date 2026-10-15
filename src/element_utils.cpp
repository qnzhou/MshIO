#include "element_utils.h"

namespace mshio {
namespace {

struct ElementInfo
{
    std::uint16_t nodes;
    std::int8_t dim;
};

// Released Gmsh 4.15.2: MElement::getInfoMSH and ElementType::getDimension.
// Zero nodes marks an undefined type or one without a fixed-width MSH record.
// The MSH registry is intentionally broader than Gmsh's element factory: some
// order-zero and incomplete types have wire definitions but no factory branch.
constexpr ElementInfo element_info_table[] = {
    {0, 0}, // 0: undefined
    {2, 1}, // 1: line
    {3, 2}, // 2: triangle
    {4, 2}, // 3: quadrangle
    {4, 3}, // 4: tetrahedron
    {8, 3}, // 5: hexahedron
    {6, 3}, // 6: prism
    {5, 3}, // 7: pyramid
    {3, 1}, // 8: line
    {6, 2}, // 9: triangle
    {9, 2}, // 10: quadrangle
    {10, 3}, // 11: tetrahedron
    {27, 3}, // 12: hexahedron
    {18, 3}, // 13: prism
    {14, 3}, // 14: pyramid
    {1, 0}, // 15: point
    {8, 2}, // 16: quadrangle
    {20, 3}, // 17: hexahedron
    {15, 3}, // 18: prism
    {13, 3}, // 19: pyramid
    {9, 2}, // 20: incomplete triangle
    {10, 2}, // 21: triangle
    {12, 2}, // 22: incomplete triangle
    {15, 2}, // 23: triangle
    {15, 2}, // 24: incomplete triangle
    {21, 2}, // 25: triangle
    {4, 1}, // 26: line
    {5, 1}, // 27: line
    {6, 1}, // 28: line
    {20, 3}, // 29: tetrahedron
    {35, 3}, // 30: tetrahedron
    {56, 3}, // 31: tetrahedron
    {22, 3}, // 32: incomplete tetrahedron
    {28, 3}, // 33: incomplete tetrahedron
    {0, 0}, // 34: variable-width polygon
    {0, 0}, // 35: variable-width polyhedron
    {16, 2}, // 36: quadrangle
    {25, 2}, // 37: quadrangle
    {36, 2}, // 38: quadrangle
    {12, 2}, // 39: incomplete quadrangle
    {16, 2}, // 40: incomplete quadrangle
    {20, 2}, // 41: incomplete quadrangle
    {28, 2}, // 42: triangle
    {36, 2}, // 43: triangle
    {45, 2}, // 44: triangle
    {55, 2}, // 45: triangle
    {66, 2}, // 46: triangle
    {49, 2}, // 47: quadrangle
    {64, 2}, // 48: quadrangle
    {81, 2}, // 49: quadrangle
    {100, 2}, // 50: quadrangle
    {121, 2}, // 51: quadrangle
    {18, 2}, // 52: incomplete triangle
    {21, 2}, // 53: incomplete triangle
    {24, 2}, // 54: incomplete triangle
    {27, 2}, // 55: incomplete triangle
    {30, 2}, // 56: incomplete triangle
    {24, 2}, // 57: incomplete quadrangle
    {28, 2}, // 58: incomplete quadrangle
    {32, 2}, // 59: incomplete quadrangle
    {36, 2}, // 60: incomplete quadrangle
    {40, 2}, // 61: incomplete quadrangle
    {7, 1}, // 62: line
    {8, 1}, // 63: line
    {9, 1}, // 64: line
    {10, 1}, // 65: line
    {11, 1}, // 66: line
    {2, 1}, // 67: border line
    {3, 2}, // 68: border triangle
    {0, 0}, // 69: variable-width border polygon
    {2, 1}, // 70: child line
    {84, 3}, // 71: tetrahedron
    {120, 3}, // 72: tetrahedron
    {165, 3}, // 73: tetrahedron
    {220, 3}, // 74: tetrahedron
    {286, 3}, // 75: tetrahedron
    {0, 0}, // 76: undefined
    {0, 0}, // 77: undefined
    {0, 0}, // 78: undefined
    {34, 3}, // 79: incomplete tetrahedron
    {40, 3}, // 80: incomplete tetrahedron
    {46, 3}, // 81: incomplete tetrahedron
    {52, 3}, // 82: incomplete tetrahedron
    {58, 3}, // 83: incomplete tetrahedron
    {1, 1}, // 84: order-zero line
    {1, 2}, // 85: order-zero triangle
    {1, 2}, // 86: order-zero quadrangle
    {1, 3}, // 87: order-zero tetrahedron
    {1, 3}, // 88: order-zero hexahedron
    {1, 3}, // 89: order-zero prism
    {40, 3}, // 90: prism
    {75, 3}, // 91: prism
    {64, 3}, // 92: hexahedron
    {125, 3}, // 93: hexahedron
    {216, 3}, // 94: hexahedron
    {343, 3}, // 95: hexahedron
    {512, 3}, // 96: hexahedron
    {729, 3}, // 97: hexahedron
    {1000, 3}, // 98: hexahedron
    {32, 3}, // 99: incomplete hexahedron
    {44, 3}, // 100: incomplete hexahedron
    {56, 3}, // 101: incomplete hexahedron
    {68, 3}, // 102: incomplete hexahedron
    {80, 3}, // 103: incomplete hexahedron
    {92, 3}, // 104: incomplete hexahedron
    {104, 3}, // 105: incomplete hexahedron
    {126, 3}, // 106: prism
    {196, 3}, // 107: prism
    {288, 3}, // 108: prism
    {405, 3}, // 109: prism
    {550, 3}, // 110: prism
    {24, 3}, // 111: incomplete prism
    {33, 3}, // 112: incomplete prism
    {42, 3}, // 113: incomplete prism
    {51, 3}, // 114: incomplete prism
    {60, 3}, // 115: incomplete prism
    {69, 3}, // 116: incomplete prism
    {78, 3}, // 117: incomplete prism
    {30, 3}, // 118: pyramid
    {55, 3}, // 119: pyramid
    {91, 3}, // 120: pyramid
    {140, 3}, // 121: pyramid
    {204, 3}, // 122: pyramid
    {285, 3}, // 123: pyramid
    {385, 3}, // 124: pyramid
    {21, 3}, // 125: incomplete pyramid
    {29, 3}, // 126: incomplete pyramid
    {37, 3}, // 127: incomplete pyramid
    {45, 3}, // 128: incomplete pyramid
    {53, 3}, // 129: incomplete pyramid
    {61, 3}, // 130: incomplete pyramid
    {69, 3}, // 131: incomplete pyramid
    {1, 3}, // 132: order-zero pyramid
    {1, 0}, // 133: XFEM point
    {2, 1}, // 134: XFEM line
    {3, 2}, // 135: XFEM triangle
    {4, 3}, // 136: XFEM tetrahedron
    {16, 3}, // 137: incomplete tetrahedron
    {0, 0}, // 138: MINI triangle (no MSH node-count definition)
    {0, 0}, // 139: MINI tetrahedron (no MSH node-count definition)
    {4, 3} // 140: trihedron
};

const ElementInfo& element_info(std::int32_t element_type)
{
    if (element_type <= 0 ||
        static_cast<size_t>(element_type) >= sizeof(element_info_table) / sizeof(ElementInfo) ||
        element_info_table[element_type].nodes == 0) {
        std::stringstream msg;
        msg << "Unsupported element type: " << element_type;
        throw UnsupportedFeature(msg.str());
    }
    return element_info_table[element_type];
}

} // namespace

void assert_element_is_supported(std::int32_t element_type)
{
    (void)element_info(element_type);
}

size_t nodes_per_element(std::int32_t element_type)
{
    return element_info(element_type).nodes;
}

std::int32_t get_element_dim(std::int32_t element_type)
{
    return element_info(element_type).dim;
}

} // namespace mshio
