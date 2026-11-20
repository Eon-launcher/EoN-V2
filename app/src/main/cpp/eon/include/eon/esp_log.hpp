// Modified by EoN Launcher V2.
#pragma once

#include <string>
#include <string_view>

namespace eon {

void logEspEvent(std::string_view level, std::string_view message);
std::string espLogSnapshot();

}  // namespace eon
