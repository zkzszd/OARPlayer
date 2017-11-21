/*
 * Copyright (C) 2006 Bilibili
 * Copyright (C) 2006 The Android Open Source Project
 * Copyright (C) 2013 Zhang Rui <bbcallen@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.wodekouwei.oarplayer;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.SurfaceTexture;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.view.Surface;
import android.view.SurfaceHolder;

import com.wodekouwei.oarplayer.annotations.AccessedByNative;
import com.wodekouwei.oarplayer.annotations.CalledByNative;
import com.wodekouwei.oarplayer.util.LogUtil;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.security.InvalidParameterException;


/**
 * @author bbcallen
 *
 *         Java wrapper of ffplay.
 */
public final class OARMediaPlayer extends AbstractMediaPlayer {
    private final static String TAG = OARMediaPlayer.class.getName();

    private static final int MEDIA_NOP = 0; // interface test message
    private static final int MEDIA_PREPARED = 1;
    private static final int MEDIA_PLAYBACK_COMPLETE = 2;
    private static final int MEDIA_BUFFERING_UPDATE = 3;
    private static final int MEDIA_SEEK_COMPLETE = 4;
    private static final int MEDIA_SET_VIDEO_SIZE = 5;
    private static final int MEDIA_TIMED_TEXT = 99;
    private static final int MEDIA_ERROR = 100;
    private static final int MEDIA_INFO = 200;

    protected static final int MEDIA_SET_VIDEO_SAR = 10001;

    //----------------------------------------
    // options
    public static final int OAR_LOG_UNKNOWN = 0;
    public static final int OAR_LOG_DEFAULT = 1;

    public static final int OAR_LOG_VERBOSE = 2;
    public static final int OAR_LOG_DEBUG = 3;
    public static final int OAR_LOG_INFO = 4;
    public static final int OAR_LOG_WARN = 5;
    public static final int OAR_LOG_ERROR = 6;
    public static final int OAR_LOG_FATAL = 7;
    public static final int OAR_LOG_SILENT = 8;

    public static final int OPT_CATEGORY_FORMAT     = 1;
    public static final int OPT_CATEGORY_CODEC      = 2;
    public static final int OPT_CATEGORY_SWS        = 3;
    public static final int OPT_CATEGORY_PLAYER     = 4;

    public static final int SDL_FCC_YV12 = 0x32315659; // YV12
    public static final int SDL_FCC_RV16 = 0x36315652; // RGB565
    public static final int SDL_FCC_RV32 = 0x32335652; // RGBX8888
    //----------------------------------------

    //----------------------------------------
    // properties
    public static final int PROP_FLOAT_VIDEO_DECODE_FRAMES_PER_SECOND       = 10001;
    public static final int PROP_FLOAT_VIDEO_OUTPUT_FRAMES_PER_SECOND       = 10002;
    public static final int FFP_PROP_FLOAT_PLAYBACK_RATE                    = 10003;

    public static final int FFP_PROP_INT64_SELECTED_VIDEO_STREAM            = 20001;
    public static final int FFP_PROP_INT64_SELECTED_AUDIO_STREAM            = 20002;
    public static final int FFP_PROP_INT64_SELECTED_TIMEDTEXT_STREAM        = 20011;

