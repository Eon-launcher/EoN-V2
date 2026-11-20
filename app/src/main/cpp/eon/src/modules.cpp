// Modified by EoN Launcher V2.
#include "eon/modules.hpp"

namespace eon {

MobAuraModule::MobAuraModule() : Module("MobAura", Category::Combat) {
    addSetting({"range", 4});
    addSetting({"cps", 8});
}
void MobAuraModule::onTick() {
    // TODO: Implement only after supported game APIs and signatures are verified.
}
void MobAuraModule::onRender() {}

ScaffoldModule::ScaffoldModule() : Module("Scaffold", Category::Movement) {
    addSetting({"delay", 2});
}
void ScaffoldModule::onTick() {
    // TODO: Implement only after supported game APIs and signatures are verified.
}
void ScaffoldModule::onRender() {}

EspModule::EspModule() : Module("ESP", Category::Render) {
    addSetting({"range", 32.0f});
}
void EspModule::onTick() {}
void EspModule::onRender() {
    // TODO: Implement only after supported game APIs and signatures are verified.
}

XrayModule::XrayModule() : Module("Xray", Category::Render) {
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
