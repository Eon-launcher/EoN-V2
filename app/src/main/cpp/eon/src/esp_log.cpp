// Modified by EoN Launcher V2.
#include "eon/esp_log.hpp"

#include <android/log.h>

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <deque>
#include <mutex>
#include <string>

namespace eon {
namespace {

constexpr std::size_t kMaxLogLines = 1000;
constexpr std::size_t kMaxLogBytes = 256 * 1024;
constexpr std::size_t kMaxLineBytes = 4096;

std::mutex gEspLogMutex;
std::deque<std::string> gEspLogLines;
std::size_t gEspLogBytes{};

int androidPriority(std::string_view level) {
    if (level == "ERROR") {
        return ANDROID_LOG_ERROR;
    }
    if (level == "WARN") {
        return ANDROID_LOG_WARN;
    }
    return ANDROID_LOG_INFO;
}

std::string timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) %
            1000;
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    localtime_r(&time, &local);
    char buffer[40]{};
    std::snprintf(buffer, sizeof(buffer),
                  "%04d-%02d-%02d %02d:%02d:%02d.%03lld",
                  local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                  local.tm_hour, local.tm_min, local.tm_sec,
                  static_cast<long long>(milliseconds.count()));
    return buffer;
}

}  // namespace

void logEspEvent(std::string_view level, std::string_view message) {
    std::string line = timestamp();
    line += " [";
    line += level;
    line += "] ";
    line.append(message.data(), message.size());
    if (line.size() > kMaxLineBytes) {
        line.resize(kMaxLineBytes);
    }

    __android_log_write(androidPriority(level), "EON_ESP", line.c_str());

    std::lock_guard lock(gEspLogMutex);
    gEspLogBytes += line.size();
    gEspLogLines.push_back(std::move(line));
    while (gEspLogLines.size() > kMaxLogLines ||
           gEspLogBytes > kMaxLogBytes) {
        gEspLogBytes -= gEspLogLines.front().size();
        gEspLogLines.pop_front();
    }
}

std::string espLogSnapshot() {
    std::lock_guard lock(gEspLogMutex);
    std::string snapshot;
    snapshot.reserve(gEspLogBytes + gEspLogLines.size());
    for (const auto& line : gEspLogLines) {
        snapshot += line;
        snapshot.push_back('\n');
    }
    return snapshot;
}

}  // namespace eon
