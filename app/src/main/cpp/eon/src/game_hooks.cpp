// Modified by EoN Launcher V2.
#include "eon/game_hooks.hpp"

#include "eon/esp_log.hpp"
#include "eon/module_manager.hpp"

#include <elf.h>
#include <link.h>
#include <pl/memory/Hook.hpp>
#include <pl/memory/Signature.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <iterator>
#include <mutex>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace eon {
namespace {

constexpr std::string_view kSupportedBuildId =
        "eb291b883dfdfaff70c4999c2eabd61885ae3e15";
constexpr std::string_view kNormalTickSignature =
        "? ? ? FC ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 "
        "? ? ? 91 ? ? ? D1 54 D0 3B D5 F3 03 00 AA ? ? ? F9 ? ? ? F8 ? ? "
        "? 39";
constexpr std::string_view kActorManagerListSignature =
        "? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 FD 03 00 "
        "91 ? ? ? F9 ? ? ? A9 ? ? ? F9 ? ? ? B4 F4 03 00 AA";
constexpr std::string_view kRenderLevelSignature =
        "? ? ? D1 ? ? ? FD ? ? ? 6D ? ? ? 6D ? ? ? 6D ? ? ? A9 ? ? ? "
        "A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? 91 57 D0 3B D5";

constexpr std::uintptr_t kActorStateVectorOffset = 0x228;
constexpr std::uintptr_t kActorAabbShapeOffset = 0x230;
constexpr std::uintptr_t kActorRotationOffset = 0x238;
constexpr std::uintptr_t kActorLevelOffset = 0x1F0;
constexpr std::uintptr_t kLevelActorManagerOffset = 0x478;
constexpr std::uintptr_t kLevelRendererPlayerOffset = 0x430;
constexpr std::uintptr_t kLevelRendererCameraPositionOffset = 0x614;
constexpr std::size_t kMaxActorCount = 2048;
using NormalTick = void (*)(void*);
using ActorManagerList = std::vector<void*> (*)(void*);
using RenderLevel = void (*)(void*, void*, void*);

struct Rotation {
    float pitch;
    float yaw;
};

std::mutex gHookMutex;
std::atomic_bool gHooksInstalled{false};
std::atomic<void*> gLocalPlayer{nullptr};
ActorManagerList gActorManagerList{};
NormalTick gNormalTickOriginal{};
RenderLevel gRenderLevelOriginal{};
pl::memory::HookHandle gNormalTickHook;
pl::memory::HookHandle gRenderLevelHook;

struct CameraState {
    esp::Vec3 position;
    bool valid{};
};

std::mutex gCameraMutex;
CameraState gCameraState;

struct ReadableRange {
    std::uintptr_t begin;
    std::uintptr_t end;
};

class ReadableMemory {
public:
    bool load() {
        std::ifstream maps("/proc/self/maps");
        if (!maps) {
            return false;
        }

        std::vector<ReadableRange> ranges;
        std::string line;
        while (std::getline(maps, line)) {
            std::uintptr_t begin = 0;
            std::uintptr_t end = 0;
            char permissions[5]{};
            if (std::sscanf(line.c_str(), "%" SCNxPTR "-%" SCNxPTR " %4s",
                            &begin, &end, permissions) == 3 &&
                permissions[0] == 'r' && end > begin) {
                ranges.push_back({begin, end});
            }
        }
        if (ranges.empty()) {
            return false;
        }

        std::lock_guard lock(mutex_);
        ranges_ = std::move(ranges);
        lastRefresh_ = std::chrono::steady_clock::now();
        return true;
    }

    bool refreshIfNeeded() {
        {
            std::lock_guard lock(mutex_);
            if (!ranges_.empty() &&
                std::chrono::steady_clock::now() - lastRefresh_ <
                        std::chrono::milliseconds(500)) {
                return true;
            }
        }
        return load();
    }

    template <typename T>
    std::optional<T> read(std::uintptr_t address) const {
        std::lock_guard lock(mutex_);
        if (!containsLocked(address, sizeof(T))) {
            return std::nullopt;
        }
        T value{};
        std::memcpy(&value, reinterpret_cast<const void*>(address),
                    sizeof(T));
        return value;
    }

