// Modified by EoN Launcher V2.
#pragma once

#include "eon/module.hpp"

namespace eon {

class MobAuraModule final : public Module {
public:
    MobAuraModule();
    void onTick() override;
    void onRender() override;
};

class ScaffoldModule final : public Module {
public:
    ScaffoldModule();
    void onTick() override;
    void onRender() override;
};

class EspModule final : public Module {
public:
    EspModule();
    void onTick() override;
    void onRender() override;
};

class XrayModule final : public Module {
public:
    XrayModule();
    void onTick() override;
    void onRender() override;
};

class FlyModule final : public Module {
public:
    FlyModule();
    void onTick() override;
    void onRender() override;
};

class SpeedModule final : public Module {
public:
    SpeedModule();
    void onTick() override;
    void onRender() override;
};

class HighJumpModule final : public Module {
public:
    HighJumpModule();
    void onTick() override;
    void onRender() override;
};

}  // namespace eon
