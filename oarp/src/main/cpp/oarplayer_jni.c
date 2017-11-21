#include <jni.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>


#include "_android.h"
#include "jni_utils.h"
#include "oarplayer.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#define JNI_CLASS_OARPLAYER     "com/wodekouwei/oarplayer/OARMediaPlayer"
#define JNI_OAR_MEDIA_EXCEPTION "com/wodekouwei/oarplayer/exceptions/OARMediaException"
#define JNI_CLASS_FIELD_MEDIAPLAYER "mNativeMediaPlayer"

#define OAR_CHECK_MPRET_GOTO(retval, env, label) \
    JNI_CHECK_GOTO((retval != EOAR_INVALID_STATE), env, "java/lang/IllegalStateException", NULL, label); \
    JNI_CHECK_GOTO((retval != EOAR_OUT_OF_MEMORY), env, "java/lang/OutOfMemoryError", NULL, label); \
    JNI_CHECK_GOTO((retval == 0), env, JNI_OAR_MEDIA_EXCEPTION, NULL, label);

static JavaVM* g_jvm;
static pthread_key_t g_thread_key;
static pthread_once_t g_key_once = PTHREAD_ONCE_INIT;

typedef struct player_fields_t {
    pthread_mutex_t mutex;
    jclass clazz;
} player_fields_t;
static player_fields_t g_clazz;

static int inject_callback(void *opaque, int type, void *data, size_t data_size);
static bool mediacodec_select_callback(void *opaque, ijkmp_mediacodecinfo_context *mcc);

static

static OARMediaPlayer *jni_get_media_player(JNIEnv* env, jobject thiz)
{
    pthread_mutex_lock(&g_clazz.mutex);

    jfieldID jfID = (*env)->GetFieldID(env, thiz, JNI_CLASS_FIELD_MEDIAPLAYER, "J");
    jlong mNativeMediaPlayer = (*env)->GetLongField(env, thiz, jfID);

    OARMediaPlayer *mp = (OARMediaPlayer *) (intptr_t) mNativeMediaPlayer;
    if (mp) {
        oarmp_inc_ref(mp);
    }

    pthread_mutex_unlock(&g_clazz.mutex);
    return mp;
}
static OARMediaPlayer *jni_set_media_player(JNIEnv* env, jobject thiz, OARMediaPlayer *mp)
{
    pthread_mutex_lock(&g_clazz.mutex);

    jfieldID jfID = (*env)->GetFieldID(env, thiz, JNI_CLASS_FIELD_MEDIAPLAYER, "J");
    jlong mNativeMediaPlayer = (*env)->GetLongField(env, thiz, jfID);

    OARMediaPlayer *old = (OARMediaPlayer *) (intptr_t) mNativeMediaPlayer;
    if (mp) {
        oarmp_inc_ref(mp);
    }
    (*env)->SetLongField(env, thiz, jfID, (intptr_t) mp);

    pthread_mutex_unlock(&g_clazz.mutex);

    // NOTE: ijkmp_dec_ref may block thread
    if (old != NULL ) {
        oarmp_dec_ref_p(&old);
    }
    return old;
}


static int message_loop(void *arg);

static void
OARMediaPlayer_setDataSourceAndHeaders(
        JNIEnv *env, jobject thiz, jstring path,
        jobjectArray keys, jobjectArray values)
{
    MPTRACE("%s\n", __func__);
    int retval = 0;
    const char *c_path = NULL;
    OARMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(path, env, "java/lang/IllegalArgumentException", "mpjni: setDataSource: null path", LABEL_RETURN);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: setDataSource: null mp", LABEL_RETURN);

    c_path = (*env)->GetStringUTFChars(env, path, NULL );
    JNI_CHECK_GOTO(c_path, env, "java/lang/OutOfMemoryError", "mpjni: setDataSource: path.string oom", LABEL_RETURN);

    LOGD("setDataSource: path %s", c_path);
    retval = oarmp_set_data_source(mp, c_path);
    (*env)->ReleaseStringUTFChars(env, path, c_path);

    OAR_CHECK_MPRET_GOTO(retval, env, LABEL_RETURN);

LABEL_RETURN:
    oarmp_dec_ref_p(&mp);
}
static void
OARMediaPlayer_setVideoSurface(JNIEnv *env, jobject thiz, jobject jsurface)
{

    MPTRACE("%s\n", __func__);
    OARMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: setVideoSurface: null mp", LABEL_RETURN);

    oarmp_android_set_surface(env, mp, jsurface);

LABEL_RETURN:
    oarmp_dec_ref_p(&mp);
    return;
}

