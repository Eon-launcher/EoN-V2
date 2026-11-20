// Modified by EoN Launcher V2.
#include "eon/module.hpp"

#include <algorithm>
#include <utility>

namespace eon {

Module::Module(std::string name, Category category,
               std::initializer_list<FunctionSlot> requiredSlots,
               bool implementationReady)
    : name_(std::move(name)),
      category_(category),
      requiredSlots_(requiredSlots),
      implementationReady_(implementationReady) {}

const std::string& Module::name() const noexcept {
    return name_;
}

Category Module::category() const noexcept {
    return category_;
}

bool Module::enabled() const noexcept {
    return enabled_.load(std::memory_order_relaxed);
}

const std::vector<Setting>& Module::settings() const noexcept {
    return settings_;
}

std::span<const FunctionSlot> Module::requiredSlots() const noexcept {
    return requiredSlots_;
}

bool Module::implementationReady() const noexcept {
    return implementationReady_.load(std::memory_order_relaxed);
}

void Module::setEnabled(bool enabled) noexcept {
    enabled_.store(enabled, std::memory_order_relaxed);
}

void Module::setImplementationReady(bool ready) noexcept {
    implementationReady_.store(ready, std::memory_order_relaxed);
}

bool Module::setSetting(std::string_view name, SettingValue value) {
    const auto setting = std::find_if(settings_.begin(), settings_.end(),
                                      [name](const Setting& candidate) {
                                          return candidate.name == name;
                                      });
    if (setting == settings_.end()) {
        return false;
    }
    setting->value = std::move(value);
    return true;
}

void Module::addSetting(Setting setting) {
    settings_.push_back(std::move(setting));
}

void Module::onTick() {}

void Module::onRender() {}

}  // namespace eon
