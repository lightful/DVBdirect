/*
 *  DVB Jet - Utility to capture multiplexed MPEG-TS files from raw DVB sources
 *  Copyright 2016 Ciriaco Garcia de Celis
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WRITER_H
#define WRITER_H

#include <cstddef>
#include <string>
#include <chrono>
#include <map>
#include <sys++/ActorThread.hpp>

struct Buffer // standard C++11 containers have no opt-out for a wasteful default or zero initialization
{
    Buffer(std::size_t bytes) : data(new char[bytes]), size(bytes) {}
    virtual ~Buffer() { delete []data; }

    template <typename Number> void setLength(Number bytes)
    {
        offset = 0;
        lg = (bytes < 0)? 0 : (std::size_t(bytes) > size)? size : std::size_t(bytes);
    }

    template <typename Number> void advance(Number bytes)
    {
        if (bytes < 0) return;
        std::size_t shift = (std::size_t(bytes) > lg)? lg : std::size_t(bytes);
        offset += shift;
        lg -= shift;
    }

    char* start() { return data + offset; }

    std::size_t length() { return lg; }

    char* const data;
    const std::size_t size;

    std::size_t offset;
    std::size_t lg;
};

struct Notif
{
    std::string subject;
    std::string msg;
    bool operator<(const Notif& that) const { return subject < that.subject; }
};

class Writer : public ActorThread<Writer> // a dedicated thread so data is not lost when the disk is transiently busy
{
    friend ActorThread<Writer>;

    Writer() : fd(-1), inError(false), dispatchBusy(false) {}

    template <typename Any> void onMessage(Any&);
    void onTimer(const bool&);

    void writeNotif(const Notif& notif);

    std::string outputFile;
    int fd;
    bool inError;
    bool dispatchBusy;

    std::map<Notif, std::chrono::steady_clock::time_point> notifications;
};

#endif /* WRITER_H_ */
