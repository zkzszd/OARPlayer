/*****************************************************************************
 * OAR_mutex.h
 *****************************************************************************
 *
 * Copyright (c) 2013 Bilibili
 * copyright (c) 2013 Zhang Rui <bbcallen@gmail.com>
 *
 * This file is part of ijkPlayer.
 *
 * ijkPlayer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * ijkPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ijkPlayer; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef OAR__OAR_MUTEX_H
#define OAR__OAR_MUTEX_H

#include <stdint.h>
#include <pthread.h>

#define SDL_MUTEX_TIMEDOUT  1
#define SDL_MUTEX_MAXWAIT   (~(uint32_t)0)

typedef struct OAR_mutex {
    pthread_mutex_t id;
} OAR_mutex;

OAR_mutex  *OAR_CreateMutex(void);
void        OAR_DestroyMutex(OAR_mutex *mutex);
void        OAR_DestroyMutexP(OAR_mutex **mutex);
int         OAR_LockMutex(OAR_mutex *mutex);
int         OAR_UnlockMutex(OAR_mutex *mutex);

typedef struct OAR_cond {
    pthread_cond_t id;
} OAR_cond;

OAR_cond   *OAR_CreateCond(void);
void        OAR_DestroyCond(OAR_cond *cond);
void        OAR_DestroyCondP(OAR_cond **mutex);
int         OAR_CondSignal(OAR_cond *cond);
int         OAR_CondBroadcast(OAR_cond *cond);
int         OAR_CondWaitTimeout(OAR_cond *cond, OAR_mutex *mutex, uint32_t ms);
int         OAR_CondWait(OAR_cond *cond, OAR_mutex *mutex);

#endif