    bool contains(std::uintptr_t address, std::size_t size) const {
        std::lock_guard lock(mutex_);
        return containsLocked(address, size);
    }

private:
    bool containsLocked(std::uintptr_t address, std::size_t size) const {
        if (address < 4096 || size == 0 ||
            address > UINTPTR_MAX - size) {
            return false;
        }
        const auto found = std::upper_bound(
                ranges_.begin(), ranges_.end(), address,
                [](std::uintptr_t value, const ReadableRange& range) {
                    return value < range.begin;
                });
        if (found == ranges_.begin()) {
            return false;
        }
        const auto& range = *std::prev(found);
        return address >= range.begin && address + size <= range.end;
    }

    mutable std::mutex mutex_;
    std::vector<ReadableRange> ranges_;
    std::chrono::steady_clock::time_point lastRefresh_{};
};

ReadableMemory gReadableMemory;

std::size_t alignNote(std::size_t size) {
    constexpr std::size_t kAlignment = 4;
    return (size + (kAlignment - 1)) & ~(kAlignment - 1);
}

std::string buildId(const dl_phdr_info& info) {
    for (ElfW(Half) index = 0; index < info.dlpi_phnum; ++index) {
        const auto& header = info.dlpi_phdr[index];
        if (header.p_type != PT_NOTE || header.p_memsz < sizeof(ElfW(Nhdr))) {
            continue;
        }
        const auto begin = static_cast<std::uintptr_t>(info.dlpi_addr) +
                           header.p_vaddr;
        const auto end = begin + header.p_memsz;
        auto cursor = begin;
        while (cursor <= end && end - cursor >= sizeof(ElfW(Nhdr))) {
            ElfW(Nhdr) note{};
            std::memcpy(&note, reinterpret_cast<const void*>(cursor),
                        sizeof(note));
            cursor += sizeof(note);
            const auto nameSize = alignNote(note.n_namesz);
            const auto descriptionSize = alignNote(note.n_descsz);
            if (nameSize > end - cursor ||
                descriptionSize > end - cursor - nameSize) {
                break;
            }
            const auto* name = reinterpret_cast<const char*>(cursor);
            const auto* description = reinterpret_cast<const unsigned char*>(
                    cursor + nameSize);
            if (note.n_type == NT_GNU_BUILD_ID && note.n_namesz == 4 &&
                std::memcmp(name, "GNU", 4) == 0) {
                constexpr char kHex[] = "0123456789abcdef";
                std::string result;
                result.reserve(note.n_descsz * 2);
                for (std::size_t byte = 0; byte < note.n_descsz; ++byte) {
                    result.push_back(kHex[description[byte] >> 4]);
                    result.push_back(kHex[description[byte] & 0x0F]);
                }
                return result;
            }
            cursor += nameSize + descriptionSize;
        }
    }
    return {};
}

int findMinecraftBuildId(dl_phdr_info* info, std::size_t, void* data) {
    if (!info->dlpi_name || !*info->dlpi_name) {
        return 0;
    }
    const std::string_view path(info->dlpi_name);
    if (path.substr(path.find_last_of('/') == std::string_view::npos
                            ? 0
                            : path.find_last_of('/') + 1) != "libminecraftpe.so") {
        return 0;
    }
    *static_cast<std::string*>(data) = buildId(*info);
    return 1;
}

std::string loadedMinecraftBuildId() {
    std::string actualBuildId;
    dl_iterate_phdr(findMinecraftBuildId, &actualBuildId);
    return actualBuildId;
}

void captureCamera(void* renderer) {
    const auto rendererAddress = reinterpret_cast<std::uintptr_t>(renderer);
    const auto renderPlayer = gReadableMemory.read<std::uintptr_t>(
            rendererAddress + kLevelRendererPlayerOffset);
    if (!renderPlayer || *renderPlayer == 0) {
        return;
    }
    const auto camera = gReadableMemory.read<esp::Vec3>(
            *renderPlayer + kLevelRendererCameraPositionOffset);
    if (!camera || !esp::finite(*camera)) {
        return;
    }
    std::lock_guard lock(gCameraMutex);
    gCameraState = {*camera, true};
}

void normalTickDetour(void* player) {
    if (gNormalTickOriginal) {
        gNormalTickOriginal(player);
    }
    if (!gHooksInstalled.load(std::memory_order_acquire) || !player) {
        return;
    }
    gLocalPlayer.store(player, std::memory_order_release);
    ModuleManager::instance().onTick();
    gLocalPlayer.store(nullptr, std::memory_order_release);
}

void renderLevelDetour(void* renderer, void* screenContext, void* view) {
    if (gRenderLevelOriginal) {
        gRenderLevelOriginal(renderer, screenContext, view);
    }
    if (gHooksInstalled.load(std::memory_order_acquire) && renderer) {
        captureCamera(renderer);
    }
}

bool readableBounds(const esp::Bounds& bounds) {
    if (!esp::finite(bounds.minimum) || !esp::finite(bounds.maximum)) {
        return false;
    }
    const float width = bounds.maximum.x - bounds.minimum.x;
    const float height = bounds.maximum.y - bounds.minimum.y;
    const float depth = bounds.maximum.z - bounds.minimum.z;
    return width > 0.0f && width < 32.0f && height > 0.0f &&
           height < 64.0f && depth > 0.0f && depth < 32.0f;
}

bool frameUnavailable(std::string& reason, std::string_view message) {
    reason = message;
    return false;
}

}  // namespace

