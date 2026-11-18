#include "eon/module_manager.hpp"

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
    if (enabled && !modFriendlyServer_) {
        return false;
    }

    const auto module = std::find_if(modules_.begin(), modules_.end(),
                                     [name](const auto& candidate) {
                                         return candidate->name() == name;
                                     });
    if (module == modules_.end()) {
        return false;
    }
    (*module)->setEnabled(enabled);
    return true;
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
