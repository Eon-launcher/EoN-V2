// Modified by EoN Launcher V2.
#pragma once

#include <atomic>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "eon/game_profile.hpp"

namespace eon {

enum class Category {
    Combat,
    Movement,
    Render,
    World
};

using SettingValue = std::variant<bool, int, float, std::string>;

struct Setting {
    std::string name;
    SettingValue value;
};

class ModuleManager;

class Module {
public:
    Module(std::string name, Category category,
           std::initializer_list<FunctionSlot> requiredSlots = {},
           bool implementationReady = true);
    virtual ~Module() = default;

    const std::string& name() const noexcept;
    Category category() const noexcept;
    bool enabled() const noexcept;
    const std::vector<Setting>& settings() const noexcept;
    std::span<const FunctionSlot> requiredSlots() const noexcept;
    bool implementationReady() const noexcept;

    virtual void onTick();
    virtual void onRender();

protected:
    void addSetting(Setting setting);

private:
    friend class ModuleManager;

    void setEnabled(bool enabled) noexcept;
    void setImplementationReady(bool ready) noexcept;
    bool setSetting(std::string_view name, SettingValue value);

    std::string name_;
    Category category_;
    std::atomic_bool enabled_{false};
    std::vector<Setting> settings_;
    std::vector<FunctionSlot> requiredSlots_;
    std::atomic_bool implementationReady_;
};

}  // namespace eon
