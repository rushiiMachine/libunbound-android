#include <jni.h>
#include <string>
#include "include/logging.hpp"
#include "include/elf_util.hpp"

static std::optional<SandHook::ElfImg> hermes;

extern "C" JNIEXPORT jint JNI_OnLoad([[maybe_unused]] JavaVM *vm, [[maybe_unused]] void *reserved) {
    JNIEnv *env;
    if (JNI_OK != vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        LOGE("Failed to get JNIEnv");
        return JNI_ERR;
    };

    LOGI("LibUnbound loaded!");

    hermes = SandHook::ElfImg("libhermes.so");
    if (!hermes->isValid()) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "libhermes has not been loaded into this process!");
        return -1;
    }

    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNI_OnUnload(JavaVM *vm, [[maybe_unused]] void *reserved) {
    hermes.reset();
}

// Frida script to obtain HBC version:
// new NativeFunction(DebugSymbol.fromName('_ZN8facebook6hermes13HermesRuntime18getBytecodeVersionEv').address, "uint32", [])()

extern "C" JNIEXPORT jlong Java_dev_rushii_libunbound_LibUnbound_getHermesRuntimeBytecodeVersion0(
        JNIEnv *env,
        [[maybe_unused]] jclass clazz
) {
    if (!hermes.has_value()) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "LibUnbound did not initialize successfully!");
        return -1;
    }

    // https://github.com/discord/hermes/blob/0.76.2-discord/API/hermes/hermes.h#L51-L52
    auto getBytecodeVersion = reinterpret_cast<uint32_t (*)()>(hermes->getSymbAddress("_ZN8facebook6hermes13HermesRuntime18getBytecodeVersionEv"));
    if (!getBytecodeVersion) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      "Failed to find native symbol for facebook::hermes::HermesRuntime::getBytecodeVersion()");
        return -1;
    }

    return getBytecodeVersion();
}
