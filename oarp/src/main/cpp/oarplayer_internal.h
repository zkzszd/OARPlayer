//
// Created by 申俊伟 on 2017/9/14.
//

#ifndef OARPLAYER_OARPLAYER_INTERNAL_H
#define OARPLAYER_OARPLAYER_INTERNAL_H

#include <pthread.h>
#include "oar_thread.h"
#include "oarplayer.h"
struct OARMediaPlayer {
    volatile int ref_count;
    pthread_mutex_t mutex;

    KernalPlayer *kernalPlayer;

    int (*msg_loop)(void*);
    OAR_Thread *msg_thread;
    OAR_Thread _msg_thread;

    int mp_state;
    char *data_source;
    void *weak_thiz;
};

#endif //OARPLAYER_OARPLAYER_INTERNAL_H
