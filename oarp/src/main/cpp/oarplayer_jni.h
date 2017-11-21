//
// Created by 申俊伟 on 2017/9/15.
//

#ifndef OARPLAYER_OARPLAYER_JNI_H
#define OARPLAYER_OARPLAYER_JNI_H

#include <jni.h>
jint OAR_JNI_SetupThreadEnv(JNIEnv **p_env);
void OAR_JNI_DetachThreadEnv();
#endif //OARPLAYER_OARPLAYER_JNI_H