static void
OARMediaPlayer_prepareAsync(JNIEnv *env, jobject thiz)
{
    MPTRACE("%s\n", __func__);
    int retval = 0;
    OARMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: prepareAsync: null mp", LABEL_RETURN);

    retval = oarmp_prepare_async(mp);
    OAR_CHECK_MPRET_GOTO(retval, env, LABEL_RETURN);

LABEL_RETURN:
    oarmp_dec_ref_p(&mp);
}

static void
OARMediaPlayer_start(JNIEnv *env, jobject thiz)
{
    MPTRACE("%s\n", __func__);
    OARMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: start: null mp", LABEL_RETURN);

    oarmp_start(mp);

LABEL_RETURN:
    oarmp_dec_ref_p(&mp);
}

static void
OARMediaPlayer_stop(JNIEnv *env, jobject thiz)
{
    OARMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: stop: null mp", LABEL_RETURN);

    oarmp_stop(mp);

LABEL_RETURN:
    oarmp_dec_ref_p(&mp);
}

static void
OARMediaPlayer_pause(JNIEnv *env, jobject thiz)
{
    OARMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: pause: null mp", LABEL_RETURN);

    oarmp_pause(mp);

LABEL_RETURN:
    oarmp_dec_ref_p(&mp);
}

static void
OARMediaPlayer_seekTo(JNIEnv *env, jobject thiz, jlong msec)
{
    MPTRACE("%s\n", __func__);
    OARMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: seekTo: null mp", LABEL_RETURN);

    oarmp_seek_to(mp, msec);

LABEL_RETURN:
    oarmp_dec_ref_p(&mp);
}

static jboolean
OARMediaPlayer_isPlaying(JNIEnv *env, jobject thiz)
{
    jboolean retval = JNI_FALSE;
    OARMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: isPlaying: null mp", LABEL_RETURN);

    retval = oarmp_is_playing(mp) ? JNI_TRUE : JNI_FALSE;

LABEL_RETURN:
    oarmp_dec_ref_p(&mp);
    return retval;
}

static jlong
OARMediaPlayer_getCurrentPosition(JNIEnv *env, jobject thiz)
{
    jlong retval = 0;
    OARMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: getCurrentPosition: null mp", LABEL_RETURN);

    retval = oarmp_get_current_position(mp);

LABEL_RETURN:
    oarmp_dec_ref_p(&mp);
    return retval;
}

static jlong
OARMediaPlayer_getDuration(JNIEnv *env, jobject thiz)
{
    jlong retval = 0;
    OARMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: getDuration: null mp", LABEL_RETURN);

    retval = oarmp_get_duration(mp);

LABEL_RETURN:
    oarmp_dec_ref_p(&mp);
    return retval;
}

static void
OARMediaPlayer_release(JNIEnv *env, jobject thiz)
{
    MPTRACE("%s\n", __func__);
    OARMediaPlayer *mp = jni_get_media_player(env, thiz);
    if (!mp)
        return;

    oarmp_android_set_surface(env, mp, NULL);
    // explicit shutdown mp, in case it is not the last mp-ref here
    oarmp_shutdown(mp);
    //only delete weak_thiz at release
    jobject weak_thiz = (jobject) oarmp_set_weak_thiz(mp, NULL );
    (*env)->DeleteGlobalRef(env, weak_thiz);
    jni_set_media_player(env, thiz, NULL);

    oarmp_dec_ref_p(&mp);
}

static void OARMediaPlayer_native_setup(JNIEnv *env, jobject thiz, jobject weak_this);
static void
OARMediaPlayer_reset(JNIEnv *env, jobject thiz)
{
    MPTRACE("%s\n", __func__);
    OARMediaPlayer *mp = jni_get_media_player(env, thiz);
    if (!mp)
        return;

    jobject weak_thiz = (jobject) oarmp_set_weak_thiz(mp, NULL );

    OARMediaPlayer_release(env, thiz);
    OARMediaPlayer_native_setup(env, thiz, weak_thiz);

    oarmp_dec_ref_p(&mp);
}

static void
OARMediaPlayer_setLoopCount(JNIEnv *env, jobject thiz, jint loop_count)
{
    //TODO
}

static jint
OARMediaPlayer_getLoopCount(JNIEnv *env, jobject thiz)
{
    //TODO
    jint loop_count = 1;
    return loop_count;
}

