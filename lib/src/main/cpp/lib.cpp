#include <jni.h>
#include <string>
#include "include/logging.hpp"
#include "include/elf_util.hpp"

extern "C" JNIEXPORT jint JNICALL
JNI_OnLoad([[maybe_unused]] JavaVM *vm, [[maybe_unused]] void *reserved) {

    LOGI("LibUnbound loaded!");

    return JNI_VERSION_1_6;
}

// Frida script to obtain HBC version:
// new NativeFunction(DebugSymbol.fromName('_ZN8facebook6hermes13HermesRuntime18getBytecodeVersionEv').address, "uint32", [])()

extern "C" JNIEXPORT jlong JNICALL
Java_dev_rushii_libunbound_LibUnbound_getHermesRuntimeBytecodeVersion0(JNIEnv *env, [[maybe_unused]] jclass clazz) {

    SandHook::ElfImg hermes("libhermes.so");
    if (!hermes.isValid()) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "libhermes has not been loaded into this process!");
        return -1;
    }

    // https://github.com/discord/hermes/blob/0.76.2-discord/API/hermes/hermes.h#L51-L52
    auto getBytecodeVersion = reinterpret_cast<uint32_t (*)()>(hermes.getSymbAddress("_ZN8facebook6hermes13HermesRuntime18getBytecodeVersionEv"));
    if (!getBytecodeVersion) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      "Failed to find native symbol for facebook::hermes::HermesRuntime::getBytecodeVersion()");
        return -1;
    }

    return getBytecodeVersion();
}