    public static final int FFP_PROP_INT64_VIDEO_DECODER                    = 20003;
    public static final int FFP_PROP_INT64_AUDIO_DECODER                    = 20004;
    public static final int     FFP_PROPV_DECODER_UNKNOWN                   = 0;
    public static final int     FFP_PROPV_DECODER_AVCODEC                   = 1;
    public static final int     FFP_PROPV_DECODER_MEDIACODEC                = 2;
    public static final int     FFP_PROPV_DECODER_VIDEOTOOLBOX              = 3;
    public static final int FFP_PROP_INT64_VIDEO_CACHED_DURATION            = 20005;
    public static final int FFP_PROP_INT64_AUDIO_CACHED_DURATION            = 20006;
    public static final int FFP_PROP_INT64_VIDEO_CACHED_BYTES               = 20007;
    public static final int FFP_PROP_INT64_AUDIO_CACHED_BYTES               = 20008;
    public static final int FFP_PROP_INT64_VIDEO_CACHED_PACKETS             = 20009;
    public static final int FFP_PROP_INT64_AUDIO_CACHED_PACKETS             = 20010;
    public static final int FFP_PROP_INT64_ASYNC_STATISTIC_BUF_BACKWARDS    = 20201;
    public static final int FFP_PROP_INT64_ASYNC_STATISTIC_BUF_FORWARDS     = 20202;
    public static final int FFP_PROP_INT64_ASYNC_STATISTIC_BUF_CAPACITY     = 20203;
    public static final int FFP_PROP_INT64_TRAFFIC_STATISTIC_BYTE_COUNT     = 20204;
    public static final int FFP_PROP_INT64_CACHE_STATISTIC_PHYSICAL_POS     = 20205;
    public static final int FFP_PROP_INT64_CACHE_STATISTIC_BUF_FORWARDS     = 20206;
    public static final int FFP_PROP_INT64_CACHE_STATISTIC_FILE_POS         = 20207;
    public static final int FFP_PROP_INT64_CACHE_STATISTIC_COUNT_BYTES      = 20208;
    public static final int FFP_PROP_INT64_BIT_RATE                         = 20100;
    public static final int FFP_PROP_INT64_TCP_SPEED                        = 20200;
    public static final int FFP_PROP_INT64_LATEST_SEEK_LOAD_DURATION        = 20300;
    //----------------------------------------

    @AccessedByNative
    private long mNativeMediaPlayer;
    @AccessedByNative
    private long mNativeMediaDataSource;

    @AccessedByNative
    private int mNativeSurfaceTexture;

    @AccessedByNative
    private int mListenerContext;

    private SurfaceHolder mSurfaceHolder;
    private EventHandler mEventHandler;
    private PowerManager.WakeLock mWakeLock = null;
    private boolean mScreenOnWhilePlaying;
    private boolean mStayAwake;

    private int mVideoWidth;
    private int mVideoHeight;
    private int mVideoSarNum;
    private int mVideoSarDen;

    private String mDataSource;


    private static volatile boolean mIsNativeInitialized = false;
    private static void initNativeOnce() {
        synchronized (OARMediaPlayer.class) {
            if (!mIsNativeInitialized) {
                native_init();
                mIsNativeInitialized = true;
            }
        }
    }