static jfloat
OARMediaPlayer_getPropertyFloat(JNIEnv *env, jobject thiz, jint id, jfloat default_value)
{
    //TODO
    jfloat value = default_value;
    return value;
}

static void
OARMediaPlayer_setPropertyFloat(JNIEnv *env, jobject thiz, jint id, jfloat value)
{
    //TODO
    return;
}

static jlong
OARMediaPlayer_getPropertyLong(JNIEnv *env, jobject thiz, jint id, jlong default_value)
{
    //TODO
    jlong value = default_value;
    return value;
}

static void
OARMediaPlayer_setPropertyLong(JNIEnv *env, jobject thiz, jint id, jlong value)
{
    //TODO
    return;
}

static void
OARMediaPlayer_setStreamSelected(JNIEnv *env, jobject thiz, jint stream, jboolean selected)
{
    //TODO
    return;
}

static void
OARMediaPlayer_setVolume(JNIEnv *env, jobject thiz, jfloat leftVolume, jfloat rightVolume)
{
    MPTRACE("%s\n", __func__);
    OARMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: setVolume: null mp", LABEL_RETURN);

    oarmp_android_set_volume(env, mp, leftVolume, rightVolume);

LABEL_RETURN:
    oarmp_dec_ref_p(&mp);
}

static jint
OARMediaPlayer_getAudioSessionId(JNIEnv *env, jobject thiz)
{
    //TODO
    jint audio_session_id = 0;
    return audio_session_id;
}

static void
OARMediaPlayer_setOption(JNIEnv *env, jobject thiz, jint category, jobject name, jobject value)
{
    //TODO
}

static void
OARMediaPlayer_setOptionLong(JNIEnv *env, jobject thiz, jint category, jobject name, jlong value)
{
    //TODO
}

static jstring
OARMediaPlayer_getColorFormatName(JNIEnv *env, jclass clazz, jint mediaCodecColorFormat)
{
    const char *codec_name = "";
    if (!codec_name)
        return NULL ;

    return (*env)->NewStringUTF(env, codec_name);
}

static jstring
OARMediaPlayer_getVideoCodecInfo(JNIEnv *env, jobject thiz)
{
    jstring jcodec_info = NULL;
    return jcodec_info;
}

static jstring
OARMediaPlayer_getAudioCodecInfo(JNIEnv *env, jobject thiz)
{
    jstring jcodec_info = NULL;
    return jcodec_info;
}


static jobject
OARMediaPlayer_getMediaMeta(JNIEnv *env, jobject thiz)
{
    jobject jret_bundle = NULL;
    return jret_bundle;
}

static void
OARMediaPlayer_native_init(JNIEnv *env)
{
    MPTRACE("%s\n", __func__);
}

static void
OARMediaPlayer_native_setup(JNIEnv *env, jobject thiz, jobject weak_this)
{
    MPTRACE("%s\n", __func__);
    OARMediaPlayer *mp = oarmp_android_create(message_loop);
    JNI_CHECK_GOTO(mp, env, "java/lang/OutOfMemoryError", "mpjni: native_setup: ijkmp_create() failed", LABEL_RETURN);

    jni_set_media_player(env, thiz, mp);
    oarmp_set_weak_thiz(mp, (*env)->NewGlobalRef(env, weak_this));
    oarmp_set_inject_opaque(mp, oarmp_get_weak_thiz(mp));
    oarmp_android_set_mediacodec_select_callback(mp, mediacodec_select_callback, oarmp_get_weak_thiz(mp));

    LABEL_RETURN:
    oarmp_dec_ref_p(&mp);
}

static void
OARMediaPlayer_native_finalize(JNIEnv *env, jobject thiz, jobject name, jobject value)
{
    MPTRACE("%s\n", __func__);
    OARMediaPlayer_release(env, thiz);
}
static bool mediacodec_select_callback(void *opaque, ijkmp_mediacodecinfo_context *mcc)
{
    JNIEnv *env = NULL;
    jobject weak_this = (jobject) opaque;
    const char *found_codec_name = NULL;

    //TODO
    /*if (JNI_OK != SDL_JNI_SetupThreadEnv(&env)) {
        LOGE("%s: SetupThreadEnv failed\n", __func__);
        return -1;
    }

    found_codec_name = J4AC_IjkMediaPlayer__onSelectCodec__withCString__asCBuffer(env, weak_this, mcc->mime_type, mcc->profile, mcc->level, mcc->codec_name, sizeof(mcc->codec_name));
    if (J4A_ExceptionCheck__catchAll(env) || !found_codec_name) {
        LOGE("%s: onSelectCodec failed\n", __func__);
        goto fail;
    }*/

fail:
    return found_codec_name;
}

