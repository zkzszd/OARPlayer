//
// Created by 申俊伟 on 2017/9/14.
//

#ifndef OARPLAYER_OARPLAYER_H_H
#define OARPLAYER_OARPLAYER_H_H

#include "kernal_player_def.h"
#include "_android.h"

#ifndef MPTRACE
#define MPTRACE LOGD
#endif

#include <jni.h>
#include <stdbool.h>
#include "kernal_player_def.h"
#include "oarplayer_internal.h"
#include "oar_msg_queue.h"

typedef struct OARMediaPlayer OARMediaPlayer;

typedef struct ijkmp_android_media_format_context {
    const char *mime_type;
    int         profile;
    int         level;
} ijkmp_android_media_format_context;

// ref_count is 1 after open
OARMediaPlayer *oarmp_android_create(int(*msg_loop)(void*));

//-------------- for android system api-------------------
void oarmp_android_set_surface(JNIEnv *env, OARMediaPlayer *mp, jobject android_surface);
void oarmp_android_set_volume(JNIEnv *env, OARMediaPlayer *mp, float left, float right);
int  oarmp_android_get_audio_session_id(JNIEnv *env, OARMediaPlayer *mp);
void oarmp_android_set_mediacodec_select_callback(OARMediaPlayer *mp, bool (*callback)(void *opaque, ijkmp_mediacodecinfo_context *mcc), void *opaque);


//----------------- common -------------------

void            oarmp_global_init();
void            oarmp_global_uninit();
void            oarmp_global_set_log_report(int use_report);
void            oarmp_global_set_log_level(int log_level);   // log_level = AV_LOG_xxx
void            oarmp_global_set_inject_callback(oar_inject_callback cb);
const char     *oarmp_version_ident();
unsigned int    oarmp_version_int();


// ref_count is 1 after open
OARMediaPlayer *oarmp_create(int (*msg_loop)(void*));
void            oarmp_set_inject_opaque(OARMediaPlayer *mp, void *opaque);

void            oarmp_set_option(OARMediaPlayer *mp, int opt_category, const char *name, const char *value);
void            oarmp_set_option_int(OARMediaPlayer *mp, int opt_category, const char *name, int64_t value);

int             oarmp_get_video_codec_info(OARMediaPlayer *mp, char **codec_info);
int             oarmp_get_audio_codec_info(OARMediaPlayer *mp, char **codec_info);
void            oarmp_set_playback_rate(OARMediaPlayer *mp, float rate);
int             oarmp_set_stream_selected(OARMediaPlayer *mp, int stream, int selected);

float           oarmp_get_property_float(OARMediaPlayer *mp, int id, float default_value);
void            oarmp_set_property_float(OARMediaPlayer *mp, int id, float value);
int64_t         oarmp_get_property_int64(OARMediaPlayer *mp, int id, int64_t default_value);
void            oarmp_set_property_int64(OARMediaPlayer *mp, int id, int64_t value);




// must be freed with free();
OARMediaPlayer   *oarmp_get_meta_l(OARMediaPlayer *mp);

// preferred to be called explicity, can be called multiple times
// NOTE: ijkmp_shutdown may block thread
void            oarmp_shutdown(OARMediaPlayer *mp);

void            oarmp_inc_ref(OARMediaPlayer *mp);
void            oarmp_dec_ref_p(OARMediaPlayer **pmp);

int             oarmp_prepare_async(OARMediaPlayer *mp);
int             oarmp_start(OARMediaPlayer *mp);
int             oarmp_pause(OARMediaPlayer *mp);
int             oarmp_stop(OARMediaPlayer *mp);
int             oarmp_seek_to(OARMediaPlayer *mp, long msec);
int             oarmp_get_state(OARMediaPlayer *mp);
bool            oarmp_is_playing(OARMediaPlayer *mp);
long            oarmp_get_current_position(OARMediaPlayer *mp);
long            oarmp_get_duration(OARMediaPlayer *mp);
long            oarmp_get_playable_duration(OARMediaPlayer *mp);
void            oarmp_set_loop(OARMediaPlayer *mp, int loop);
int             oarmp_get_loop(OARMediaPlayer *mp);

void           *oarmp_get_weak_thiz(OARMediaPlayer *mp);
void           *oarmp_set_weak_thiz(OARMediaPlayer *mp, void *weak_thiz);

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
int             oarmp_get_msg(OARMediaPlayer *mp, AVMessage *msg, int block);

#endif //OARPLAYER_OARPLAYER_H_H
