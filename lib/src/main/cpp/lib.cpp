#include <jni.h>
#include <string>
#include "elf_util.hpp"
#include "logging.hpp"

static std::optional<uint32_t (*)()> HERMES_getBytecodeVersion;
static std::optional<bool (*)(const uint8_t *data, size_t len)> HERMES_isHermesBytecode;

extern "C" JNIEXPORT jint JNI_OnLoad([[maybe_unused]] JavaVM *vm, [[maybe_unused]] void *reserved) {
    JNIEnv *env;
    if (JNI_OK != vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        LOGF("Failed to get JNIEnv");
        return JNI_ERR;
    }

    // Open and parse symbols of libhermes
    SandHook::ElfImg hermes("libhermes.so");
    if (!hermes.isValid()) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "libhermes has not been loaded into this process!");
        return JNI_ERR;
    }

    // Locate symbols

    // https://github.com/discord/hermes/blob/0.76.2-discord/API/hermes/hermes.h#L51-L52
    auto getBytecodeVersion = reinterpret_cast<uint32_t (*)()>(hermes.getSymbAddress(
            "_ZN8facebook6hermes13HermesRuntime18getBytecodeVersionEv"));
    if (!getBytecodeVersion) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      "Failed to find native symbol for facebook::hermes::HermesRuntime::getBytecodeVersion()");
        return JNI_ERR;
    }
    HERMES_getBytecodeVersion = getBytecodeVersion;

    // https://github.com/discord/hermes/blob/0.76.2-discord/API/hermes/hermes.h#L50
    auto isHermesBytecode = reinterpret_cast<bool (*)(const uint8_t *data, size_t len)>(hermes.getSymbAddress(
            "_ZN8facebook6hermes13HermesRuntime16isHermesBytecodeEPKhm"));
    if (!isHermesBytecode) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      "Failed to find native symbol for facebook::hermes::HermesRuntime::isHermesBytecode(uint8_t*, size_t)");
        return false;
    }
    HERMES_isHermesBytecode = isHermesBytecode;

    LOGI("LibUnbound loaded!");
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jlong Java_dev_rushii_libunbound_LibUnbound_getHermesRuntimeBytecodeVersion0(
        JNIEnv *env,
        [[maybe_unused]] jclass clazz
) {
    LOGD("getHermesRuntimeBytecodeVersion0");
    if (!HERMES_getBytecodeVersion) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "LibUnbound did not initialize successfully!");
        return -1;
    }

    // Frida script to obtain HBC version:
    // new NativeFunction(DebugSymbol.fromName('_ZN8facebook6hermes13HermesRuntime18getBytecodeVersionEv').address, "uint32", [])()

    return (*HERMES_getBytecodeVersion)();
}

extern "C" JNIEXPORT jboolean Java_dev_rushii_libunbound_LibUnbound_isHermesBytecode0(
        JNIEnv *env,
        [[maybe_unused]] jclass clazz,
        jbyteArray jBytes
) {
    if (!HERMES_isHermesBytecode) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "LibUnbound did not initialize successfully!");
        return false;
    }

    jsize jBytesLength = env->GetArrayLength(jBytes);
    if (jBytesLength <= 0) {
        return false;
    }

    jbyte *bytes = env->GetByteArrayElements(jBytes, nullptr);
    if (!bytes) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), "Failed to obtain jByteArray");
        return false;
    }

    bool isHBC = (*HERMES_isHermesBytecode)(reinterpret_cast<uint8_t *>(bytes), jBytesLength);

    env->ReleaseByteArrayElements(jBytes, bytes, 0);
    return isHBC;
}
