// Modified by EoN Launcher V2.
#include "eon/config.hpp"
#include "eon/esp_log.hpp"
#include "eon/game_hooks.hpp"
#include "eon/game_profile.hpp"
#include "eon/module_manager.hpp"

#include <android/log.h>

#include <pl/Config.hpp>
#include <pl/Mod.hpp>
#include <pl/ModMenu.hpp>
#include <pl/ModMenuConfig.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>

extern "C" int eon_init();

namespace {

using pl::modmenu::ConfigControlTypeV2;
using pl::modmenu::ConfigNodeV2;

enum class SettingId {
    MobAuraRange,
    MobAuraCps,
    ScaffoldDelay,
    EspRange,
    XrayOpacity,
    FlySpeed,
    SpeedMultiplier,
    HighJumpHeight,
};

struct SettingDescriptor {
    SettingId id;
    std::string_view configKey;
    std::string_view settingKey;
    std::string_view title;
    std::string_view description;
    std::string_view defaultValue;
    std::string_view minValue;
    std::string_view maxValue;
    std::string_view step;
    std::string_view unit;
    ConfigControlTypeV2 type;
};

struct ModuleDescriptor {
    std::string_view id;
    std::string_view name;
    std::string_view description;
    std::string_view categoryId;
    std::string_view categoryTitle;
    const SettingDescriptor* settings;
    std::size_t settingCount;
    bool profileGated{false};
};

constexpr std::array kMobAuraSettings{
        SettingDescriptor{SettingId::MobAuraRange, "mob_aura_range", "range",
                          "Range", "Maximum target distance.", "4", "2", "8",
                          "1", "blocks", ConfigControlTypeV2::SliderInt},
        SettingDescriptor{SettingId::MobAuraCps, "mob_aura_cps", "cps",
                          "Clicks per second", "Configured attack rate.", "8",
                          "1", "20", "1", "CPS",
                          ConfigControlTypeV2::SliderInt},
};
constexpr std::array kScaffoldSettings{
        SettingDescriptor{SettingId::ScaffoldDelay, "scaffold_delay", "delay",
                          "Placement delay", "Configured delay between placements.",
                          "2", "0", "20", "1", "ticks",
                          ConfigControlTypeV2::SliderInt},
};
constexpr std::array kEspSettings{
        SettingDescriptor{SettingId::EspRange, "esp_range", "range", "Range",
                          "Maximum display distance.", "32", "8", "128", "1",
                          "blocks", ConfigControlTypeV2::SliderFloat},
};
constexpr std::array kXraySettings{
        SettingDescriptor{SettingId::XrayOpacity, "xray_opacity", "opacity",
                          "Opacity", "Configured overlay opacity.", "60", "10",
                          "100", "5", "%", ConfigControlTypeV2::SliderInt},
};
constexpr std::array kFlySettings{
        SettingDescriptor{SettingId::FlySpeed, "fly_speed", "speed", "Speed",
                          "Configured movement speed.", "1.0", "0.1", "5.0",
                          "0.1", "x", ConfigControlTypeV2::SliderFloat},
};
constexpr std::array kSpeedSettings{
        SettingDescriptor{SettingId::SpeedMultiplier, "speed_multiplier",
                          "multiplier", "Multiplier",
                          "Configured movement multiplier.", "1.2", "1.0",
                          "3.0", "0.1", "x",
                          ConfigControlTypeV2::SliderFloat},
};
constexpr std::array kHighJumpSettings{
        SettingDescriptor{SettingId::HighJumpHeight, "high_jump_height", "height",
                          "Height", "Configured jump height.", "1.5", "1.0",
                          "5.0", "0.1", "blocks",
                          ConfigControlTypeV2::SliderFloat},
};

constexpr std::array kModules{
        ModuleDescriptor{"eon.combat.mob_aura", "MobAura",
                         "Combat module settings.", "combat", "Combat",
                         kMobAuraSettings.data(), kMobAuraSettings.size(),
                         true},
        ModuleDescriptor{"eon.movement.scaffold", "Scaffold",
                         "Movement module settings.", "movement", "Movement",
                         kScaffoldSettings.data(), kScaffoldSettings.size(),
                         true},
        ModuleDescriptor{"eon.render.esp", "ESP",
                         "Player and mob ESP settings.",
                         "render", "Render", kEspSettings.data(),
                         kEspSettings.size(), true},
        ModuleDescriptor{"eon.render.xray", "Xray", "Render module settings.",
                         "render", "Render", kXraySettings.data(),
                         kXraySettings.size(), true},
        ModuleDescriptor{"eon.movement.fly", "Fly", "Movement module settings.",
                         "movement", "Movement", kFlySettings.data(),
                         kFlySettings.size(), true},
        ModuleDescriptor{"eon.movement.speed", "Speed",
                         "Movement module settings.", "movement", "Movement",
                         kSpeedSettings.data(), kSpeedSettings.size(), true},
        ModuleDescriptor{"eon.movement.high_jump", "HighJump",
                         "Movement module settings.", "movement", "Movement",
                         kHighJumpSettings.data(), kHighJumpSettings.size(),
                         true},
};

std::mutex gConfigMutex;
std::unique_ptr<pl::config::ConfigFile<eon::EonConfig>> gConfigFile;

void logError(std::string_view message) {
    __android_log_print(ANDROID_LOG_ERROR, "EON", "%.*s",
                        static_cast<int>(message.size()), message.data());
}

const ModuleDescriptor* findModule(std::string_view moduleId) {
    for (const auto& module : kModules) {
        if (module.id == moduleId) {
            return &module;
        }
    }
    return nullptr;
}

bool parseInt(std::string_view text, int& value) {
    const auto result =
            std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

bool parseFloat(std::string_view text, float& value) {
    const std::string owned(text);
    char* end = nullptr;
    const float parsed = std::strtof(owned.c_str(), &end);
    if (end != owned.c_str() + owned.size() || end == owned.c_str() ||
        !std::isfinite(parsed)) {
        return false;
    }
    value = parsed;
    return true;
}

std::string settingValue(const eon::EonConfig& config, SettingId id) {
    switch (id) {
        case SettingId::MobAuraRange:
            return std::to_string(config.mob_aura_range);
        case SettingId::MobAuraCps:
            return std::to_string(config.mob_aura_cps);
        case SettingId::ScaffoldDelay:
            return std::to_string(config.scaffold_delay);
        case SettingId::EspRange:
            return std::to_string(config.esp_range);
        case SettingId::XrayOpacity:
            return std::to_string(config.xray_opacity);
        case SettingId::FlySpeed:
            return std::to_string(config.fly_speed);
        case SettingId::SpeedMultiplier:
            return std::to_string(config.speed_multiplier);
        case SettingId::HighJumpHeight:
            return std::to_string(config.high_jump_height);
    }
    return {};
}

eon::SettingValue moduleSettingValue(const eon::EonConfig& config,
                                     SettingId id) {
    switch (id) {
        case SettingId::MobAuraRange:
            return config.mob_aura_range;
        case SettingId::MobAuraCps:
            return config.mob_aura_cps;
        case SettingId::ScaffoldDelay:
            return config.scaffold_delay;
        case SettingId::EspRange:
            return config.esp_range;
        case SettingId::XrayOpacity:
            return config.xray_opacity;
        case SettingId::FlySpeed:
            return config.fly_speed;
        case SettingId::SpeedMultiplier:
            return config.speed_multiplier;
        case SettingId::HighJumpHeight:
            return config.high_jump_height;
    }
    return 0;
}

bool updateConfigValue(eon::EonConfig& config,
                       const SettingDescriptor& setting,
                       std::string_view value) {
    if (setting.type == ConfigControlTypeV2::SliderInt) {
        int parsed = 0;
        int minimum = 0;
        int maximum = 0;
        if (!parseInt(value, parsed) || !parseInt(setting.minValue, minimum) ||
            !parseInt(setting.maxValue, maximum)) {
            return false;
        }
        parsed = std::clamp(parsed, minimum, maximum);
        switch (setting.id) {
            case SettingId::MobAuraRange:
                config.mob_aura_range = parsed;
                break;
            case SettingId::MobAuraCps:
                config.mob_aura_cps = parsed;
                break;
            case SettingId::ScaffoldDelay:
                config.scaffold_delay = parsed;
                break;
            case SettingId::XrayOpacity:
                config.xray_opacity = parsed;
                break;
            default:
                return false;
        }
        return true;
    }

    float parsed = 0.0f;
    float minimum = 0.0f;
    float maximum = 0.0f;
    if (!parseFloat(value, parsed) || !parseFloat(setting.minValue, minimum) ||
        !parseFloat(setting.maxValue, maximum)) {
        return false;
    }
    parsed = std::clamp(parsed, minimum, maximum);
    switch (setting.id) {
        case SettingId::EspRange:
            config.esp_range = parsed;
            break;
        case SettingId::FlySpeed:
            config.fly_speed = parsed;
            break;
        case SettingId::SpeedMultiplier:
            config.speed_multiplier = parsed;
            break;
        case SettingId::HighJumpHeight:
            config.high_jump_height = parsed;
            break;
        default:
            return false;
    }
    return true;
}

bool updateConfigValue(eon::EonConfig& config, std::string_view key,
                       std::string_view value) {
    if (key == "mod_friendly_server") {
        if (value == "true") {
            config.mod_friendly_server = true;
            return true;
        }
        if (value == "false") {
            config.mod_friendly_server = false;
            return true;
        }
        return false;
    }

    for (const auto& module : kModules) {
        for (std::size_t index = 0; index < module.settingCount; ++index) {
            const auto& setting = module.settings[index];
            if (setting.configKey == key) {
                return updateConfigValue(config, setting, value);
            }
        }
    }
    return false;
}

void applyModuleSettings(const eon::EonConfig& config) {
    auto& manager = eon::ModuleManager::instance();
    for (const auto& module : kModules) {
        for (std::size_t index = 0; index < module.settingCount; ++index) {
            const auto& setting = module.settings[index];
            if (!manager.setSetting(module.name, setting.settingKey,
                                    moduleSettingValue(config, setting.id))) {
                logError("Could not apply a registered EON module setting.");
            }
        }
    }
}

ConfigNodeV2 makeSafetyNote() {
    ConfigNodeV2 node;
    node.id = "permitted-use-note";
    node.category = "safety";
    node.title = "Permitted use only";
    node.description =
            "Use these features only in worlds and on servers whose rules "
            "explicitly permit them.";
    node.type = ConfigControlTypeV2::Info;
    return node;
}

ConfigNodeV2 makeSafeguardNode(const eon::EonConfig& config) {
    ConfigNodeV2 node;
    node.id = "mod_friendly_server";
    node.key = "mod_friendly_server";
    node.category = "safety";
    node.title = "Mod-friendly server";
    node.description =
            "Enable module toggles for this game session only.";
    node.type = ConfigControlTypeV2::Toggle;
    node.defaultValue = "false";
    node.currentValue = config.mod_friendly_server ? "true" : "false";
    node.hasCurrentValue = true;
    return node;
}

ConfigNodeV2 makeProfileStatusNode(const ModuleDescriptor& module) {
    ConfigNodeV2 node;
    node.id = "game-profile-status";
    node.category = std::string(module.categoryId);
    node.title = module.name == "ESP" ? "EON ESP status"
                                      : "Runtime availability";
    node.description = eon::ModuleManager::instance().availabilityMessage(
            module.name);
    if (module.name == "ESP") {
        node.description.insert(0, "EON ");
    }
    node.type = ConfigControlTypeV2::Info;
    return node;
}

ConfigNodeV2 makeSettingNode(const SettingDescriptor& setting,
                             std::string_view category,
                             const eon::EonConfig& config) {
    ConfigNodeV2 node;
    node.id = std::string(setting.configKey);
    node.key = std::string(setting.configKey);
    node.category = std::string(category);
    node.title = std::string(setting.title);
    node.description = std::string(setting.description);
    node.section = "Settings";
    node.type = setting.type;
    node.defaultValue = std::string(setting.defaultValue);
    node.currentValue = settingValue(config, setting.id);
    node.hasCurrentValue = true;
    node.minValue = std::string(setting.minValue);
    node.maxValue = std::string(setting.maxValue);
    node.step = std::string(setting.step);
    node.unit = std::string(setting.unit);
    return node;
}

bool publishSchema(const ModuleDescriptor& module,
                   const eon::EonConfig& config) {
    pl::modmenu::ConfigSchemaBuilder builder;
    builder.category("safety", "Safety")
            .category(std::string(module.categoryId),
                      std::string(module.categoryTitle))
            .defaultCategory("safety")
            .node(makeSafetyNote())
            .node(makeSafeguardNode(config));
    if (module.profileGated) {
        builder.node(makeProfileStatusNode(module));
    }
    for (std::size_t index = 0; index < module.settingCount; ++index) {
        builder.node(makeSettingNode(module.settings[index], module.categoryId,
                                     config));
    }

    if (!pl::modmenu::setConfigSchemaJson(module.id, builder.toJson())) {
        logError("The preloader rejected an EON module config schema.");
        return false;
    }
    return true;
}

void publishSchemas(const eon::EonConfig& config) {
    for (const auto& module : kModules) {
        publishSchema(module, config);
    }
}

void resetMenuModules() {
    for (const auto& module : kModules) {
        pl::modmenu::setModuleEnabled(module.id, false);
    }
}

void onModuleToggle(std::string_view moduleId, std::string_view moduleName,
                    bool enabled) {
    auto& manager = eon::ModuleManager::instance();
    if (moduleName == "ESP" && enabled && manager.modFriendlyServer()) {
        std::string hookUnavailableReason;
        const bool hooksReady = eon::installGameHooks(hookUnavailableReason);
        if (!manager.setImplementationReady("ESP", hooksReady)) {
            logError("Could not update EON ESP runtime availability.");
        }
    }
    if (!manager.setEnabled(moduleName, enabled) && enabled) {
        pl::modmenu::setModuleEnabled(moduleId, false);
        if (moduleName == "ESP") {
            eon::logEspEvent("WARN", manager.availabilityMessage(moduleName));
        }
    } else if (moduleName == "ESP") {
        eon::logEspEvent("INFO", enabled ? "ESP module enabled."
                                         : "ESP module disabled.");
    }
}

void onConfigChanged(std::string_view moduleId, std::string_view key,
                    std::string_view value) {
    const auto* module = findModule(moduleId);
    if (!module) {
        logError("Received a config update for an unknown EON module.");
        return;
    }

    eon::EonConfig updated;
    {
        std::lock_guard lock(gConfigMutex);
        if (!gConfigFile) {
            logError("Received an EON config update before config loading.");
            return;
        }
        if (key != "mod_friendly_server") {
            bool belongsToModule = false;
            for (std::size_t index = 0; index < module->settingCount; ++index) {
                belongsToModule |= module->settings[index].configKey == key;
            }
            if (!belongsToModule) {
                logError("Rejected a config key not owned by the EON module.");
                return;
            }
        }
        const eon::EonConfig previous = gConfigFile->value();
        updated = gConfigFile->value();
        if (!updateConfigValue(updated, key, value)) {
            logError("Rejected an invalid EON module config value.");
            return;
        }
        gConfigFile->value() = updated;
        if (!gConfigFile->save()) {
            gConfigFile->value() = previous;
            logError("Could not persist the EON module config.");
            return;
        }
    }

    auto& manager = eon::ModuleManager::instance();
    if (key == "mod_friendly_server") {
        manager.setModFriendlyServer(updated.mod_friendly_server);
        if (!updated.mod_friendly_server) {
            resetMenuModules();
        }
    } else {
        applyModuleSettings(updated);
        if (key == "esp_range") {
            eon::logEspEvent("INFO", "ESP display range changed to " +
                                             std::string(value) + ".");
        }
    }
    publishSchemas(updated);
}

void unregisterMenuModules() {
    for (const auto& module : kModules) {
        pl::modmenu::clearConfigSchema(module.id);
        pl::modmenu::unregisterModule(module.id);
    }
}

class EonLifecycle {
public:
    bool load(pl::mod::ModContext& context) {
        if (eon_init() != static_cast<int>(kModules.size())) {
            context.logger().error("EON did not initialize all native modules.");
            return false;
        }
        const auto gameProfilePath =
                context.configDir() / "profiles" / "1.26.60.4.json";
        if (!eon::Game::instance().loadProfile(gameProfilePath)) {
            const auto profileMessage =
                    eon::Game::instance().unavailableMessage(
                            "Game profile",
                            std::span<const eon::FunctionSlot>{});
            context.logger().warn(
                    "EON game profile is unavailable or invalid at {}: {}",
                    gameProfilePath.string(), profileMessage);
            eon::logEspEvent("WARN",
                             "EON game profile unavailable: " + profileMessage);
        }
        eon::ModuleManager::instance().resetSession();
        std::string hookUnavailableReason;
        const bool espHooksReady =
                eon::installGameHooks(hookUnavailableReason);
        if (!eon::ModuleManager::instance().setImplementationReady(
                    "ESP", espHooksReady)) {
            context.logger().error(
                    "Could not update EON ESP runtime availability.");
        }
        if (!espHooksReady) {
            context.logger().warn("EON ESP is unavailable: {}",
                                  hookUnavailableReason);
        }

        const auto configPath = context.configDir() / "config.json";
        const auto schemaPath = context.configDir() / "config.schema.json";
        auto config = std::make_unique<
                pl::config::ConfigFile<eon::EonConfig>>(
                eon::EonConfig{}, configPath, schemaPath);
        if (!config->load()) {
            context.logger().error("Could not load or initialize EON config.");
            return false;
        }
        config->value().mod_friendly_server = false;
        if (!config->save()) {
            context.logger().error("Could not reset the EON session safeguard.");
            return false;
        }

        const eon::EonConfig loaded = config->value();
        {
            std::lock_guard lock(gConfigMutex);
            gConfigFile = std::move(config);
        }
        applyModuleSettings(loaded);

        for (const auto& module : kModules) {
            auto builder =
                    pl::modmenu::ModuleBuilder(std::string(module.id),
                                               std::string(module.name))
                            .description(std::string(module.description))
                            .defaultEnabled(false)
                            .onToggle([name = std::string(module.name)](
                                              std::string_view id, bool enabled) {
                                onModuleToggle(id, name, enabled);
                            })
                            .onConfigChanged(onConfigChanged);
            if (!builder.registerModule() || !publishSchema(module, loaded)) {
                context.logger().error(
                        "Could not register EON module {} with the mod menu.",
                        module.name);
                unregisterMenuModules();
                std::lock_guard lock(gConfigMutex);
                gConfigFile.reset();
                return false;
            }
        }
        return true;
    }

    bool enable() {
        std::string hookUnavailableReason;
        const bool espHooksReady =
                eon::installGameHooks(hookUnavailableReason);
        if (!eon::ModuleManager::instance().setImplementationReady(
                    "ESP", espHooksReady)) {
            logError("Could not update EON ESP runtime availability.");
        }
        if (!espHooksReady) {
            logError(hookUnavailableReason);
        }
        return true;
    }

    bool disable() {
        eon::logEspEvent("INFO", "EON game session disabled.");
        eon::ModuleManager::instance().resetSession();
        resetMenuModules();
        eon::uninstallGameHooks();
        return true;
    }

    bool unload() {
        eon::logEspEvent("INFO", "EON native module unloaded.");
        eon::ModuleManager::instance().resetSession();
        resetMenuModules();
        eon::uninstallGameHooks();
        bool saved = true;
        {
            std::lock_guard lock(gConfigMutex);
            if (gConfigFile) {
                gConfigFile->value().mod_friendly_server = false;
                saved = gConfigFile->save();
                gConfigFile.reset();
            }
        }
        if (!saved) {
            logError("Could not save EON config during unload.");
        }
        return saved;
    }
};

}  // namespace

PL_REGISTER_MOD(EonLifecycle, EonLifecycle{});
