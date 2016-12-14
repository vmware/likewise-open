/*
 * Copyright (c) 2008, Likewise Software, Inc.
 * All rights reserved.
 */

/*
 * Copyright (c) 2007, Novell, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Novell, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dcethread-private.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#ifndef _WIN32
#include <sys/time.h>
#else
#include <sys/timeb.h>
#include <sys/types.h>
#endif

static pthread_once_t dcethread_init_once = DCETHREAD_ONCE_INIT;
static pthread_once_t dcethread_fini_once = DCETHREAD_ONCE_INIT;
static pthread_key_t dcethread_self_key;
static pthread_attr_t dcethread_attr_default;
static pthread_mutexattr_t dcethread_mutexattr_default;
static pthread_condattr_t dcethread_condattr_default;
static int dcethread_init_called;

#ifdef _WIN32
/* Use pthread_cond_signal() vs pthread_kill() during shutdown */
static int dcethread_terminate_signaled;
static pthread_mutex_t dcethread_terminate_mutex;
static pthread_cond_t dcethread_terminate_cond;
static SOCKET select_wake_socket;
#endif

#define _DCETHREAD_MS_DELAY_IN_NS 100000000
#ifdef SIGRTMIN
#    define INTERRUPT_SIGNAL (SIGRTMIN + 5)
#else
#    define INTERRUPT_SIGNAL (SIGXCPU)
#endif

#ifndef RPC_SOCKET_T
#ifdef _WIN32
#define RPC_SOCKET_T SOCKET
#else
#define RPC_SOCKET_T int
#endif
#endif

static RPC_SOCKET_T select_wake_socket;

RPC_SOCKET_T
dcethread_get_shutdown_sock(void)
{
    if (select_wake_socket == -1)
    {
        select_wake_socket = socket(AF_INET, SOCK_STREAM, 0);
    }
    return select_wake_socket;
}


static void
interrupt_signal_handler(int sig)
{
    dcethread* thread = dcethread__self();

    /* In asynchronous interrupt mode all bets
       are off anyway, so don't bother to lock */
    if (thread->flag.async)
    {
	dcethread__dispatchinterrupt(thread);
    }
}

static void
self_destructor(void* data)
{
    if (data)
    {
        dcethread* self = (dcethread*) data;
        dcethread__lock(self);
        dcethread__change_state(self, DCETHREAD_STATE_DEAD);
        dcethread__release(self);
        dcethread__unlock(self);
    }
}

#ifdef _WIN32
/*
 * Mimic shutdown semantics of UNIX using pthread_kill(). However, use
 * condition variables and wait until the terminate condition has
 * been set and signaled. Afterwards, call interrupt_signal_handler().
 */
static void *
terminate_thread_startroutine(void *args)
{
    pthread_mutex_lock(&dcethread_terminate_mutex);
    while (!dcethread_terminate_signaled)
    {
        pthread_cond_wait(&dcethread_terminate_cond, &dcethread_terminate_mutex);
    }
    pthread_mutex_unlock(&dcethread_terminate_mutex);

    // Signal value is ignored by handler
    interrupt_signal_handler(SIGTERM);
    return NULL;
}

#endif

static void
init(void)
{
    int cancelstate, oldstate;
    int sts;
#ifndef _WIN32
    struct sigaction act;
#else
    pthread_attr_t terminate_thread_attr;
    pthread_t terminate_thread;
#endif

    sts = pthread_key_create(&dcethread_self_key, self_destructor);
    assert(sts == 0); /* There is no resonable way to recover if this fails */

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancelstate);
    pthread_attr_init(&dcethread_attr_default);
    pthread_mutexattr_init(&dcethread_mutexattr_default);

#if defined(RPC_MUTEX_DEBUG) && defined(__USE_UNIX98)
    pthread_mutexattr_settype(&dcethread_mutexattr_default, PTHREAD_MUTEX_ERRORCHECK);