inline static void post_event(JNIEnv *env, jobject weak_this, int what, int arg1, int arg2)
{
    // MPTRACE("post_event(%p, %p, %d, %d, %d)", (void*)env, (void*) weak_this, what, arg1, arg2);
    jmethodID  method_postEventFromNative = (*env)->GetStaticMethodID(env, &g_clazz.clazz, "postEventFromNative",
                                                                      "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    (*env)->CallStaticVoidMethod(env, method_postEventFromNative, &g_clazz.clazz, weak_this,  what, arg1, arg2, NULL);
    // MPTRACE("post_event()=void");
}

static void message_loop_n(JNIEnv *env, OARMediaPlayer *mp)
{
    jobject weak_thiz = (jobject) oarmp_get_weak_thiz(mp);
    JNI_CHECK_GOTO(weak_thiz, env, NULL, "mpjni: message_loop_n: null weak_thiz", LABEL_RETURN);

    while (1) {
        AVMessage msg;

        int retval = oarmp_get_msg(mp, &msg, 1);
        if (retval < 0)
            break;

        // block-get should never return 0
        assert(retval > 0);

        switch (msg.what) {
            case OAR_MSG_FLUSH:
                        MPTRACE("FFP_MSG_FLUSH:\n");
                post_event(env, weak_thiz, MEDIA_NOP, 0, 0);
                break;
            case OAR_MSG_ERROR:
                        MPTRACE("FFP_MSG_ERROR: %d\n", msg.arg1);
                post_event(env, weak_thiz, MEDIA_ERROR, MEDIA_ERROR_IJK_PLAYER, msg.arg1);
                break;
            case OAR_MSG_PREPARED:
                        MPTRACE("FFP_MSG_PREPARED:\n");
                post_event(env, weak_thiz, MEDIA_PREPARED, 0, 0);
                break;
            case OAR_MSG_COMPLETED:
                        MPTRACE("FFP_MSG_COMPLETED:\n");
                post_event(env, weak_thiz, MEDIA_PLAYBACK_COMPLETE, 0, 0);
                break;
            case OAR_MSG_VIDEO_SIZE_CHANGED:
                        MPTRACE("FFP_MSG_VIDEO_SIZE_CHANGED: %d, %d\n", msg.arg1, msg.arg2);
                post_event(env, weak_thiz, MEDIA_SET_VIDEO_SIZE, msg.arg1, msg.arg2);
                break;
            case OAR_MSG_SAR_CHANGED:
                        MPTRACE("FFP_MSG_SAR_CHANGED: %d, %d\n", msg.arg1, msg.arg2);
                post_event(env, weak_thiz, MEDIA_SET_VIDEO_SAR, msg.arg1, msg.arg2);
                break;
            case OAR_MSG_VIDEO_RENDERING_START:
                        MPTRACE("FFP_MSG_VIDEO_RENDERING_START:\n");
                post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_VIDEO_RENDERING_START, 0);
                break;
            case OAR_MSG_AUDIO_RENDERING_START:
                        MPTRACE("FFP_MSG_AUDIO_RENDERING_START:\n");
                post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_AUDIO_RENDERING_START, 0);
                break;
            case OAR_MSG_VIDEO_ROTATION_CHANGED:
                        MPTRACE("FFP_MSG_VIDEO_ROTATION_CHANGED: %d\n", msg.arg1);
                post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_VIDEO_ROTATION_CHANGED, msg.arg1);
                break;
            case OAR_MSG_BUFFERING_START:
                        MPTRACE("FFP_MSG_BUFFERING_START:\n");
                post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_BUFFERING_START, 0);
                break;
            case OAR_MSG_BUFFERING_END:
                        MPTRACE("FFP_MSG_BUFFERING_END:\n");
                post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_BUFFERING_END, 0);
                break;
            case OAR_MSG_BUFFERING_UPDATE:
                // MPTRACE("FFP_MSG_BUFFERING_UPDATE: %d, %d", msg.arg1, msg.arg2);
                post_event(env, weak_thiz, MEDIA_BUFFERING_UPDATE, msg.arg1, msg.arg2);
                break;
            case OAR_MSG_BUFFERING_BYTES_UPDATE:
                break;
            case OAR_MSG_BUFFERING_TIME_UPDATE:
                break;
            case OAR_MSG_SEEK_COMPLETE:
                        MPTRACE("FFP_MSG_SEEK_COMPLETE:\n");
                post_event(env, weak_thiz, MEDIA_SEEK_COMPLETE, 0, 0);
                break;
            case OAR_MSG_PLAYBACK_STATE_CHANGED:
                break;
            default:
                LOGE("unknown FFP_MSG_xxx(%d)\n", msg.what);
                break;
        }
    }

