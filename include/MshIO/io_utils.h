#pragma once

#include <fstream>

namespace MshIO {

void eat_white_space(std::istream& in) {
    char ch = static_cast<char>(in.peek());
    while (ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t') {
        in.get();
        ch = static_cast<char>(in.peek());
    }
}

}