#endif
    pthread_condattr_init(&dcethread_condattr_default);
    dcethread__init_exceptions();
    pthread_setcancelstate(cancelstate, &oldstate);

#ifndef _WIN32
    sigemptyset(&act.sa_mask);
    act.sa_handler = interrupt_signal_handler;
    act.sa_flags = 0;
    sigaction(INTERRUPT_SIGNAL, &act, NULL);
#else
    pthread_mutex_init(&dcethread_terminate_mutex, NULL);
    pthread_cond_init(&dcethread_terminate_cond, NULL);
    pthread_attr_init(&terminate_thread_attr);
    pthread_attr_setdetachstate(&terminate_thread_attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&terminate_thread, &terminate_thread_attr,
                   terminate_thread_startroutine, NULL);
    select_wake_socket = -1;
#endif

    if (getenv("DCETHREAD_DEBUG"))
#if defined(_WIN32)
    {
        dcethread__debug_set_callback(dcethread__output_debug_string_log_callback, NULL);
    }
#else
    {
	dcethread__debug_set_callback(dcethread__default_log_callback, NULL);
    }
#endif
    dcethread_init_called = 1;
}


static void
fini(void)
{
    int cancelstate, oldstate;
#ifndef _WIN32
    struct sigaction act;
#else
#if 0
    pthread_attr_t terminate_thread_attr;
    pthread_t terminate_thread;
#endif
#endif

    if (!dcethread_init_called)
    {
        return;
    }
    pthread_key_delete(dcethread_self_key);
    dcethread_self_key = 0;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancelstate);
    pthread_attr_destroy(&dcethread_attr_default);
    pthread_mutexattr_destroy(&dcethread_mutexattr_default);
    pthread_condattr_destroy(&dcethread_condattr_default);
    dcethread__fini_exceptions();
    pthread_setcancelstate(cancelstate, &oldstate);

#ifndef _WIN32
    sigemptyset(&act.sa_mask);
    act.sa_handler = NULL;
    act.sa_flags = 0;
    sigaction(INTERRUPT_SIGNAL, &act, NULL);
#else
#if 0 /* Windows wakeup select mechanism */
    pthread_mutex_init(&dcethread_terminate_mutex, NULL);
    pthread_cond_init(&dcethread_terminate_cond, NULL);
    pthread_attr_init(&terminate_thread_attr);
    pthread_attr_setdetachstate(&terminate_thread_attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&terminate_thread, &terminate_thread_attr,
                   terminate_thread_startroutine, NULL);
    select_wake_socket = -1;
#endif
#endif
}

#ifdef __SUNPRO_C
#pragma init dcethread__init
#else
void dcethread__init(void) DCETHREAD_ATTR_CONSTRUCTOR;
void dcethread__fini(void) DCETHREAD_ATTR_DESTRUCTOR;
#endif

void dcethread__init(void)
{
    pthread_once(&dcethread_init_once, init);
}

void dcethread__fini(void)
{
    pthread_once(&dcethread_fini_once, fini);
}

int
dcethread__interrupt_syscall(dcethread* thread, void* data)
{
#ifndef _WIN32
    /*
     * TBD: Adam-Investigate getting rid of pthread_kill()
     * and use generic mechanism below
     */
    pthread_kill(thread->pthread, INTERRUPT_SIGNAL);
#else

    DCETHREAD_TRACE("dcethread__interrupt_syscall: Thread %p: called", thread);
    pthread_mutex_lock(&dcethread_terminate_mutex);
    dcethread_terminate_signaled = 1;
    pthread_mutex_unlock(&dcethread_terminate_mutex);
    pthread_cond_signal(&dcethread_terminate_cond);
    if (select_wake_socket != -1)
    {
        closesocket(select_wake_socket);
        select_wake_socket = -1;
        DCETHREAD_TRACE("dcethread__interrupt_syscall: "
                        "Thread %p: select_wake_socket=%d",
                        thread, select_wake_socket);
    }

#endif
    return 0;
}

