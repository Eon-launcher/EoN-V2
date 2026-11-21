// Modified by EoN Launcher V2.
#include "eon/modules.hpp"
#include "eon/esp_log.hpp"
#include "eon/game_hooks.hpp"

#include <pl/ModMenu.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace eon {

MobAuraModule::MobAuraModule()
    : Module("MobAura", Category::Combat, {}, false) {
    addSetting({"range", 4});
    addSetting({"cps", 8});
}
void MobAuraModule::onTick() {
    // TODO: Implement only after supported game APIs and signatures are verified.
}
void MobAuraModule::onRender() {}

ScaffoldModule::ScaffoldModule()
    : Module("Scaffold", Category::Movement, {}, false) {
    addSetting({"delay", 2});
}
void ScaffoldModule::onTick() {
    // TODO: Implement only after supported game APIs and signatures are verified.
}
void ScaffoldModule::onRender() {}

EspModule::EspModule() : Module("ESP", Category::Render, {}, false) {
    addSetting({"range", 32.0f});
}
void EspModule::onTick() {
    static std::string lastFrameStatus;

    float range = 32.0f;
    for (const auto& setting : settings()) {
        if (setting.name == "range") {
            if (const auto* value = std::get_if<float>(&setting.value)) {
                range = *value;
            }
            break;
        }
    }

    std::vector<pl::modmenu::DrawCommand> commands;
    const auto surface = pl::modmenu::getHudSurfaceSize();
    EspWorldFrame frame;
    if (surface.width <= 0.0f || surface.height <= 0.0f) {
        constexpr std::string_view status = "HUD drawing surface is not ready";
        if (lastFrameStatus != status) {
            logEspEvent("WARN", status);
            lastFrameStatus = status;
        }
    } else {
        std::string unavailableReason;
        if (!readEspWorldFrame(frame, unavailableReason)) {
            if (lastFrameStatus != unavailableReason) {
                logEspEvent("WARN", unavailableReason);
                lastFrameStatus = unavailableReason;
            }
        } else {
            if (lastFrameStatus != "ready") {
                logEspEvent("INFO", "ESP frame acquisition active; " +
                                            std::to_string(frame.entities.size()) +
                                            " actor bounds acquired.");
                lastFrameStatus = "ready";
            }
            constexpr std::array<std::array<std::size_t, 2>, 12> edges{{
                    {{0, 1}}, {{1, 3}}, {{3, 2}}, {{2, 0}},
                    {{4, 5}}, {{5, 7}}, {{7, 6}}, {{6, 4}},
                    {{0, 4}}, {{1, 5}}, {{2, 6}}, {{3, 7}},
            }};
            constexpr std::size_t maxHudDrawCommands = 4096;
            commands.reserve(std::min(
                    frame.entities.size(),
                    maxHudDrawCommands / edges.size()) * edges.size());
            std::size_t projectedBoxes = 0;
            for (const auto& bounds : frame.entities) {
                const auto projected = esp::projectBounds(
                        bounds, frame.camera, surface.width, surface.height,
                        range);
                if (!projected) {
                    continue;
                }
                if (commands.size() + edges.size() > maxHudDrawCommands) {
                    break;
                }
                ++projectedBoxes;

                for (const auto& edge : edges) {
                    const auto& start = projected->corners[edge[0]];
                    const auto& end = projected->corners[edge[1]];
                    pl::modmenu::DrawCommand line;
                    line.type = pl::modmenu::DrawCommandType::Line;
                    line.x = start.x;
                    line.y = start.y;
                    line.w = end.x - start.x;
                    line.h = end.y - start.y;
                    line.color = 0xFFFFFFFF;
                    line.size = 2.0f;
                    commands.push_back(std::move(line));
                }
            }
            static int lastDrawState = -1;
            const int drawState = projectedBoxes == 0 ? 0 : 1;
            if (lastDrawState != drawState) {
                if (drawState == 0) {
                    logEspEvent(
                            "WARN",
                            frame.entities.empty()
                                    ? "No entity bounds are available to draw."
                                    : "Actor bounds are available, but none pass "
                                      "ESP projection or range checks.");
                } else {
                    logEspEvent("INFO", "ESP is drawing " +
                                                std::to_string(projectedBoxes) +
                                                " entity boxes.");
                }
                lastDrawState = drawState;
            }
        }
    }
    pl::modmenu::submitDrawCommands("eon.render.esp", commands);
}
void EspModule::onRender() {
}

XrayModule::XrayModule() : Module("Xray", Category::Render, {}, false) {
    addSetting({"opacity", 60});
}
void XrayModule::onTick() {}
void XrayModule::onRender() {
    // TODO: Implement only after supported game APIs and signatures are verified.
}

FlyModule::FlyModule()
    : Module("Fly", Category::Movement,
             {FunctionSlot::LocalPlayer_normalTick,
              FunctionSlot::Player_getAbilities,
              FunctionSlot::LayeredAbilities_getLayer,
              FunctionSlot::Abilities_setAbility},
             false) {
    addSetting({"speed", 1.0f});
}
void FlyModule::onTick() {
    // Fail closed until the 1.26.60.4 function ABIs are verified.
}
void FlyModule::onRender() {}

SpeedModule::SpeedModule()
    : Module("Speed", Category::Movement,
             {FunctionSlot::SpeedMoveSystem},
             false) {
    addSetting({"multiplier", 1.2f});
}
void SpeedModule::onTick() {
    // Fail closed until the 1.26.60.4 function ABI is verified.
}
void SpeedModule::onRender() {}

HighJumpModule::HighJumpModule()
    : Module("HighJump", Category::Movement,
             {FunctionSlot::JumpHeight},
             false) {
    addSetting({"height", 1.5f});
}
void HighJumpModule::onTick() {
    // Fail closed until the 1.26.60.4 function ABI is verified.
}
void HighJumpModule::onRender() {}

}  // namespace eon
