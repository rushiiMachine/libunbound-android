#include <jni.h>
#include <string>
#include <memory>
#include "elf_util.hpp"
#include "logging.hpp"

static std::shared_ptr<SandHook::ElfImg> HERMES;

extern "C" JNIEXPORT jint JNI_OnLoad([[maybe_unused]] JavaVM *vm, [[maybe_unused]] void *reserved) {
    JNIEnv *env;
    if (JNI_OK != vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        LOGF("Failed to get JNIEnv");
        return JNI_ERR;
    }

    HERMES = std::make_shared<SandHook::ElfImg>("libhermes.so");
    if (!HERMES->isValid()) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "libhermes has not been loaded into this process!");
        return JNI_ERR;
    }

    LOGI("LibUnbound loaded!");
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNI_OnUnload([[maybe_unused]] JavaVM *vm, [[maybe_unused]] void *reserved) {
    HERMES.reset();
}

// Frida script to obtain HBC version:
// new NativeFunction(DebugSymbol.fromName('_ZN8facebook6hermes13HermesRuntime18getBytecodeVersionEv').address, "uint32", [])()

extern "C" JNIEXPORT jlong Java_dev_rushii_libunbound_LibUnbound_getHermesRuntimeBytecodeVersion0(
        JNIEnv *env,
        [[maybe_unused]] jclass clazz
) {
    LOGD("getHermesRuntimeBytecodeVersion0");
    if (!HERMES) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "LibUnbound did not initialize successfully!");
        return -1;
    }

    // https://github.com/discord/hermes/blob/0.76.2-discord/API/hermes/hermes.h#L51-L52
    auto getBytecodeVersion = reinterpret_cast<uint32_t (*)()>(HERMES->getSymbAddress(
            "_ZN8facebook6hermes13HermesRuntime18getBytecodeVersionEv"));
    if (!getBytecodeVersion) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      "Failed to find native symbol for facebook::hermes::HermesRuntime::getBytecodeVersion()");
        return -1;
    }

    return getBytecodeVersion();
}

extern "C" JNIEXPORT jboolean Java_dev_rushii_libunbound_LibUnbound_isHermesBytecode0(
        JNIEnv *env,
        [[maybe_unused]] jclass clazz,
        jbyteArray jBytes
) {
    if (!HERMES) {
        env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                      "LibUnbound did not initialize successfully!");
        return false;
    }

    // https://github.com/discord/hermes/blob/0.76.2-discord/API/hermes/hermes.h#L50
    using isHermesBytecode_t = bool (*)(const uint8_t *data, size_t len);
    auto isHermesBytecode = reinterpret_cast<isHermesBytecode_t>(HERMES->getSymbAddress(
            "_ZN8facebook6hermes13HermesRuntime16isHermesBytecodeEPKhm"));
    if (!isHermesBytecode) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                      "Failed to find native symbol for facebook::hermes::HermesRuntime::isHermesBytecode(uint8_t*, size_t)");
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

    bool isHBC = isHermesBytecode(reinterpret_cast<uint8_t *>(bytes), jBytesLength);

    env->ReleaseByteArrayElements(jBytes, bytes, 0);
    return isHBC;
}