static int
my_clock_gettime(struct timespec* tp)
{
#ifdef CLOCK_REALTIME
  return clock_gettime(CLOCK_REALTIME, tp);
#else

#ifdef _WIN32

    struct _timeb timev;
    _ftime(&timev);
    tp->tv_sec = timev.time;
    tp->tv_nsec = timev.millitm * 1000;
    return 0;
    
#else

  int result;
  struct timeval tv;

  if ((result = gettimeofday(&tv, NULL)))
    return result;

  tp->tv_sec = tv.tv_sec;
  tp->tv_nsec = tv.tv_usec * 1000;

  return 0;
#endif
#endif
}

int
dcethread__interrupt_condwait(dcethread* thread, void* data)
{
    condwait_info* info = (condwait_info*) data;

    if (pthread_equal(info->mutex->owner, pthread_self()))
    {
        DCETHREAD_TRACE("Thread %p: already owned mutex used for interrupt", thread);
        if (pthread_cond_broadcast(info->cond))
        {
            DCETHREAD_ERROR("Thread %p: broadcast failed", thread);
            return 0;
        }
        else
        {
            DCETHREAD_TRACE("Thread %p: broadcast to interrupt condwait", thread);
            return 1;
        }
    }
    else if (!pthread_mutex_trylock((pthread_mutex_t*) &info->mutex->mutex))
    {
        info->mutex->owner = pthread_self();
        if (pthread_cond_broadcast(info->cond))
        {
            DCETHREAD_ERROR("Thread %p: broadcast failed", thread);
#ifndef _WIN32
            info->mutex->owner = (pthread_t) -1;
#else
            memset((void *) &info->mutex->owner, -1, sizeof(info->mutex->owner));
#endif
            pthread_mutex_unlock((pthread_mutex_t*) &info->mutex->mutex);
            return 0;
        }
        else
        {
            DCETHREAD_TRACE("Thread %p: broadcast to interrupt condwait", thread);
#ifndef _WIN32
            info->mutex->owner = (pthread_t) -1;
#else
            memset((void *) &info->mutex->owner, -1, sizeof(info->mutex->owner));
#endif
            pthread_mutex_unlock((pthread_mutex_t*) &info->mutex->mutex);
            return 1;
        }
    }
    else
    {
        DCETHREAD_VERBOSE("Thread %p: could not acquire lock to interrupt condwait", thread);
        return 0;
    }
}

dcethread*
dcethread__new (void)
{
    dcethread* thread;

    thread = calloc(1, sizeof(dcethread));

    thread->refs = 1;
    thread->flag.interruptible = 1;
    thread->flag.joinable = 0;
    thread->flag.async = 0;
    thread->state = DCETHREAD_STATE_CREATED;

    pthread_mutex_init((pthread_mutex_t*) &thread->lock, NULL);
    pthread_cond_init((pthread_cond_t*) &thread->state_change, NULL);

    /* Set default interrupt handler that throws an interrupt exception */
    thread->handle_interrupt = dcethread__exc_handle_interrupt;
    thread->handle_interrupt_data = (void*) &dcethread_interrupt_e;

    /* Set default interrupt method that pokes the thread with a signal */
    thread->interrupt = dcethread__interrupt_syscall;
    thread->interrupt_data = NULL;
    DCETHREAD_TRACE("dcethread_new: %p interrupt %p", thread, thread->interrupt);

    return thread;
}

dcethread*
dcethread__self(void)
{
    dcethread* thread;

    /* Ensure thread system is initialized */
    dcethread__init();

    /* Get self pointer from TLS */
    thread = (dcethread*) pthread_getspecific(dcethread_self_key);
    if (!thread)
    {
	/* Lazily create it if it didn't already exist */
	thread = dcethread__new();
	thread->pthread = pthread_self();
	pthread_setspecific(dcethread_self_key, (void*) thread);
	thread->state = DCETHREAD_STATE_ACTIVE;
    }

    return thread;
}

