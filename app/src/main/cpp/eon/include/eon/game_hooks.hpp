// Modified by EoN Launcher V2.
#pragma once

#include "eon/esp_projection.hpp"

#include <string>
#include <vector>

namespace eon {

struct EspWorldFrame {
    esp::Camera camera;
    std::vector<esp::Bounds> entities;
};

bool installGameHooks(std::string& unavailableReason);
void uninstallGameHooks() noexcept;
bool readEspWorldFrame(EspWorldFrame& frame, std::string& unavailableReason);

}  // namespace eon
