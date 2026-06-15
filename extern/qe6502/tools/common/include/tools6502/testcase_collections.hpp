#pragma once

#include <tools6502/common.hpp>

#include <cstdint>
#include <map>
#include <vector>

namespace tools6502 {

std::map<std::uint8_t, std::vector<testcase>> get_nmos6502_opcode_testcases();

} // namespace tools6502
