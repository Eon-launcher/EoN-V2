#include "eon/module_manager.hpp"
#include "eon/modules.hpp"

#include <android/log.h>
#include <jni.h>

#include <mutex>

namespace {

constexpr char kLogTag[] = "EON";
std::once_flag gInitialization;

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
    });
    manager.resetSession();
    __android_log_print(ANDROID_LOG_INFO, kLogTag, "EON loaded: %zu modules", manager.moduleCount());
    return static_cast<int>(manager.moduleCount());
}

extern "C" JNIEXPORT void JNICALL
Java_org_levimc_launcher_core_mods_EonNative_beginSession(JNIEnv*, jclass) {
    eon_init();
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM*, void*) {
    eon_init();
    return JNI_VERSION_1_6;
}
