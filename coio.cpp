/*
 * Copyright (c) 2015 Moritz Bitsch <moritzbitsch@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#define _POSIX_C_SOURCE 200809L

#include <iostream>
#include <math.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "coioimpl.h"

#if defined(__APPLE__)
#include <mach/clock.h>
#include <mach/mach.h>
#endif

CoioTaskList coio_ready_list = { 0, 0 };
CoioTaskList coio_sleeping = { 0, 0 };
cothread_t coio_sched_ctx;
CoioTask* coio_current = NULL;
unsigned long coio_taskcount = 0;

static int
msleep(uvlong ms)
{
	struct timespec req, rem;

	if (ms > 999) {
		req.tv_sec = (int)(ms / 1000);
		req.tv_nsec = (ms - ((long)req.tv_sec * 1000)) * 1000000;
	}
	else {
		req.tv_sec = 0;
		req.tv_nsec = ms * 1000000;
	}

	return nanosleep(&req, &rem);
}

static void
_process_events()
{
	uvlong now;
	int ms = 5;
	CoioTask* t;

	if ((t = coio_sleeping.head) != NULL && t->timeout != 0) {
		now = coio_now();
		if (now >= t->timeout) {
			ms = 0;
		}
		else {
			ms = (t->timeout - now);
		}
	}
	/* TODO:do I/O polling instead of usleep */
	msleep(ms);

	/* handle CLOCK_MONOTONIC bugs (VirtualBox anyone?) */
	while (!coio_ready_list.head) {
		/* wake up timed out tasks */
		now = coio_now();
		while ((t = coio_sleeping.head) && t->timeout && now >= t->timeout) {
			coio_ready(t);
		}
	}
}

void
coio_init()
{
	/* initialize empty ctx for scheduler */
	coio_sched_ctx = co_active();
}

int
coio_main()
{
	coio_init();

	/* scheduler mainloop */
	for (;;) {
		if (!coio_ready_list.head && coio_sleeping.head)
			_process_events();

		if (!coio_ready_list.head)
			break;

		coio_current = coio_ready_list.head;
		coio_current->ready = 0;
		coio_del(&coio_ready_list, coio_current);
		co_switch(coio_current->ctx);

		if (coio_current->eptr) {
			std::rethrow_exception(coio_current->eptr);
		}

		if (coio_current->done) {
			coio_taskcount--;
			co_delete(coio_current->ctx);
			delete coio_current;
		}
		coio_current = nullptr;
	}

	if (coio_taskcount) {
		return -1;
	}
	return 0;
}

static CoioTask* construction_task = nullptr;

int
coio_create(const char* name, coio_func f, unsigned int stacksize)
{
	CoioTask* task;

	task = new CoioTask();
	if (!task)
		return -1;

	task->name = name;
	task->func = f;

	construction_task = task;

	task->ctx = co_create(stacksize, []() {
		auto task = construction_task;
		co_switch(coio_sched_ctx);
		try {
			task->func();
		} catch (...) {
			task->eptr = std::current_exception();
		}
		task->done = 1;
		co_switch(coio_sched_ctx);
	});

	co_switch(task->ctx);

	coio_add(&coio_ready_list, task);
	coio_taskcount++;

	return 0;
}

uvlong
coio_timeout(CoioTask* task, int ms)
{
	CoioTask* t;

	if (ms >= 0) {
		task->timeout = coio_now() + ms;
		for (t = coio_sleeping.head;
		     t != NULL && t->timeout && t->timeout < task->timeout;
		     t = t->next)
			;
	}
	else {
		task->timeout = 0;
		t = NULL;
	}

	if (t) {
		task->prev = t->prev;
		task->next = t;
	}
	else {
		task->prev = coio_sleeping.tail;
		task->next = NULL;
	}

	t = coio_current;

	if (t->prev) {
		t->prev->next = t;
	}
	else {
		coio_sleeping.head = t;
	}

	if (t->next) {
		t->next->prev = t;
	}
	else {
		coio_sleeping.tail = t;
	}

	return task->timeout;
}

int
coio_delay(int ms)
{
	uvlong when;
	when = coio_timeout(coio_current, ms);
	coio_transfer();
	return (coio_now() - when);
}

void
coio_yield()
{
	coio_ready(coio_current);
	coio_transfer();
}

void
coio_add(CoioTaskList* lst, CoioTask* task)
{
	if (lst->tail) {
		lst->tail->next = task;
		task->prev = lst->tail;
	}
	else {
		lst->head = task;
		task->prev = NULL;
	}
	lst->tail = task;
	task->next = NULL;
}

void
coio_del(CoioTaskList* lst, CoioTask* task)
{
	if (task->prev) {
		task->prev->next = task->next;
	}
	else if (lst->head == task) {
		lst->head = task->next;
	}

	if (task->next) {
		task->next->prev = task->prev;
	}
	else if (lst->tail == task) {
		lst->tail = task->prev;
	}
}

void
coio_ready(CoioTask* task)
{
	task->timeout = 0;
	if (!task->ready) {
		task->ready = 1;
		coio_del(&coio_sleeping, task);
		coio_add(&coio_ready_list, task);
	}
}

void
coio_transfer()
{
	co_switch(coio_sched_ctx);
}

uvlong
coio_now()
{
#if defined(__APPLE__)
	clock_serv_t cclock;
	mach_timespec_t ts;

	host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
	clock_get_time(cclock, &ts);
	mach_port_deallocate(mach_task_self(), cclock);
#else
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
		return -1;
#endif

	return (uvlong)ts.tv_sec * 1000 + round(ts.tv_nsec / 1000000.0);
}

void
coio_debug()
{
	CoioTask* t;

	fprintf(stderr, ">>>\nCurrent tasks: %s\n", coio_current->name);

	fprintf(stderr, "Sleeping tasks: ");
	for (t = coio_sleeping.head; t != NULL; t = t->next) {
		fprintf(stderr, "%s ", t->name);
	}

	fprintf(stderr, "\nReady tasks: ");
	for (t = coio_ready_list.head; t != NULL; t = t->next) {
		fprintf(stderr, "%s ", t->name);
	}
	fprintf(stderr, "\n");
}

bool
coio_active()
{
	return coio_current != nullptr;
}

void
coio_active_throw()
{
	if (!coio_active()) {
		throw std::logic_error("Not inside coroutine");
	}
}

CoioMutex::CoioMutex() {
	owner = nullptr;
}

void
CoioMutex::lock() {
	if (owner == nullptr) {
		owner = coio_current;
	} else {
		waiting.push_back(coio_current);
		coio_delay(-1);
	}
}

void
CoioMutex::unlock() {
	if (!waiting.empty()) {
		owner = waiting.back();
		waiting.pop_back();
		coio_ready(owner);
	} else {
		owner = nullptr;
	}
}
