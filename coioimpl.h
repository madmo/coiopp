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
#ifndef COIOIMPL_H
#define COIOIMPL_H

#include "coio.h"
#include "libco/libco.h"

#include <exception>

typedef struct CoioTaskList CoioTaskList;

struct CoioTask
{
	cothread_t ctx;

	const char* name;
	coio_func func;

	uvlong timeout;
	int ready;
	int done;

	std::exception_ptr eptr;

	/* linked list support */
	CoioTask* next;
	CoioTask* prev;
};

struct CoioTaskList
{
	CoioTask* head;
	CoioTask* tail;
};

extern CoioTaskList coio_ready_list;
extern CoioTaskList coio_sleeping;
extern cothread_t coio_sched_ctx;
extern unsigned long coio_taskcount;

void
coio_add(CoioTaskList* lst, CoioTask* task);
void
coio_del(CoioTaskList* lst, CoioTask* task);
void
coio_transfer();
void
coio_debug();

#endif
