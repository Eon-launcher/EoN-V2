// Modified by EoN Launcher V2.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace eon {

enum class FunctionSlot : std::size_t {
    // Local-player tick entry point; Fly uses it to access the local player.
    LocalPlayer_normalTick,
    // Client tick entry point; reserved for verified player acquisition.
    ClientInstance_onTick,
    // Player ability accessor; Fly uses it to reach layered abilities.
    Player_getAbilities,
    // Ability-layer accessor; Fly uses it to reach the creative ability layer.
    LayeredAbilities_getLayer,
    // Ability setter; Fly uses it to toggle the flying ability.
    Abilities_setAbility,
    // Default movement-system function; Speed uses it to adjust movement.
    SpeedMoveSystem,
    // Jump-height function; HighJump uses it to adjust jump height.
    JumpHeight,
    // Gravity function; not required by the abilities-based Fly module.
    Gravity,
    // Climbing-state setter; not required by the abilities-based Fly module.
    Actor_setClimbing,
    // Jump-delay setter; not required by the current HighJump behavior.
    MobJump_setNoJumpDelay,
    Count,
};

inline constexpr std::size_t kFunctionSlotCount =
        static_cast<std::size_t>(FunctionSlot::Count);

std::string_view functionSlotName(FunctionSlot slot) noexcept;
std::optional<FunctionSlot> parseFunctionSlot(std::string_view name) noexcept;

struct GameProfile {
    std::string gameVersion{"1.26.60.4"};
    std::string architecture{"arm64-v8a"};
    std::optional<std::string> configuredRuntimeVersion;
    std::array<std::optional<std::uintptr_t>, kFunctionSlotCount> functions{};
    std::vector<std::pair<std::string, std::optional<std::uintptr_t>>> fields;
};

class Game {
public:
    static Game& instance();

    bool loadProfile(const std::filesystem::path& profilePath);
    bool ready(std::span<const FunctionSlot> requiredSlots) const;
    std::vector<std::string> missingSlots(
            std::span<const FunctionSlot> requiredSlots) const;
    std::string unavailableMessage(
            std::string_view moduleName,
            std::span<const FunctionSlot> requiredSlots) const;
    std::uintptr_t resolveFunction(FunctionSlot slot) const;
    std::optional<std::uintptr_t> fieldOffset(
            std::string_view fieldName) const;
    std::uintptr_t minecraftLibraryBase() const noexcept;

private:
    Game() = default;

    mutable std::mutex mutex_;
    GameProfile profile_;
    bool profileLoaded_{false};
    bool profileFilePresent_{false};
    std::string profileError_;
};

}  // namespace eon
