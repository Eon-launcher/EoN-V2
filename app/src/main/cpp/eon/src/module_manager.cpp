// Modified by EoN Launcher V2.
#include "eon/module_manager.hpp"
#include "eon/game_profile.hpp"

#include <algorithm>
#include <utility>

namespace eon {

ModuleManager& ModuleManager::instance() {
    static ModuleManager manager;
    return manager;
}

void ModuleManager::registerModule(std::unique_ptr<Module> module) {
    if (module) {
        modules_.push_back(std::move(module));
    }
}

bool ModuleManager::setEnabled(std::string_view name, bool enabled) {
    std::lock_guard lock(stateMutex_);
    const auto module = std::find_if(modules_.begin(), modules_.end(),
                                     [name](const auto& candidate) {
                                         return candidate->name() == name;
                                     });
    if (module == modules_.end()) {
        return false;
    }
    if (enabled &&
        (!modFriendlyServer_ ||
         !(*module)->implementationReady() ||
         (!(*module)->requiredSlots().empty() &&
          !Game::instance().ready((*module)->requiredSlots())))) {
        return false;
    }
    (*module)->setEnabled(enabled);
    return true;
}

bool ModuleManager::setSetting(std::string_view moduleName,
                               std::string_view settingName,
                               SettingValue value) {
    std::lock_guard lock(stateMutex_);
    const auto module = std::find_if(modules_.begin(), modules_.end(),
                                     [moduleName](const auto& candidate) {
                                         return candidate->name() == moduleName;
                                     });
    return module != modules_.end() &&
           (*module)->setSetting(settingName, std::move(value));
}

std::string ModuleManager::availabilityMessage(std::string_view name) const {
    std::lock_guard lock(stateMutex_);
    const auto module = std::find_if(modules_.begin(), modules_.end(),
                                     [name](const auto& candidate) {
                                         return candidate->name() == name;
                                     });
    if (module == modules_.end()) {
        return std::string(name) + " unavailable: module is not registered";
    }

    std::vector<std::string> reasons;
    if (!(*module)->implementationReady()) {
        reasons.emplace_back(
                "verified 1.26.60.4 ARM64 hook ABI is not implemented");
    }
    if (!(*module)->requiredSlots().empty() &&
        !Game::instance().ready((*module)->requiredSlots())) {
        const auto profileMessage = Game::instance().unavailableMessage(
                name, (*module)->requiredSlots());
        const auto separator = profileMessage.find(": ");
        reasons.emplace_back(
                separator == std::string::npos
                        ? profileMessage
                        : profileMessage.substr(separator + 2));
    }
    if (!modFriendlyServer_) {
        reasons.emplace_back("Mod-friendly server switch is off");
    }
    if (reasons.empty()) {
        return std::string(name) + " available";
    }

    std::string message(name);
    message += " unavailable: ";
    for (std::size_t index = 0; index < reasons.size(); ++index) {
        if (index != 0) {
            message += "; ";
        }
        message += reasons[index];
    }
    return message;
}

void ModuleManager::setModFriendlyServer(bool enabled) {
    std::lock_guard lock(stateMutex_);
    modFriendlyServer_ = enabled;
    if (!enabled) {
        for (const auto& module : modules_) {
            module->setEnabled(false);
        }
    }
}

bool ModuleManager::modFriendlyServer() const {
    std::lock_guard lock(stateMutex_);
    return modFriendlyServer_;
}

std::size_t ModuleManager::moduleCount() const noexcept {
    return modules_.size();
}

const std::vector<std::unique_ptr<Module>>& ModuleManager::modules() const noexcept {
    return modules_;
}

void ModuleManager::onTick() {
    for (const auto& module : modules_) {
        if (module->enabled()) {
            module->onTick();
        }
    }
}

void ModuleManager::onRender() {
    for (const auto& module : modules_) {
        if (module->enabled()) {
            module->onRender();
        }
    }
}

void ModuleManager::resetSession() {
    setModFriendlyServer(false);
}

}  // namespace eon
