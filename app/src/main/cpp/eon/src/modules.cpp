// Modified by EoN Launcher V2.
#include "eon/modules.hpp"
#include "eon/game_hooks.hpp"

#include <pl/ModMenu.hpp>

#include <string>
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
    if (surface.width > 0.0f && surface.height > 0.0f &&
        readEspWorldFrame(frame)) {
        commands.reserve(frame.entities.size() * 2);
        for (const auto& bounds : frame.entities) {
            const auto projected = esp::projectBounds(
                    bounds, frame.camera, surface.width, surface.height, range);
            if (!projected) {
                continue;
            }

            pl::modmenu::DrawCommand box;
            box.type = pl::modmenu::DrawCommandType::Rect;
            box.x = projected->left;
            box.y = projected->top;
            box.w = projected->right - projected->left;
            box.h = projected->bottom - projected->top;
            box.color = 0xFF40FF80;
            box.size = 2.0f;
            commands.push_back(std::move(box));

            pl::modmenu::DrawCommand distance;
            distance.type = pl::modmenu::DrawCommandType::Text;
            distance.x = projected->left;
            distance.y = projected->top - 16.0f;
            distance.h = 14.0f;
            distance.size = 14.0f;
            distance.color = 0xFFFFFFFF;
            distance.text =
                    std::to_string(static_cast<int>(projected->distance)) +
                    "m";
            commands.push_back(std::move(distance));
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
