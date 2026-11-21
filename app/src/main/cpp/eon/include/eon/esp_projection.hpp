// Modified by EoN Launcher V2.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>

namespace eon::esp {

struct Vec3 {
    float x{};
    float y{};
    float z{};
};

struct Bounds {
    Vec3 minimum;
    Vec3 maximum;
};

struct Camera {
    Vec3 position;
    float yawDegrees{};
    float pitchDegrees{};
    float verticalFovDegrees{70.0f};
};

struct ScreenPoint {
    float x{};
    float y{};
};

struct ProjectedBounds {
    std::array<ScreenPoint, 8> corners;
    float distance{};
};

inline bool finite(const Vec3& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z);
}

inline std::optional<ProjectedBounds> projectBounds(
        const Bounds& bounds, const Camera& camera, float screenWidth,
        float screenHeight, float maximumDistance) noexcept {
    if (!finite(bounds.minimum) || !finite(bounds.maximum) ||
        !finite(camera.position) || !std::isfinite(camera.yawDegrees) ||
        !std::isfinite(camera.pitchDegrees) ||
        !std::isfinite(camera.verticalFovDegrees) ||
        !std::isfinite(screenWidth) || !std::isfinite(screenHeight) ||
        !std::isfinite(maximumDistance) || screenWidth <= 0.0f ||
        screenHeight <= 0.0f || maximumDistance <= 0.0f ||
        camera.verticalFovDegrees <= 1.0f ||
        camera.verticalFovDegrees >= 179.0f ||
        bounds.maximum.x <= bounds.minimum.x ||
        bounds.maximum.y <= bounds.minimum.y ||
        bounds.maximum.z <= bounds.minimum.z) {
        return std::nullopt;
    }

    const Vec3 center{
            (bounds.minimum.x + bounds.maximum.x) * 0.5f,
            (bounds.minimum.y + bounds.maximum.y) * 0.5f,
            (bounds.minimum.z + bounds.maximum.z) * 0.5f};
    const float dx = center.x - camera.position.x;
    const float dy = center.y - camera.position.y;
    const float dz = center.z - camera.position.z;
    const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (!std::isfinite(distance) || distance > maximumDistance) {
        return std::nullopt;
    }

    constexpr float kDegreesToRadians = 0.01745329251994329577f;
    const float yaw = camera.yawDegrees * kDegreesToRadians;
    const float pitch = camera.pitchDegrees * kDegreesToRadians;
    const float sinYaw = std::sin(yaw);
    const float cosYaw = std::cos(yaw);
    const float sinPitch = std::sin(pitch);
    const float cosPitch = std::cos(pitch);
    const float focalY =
            screenHeight /
            (2.0f * std::tan(camera.verticalFovDegrees *
                             kDegreesToRadians * 0.5f));
    const float focalX = focalY;
    if (!std::isfinite(focalX) || focalX <= 0.0f) {
        return std::nullopt;
    }

    ProjectedBounds result;
    result.distance = distance;
    float minimumX = std::numeric_limits<float>::max();
    float minimumY = std::numeric_limits<float>::max();
    float maximumX = std::numeric_limits<float>::lowest();
    float maximumY = std::numeric_limits<float>::lowest();
    for (unsigned mask = 0; mask < 8; ++mask) {
        const Vec3 point{
                (mask & 1u) ? bounds.maximum.x : bounds.minimum.x,
                (mask & 2u) ? bounds.maximum.y : bounds.minimum.y,
                (mask & 4u) ? bounds.maximum.z : bounds.minimum.z};
        const float relX = point.x - camera.position.x;
        const float relY = point.y - camera.position.y;
        const float relZ = point.z - camera.position.z;
        const float right = relX * cosYaw + relZ * sinYaw;
        const float up = relX * sinYaw * sinPitch + relY * cosPitch -
                         relZ * cosYaw * sinPitch;
        const float depth = -relX * sinYaw * cosPitch -
                            relY * sinPitch + relZ * cosYaw * cosPitch;
        if (!std::isfinite(depth) || depth <= 0.05f) {
            return std::nullopt;
        }

        const float x = screenWidth * 0.5f + right * focalX / depth;
        const float y = screenHeight * 0.5f - up * focalY / depth;
        if (!std::isfinite(x) || !std::isfinite(y)) {
            return std::nullopt;
        }
        result.corners[mask] = {x, y};
        minimumX = std::min(minimumX, x);
        minimumY = std::min(minimumY, y);
        maximumX = std::max(maximumX, x);
        maximumY = std::max(maximumY, y);
    }

    if (maximumX < 0.0f || minimumX > screenWidth ||
        maximumY < 0.0f || minimumY > screenHeight) {
        return std::nullopt;
    }

    return result;
}

}  // namespace eon::esp
