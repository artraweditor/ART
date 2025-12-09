/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2016 Adam Reichold <adam.reichold@t-online.de>
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
#include "threadutils.h"

void MyReaderLock::acquire()
{
    if (locked) {
        return;
    }

    std::unique_lock<std::mutex> lock(mutex.mutex);

    if (mutex.writerCount == 0) {
        // There's no writer operating, we can increment the writer count which
        // will lock writers.
        ++mutex.writerCount;
    } else if (mutex.readerCount == 0) {
        // The writer count is non null, but a reader can be the owner of the
        // writer lock, which will be the case if the reader count is not zero
        // too.
        while (mutex.writerCount != 0) {
            mutex.cond.wait(lock);
        }

        // Then, we can increment the writer count.
        ++mutex.writerCount;
    }

    // Finally, we can increment the reader count as well.
    ++mutex.readerCount;

    locked = true;
}

void MyReaderLock::release()
{
    if (!locked) {
        return;
    }

    std::unique_lock<std::mutex> lock(mutex.mutex);

    // decrement the writer number first...
    --mutex.readerCount;

    if (mutex.readerCount == 0) {
        // ...if no more reader, we decrement the writer count as well...
        --mutex.writerCount;

        // ...and signal the next waiting reader/writer that it's free
        mutex.cond.notify_all();
    }

    locked = false;
}

void MyWriterLock::acquire()
{
    if (locked) {
        return;
    }

    std::unique_lock<std::mutex> lock(mutex.mutex);

    // The writer count is not zero, so we have to wait for it to be zero
    // again...
    while (mutex.writerCount != 0) {
        mutex.cond.wait(lock);
    }

    // ...then we can increment the writer count.
    ++mutex.writerCount;

    locked = true;
}

void MyWriterLock::release()
{
    if (!locked) {
        return;
    }

    std::unique_lock<std::mutex> lock(mutex.mutex);

    // Decrement the writer number first...
    if (--mutex.writerCount == 0) {
        // ...and if the writer count is zero again, we can wake up the next
        // writer or reader.
        mutex.cond.notify_all();
    }

    locked = false;
}