void
dcethread__init_self(dcethread* thread)
{
    /* Ensure thread system is initialized */
    dcethread__init();

    pthread_setspecific(dcethread_self_key, (void*) thread);
    dcethread__lock(thread);
    dcethread__change_state(thread, DCETHREAD_STATE_ACTIVE);
    dcethread__unlock(thread);
}

static void
dcethread__sanity(dcethread* thread)
{
    if (!thread)
        DCETHREAD_ERROR("NULL thread encountered");
    if (thread->refs < 0)
        DCETHREAD_ERROR("Thread %p: ref count < 0", thread);
    if (!thread->flag.locked)
        DCETHREAD_ERROR("Thread %p: not locked when expected", thread);
    switch (thread->state)
    {
    case DCETHREAD_STATE_CREATED:
    case DCETHREAD_STATE_ACTIVE:
    case DCETHREAD_STATE_BLOCKED:
    case DCETHREAD_STATE_INTERRUPT:
        if (thread->refs == 0)
            DCETHREAD_ERROR("Thread %p: ref count = 0 in living thread");
        break;
    case DCETHREAD_STATE_DEAD:
        break;
    }
}

void
dcethread__delete(dcethread* thread)
{
    DCETHREAD_TRACE("Thread %p: deleted", thread);
    pthread_mutex_destroy((pthread_mutex_t*) &thread->lock);
    pthread_cond_destroy((pthread_cond_t*) &thread->state_change);

    /* There is no dcethread_join() API, so detach joinable threads */
    if (thread->flag.joinable)
        pthread_detach(thread->pthread);
    free((void*) thread);
}

void
dcethread__retain(dcethread* thread)
{
    dcethread__sanity(thread);
    if (thread->refs == 0)
    {
	DCETHREAD_ERROR("Attempted to retain freed thread %p", thread);
    }
    else
    {
	thread->refs++;
	DCETHREAD_TRACE("Thread %p: ref count increased to %i", thread, thread->refs);
    }
}

void
dcethread__release(dcethread* thread)
{
    dcethread__sanity(thread);
    if (thread->refs <= 0)
    {
	DCETHREAD_ERROR("Thread %p: attempted to release freed thread", thread);
    }
    else
    {
	thread->refs--;
	DCETHREAD_TRACE("Thread %p: ref count decreased to %i", thread, thread->refs);
    }
}

void
dcethread__lock(dcethread* thread)
{
    if (pthread_mutex_lock((pthread_mutex_t*) &thread->lock))
        DCETHREAD_ERROR("Thread %p: failed to lock mutex", thread);
    thread->flag.locked = 1;
    dcethread__sanity(thread);
    DCETHREAD_TRACE("Thread %p: locked", thread);
}

void
dcethread__unlock(dcethread* thread)
{
    unsigned int refs;

    dcethread__sanity(thread);

    /* Access reference count while thread is still locked
       in order to avoid race conditions */
    refs = thread->refs;

    thread->flag.locked = 0;
    if (pthread_mutex_unlock((pthread_mutex_t*) &thread->lock))
        DCETHREAD_ERROR("Thread %p: failed to unlock mutex", thread);
    DCETHREAD_TRACE("Thread %p: unlocked", thread);

    if (refs == 0)
    {
	dcethread__delete(thread);
    }
}

void
dcethread__wait(dcethread* thread)
{
    dcethread__sanity(thread);
    thread->flag.locked = 0;
    pthread_cond_wait((pthread_cond_t*) &thread->state_change,
                      (pthread_mutex_t*) &thread->lock);
    thread->flag.locked = 1;
}

void
dcethread__timedwait(dcethread* thread, struct timespec* ts)
{
    dcethread__sanity(thread);
    thread->flag.locked = 0;
    pthread_cond_timedwait((pthread_cond_t*) &thread->state_change,
                           (pthread_mutex_t*) &thread->lock, ts);
    thread->flag.locked = 1;
}

