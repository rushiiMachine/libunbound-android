#include <jni.h>
#include <string>
#include "logging.hpp"

extern "C" JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGI("LibUnbound loaded!");

    return JNI_VERSION_1_6;
}