    /**
     * Default constructor. Consider using one of the create() methods for
     * synchronously instantiating a IjkMediaPlayer from a Uri or resource.
     * <p>
     * When done with the IjkMediaPlayer, you should call {@link #release()}, to
     * free the resources. If not released, too many IjkMediaPlayer instances
     * may result in an exception.
     * </p>
     */
    public OARMediaPlayer() {
        initPlayer();
    }
    private void initPlayer() {
        System.loadLibrary("oarp-lib");
        initNativeOnce();

        Looper looper;
        if ((looper = Looper.myLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else if ((looper = Looper.getMainLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else {
            mEventHandler = null;
        }

        /*
         * Native setup requires a weak reference to our object. It's easier to
         * create it here than in C++.
         */
        native_setup(new WeakReference<OARMediaPlayer>(this));
    }

    /*
     * Update the IjkMediaPlayer SurfaceTexture. Call after setting a new
     * display surface.
     */
    private native void _setVideoSurface(Surface surface);

    /**
     * Sets the {@link SurfaceHolder} to use for displaying the video portion of
     * the media.
     *
     * Either a surface holder or surface must be set if a display or video sink
     * is needed. Not calling this method or {@link #setSurface(Surface)} when
     * playing back a video will result in only the audio track being played. A
     * null surface holder or surface will result in only the audio track being
     * played.
     *
     * @param sh
     *            the SurfaceHolder to use for video display
     */
    @Override
    public void setDisplay(SurfaceHolder sh) {
        mSurfaceHolder = sh;
        Surface surface;
        if (sh != null) {
            surface = sh.getSurface();
        } else {
            surface = null;
        }
        _setVideoSurface(surface);
        updateSurfaceScreenOn();
    }

    /**
     * Sets the {@link Surface} to be used as the sink for the video portion of
     * the media. This is similar to {@link #setDisplay(SurfaceHolder)}, but
     * does not support {@link #setScreenOnWhilePlaying(boolean)}. Setting a
     * Surface will un-set any Surface or SurfaceHolder that was previously set.
     * A null surface will result in only the audio track being played.
     *
     * If the Surface sends frames to a {@link SurfaceTexture}, the timestamps
     * returned from {@link SurfaceTexture#getTimestamp()} will have an
     * unspecified zero point. These timestamps cannot be directly compared
     * between different media sources, different instances of the same media
     * source, or multiple runs of the same program. The timestamp is normally
     * monotonically increasing and is unaffected by time-of-day adjustments,
     * but it is reset when the position is set.
     *
     * @param surface
     *            The {@link Surface} to be used for the video portion of the
     *            media.
     */
    @Override
    public void setSurface(Surface surface) {
        if (mScreenOnWhilePlaying && surface != null) {
            LogUtil.w(TAG,
                    "setScreenOnWhilePlaying(true) is ineffective for Surface");
        }
        mSurfaceHolder = null;
        _setVideoSurface(surface);
        updateSurfaceScreenOn();
    }

    /**
     * Sets the data source (file-path or http/rtsp URL) to use.
     *
     * @param path
     *            the path of the file, or the http/rtsp URL of the stream you
     *            want to play
     * @throws IllegalStateException
     *             if it is called in an invalid state
     *
     *             <p>
     *             When <code>path</code> refers to a local file, the file may
     *             actually be opened by a process other than the calling
     *             application. This implies that the pathname should be an
     *             absolute path (as any other process runs with unspecified
     *             current working directory), and that the pathname should
     *             reference a world-readable file.
     */
    @Override
    public void setDataSource(String path)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        mDataSource = path;
        _setDataSource(path, null, null);
    }


    private native void _setDataSource(String path, String[] keys, String[] values)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException;

    @Override
    public String getDataSource() {
        return mDataSource;
    }

    @Override
    public void prepareAsync() throws IllegalStateException {
        _prepareAsync();
    }

    public native void _prepareAsync() throws IllegalStateException;

    @Override
    public void start() throws IllegalStateException {
        stayAwake(true);
        _start();
    }

    private native void _start() throws IllegalStateException;

    @Override
    public void stop() throws IllegalStateException {
        stayAwake(false);
        _stop();
    }

    private native void _stop() throws IllegalStateException;

    @Override
    public void pause() throws IllegalStateException {
        stayAwake(false);
        _pause();
    }

    private native void _pause() throws IllegalStateException;

    @SuppressLint("Wakelock")
    @Override
    public void setWakeMode(Context context, int mode) {
        boolean washeld = false;
        if (mWakeLock != null) {
            if (mWakeLock.isHeld()) {
                washeld = true;
                mWakeLock.release();
            }
            mWakeLock = null;
        }

        PowerManager pm = (PowerManager) context
                .getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(mode | PowerManager.ON_AFTER_RELEASE,
                OARMediaPlayer.class.getName());
        mWakeLock.setReferenceCounted(false);
        if (washeld) {
            mWakeLock.acquire();
        }
    }

    @Override
    public void setScreenOnWhilePlaying(boolean screenOn) {
        if (mScreenOnWhilePlaying != screenOn) {
            if (screenOn && mSurfaceHolder == null) {
                LogUtil.w(TAG,
                        "setScreenOnWhilePlaying(true) is ineffective without a SurfaceHolder");
            }
            mScreenOnWhilePlaying = screenOn;
            updateSurfaceScreenOn();
        }
    }

    @SuppressLint("Wakelock")
    private void stayAwake(boolean awake) {
        if (mWakeLock != null) {
            if (awake && !mWakeLock.isHeld()) {
                mWakeLock.acquire();
            } else if (!awake && mWakeLock.isHeld()) {
                mWakeLock.release();
            }
        }
        mStayAwake = awake;
        updateSurfaceScreenOn();
    }

    private void updateSurfaceScreenOn() {
        if (mSurfaceHolder != null) {
            mSurfaceHolder.setKeepScreenOn(mScreenOnWhilePlaying && mStayAwake);
        }
    }

    private native void _setStreamSelected(int stream, boolean select);

    @Override
    public int getVideoWidth() {
        return mVideoWidth;
    }

    @Override
    public int getVideoHeight() {
        return mVideoHeight;
    }

    @Override
    public int getVideoSarNum() {
        return mVideoSarNum;
    }

    @Override
    public int getVideoSarDen() {
        return mVideoSarDen;
    }

    @Override
    public native boolean isPlaying();

    @Override
    public native void seekTo(long msec) throws IllegalStateException;

    @Override
    public native long getCurrentPosition();

    @Override
    public native long getDuration();

    /**
     * Releases resources associated with this IjkMediaPlayer object. It is
     * considered good practice to call this method when you're done using the
     * IjkMediaPlayer. In particular, whenever an Activity of an application is
     * paused (its onPause() method is called), or stopped (its onStop() method
     * is called), this method should be invoked to release the IjkMediaPlayer
     * object, unless the application has a special need to keep the object
     * around. In addition to unnecessary resources (such as memory and
     * instances of codecs) being held, failure to call this method immediately
     * if a IjkMediaPlayer object is no longer needed may also lead to
     * continuous battery consumption for mobile devices, and playback failure
     * for other applications if no multiple instances of the same codec are
     * supported on a device. Even if multiple instances of the same codec are
     * supported, some performance degradation may be expected when unnecessary
     * multiple instances are used at the same time.
     */
    @Override
    public void release() {
        stayAwake(false);
        updateSurfaceScreenOn();
        resetListeners();
        _release();
    }

    private native void _release();

    @Override
    public void reset() {
        stayAwake(false);
        _reset();
        // make sure none of the listeners get called anymore
        mEventHandler.removeCallbacksAndMessages(null);

        mVideoWidth = 0;
        mVideoHeight = 0;
    }

    private native void _reset();

    /**
     * Sets the player to be looping or non-looping.
     *
     * @param looping whether to loop or not
     */
    @Override
    public void setLooping(boolean looping) {
        int loopCount = looping ? 0 : 1;
        setOption(OPT_CATEGORY_PLAYER, "loop", loopCount);
        _setLoopCount(loopCount);
    }

    private native void _setLoopCount(int loopCount);

    /**
     * Checks whether the MediaPlayer is looping or non-looping.
     *
     * @return true if the MediaPlayer is currently looping, false otherwise
     */
    @Override
    public boolean isLooping() {
        int loopCount = _getLoopCount();
        return loopCount != 1;
    }

    private native int _getLoopCount();

    public void setSpeed(float speed) {
        _setPropertyFloat(FFP_PROP_FLOAT_PLAYBACK_RATE, speed);
    }

    public float getSpeed(float speed) {
        return _getPropertyFloat(FFP_PROP_FLOAT_PLAYBACK_RATE, .0f);
    }

    public int getVideoDecoder() {
        return (int)_getPropertyLong(FFP_PROP_INT64_VIDEO_DECODER, FFP_PROPV_DECODER_UNKNOWN);
    }

    public float getVideoOutputFramesPerSecond() {
        return _getPropertyFloat(PROP_FLOAT_VIDEO_OUTPUT_FRAMES_PER_SECOND, 0.0f);
    }

    public float getVideoDecodeFramesPerSecond() {
        return _getPropertyFloat(PROP_FLOAT_VIDEO_DECODE_FRAMES_PER_SECOND, 0.0f);
    }

    public long getVideoCachedDuration() {
        return _getPropertyLong(FFP_PROP_INT64_VIDEO_CACHED_DURATION, 0);
    }

    public long getAudioCachedDuration() {
        return _getPropertyLong(FFP_PROP_INT64_AUDIO_CACHED_DURATION, 0);
    }

    public long getVideoCachedBytes() {
        return _getPropertyLong(FFP_PROP_INT64_VIDEO_CACHED_BYTES, 0);
    }

    public long getAudioCachedBytes() {
        return _getPropertyLong(FFP_PROP_INT64_AUDIO_CACHED_BYTES, 0);
    }

    public long getVideoCachedPackets() {
        return _getPropertyLong(FFP_PROP_INT64_VIDEO_CACHED_PACKETS, 0);
    }

    public long getAudioCachedPackets() {
        return _getPropertyLong(FFP_PROP_INT64_AUDIO_CACHED_PACKETS, 0);
    }

    public long getAsyncStatisticBufBackwards() {
        return _getPropertyLong(FFP_PROP_INT64_ASYNC_STATISTIC_BUF_BACKWARDS, 0);
    }

    public long getAsyncStatisticBufForwards() {
        return _getPropertyLong(FFP_PROP_INT64_ASYNC_STATISTIC_BUF_FORWARDS, 0);
    }

    public long getAsyncStatisticBufCapacity() {
        return _getPropertyLong(FFP_PROP_INT64_ASYNC_STATISTIC_BUF_CAPACITY, 0);
    }

    public long getTrafficStatisticByteCount() {
        return _getPropertyLong(FFP_PROP_INT64_TRAFFIC_STATISTIC_BYTE_COUNT, 0);
    }

    public long getCacheStatisticPhysicalPos() {
        return _getPropertyLong(FFP_PROP_INT64_CACHE_STATISTIC_PHYSICAL_POS, 0);
    }

    public long getCacheStatisticBufForwards() {
        return _getPropertyLong(FFP_PROP_INT64_CACHE_STATISTIC_BUF_FORWARDS, 0);
    }

    public long getCacheStatisticFilePos() {
        return _getPropertyLong(FFP_PROP_INT64_CACHE_STATISTIC_FILE_POS, 0);
    }

    public long getCacheStatisticCountBytes() {
        return _getPropertyLong(FFP_PROP_INT64_CACHE_STATISTIC_COUNT_BYTES, 0);
    }

    public long getBitRate() {
        return _getPropertyLong(FFP_PROP_INT64_BIT_RATE, 0);
    }

    public long getTcpSpeed() {
        return _getPropertyLong(FFP_PROP_INT64_TCP_SPEED, 0);
    }

    public long getSeekLoadDuration() {
        return _getPropertyLong(FFP_PROP_INT64_LATEST_SEEK_LOAD_DURATION, 0);
    }

    private native float _getPropertyFloat(int property, float defaultValue);
    private native void  _setPropertyFloat(int property, float value);
    private native long  _getPropertyLong(int property, long defaultValue);
    private native void  _setPropertyLong(int property, long value);

    @Override
    public native void setVolume(float leftVolume, float rightVolume);

    @Override
    public native int getAudioSessionId();


    @Override
    public void setLogEnabled(boolean enable) {
        // do nothing
    }

    @Override
    public boolean isPlayable() {
        return true;
    }

    private native String _getVideoCodecInfo();
    private native String _getAudioCodecInfo();

    public void setOption(int category, String name, String value)
    {
        _setOption(category, name, value);
    }

    public void setOption(int category, String name, long value)
    {
        _setOption(category, name, value);
    }

    private native void _setOption(int category, String name, String value);
    private native void _setOption(int category, String name, long value);

    public Bundle getMediaMeta() {
        return _getMediaMeta();
    }
    private native Bundle _getMediaMeta();

    public static String getColorFormatName(int mediaCodecColorFormat) {
        return _getColorFormatName(mediaCodecColorFormat);
    }

    private static native String _getColorFormatName(int mediaCodecColorFormat);

    @Override
    public void setAudioStreamType(int streamtype) {
        // do nothing
    }

    @Override
    public void setKeepInBackground(boolean keepInBackground) {
        // do nothing
    }

    private static native void native_init();

    private native void native_setup(Object IjkMediaPlayer_this);

    private native void native_finalize();

    private native void native_message_loop(Object IjkMediaPlayer_this);

    protected void finalize() throws Throwable {
        super.finalize();
        native_finalize();
    }

    private static class EventHandler extends Handler {
        private final WeakReference<OARMediaPlayer> mWeakPlayer;

        public EventHandler(OARMediaPlayer mp, Looper looper) {
            super(looper);
            mWeakPlayer = new WeakReference<OARMediaPlayer>(mp);
        }

        @Override
        public void handleMessage(Message msg) {
            OARMediaPlayer player = mWeakPlayer.get();
            if (player == null || player.mNativeMediaPlayer == 0) {
                LogUtil.w(TAG,
                        "IjkMediaPlayer went away with unhandled events");
                return;
            }

            switch (msg.what) {
            case MEDIA_PREPARED:
                player.notifyOnPrepared();
                return;

            case MEDIA_PLAYBACK_COMPLETE:
                player.stayAwake(false);
                player.notifyOnCompletion();
                return;

            case MEDIA_BUFFERING_UPDATE:
                long bufferPosition = msg.arg1;
                if (bufferPosition < 0) {
                    bufferPosition = 0;
                }

                long percent = 0;
                long duration = player.getDuration();
                if (duration > 0) {
                    percent = bufferPosition * 100 / duration;
                }
                if (percent >= 100) {
                    percent = 100;
                }

                // LogUtil.efmt(TAG, "Buffer (%d%%) %d/%d",  percent, bufferPosition, duration);
                player.notifyOnBufferingUpdate((int)percent);
                return;

            case MEDIA_SEEK_COMPLETE:
                player.notifyOnSeekComplete();
                return;

            case MEDIA_SET_VIDEO_SIZE:
                player.mVideoWidth = msg.arg1;
                player.mVideoHeight = msg.arg2;
                player.notifyOnVideoSizeChanged(player.mVideoWidth, player.mVideoHeight,
                        player.mVideoSarNum, player.mVideoSarDen);
                return;

            case MEDIA_ERROR:
                LogUtil.e(TAG, "Error (" + msg.arg1 + "," + msg.arg2 + ")");
                if (!player.notifyOnError(msg.arg1, msg.arg2)) {
                    player.notifyOnCompletion();
                }
                player.stayAwake(false);
                return;

            case MEDIA_INFO:
                switch (msg.arg1) {
                    case MEDIA_INFO_VIDEO_RENDERING_START:
                        LogUtil.i(TAG, "Info: MEDIA_INFO_VIDEO_RENDERING_START\n");
                        break;
                }
                player.notifyOnInfo(msg.arg1, msg.arg2);
                // No real default action so far.
                return;
            case MEDIA_TIMED_TEXT:
                return;
            case MEDIA_NOP: // interface test message - ignore
                break;

            case MEDIA_SET_VIDEO_SAR:
                player.mVideoSarNum = msg.arg1;
                player.mVideoSarDen = msg.arg2;
                player.notifyOnVideoSizeChanged(player.mVideoWidth, player.mVideoHeight,
                        player.mVideoSarNum, player.mVideoSarDen);
                break;

            default:
                LogUtil.e(TAG, "Unknown message type " + msg.what);
            }
        }
    }

    /*
     * Called from native code when an interesting event happens. This method
     * just uses the EventHandler system to post the event back to the main app
     * thread. We use a weak reference to the original IjkMediaPlayer object so
     * that the native code is safe from the object disappearing from underneath
     * it. (This is the cookie passed to native_setup().)
     */
    @CalledByNative
    private static void postEventFromNative(Object weakThiz, int what,
                                            int arg1, int arg2, Object obj) {
        if (weakThiz == null)
            return;

        @SuppressWarnings("rawtypes")
        OARMediaPlayer mp = (OARMediaPlayer) ((WeakReference) weakThiz).get();
        if (mp == null) {
            return;
        }

        if (what == MEDIA_INFO && arg1 == MEDIA_INFO_STARTED_AS_NEXT) {
            // this acquires the wakelock if needed, and sets the client side
            // state
            mp.start();
        }
        if (mp.mEventHandler != null) {
            Message m = mp.mEventHandler.obtainMessage(what, arg1, arg2, obj);
            mp.mEventHandler.sendMessage(m);
        }
    }

    /*
     * ControlMessage
     */

    private OnControlMessageListener mOnControlMessageListener;
    public void setOnControlMessageListener(OnControlMessageListener listener) {
        mOnControlMessageListener = listener;
    }

    public interface OnControlMessageListener {
        String onControlResolveSegmentUrl(int segment);
    }

    /*
     * NativeInvoke
     */

    private OnNativeInvokeListener mOnNativeInvokeListener;
    public void setOnNativeInvokeListener(OnNativeInvokeListener listener) {
        mOnNativeInvokeListener = listener;
    }

    public interface OnNativeInvokeListener {

        int CTRL_WILL_TCP_OPEN = 0x20001;               // NO ARGS
        int CTRL_DID_TCP_OPEN = 0x20002;                // ARG_ERROR, ARG_FAMILIY, ARG_IP, ARG_PORT, ARG_FD

        int CTRL_WILL_HTTP_OPEN = 0x20003;              // ARG_URL, ARG_SEGMENT_INDEX, ARG_RETRY_COUNTER
        int CTRL_WILL_LIVE_OPEN = 0x20005;              // ARG_URL, ARG_RETRY_COUNTER
        int CTRL_WILL_CONCAT_RESOLVE_SEGMENT = 0x20007; // ARG_URL, ARG_SEGMENT_INDEX, ARG_RETRY_COUNTER

        int EVENT_WILL_HTTP_OPEN = 0x1;                 // ARG_URL
        int EVENT_DID_HTTP_OPEN = 0x2;                  // ARG_URL, ARG_ERROR, ARG_HTTP_CODE
        int EVENT_WILL_HTTP_SEEK = 0x3;                 // ARG_URL, ARG_OFFSET
        int EVENT_DID_HTTP_SEEK = 0x4;                  // ARG_URL, ARG_OFFSET, ARG_ERROR, ARG_HTTP_CODE

        String ARG_URL = "url";
        String ARG_SEGMENT_INDEX = "segment_index";
        String ARG_RETRY_COUNTER = "retry_counter";

        String ARG_ERROR = "error";
        String ARG_FAMILIY = "family";
        String ARG_IP = "ip";
        String ARG_PORT = "port";
        String ARG_FD = "fd";

        String ARG_OFFSET = "offset";
        String ARG_HTTP_CODE = "http_code";

        /*
         * @return true if invoke is handled
         * @throws Exception on any error
         */
        boolean onNativeInvoke(int what, Bundle args);
    }

    @CalledByNative
    private static boolean onNativeInvoke(Object weakThiz, int what, Bundle args) {
        LogUtil.ifmt(TAG, "onNativeInvoke %d", what);
        if (weakThiz == null || !(weakThiz instanceof WeakReference<?>))
            throw new IllegalStateException("<null weakThiz>.onNativeInvoke()");

        @SuppressWarnings("unchecked")
        WeakReference<OARMediaPlayer> weakPlayer = (WeakReference<OARMediaPlayer>) weakThiz;
        OARMediaPlayer player = weakPlayer.get();
        if (player == null)
            throw new IllegalStateException("<null weakPlayer>.onNativeInvoke()");

        OnNativeInvokeListener listener = player.mOnNativeInvokeListener;
        if (listener != null && listener.onNativeInvoke(what, args))
            return true;

        switch (what) {
            case OnNativeInvokeListener.CTRL_WILL_CONCAT_RESOLVE_SEGMENT: {
                OnControlMessageListener onControlMessageListener = player.mOnControlMessageListener;
                if (onControlMessageListener == null)
                    return false;

                int segmentIndex = args.getInt(OnNativeInvokeListener.ARG_SEGMENT_INDEX, -1);
                if (segmentIndex < 0)
                    throw new InvalidParameterException("onNativeInvoke(invalid segment index)");

                String newUrl = onControlMessageListener.onControlResolveSegmentUrl(segmentIndex);
                if (newUrl == null)
                    throw new RuntimeException(new IOException("onNativeInvoke() = <NULL newUrl>"));

                args.putString(OnNativeInvokeListener.ARG_URL, newUrl);
                return true;
            }
            default:
                return false;
        }
    }


    public void resetListeners() {
        super.resetListeners();
    }

    public static native void native_profileBegin(String libName);
    public static native void native_profileEnd();
    public static native void native_setLogLevel(int level);
}