static const char*
state_name(int state)
{
#define CASE(token) case token: return #token
    switch (state)
    {
	CASE(DCETHREAD_STATE_CREATED);
	CASE(DCETHREAD_STATE_ACTIVE);
	CASE(DCETHREAD_STATE_BLOCKED);
	CASE(DCETHREAD_STATE_INTERRUPT);
	CASE(DCETHREAD_STATE_DEAD);
    }
    return "UNKNOWN";
#undef CASE
}

void
dcethread__change_state(dcethread* thread, int state)
{
    DCETHREAD_TRACE("Thread %p: state transition %s -> %s",
		thread,
		state_name(thread->state),
		state_name(state));
    thread->state = state;
    pthread_cond_broadcast((pthread_cond_t*) &thread->state_change);
}

#ifndef HAVE_PTHREAD_LOCK_GLOBAL_NP
static pthread_mutex_t dcethread_g_global_lock = PTHREAD_MUTEX_INITIALIZER;
#endif /* HAVE_PTHREAD_LOCK_GLOBAL_NP */

void dcethread__lock_global(void)
{
#ifdef HAVE_PTHREAD_LOCK_GLOBAL_NP
    pthread_lock_global_np();
#else
    pthread_mutex_lock(&dcethread_g_global_lock);
#endif /* HAVE_PTHREAD_LOCK_GLOBAL_NP */
}

void dcethread__unlock_global(void)
{
#ifdef HAVE_PTHREAD_UNLOCK_GLOBAL_NP
    pthread_unlock_global_np();
#else
    pthread_mutex_unlock(&dcethread_g_global_lock);
#endif /* HAVE_PTHREAD_UNLOCK_GLOBAL_NP */
}

void
dcethread__dispatchinterrupt(dcethread* thread)
{
    DCETHREAD_TRACE("Thread %p: interrupt acknowledged", thread);
    thread->handle_interrupt(thread, thread->handle_interrupt_data);
}

void
dcethread__interrupt(dcethread* thread)
{
    int count = 0;
    int old_state = thread->state;

    if (old_state == DCETHREAD_STATE_INTERRUPT ||
        old_state == DCETHREAD_STATE_DEAD)
    {
        /* Don't bother */
        return;
    }

    DCETHREAD_TRACE("Thread %p: interrupt posted", thread);
    dcethread__change_state(thread, DCETHREAD_STATE_INTERRUPT);

    /* We need to poke the thread and wait for an acknowledgement of the interrupt if: */
    if (thread != dcethread__self() &&         /* The interrupted thread is not us, and */
        thread->flag.interruptible &&          /* The thread can be interrupted, and */
        old_state == DCETHREAD_STATE_BLOCKED)  /* The thread was blocked */
    {
        /* FIXME: potential livelock here if another thread
           re-interrupts when the lock is released */
        while (thread->state == DCETHREAD_STATE_INTERRUPT)
        {
            struct timespec waittime;

            if (count > 2)
                DCETHREAD_WARNING("Thread %p: still not interrupted after %i ms", thread, count * 100);

            if (thread->interrupt(thread, thread->interrupt_data))
            {
                /* Interrupt is guaranteed to have succeeded, so
                   leave state change wait loop */
                break;
            }

            count++;

            my_clock_gettime(&waittime);
            waittime.tv_nsec += _DCETHREAD_MS_DELAY_IN_NS;

            if (waittime.tv_nsec > _DCETHREAD_MS_DELAY_IN_NS)
            {
	       waittime.tv_nsec -= _DCETHREAD_MS_DELAY_IN_NS;
	       waittime.tv_sec += 1;
	    }

            /* Wait for state change */
            dcethread__timedwait(thread, &waittime);
        }
    }
    DCETHREAD_TRACE("Thread %p: interrupt completed!!", thread);
}