LABEL_RETURN:
    ;
}

static int message_loop(void *arg)
{
            MPTRACE("%s\n", __func__);

    JNIEnv *env = NULL;
    (*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL );

    OARMediaPlayer *mp = (OARMediaPlayer*) arg;
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: native_message_loop: null mp", LABEL_RETURN);

    message_loop_n(env, mp);

    LABEL_RETURN:
    oarmp_dec_ref_p(&mp);
    (*g_jvm)->DetachCurrentThread(g_jvm);

            MPTRACE("message_loop exit");
    return 0;
}

// ----------------------------------------------------------------------------
void monstartup(const char *libname);
void moncleanup(void);


static void
OARMediaPlayer_native_profileBegin(JNIEnv *env, jclass clazz, jstring libName)
{
    MPTRACE("%s\n", __func__);

    const char *c_lib_name = NULL;
    static int s_monstartup = 0;

    if (!libName)
        return;

    if (s_monstartup) {
        LOGW("monstartup already called\b");
        return;
    }

    c_lib_name = (*env)->GetStringUTFChars(env, libName, NULL );
    JNI_CHECK_GOTO(c_lib_name, env, "java/lang/OutOfMemoryError", "mpjni: monstartup: libName.string oom", LABEL_RETURN);

    s_monstartup = 1;
    monstartup(c_lib_name);
    LOGD("monstartup: %s\n", c_lib_name);

LABEL_RETURN:
    if (c_lib_name)
        (*env)->ReleaseStringUTFChars(env, libName, c_lib_name);
}

static void
OARMediaPlayer_native_profileEnd(JNIEnv *env, jclass clazz)
{
    MPTRACE("%s\n", __func__);
    static int s_moncleanup = 0;

    if (s_moncleanup) {
        LOGW("moncleanu already called\b");
        return;
    }

    s_moncleanup = 1;
    moncleanup();
    LOGD("moncleanup\n");
}

static void
OARMediaPlayer_native_setLogLevel(JNIEnv *env, jclass clazz, jint level)
{
    MPTRACE("%s(%d)\n", __func__, level);
    oarmp_global_set_log_level(level);
    LOGD("moncleanup\n");
}