bool installGameHooks(std::string& unavailableReason) {
    std::lock_guard lock(gHookMutex);
    if (gHooksInstalled.load(std::memory_order_acquire)) {
        unavailableReason.clear();
        logEspEvent("INFO", "Verified game hooks are already installed.");
        return true;
    }
    logEspEvent("INFO", "Checking the Minecraft build and ESP hook signatures.");
    const std::string actualBuildId = loadedMinecraftBuildId();
    if (actualBuildId != kSupportedBuildId) {
        unavailableReason = "libminecraftpe.so build ID mismatch (loaded: ";
        unavailableReason += actualBuildId.empty() ? "not found" : actualBuildId;
        unavailableReason += "; expected: ";
        unavailableReason += kSupportedBuildId;
        unavailableReason += ")";
        logEspEvent("WARN", unavailableReason);
        return false;
    }
    if (!gReadableMemory.load()) {
        unavailableReason = "could not read the process memory map";
        logEspEvent("WARN", unavailableReason);
        return false;
    }

    const auto tick = pl::memory::resolveSignature(kNormalTickSignature,
                                                    "libminecraftpe.so");
    const auto actorList = pl::memory::resolveSignature(
            kActorManagerListSignature, "libminecraftpe.so");
    const auto render = pl::memory::resolveSignature(kRenderLevelSignature,
                                                      "libminecraftpe.so");
    if (!tick || !actorList || !render) {
        unavailableReason = "verified ARM64 function signatures not found:";
        if (!tick) {
            unavailableReason += " NormalTick";
        }
        if (!actorList) {
            unavailableReason += " ActorManagerList";
        }
        if (!render) {
            unavailableReason += " RenderLevel";
        }
        logEspEvent("WARN", unavailableReason);
        return false;
    }

    gActorManagerList = reinterpret_cast<ActorManagerList>(actorList);
    gNormalTickHook = pl::memory::HookHandle(
            reinterpret_cast<void*>(tick),
            reinterpret_cast<void*>(normalTickDetour),
            reinterpret_cast<void**>(&gNormalTickOriginal));
    gRenderLevelHook = pl::memory::HookHandle(
            reinterpret_cast<void*>(render),
            reinterpret_cast<void*>(renderLevelDetour),
            reinterpret_cast<void**>(&gRenderLevelOriginal));
    if (!gNormalTickHook.installed() || !gRenderLevelHook.installed() ||
        !gNormalTickOriginal || !gRenderLevelOriginal) {
        gRenderLevelHook.reset();
        gNormalTickHook.reset();
        gActorManagerList = nullptr;
        gNormalTickOriginal = nullptr;
        gRenderLevelOriginal = nullptr;
        unavailableReason = "could not install the verified game hooks";
        logEspEvent("ERROR", unavailableReason);
        return false;
    }

    gHooksInstalled.store(true, std::memory_order_release);
    unavailableReason.clear();
    logEspEvent("INFO", "Verified NormalTick and RenderLevel hooks installed.");
    return true;
}

void uninstallGameHooks() noexcept {
    ModuleManager::instance().setImplementationReady("ESP", false);
    std::lock_guard lock(gHookMutex);
    gHooksInstalled.store(false, std::memory_order_release);
    gRenderLevelHook.reset();
    gNormalTickHook.reset();
    gActorManagerList = nullptr;
    gNormalTickOriginal = nullptr;
    gRenderLevelOriginal = nullptr;
    gLocalPlayer.store(nullptr, std::memory_order_release);
    std::lock_guard cameraLock(gCameraMutex);
    gCameraState = {};
}

