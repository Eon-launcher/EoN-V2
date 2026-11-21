// Modified by EoN Launcher V2.
#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace eon {

void logEspEvent(std::string_view level, std::string_view message);
std::string espLogSnapshot();
bool configureEspLogFile(const std::filesystem::path& path);

}  // namespace eon
