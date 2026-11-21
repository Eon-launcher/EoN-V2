// Modified by EoN Launcher V2.
#include "eon/esp_log.hpp"
#include "eon/module_manager.hpp"
#include "eon/modules.hpp"

#include <android/log.h>
#include <jni.h>

#include <mutex>
#include <string>

namespace {

constexpr char kLogTag[] = "EON";
std::once_flag gInitialization;
int gModuleCount = 0;

}  // namespace

extern "C" int eon_init() {
    auto& manager = eon::ModuleManager::instance();
    std::call_once(gInitialization, [&manager] {
        manager.registerModule(std::make_unique<eon::MobAuraModule>());
        manager.registerModule(std::make_unique<eon::ScaffoldModule>());
        manager.registerModule(std::make_unique<eon::EspModule>());
        manager.registerModule(std::make_unique<eon::XrayModule>());
        manager.registerModule(std::make_unique<eon::FlyModule>());
        manager.registerModule(std::make_unique<eon::SpeedModule>());
        manager.registerModule(std::make_unique<eon::HighJumpModule>());
        gModuleCount = static_cast<int>(manager.moduleCount());
        eon::logEspEvent("INFO", "EON ESP diagnostics initialized.");
        __android_log_print(ANDROID_LOG_INFO, kLogTag, "EON loaded: %d modules",
                            gModuleCount);
    });
    return gModuleCount;
}

extern "C" JNIEXPORT void JNICALL
Java_org_levimc_launcher_core_mods_EonNative_beginSession(JNIEnv*, jclass) {
    eon_init();
    eon::ModuleManager::instance().resetSession();
    eon::logEspEvent("INFO", "EON game session started.");
}

extern "C" JNIEXPORT jstring JNICALL
Java_org_levimc_launcher_core_mods_EonNative_getEspLogs(JNIEnv* env, jclass) {
    const std::string logs = eon::espLogSnapshot();
    return env->NewStringUTF(logs.c_str());
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM*, void*) {
    eon_init();
    return JNI_VERSION_1_6;
}
