// Modified by EoN Launcher V2.
#pragma once

namespace eon {

struct EonConfig {
    int version{1};
    bool mod_friendly_server{false};
    int mob_aura_range{4};
    int mob_aura_cps{8};
    int scaffold_delay{2};
    float esp_range{32.0f};
    int xray_opacity{60};
    float fly_speed{1.0f};
    float speed_multiplier{1.2f};
    float high_jump_height{1.5f};
};

}  // namespace eon