static JNINativeMethod g_methods[] = {
        {
                "_setDataSource",
                                        "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;)V",
                                                                                              (void *) OARMediaPlayer_setDataSourceAndHeaders
        },
        { "_setVideoSurface",       "(Landroid/view/Surface;)V", (void *) OARMediaPlayer_setVideoSurface },
        { "_prepareAsync",          "()V",      (void *) OARMediaPlayer_prepareAsync },
        { "_start",                 "()V",      (void *) OARMediaPlayer_start },
        { "_stop",                  "()V",      (void *) OARMediaPlayer_stop },
        { "seekTo",                 "(J)V",     (void *) OARMediaPlayer_seekTo },
        { "_pause",                 "()V",      (void *) OARMediaPlayer_pause },
        { "isPlaying",              "()Z",      (void *) OARMediaPlayer_isPlaying },
        { "getCurrentPosition",     "()J",      (void *) OARMediaPlayer_getCurrentPosition },
        { "getDuration",            "()J",      (void *) OARMediaPlayer_getDuration },
        { "_release",               "()V",      (void *) OARMediaPlayer_release },
        { "_reset",                 "()V",      (void *) OARMediaPlayer_reset },
        { "setVolume",              "(FF)V",    (void *) OARMediaPlayer_setVolume },
        { "getAudioSessionId",      "()I",      (void *) OARMediaPlayer_getAudioSessionId },
        { "native_init",            "()V",      (void *) OARMediaPlayer_native_init },
        { "native_setup",           "(Ljava/lang/Object;)V", (void *) OARMediaPlayer_native_setup },
        { "native_finalize",        "()V",      (void *) OARMediaPlayer_native_finalize },

        { "_setOption",             "(ILjava/lang/String;Ljava/lang/String;)V", (void *) OARMediaPlayer_setOption },
        { "_setOption",             "(ILjava/lang/String;J)V",                  (void *) OARMediaPlayer_setOptionLong },

        { "_getColorFormatName",    "(I)Ljava/lang/String;",    (void *) OARMediaPlayer_getColorFormatName },
        { "_getVideoCodecInfo",     "()Ljava/lang/String;",     (void *) OARMediaPlayer_getVideoCodecInfo },
        { "_getAudioCodecInfo",     "()Ljava/lang/String;",     (void *) OARMediaPlayer_getAudioCodecInfo },
        { "_getMediaMeta",          "()Landroid/os/Bundle;",    (void *) OARMediaPlayer_getMediaMeta },
        { "_setLoopCount",          "(I)V",                     (void *) OARMediaPlayer_setLoopCount },
        { "_getLoopCount",          "()I",                      (void *) OARMediaPlayer_getLoopCount },
        { "_getPropertyFloat",      "(IF)F",                    (void *) OARMediaPlayer_getPropertyFloat },
        { "_setPropertyFloat",      "(IF)V",                    (void *) OARMediaPlayer_setPropertyFloat },
        { "_getPropertyLong",       "(IJ)J",                    (void *) OARMediaPlayer_getPropertyLong },
        { "_setPropertyLong",       "(IJ)V",                    (void *) OARMediaPlayer_setPropertyLong },
        { "_setStreamSelected",     "(IZ)V",                    (void *) OARMediaPlayer_setStreamSelected },

        { "native_profileBegin",    "(Ljava/lang/String;)V",    (void *) OARMediaPlayer_native_profileBegin },
        { "native_profileEnd",      "()V",                      (void *) OARMediaPlayer_native_profileEnd },

        { "native_setLogLevel",     "(I)V",                     (void *) OARMediaPlayer_native_setLogLevel },
};

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv* env = NULL;

    g_jvm = vm;
    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    assert(env != NULL);

    pthread_mutex_init(&g_clazz.mutex, NULL );

    // FindClass returns LocalReference
    OAR_FIND_JAVA_CLASS(env, g_clazz.clazz, JNI_CLASS_OARPLAYER);
    (*env)->RegisterNatives(env, g_clazz.clazz, g_methods, NELEM(g_methods) );



    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *jvm, void *reserved)
{

    pthread_mutex_destroy(&g_clazz.mutex);
}

static void OAR_JNI_ThreadDestroyed(void* value)
{
    JNIEnv *env = (JNIEnv*) value;
    if (env != NULL) {
        LOGE("%s: [%d] didn't call SDL_JNI_DetachThreadEnv() explicity\n", __func__, (int)gettid());
        (*g_jvm)->DetachCurrentThread(g_jvm);
        pthread_setspecific(g_thread_key, NULL);
    }
}
static void make_thread_key()
{
    pthread_key_create(&g_thread_key, OAR_JNI_ThreadDestroyed);
}

jint OAR_JNI_SetupThreadEnv(JNIEnv **p_env)
{
    JavaVM *jvm = g_jvm;
    if (!jvm) {
        LOGE("SDL_JNI_GetJvm: AttachCurrentThread: NULL jvm");
        return -1;
    }

    pthread_once(&g_key_once, make_thread_key);

    JNIEnv *env = (JNIEnv*) pthread_getspecific(g_thread_key);
    if (env) {
        *p_env = env;
        return 0;
    }

    if ((*jvm)->AttachCurrentThread(jvm, &env, NULL) == JNI_OK) {
        pthread_setspecific(g_thread_key, env);
        *p_env = env;
        return 0;
    }

    return -1;
}
void OAR_JNI_DetachThreadEnv()
{
    JavaVM *jvm = g_jvm;

    LOGI("%s: [%d]\n", __func__, (int)gettid());

    pthread_once(&g_key_once, make_thread_key);

    JNIEnv *env = pthread_getspecific(g_thread_key);
    if (!env)
        return;
    pthread_setspecific(g_thread_key, NULL);

    if ((*jvm)->DetachCurrentThread(jvm) == JNI_OK)
        return;

    return;
}
void monstartup(const char *libname)
{
    __android_log_print(ANDROID_LOG_DEBUG, "aprof-fake", "fake-monstartup %s\n", libname);
}

void moncleanup(void)
{
    __android_log_print(ANDROID_LOG_DEBUG, "aprof-fake", "fake-momcleanup\n");
}
