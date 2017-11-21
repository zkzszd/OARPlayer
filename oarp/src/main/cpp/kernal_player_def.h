//
// Created by 申俊伟 on 2017/9/18.
//

#ifndef OARPLAYER_KERNAL_PLAYER_DEF_H
#define OARPLAYER_KERNAL_PLAYER_DEF_H

#define EOAR_FAILED             -1
#define EOAR_OUT_OF_MEMORY      -2
#define EOAR_INVALID_STATE      -3
#define EOAR_NULL_IS_PTR        -4

//---------------for android system api---------------------
enum media_event_type {
    MEDIA_NOP               = 0,        // interface test message
    MEDIA_PREPARED          = 1,
    MEDIA_PLAYBACK_COMPLETE = 2,
    MEDIA_BUFFERING_UPDATE  = 3,        // arg1 = percentage, arg2 = cached duration
    MEDIA_SEEK_COMPLETE     = 4,
    MEDIA_SET_VIDEO_SIZE    = 5,        // arg1 = width, arg2 = height
    MEDIA_TIMED_TEXT        = 99,       // not supported yet
    MEDIA_ERROR             = 100,      // arg1, arg2
    MEDIA_INFO              = 200,      // arg1, arg2


    MEDIA_SET_VIDEO_SAR     = 10001,    // arg1 = sar.num, arg2 = sar.den
};

enum media_error_type {
    // 0xx
            MEDIA_ERROR_UNKNOWN = 1,
    // 1xx
            MEDIA_ERROR_SERVER_DIED = 100,
    // 2xx
            MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200,
    // 3xx

    // -xx
            MEDIA_ERROR_IO          = -1004,
    MEDIA_ERROR_MALFORMED   = -1007,
    MEDIA_ERROR_UNSUPPORTED = -1010,
    MEDIA_ERROR_TIMED_OUT   = -110,

    MEDIA_ERROR_IJK_PLAYER  = -10000,
};

enum media_info_type {
    // 0xx
            MEDIA_INFO_UNKNOWN = 1,
    // The player was started because it was used as the next player for another
    // player, which just completed playback
            MEDIA_INFO_STARTED_AS_NEXT = 2,
    // The player just pushed the very first video frame for rendering
            MEDIA_INFO_VIDEO_RENDERING_START = 3,
    // 7xx
    // The video is too complex for the decoder: it can't decode frames fast
    // enough. Possibly only the audio plays fine at this stage.
            MEDIA_INFO_VIDEO_TRACK_LAGGING = 700,
    // MediaPlayer is temporarily pausing playback internally in order to
    // buffer more data.
            MEDIA_INFO_BUFFERING_START = 701,
    // MediaPlayer is resuming playback after filling buffers.
            MEDIA_INFO_BUFFERING_END = 702,
    // Bandwidth in recent past
            MEDIA_INFO_NETWORK_BANDWIDTH = 703,

    // 8xx
    // Bad interleaving means that a media has been improperly interleaved or not
    // interleaved at all, e.g has all the video samples first then all the audio
    // ones. Video is playing but a lot of disk seek may be happening.
            MEDIA_INFO_BAD_INTERLEAVING = 800,
    // The media is not seekable (e.g live stream).
            MEDIA_INFO_NOT_SEEKABLE = 801,
    // New media metadata is available.
            MEDIA_INFO_METADATA_UPDATE = 802,

    //9xx
            MEDIA_INFO_TIMED_TEXT_ERROR = 900,

    //100xx
            MEDIA_INFO_VIDEO_ROTATION_CHANGED = 10001,
    MEDIA_INFO_AUDIO_RENDERING_START = 10002,
};


typedef struct ijkmp_mediacodecinfo_context
{
    char mime_type[128];    //< in
    int  profile;           //< in
    int  level;             //< in
    char codec_name[128];   //< out
} ijkmp_mediacodecinfo_context;



//-----------------------common-----------------------------
typedef struct Clock {
    double pts;           /* clock base */
    double pts_drift;     /* clock base minus time at which we updated the clock */
    double last_updated;
    double speed;
    int serial;           /* clock is based on a packet with this serial */
    int paused;
    int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};


typedef struct KernalPlayer {


} KernalPlayer;
#endif //OARPLAYER_KERNAL_PLAYER_DEF_H
