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
#ifndef COIO_H
#define COIO_H

#include <functional>
#include <vector>

typedef struct CoioTask CoioTask;
typedef std::function<void()> coio_func;

typedef unsigned long long uvlong;

extern CoioTask* coio_current;

void
coio_init();
int
coio_main();
int
coio_create(const char* name, coio_func f, unsigned int stacksize);
void
coio_yield();
uvlong
coio_now();
int
coio_delay(int ms);
void
coio_ready(CoioTask* task);
bool
coio_active();
void
coio_active_throw();

class CoioMutex {
	CoioTask* owner;
	std::vector<CoioTask*> waiting;
public:
	CoioMutex();
	~CoioMutex() = default;

	CoioMutex(const CoioMutex&) = delete;
	CoioMutex& operator=(const CoioMutex&) = delete;

	void lock();
	void unlock();
};

#endif
