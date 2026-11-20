// Modified by EoN Launcher V2.
#include "eon/game_profile.hpp"

#include <link.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <limits>
#include <mutex>
#include <sstream>
#include <system_error>

namespace eon {
namespace {

constexpr std::string_view kSupportedGameVersion = "1.26.60.4";
constexpr std::string_view kSupportedArchitecture = "arm64-v8a";

constexpr std::array kFunctionSlotNames{
        std::string_view{"LocalPlayer_normalTick"},
        std::string_view{"ClientInstance_onTick"},
        std::string_view{"Player_getAbilities"},
        std::string_view{"LayeredAbilities_getLayer"},
        std::string_view{"Abilities_setAbility"},
        std::string_view{"SpeedMoveSystem"},
        std::string_view{"JumpHeight"},
        std::string_view{"Gravity"},
        std::string_view{"Actor_setClimbing"},
        std::string_view{"MobJump_setNoJumpDelay"},
};

std::optional<std::uintptr_t> parseOffset(const nlohmann::json& value) {
    if (value.is_null()) {
        return std::nullopt;
    }

    if (value.is_number_unsigned()) {
        return value.get<std::uintptr_t>();
    }
    if (value.is_number_integer()) {
        const auto offset = value.get<std::int64_t>();
        if (offset >= 0) {
            return static_cast<std::uintptr_t>(offset);
        }
        return std::nullopt;
    }
    if (!value.is_string()) {
        return std::nullopt;
    }

    std::string text = value.get<std::string>();
    if (text.starts_with("0x") || text.starts_with("0X")) {
        text.erase(0, 2);
    }
    if (text.empty()) {
        return std::nullopt;
    }

    std::uintptr_t offset{};
    std::istringstream input(text);
    input >> std::hex >> offset;
    if (!input || !input.eof()) {
        return std::nullopt;
    }
    return offset;
}

bool parseProfile(const nlohmann::json& json, GameProfile& profile,
                  std::string& error) {
    if (!json.is_object()) {
        error = "profile root must be an object";
        return false;
    }
    if (!json.contains("game_version") ||
        !json["game_version"].is_string()) {
        error = "profile game_version must be a string";
        return false;
    }
    if (!json.contains("architecture") ||
        !json["architecture"].is_string()) {
        error = "profile architecture must be a string";
        return false;
    }

    profile.gameVersion = json["game_version"].get<std::string>();
    profile.architecture = json["architecture"].get<std::string>();
    profile.configuredRuntimeVersion.reset();

    if (json.contains("running_game_version") &&
        !json["running_game_version"].is_null()) {
        if (!json["running_game_version"].is_string()) {
            error = "running_game_version must be a string or null";
            return false;
        }
        const auto runtimeVersion =
                json["running_game_version"].get<std::string>();
        if (!runtimeVersion.empty()) {
            profile.configuredRuntimeVersion = runtimeVersion;
        }
    }

    if (json.contains("functions")) {
        if (!json["functions"].is_object()) {
            error = "profile functions must be an object";
            return false;
        }
        for (auto it = json["functions"].begin();
             it != json["functions"].end(); ++it) {
            const auto slot = parseFunctionSlot(it.key());
            if (!slot) {
                error = "unknown function slot: " + it.key();
                return false;
            }
            const auto offset = parseOffset(it.value());
            if (!it.value().is_null() && !offset) {
                error = "invalid offset for function slot: " + it.key();
                return false;
            }
            profile.functions[static_cast<std::size_t>(*slot)] = offset;
        }
    }

    profile.fields.clear();
    if (json.contains("fields")) {
        if (!json["fields"].is_object()) {
            error = "profile fields must be an object";
            return false;
        }
        for (auto it = json["fields"].begin();
             it != json["fields"].end(); ++it) {
            const auto offset = parseOffset(it.value());
            if (!it.value().is_null() && !offset) {
                error = "invalid offset for field: " + it.key();
                return false;
            }
            profile.fields.emplace_back(it.key(), offset);
        }
    }
    return true;
}

struct LibrarySearch {
    std::uintptr_t base{};
    bool found{};
};

int findMinecraftLibrary(struct dl_phdr_info* info, std::size_t, void* data) {
    if (info->dlpi_name == nullptr || info->dlpi_name[0] == '\0') {
        return 0;
    }

    const std::string_view path(info->dlpi_name);
    const auto separator = path.find_last_of('/');
    const auto name = separator == std::string_view::npos
                              ? path
                              : path.substr(separator + 1);
    if (name != "libminecraftpe.so") {
        return 0;
    }

    auto* result = static_cast<LibrarySearch*>(data);
    result->base = static_cast<std::uintptr_t>(info->dlpi_addr);
    result->found = true;
    return 1;
}

bool isArm64() noexcept {
#if defined(__aarch64__)
    return sizeof(void*) == 8;
#else
    return false;
#endif
}

}  // namespace

std::string_view functionSlotName(FunctionSlot slot) noexcept {
    const auto index = static_cast<std::size_t>(slot);
    return index < kFunctionSlotNames.size() ? kFunctionSlotNames[index]
                                             : std::string_view{};
}

std::optional<FunctionSlot> parseFunctionSlot(std::string_view name) noexcept {
    const auto found =
            std::find(kFunctionSlotNames.begin(), kFunctionSlotNames.end(),
                      name);
    if (found == kFunctionSlotNames.end()) {
        return std::nullopt;
    }
    return static_cast<FunctionSlot>(
            std::distance(kFunctionSlotNames.begin(), found));
}

Game& Game::instance() {
    static Game game;
    return game;
}

bool Game::loadProfile(const std::filesystem::path& profilePath) {
    std::lock_guard lock(mutex_);
    profile_ = GameProfile{};
    profileLoaded_ = false;
    profileFilePresent_ = false;
    profileError_.clear();

    std::ifstream input(profilePath);
    if (!input) {
        profileError_ = "profile file is unavailable";
        return false;
    }
    profileFilePresent_ = true;

    try {
        const auto json = nlohmann::json::parse(input, nullptr, false);
        if (json.is_discarded()) {
            profileError_ = "profile JSON is invalid";
            return false;
        }
        if (!parseProfile(json, profile_, profileError_)) {
            return false;
        }
    } catch (const nlohmann::json::exception& error) {
        profileError_ = error.what();
        return false;
    }

    profileLoaded_ = true;
    return true;
}

bool Game::ready(std::span<const FunctionSlot> requiredSlots) const {
    std::lock_guard lock(mutex_);
    if (!profileLoaded_ || profile_.gameVersion != kSupportedGameVersion ||
        profile_.architecture != kSupportedArchitecture || !isArm64() ||
        !profile_.configuredRuntimeVersion ||
        *profile_.configuredRuntimeVersion != profile_.gameVersion ||
        minecraftLibraryBase() == 0) {
        return false;
    }
    for (const auto slot : requiredSlots) {
        const auto index = static_cast<std::size_t>(slot);
        if (index >= profile_.functions.size() ||
            !profile_.functions[index] || *profile_.functions[index] == 0) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> Game::missingSlots(
        std::span<const FunctionSlot> requiredSlots) const {
    std::lock_guard lock(mutex_);
    std::vector<std::string> missing;
    for (const auto slot : requiredSlots) {
        const auto index = static_cast<std::size_t>(slot);
        if (index >= profile_.functions.size() ||
            !profile_.functions[index] || *profile_.functions[index] == 0) {
            missing.emplace_back(functionSlotName(slot));
        }
    }
    return missing;
}

std::string Game::unavailableMessage(
        std::string_view moduleName,
        std::span<const FunctionSlot> requiredSlots) const {
    std::lock_guard lock(mutex_);
    std::string result(moduleName);
    result += " unavailable: ";

    std::vector<std::string> reasons;
    if (!profileFilePresent_) {
        reasons.emplace_back("profile file unavailable");
    } else if (!profileLoaded_) {
        reasons.emplace_back("profile invalid: " + profileError_);
    }
    if (profile_.gameVersion != kSupportedGameVersion) {
        reasons.emplace_back("profile game_version is not 1.26.60.4");
    }
    if (profile_.architecture != kSupportedArchitecture || !isArm64()) {
        reasons.emplace_back("profile requires arm64-v8a");
    }
    if (!profile_.configuredRuntimeVersion) {
        reasons.emplace_back("running_game_version is not configured");
    } else if (*profile_.configuredRuntimeVersion != profile_.gameVersion) {
        reasons.emplace_back("running game version does not match the profile");
    }
    if (minecraftLibraryBase() == 0) {
        reasons.emplace_back("libminecraftpe.so base address is unavailable");
    }

    for (const auto slot : requiredSlots) {
        const auto index = static_cast<std::size_t>(slot);
        if (index >= profile_.functions.size() ||
            !profile_.functions[index] || *profile_.functions[index] == 0) {
            reasons.emplace_back("missing " +
                                 std::string(functionSlotName(slot)));
        }
    }

    if (reasons.empty()) {
        result += "profile is ready";
        return result;
    }
    for (std::size_t index = 0; index < reasons.size(); ++index) {
        if (index != 0) {
            result += "; ";
        }
        result += reasons[index];
    }
    return result;
}

std::uintptr_t Game::resolveFunction(FunctionSlot slot) const {
    const std::array required{slot};
    if (!ready(required)) {
        return 0;
    }

    std::lock_guard lock(mutex_);
    const auto index = static_cast<std::size_t>(slot);
    const auto base = minecraftLibraryBase();
    const auto offset = *profile_.functions[index];
    if (offset > std::numeric_limits<std::uintptr_t>::max() - base) {
        return 0;
    }
    return base + offset;
}

std::optional<std::uintptr_t> Game::fieldOffset(
        std::string_view fieldName) const {
    std::lock_guard lock(mutex_);
    if (!profileLoaded_ || profile_.gameVersion != kSupportedGameVersion ||
        profile_.architecture != kSupportedArchitecture || !isArm64() ||
        !profile_.configuredRuntimeVersion ||
        *profile_.configuredRuntimeVersion != profile_.gameVersion) {
        return std::nullopt;
    }
    const auto found = std::find_if(
            profile_.fields.begin(), profile_.fields.end(),
            [fieldName](const auto& field) { return field.first == fieldName; });
    return found == profile_.fields.end() ? std::nullopt : found->second;
}

std::uintptr_t Game::minecraftLibraryBase() const noexcept {
    LibrarySearch result;
    dl_iterate_phdr(findMinecraftLibrary, &result);
    return result.found ? result.base : 0;
}

}  // namespace eon
