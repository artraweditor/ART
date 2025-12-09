/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <condition_variable>
#include <mutex>

#include "../rtengine/noncopyable.h"

class MyMutex: public std::mutex {
public:
    class MyLock;
    bool trylock() { return try_lock(); }
};

class MyMutex::MyLock: public std::unique_lock<MyMutex> {
public:
    explicit MyLock(MyMutex &mutex): std::unique_lock<MyMutex>(mutex) {}

    void acquire() { lock(); }
    bool try_acquire() { return try_lock(); }
    void release() { unlock(); }
};

class MyRWMutex: public rtengine::NonCopyable {
public:
    friend class MyReaderLock;
    friend class MyWriterLock;

private:
    std::mutex mutex;
    std::condition_variable cond;

    std::size_t writerCount = 0;
    std::size_t readerCount = 0;
};

class MyReaderLock: public rtengine::NonCopyable {
public:
    ~MyReaderLock();

    explicit MyReaderLock(MyRWMutex &mutex);

    void acquire();
    void release();

private:
    MyRWMutex &mutex;
    bool locked;
};

class MyWriterLock: public rtengine::NonCopyable {
public:
    ~MyWriterLock();

    explicit MyWriterLock(MyRWMutex &mutex);

    void acquire();
    void release();

private:
    MyRWMutex &mutex;
    bool locked;
};

inline MyReaderLock::MyReaderLock(MyRWMutex &mutex): mutex(mutex), locked(false)
{
    acquire();
}

inline MyWriterLock::MyWriterLock(MyRWMutex &mutex): mutex(mutex), locked(false)
{
    acquire();
}

inline MyReaderLock::~MyReaderLock()
{
    if (locked) {
        release();
    }
}

inline MyWriterLock::~MyWriterLock()
{
    if (locked) {
        release();
    }
}

#define MYREADERLOCK(ln, e) MyReaderLock ln(e);
#define MYWRITERLOCK(ln, e) MyWriterLock ln(e);
#define MYREADERLOCK_ACQUIRE(ln) ln.acquire();
#define MYWRITERLOCK_ACQUIRE(ln) ln.acquire();
#define MYREADERLOCK_RELEASE(ln) ln.release();
#define MYWRITERLOCK_RELEASE(ln) ln.release();