bool readEspWorldFrame(EspWorldFrame& frame,
                       std::string& unavailableReason) {
    unavailableReason.clear();
    if (!gHooksInstalled.load(std::memory_order_acquire) ||
        !gActorManagerList) {
        return frameUnavailable(unavailableReason,
                                "verified game hooks are not installed");
    }
    if (!gReadableMemory.refreshIfNeeded()) {
        return frameUnavailable(unavailableReason,
                                "could not refresh the process memory map");
    }

    const auto localPlayer =
            reinterpret_cast<std::uintptr_t>(
                    gLocalPlayer.load(std::memory_order_acquire));
    if (!localPlayer) {
        return frameUnavailable(unavailableReason,
                                "waiting for the local player tick");
    }

    static bool dumped = false;
    if (!dumped) {
        dumped = true;
        auto describe = [](std::uintptr_t a) -> std::string {
            std::ifstream m("/proc/self/maps");
            std::string l;
            while (std::getline(m, l)) {
                std::uintptr_t b = 0, e = 0;
                if (std::sscanf(l.c_str(), "%" SCNxPTR "-%" SCNxPTR, &b, &e) == 2
                    && a >= b && a < e) return l;
            }
            return "not mapped";
        };
        char buf[64];
        std::snprintf(buf, sizeof buf, "player=0x%" PRIxPTR, localPlayer);
        logEspEvent("INFO", std::string(buf) + " -> " + describe(localPlayer));
        if (auto vt = gReadableMemory.read<std::uintptr_t>(localPlayer)) {
            std::snprintf(buf, sizeof buf, "vtable=0x%" PRIxPTR, *vt);
            logEspEvent("INFO", std::string(buf) + " -> " + describe(*vt));
        } else {
            logEspEvent("WARN", "player object not readable");
        }
    }

    const auto& memory = gReadableMemory;
    const auto localLevel =
            memory.read<std::uintptr_t>(localPlayer + kActorLevelOffset);
    if (!localLevel) {
        return frameUnavailable(unavailableReason,
                                "local player level field at +0x1F0 is not readable");
    }
    if (!*localLevel) {
        return frameUnavailable(unavailableReason,
                                "local player level pointer at +0x1F0 is null");
    }
    const auto actorManager =
            memory.read<std::uintptr_t>(*localLevel + kLevelActorManagerOffset);
    if (!actorManager || !*actorManager) {
        return frameUnavailable(unavailableReason,
                                "level actor manager is unavailable");
    }
    const auto localStateVector =
            memory.read<std::uintptr_t>(localPlayer + kActorStateVectorOffset);
    const auto rotationComponent =
            memory.read<std::uintptr_t>(localPlayer + kActorRotationOffset);
    if (!localStateVector || !*localStateVector || !rotationComponent ||
        !*rotationComponent) {
        return frameUnavailable(unavailableReason,
                                "local player position or rotation is unavailable");
    }
    const auto localPosition = memory.read<esp::Vec3>(*localStateVector);
    const auto rotation = memory.read<Rotation>(*rotationComponent);
    if (!localPosition || !rotation || !esp::finite(*localPosition) ||
        !std::isfinite(rotation->pitch) || !std::isfinite(rotation->yaw)) {
        return frameUnavailable(unavailableReason,
                                "local player position or rotation is invalid");
    }

    frame.camera.position = {
            localPosition->x, localPosition->y + 1.62f, localPosition->z};
    frame.camera.pitchDegrees = rotation->pitch;
    frame.camera.yawDegrees = rotation->yaw;
    {
        std::lock_guard lock(gCameraMutex);
        if (gCameraState.valid) {
            frame.camera.position = gCameraState.position;
        }
    }

    auto actors = gActorManagerList(reinterpret_cast<void*>(*actorManager));
    if (actors.size() > kMaxActorCount) {
        return frameUnavailable(unavailableReason,
                                "actor list exceeds the safety limit");
    }

    frame.entities.clear();
    frame.entities.reserve(actors.size());
    for (const auto actor : actors) {
        const auto address = reinterpret_cast<std::uintptr_t>(actor);
        if (!actor || address == localPlayer ||
            !memory.contains(address, kActorAabbShapeOffset + sizeof(void*))) {
            continue;
        }
        const auto aabbComponent =
                memory.read<std::uintptr_t>(address + kActorAabbShapeOffset);
        if (!aabbComponent || !*aabbComponent) {
            continue;
        }
        const auto bounds = memory.read<esp::Bounds>(*aabbComponent);
        if (bounds && readableBounds(*bounds)) {
            frame.entities.push_back(*bounds);
        }
    }
    return true;
}

}  // namespace eon
