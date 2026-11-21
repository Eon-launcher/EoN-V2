// Modified by EoN Launcher V2.
#include "eon/esp_log.hpp"

#include <android/log.h>

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <system_error>

namespace eon {
namespace {

constexpr std::size_t kMaxLogLines = 1000;
constexpr std::size_t kMaxLogBytes = 256 * 1024;
constexpr std::size_t kMaxLineBytes = 4096;
constexpr char kLogTag[] = "EON_ESP";

std::mutex gEspLogMutex;
std::deque<std::string> gEspLogLines;
std::size_t gEspLogBytes{};
std::filesystem::path gEspLogFile;
bool gLogFileFailureReported{};

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

bool writeLogLines(std::ofstream& output,
                   const std::deque<std::string>& lines) {
    for (const auto& line : lines) {
        output.write(line.data(), static_cast<std::streamsize>(line.size()));
        output.put('\n');
    }
    output.flush();
    return output.good();
}

bool writeLogLine(std::ofstream& output, const std::string& line) {
    output.write(line.data(), static_cast<std::streamsize>(line.size()));
    output.put('\n');
    output.flush();
    return output.good();
}

void reportLogFileFailure() {
    if (!gLogFileFailureReported) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                            "Could not persist ESP diagnostics to %s",
                            gEspLogFile.c_str());
        gLogFileFailureReported = true;
    }
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

    __android_log_write(androidPriority(level), kLogTag, line.c_str());

    std::lock_guard lock(gEspLogMutex);
    gEspLogBytes += line.size() + 1;
    gEspLogLines.push_back(std::move(line));
    while (gEspLogLines.size() > kMaxLogLines ||
           gEspLogBytes > kMaxLogBytes) {
        gEspLogBytes -= gEspLogLines.front().size() + 1;
        gEspLogLines.pop_front();
    }

    if (gEspLogFile.empty()) {
        return;
    }

    const auto& latestLine = gEspLogLines.back();
    std::error_code error;
    const auto fileSize = std::filesystem::file_size(gEspLogFile, error);
    const auto latestLineBytes = latestLine.size() + 1;
    const bool replace = error || fileSize > kMaxLogBytes ||
                         latestLineBytes > kMaxLogBytes - fileSize;
    std::ofstream output(gEspLogFile,
                         std::ios::binary |
                                 (replace ? std::ios::trunc : std::ios::app));
    if (!output || !(replace ? writeLogLines(output, gEspLogLines)
                             : writeLogLine(output, latestLine))) {
        reportLogFileFailure();
        return;
    }
    gLogFileFailureReported = false;
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

bool configureEspLogFile(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                            "Could not create ESP diagnostics directory %s: %s",
                            path.parent_path().c_str(), error.message().c_str());
        return false;
    }

    std::lock_guard lock(gEspLogMutex);
    std::error_code sizeError;
    const auto fileSize = std::filesystem::file_size(path, sizeError);
    const bool replace =
            sizeError || fileSize > kMaxLogBytes ||
            gEspLogBytes > kMaxLogBytes - fileSize;
    std::ofstream output(path, std::ios::binary |
                                      (replace ? std::ios::trunc
                                               : std::ios::app));
    if (!output || !writeLogLines(output, gEspLogLines)) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                            "Could not open ESP diagnostics file %s",
                            path.c_str());
        return false;
    }
    gEspLogFile = path;
    gLogFileFailureReported = false;
    return true;
}

}  // namespace eon
