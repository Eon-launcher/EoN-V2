#include "eon/modules.hpp"

namespace eon {

MobAuraModule::MobAuraModule() : Module("MobAura", Category::Combat) {}
void MobAuraModule::onTick() {
    // TODO: Implement only after supported game APIs and signatures are verified.
}
void MobAuraModule::onRender() {}

ScaffoldModule::ScaffoldModule() : Module("Scaffold", Category::World) {}
void ScaffoldModule::onTick() {
    // TODO: Implement only after supported game APIs and signatures are verified.
}
void ScaffoldModule::onRender() {}

EspModule::EspModule() : Module("ESP", Category::Render) {}
void EspModule::onTick() {}
void EspModule::onRender() {
    // TODO: Implement only after supported game APIs and signatures are verified.
}

XrayModule::XrayModule() : Module("Xray", Category::Render) {}
void XrayModule::onTick() {}
void XrayModule::onRender() {
    // TODO: Implement only after supported game APIs and signatures are verified.
}

FlyModule::FlyModule() : Module("Fly", Category::Movement) {}
void FlyModule::onTick() {
    // TODO: Implement only after supported game APIs and signatures are verified.
}
void FlyModule::onRender() {}

SpeedModule::SpeedModule() : Module("Speed", Category::Movement) {}
void SpeedModule::onTick() {
    // TODO: Implement only after supported game APIs and signatures are verified.
}
void SpeedModule::onRender() {}

HighJumpModule::HighJumpModule() : Module("HighJump", Category::Movement) {}
void HighJumpModule::onTick() {
    // TODO: Implement only after supported game APIs and signatures are verified.
}
void HighJumpModule::onRender() {}

}  // namespace eon
