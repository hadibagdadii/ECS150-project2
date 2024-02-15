#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "uthread.h"
#include "queue.h"

// Declare static variables for thread_queue and blocked_queue
static queue_t thread_queue = NULL;
static queue_t blocked_queue = NULL;

struct uthread_tcb {
	uthread_ctx_t context;
    void *stack;
};

struct uthread_tcb *uthread_current(void)
{
	/* Phase 2/3: Return the current thread's TCB */
    return queue_length(thread_queue) > 0 ? (struct uthread_tcb *)queue_peek(thread_queue) : NULL;
}

void uthread_yield(void)
{
	/* Phase 2: Save the current thread's context and switch to the next thread */
    struct uthread_tcb *current_thread = uthread_current();
    if (current_thread != NULL) {
        struct uthread_tcb *next_thread = queue_dequeue(thread_queue);
        if (next_thread != NULL) {
            uthread_ctx_switch(&current_thread->context, &next_thread->context);
        }
        queue_enqueue(thread_queue, current_thread);
    }
}

void uthread_exit(void)
{
	/* Phase 2: Remove the current thread from the queue and free its resources */
    struct uthread_tcb *current_thread = uthread_current();
    if (current_thread != NULL) {
        queue_delete(thread_queue, current_thread);
        uthread_ctx_destroy_stack(current_thread->stack);
        free(current_thread);
    }
    uthread_yield(); // Yield to the next thread
}

int uthread_create(uthread_func_t func, void *arg)
{
	/* Phase 2: Create a new thread and initialize its context */
    struct uthread_tcb *new_thread = malloc(sizeof(struct uthread_tcb));
    if (new_thread == NULL) {
        return -1; // Memory allocation failed
    }

    new_thread->stack = uthread_ctx_alloc_stack();
    if (new_thread->stack == NULL) {
        free(new_thread);
        return -1; // Stack allocation failed
    }

    // Initialize context for the new thread
    if (uthread_ctx_init(&new_thread->context, new_thread->stack, (uthread_func_t)func, arg) == -1) {
        uthread_ctx_destroy_stack(new_thread->stack);
        free(new_thread);
        return -1; // Context initialization failed
    }

    // Add the new thread to the queue
    queue_enqueue(thread_queue, new_thread);

    return 0; // Success
}

int uthread_run(bool preempt, uthread_func_t func, void *arg)
{
	/* Phase 2: Initialize the thread queue and start preemption */
    thread_queue = queue_create();
    if (thread_queue == NULL) {
        return -1; // Queue creation failed
    }

    preempt_start(preempt);

    // Create the initial thread
    if (uthread_create(func, arg) == -1) {
        preempt_stop();
        queue_destroy(thread_queue);
        return -1; // Initial thread creation failed
    }

    // Start executing threads
    while (queue_length(thread_queue) > 0) {
        uthread_yield();
    }

    // All threads finished executing
    preempt_stop();
    queue_destroy(thread_queue);

    return 0; // Success
}

void uthread_block(void)
{
	/* Phase 3: Block the currently running thread */
    struct uthread_tcb *current_thread = uthread_current();
    if (current_thread != NULL) {
        queue_enqueue(blocked_queue, current_thread);
        uthread_yield();
    }
}

void uthread_unblock(struct uthread_tcb *uthread)
{
	/* Phase 3: Unblock the specified thread */
    queue_delete(blocked_queue, uthread);
    queue_enqueue(thread_queue, uthread);
}
