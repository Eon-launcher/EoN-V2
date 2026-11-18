#pragma once

#include <atomic>
#include <string>
#include <variant>
#include <vector>

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
    Module(std::string name, Category category);
    virtual ~Module() = default;

    const std::string& name() const noexcept;
    Category category() const noexcept;
    bool enabled() const noexcept;
    const std::vector<Setting>& settings() const noexcept;

    virtual void onTick();
    virtual void onRender();

protected:
    void addSetting(Setting setting);

private:
    friend class ModuleManager;

    void setEnabled(bool enabled) noexcept;

    std::string name_;
    Category category_;
    std::atomic_bool enabled_{false};
    std::vector<Setting> settings_;
};

}  // namespace eon
