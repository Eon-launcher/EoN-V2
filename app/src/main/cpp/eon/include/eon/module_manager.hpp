#pragma once

#include "eon/module.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

namespace eon {

class ModuleManager {
public:
    static ModuleManager& instance();

    void registerModule(std::unique_ptr<Module> module);
    bool setEnabled(std::string_view name, bool enabled);
    void setModFriendlyServer(bool enabled);
    bool modFriendlyServer() const;
    std::size_t moduleCount() const noexcept;
    const std::vector<std::unique_ptr<Module>>& modules() const noexcept;
    void onTick();
    void onRender();
    void resetSession();

private:
    ModuleManager() = default;

    mutable std::mutex stateMutex_;
    bool modFriendlyServer_{false};
    std::vector<std::unique_ptr<Module>> modules_;
};

}  // namespace eon