void
dcethread__set_interrupt_handler(dcethread* thread, void (*handle_interrupt)(dcethread*, void*), void* data)
{
    thread->handle_interrupt = handle_interrupt;
    thread->handle_interrupt_data = data;
}

int
dcethread__begin_block(dcethread* thread, int (*interrupt)(dcethread*, void*), void* data,
                       int (**old_interrupt)(dcethread*, void*), void** old_data)
{
    int state;
    int interruptible;

    dcethread__lock(thread);
    state = thread->state;
    interruptible = thread->flag.interruptible;
    /* If thread is currently active */
    if (state == DCETHREAD_STATE_ACTIVE)
    {
        DCETHREAD_TRACE("dcethread__begin_block: interrupt=%p old interrupt=%p",
                        interrupt, thread->interrupt);
	/* Set up interruption callbacks */
	if (old_interrupt)
	    *old_interrupt = thread->interrupt;
	if (old_data)
	    *old_data = thread->interrupt_data;
	if (interrupt)
	    thread->interrupt = interrupt;
	if (data)
	    thread->interrupt_data = data;
	
	/* Change to blocked state */
	dcethread__change_state(thread, DCETHREAD_STATE_BLOCKED);
    }
    /* If an interrupt request has been posted (and we can be interrupted) */
    else if (state == DCETHREAD_STATE_INTERRUPT && interruptible)
    {
	/* Clear request */
	dcethread__change_state(thread, DCETHREAD_STATE_ACTIVE);
    }
    dcethread__unlock(thread);

    return state == DCETHREAD_STATE_INTERRUPT && interruptible;
}

int
dcethread__poll_end_block(dcethread* thread, int (*interrupt)(dcethread*, void*), void* data)
{
    int state;
    int interruptible;

    dcethread__lock(thread);
    state = thread->state;
    interruptible = thread->flag.interruptible;

    if (state == DCETHREAD_STATE_INTERRUPT)
    {
        DCETHREAD_TRACE("dcethread__poll_end_block: interrupt=%p old interrupt=%p",
                        interrupt, thread->interrupt);
        if (interrupt)
            thread->interrupt = interrupt;
        if (data)
            thread->interrupt_data = data;
        if ((interruptible && state == DCETHREAD_STATE_INTERRUPT) ||
            (state == DCETHREAD_STATE_BLOCKED))
            dcethread__change_state(thread, DCETHREAD_STATE_ACTIVE);
    }

    dcethread__unlock(thread);

    return state == DCETHREAD_STATE_INTERRUPT && interruptible;
}

int
dcethread__end_block(dcethread* thread, int (*interrupt)(dcethread*, void*), void* data)
{
    int state;
    int interruptible;

    dcethread__lock(thread);
    state = thread->state;
    interruptible = thread->flag.interruptible;

    /* Switch state back to active if:
       - We were not interrupted during the block
       - We were interrupted and the thread is interruptible

       If we were interrupted but are not currently interruptible,
       we want to leave the state alone so that is is caught when
       interruptiblility is enabled again.
    */
    if ((interruptible && state == DCETHREAD_STATE_INTERRUPT) ||
        (state == DCETHREAD_STATE_BLOCKED))
    {
        DCETHREAD_TRACE("dcethread__end_block: interrupt=%p old interrupt=%p",
                        interrupt, thread->interrupt);
        if (interrupt)
            thread->interrupt = interrupt;
        if (data)
            thread->interrupt_data = data;
        dcethread__change_state(thread, DCETHREAD_STATE_ACTIVE);
    }

    dcethread__unlock(thread);

    return state == DCETHREAD_STATE_INTERRUPT && interruptible;
}

void
dcethread__cleanup_self(dcethread* self)
{
    dcethread__change_state(self, DCETHREAD_STATE_DEAD);
    dcethread__release(self);
    pthread_setspecific(dcethread_self_key, NULL);
}
