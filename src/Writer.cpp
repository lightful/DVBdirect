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

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory>
#include <cerrno>
#include <cstring>
#include <sys++/String.hpp>
#include "Config.hpp"
#include "Writer.h"

#define NOTIF_FREQ 5 // seconds

template <> void Writer::onMessage(std::shared_ptr<Config>& config)
{
    outputFile = config->outputFile;
    timerStart(true, std::chrono::seconds(NOTIF_FREQ), TimerCycle::Periodic);
}

template <> void Writer::onMessage(std::shared_ptr<Buffer>& buffer)
{
    bool written = false;

    if ((fd < 0) && !outputFile.empty())
    {
        if ((fd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0664)) < 0)
        {
            std::string problem = strerror(errno);
            writeNotif({ "open error", VA_STR(": " << problem << " for '" << outputFile << "'") });
            inError = true;
        }
    }

    if (fd >= 0)
    {
        auto bytes = write(fd, buffer->start(), buffer->length());
        written = (bytes == (decltype(bytes)) buffer->length());
        if (!written)
        {
            inError = true;
            std::string problem = strerror(errno);
            writeNotif({ "write error", VA_STR(": " << problem << char(7)) }); // disk full?
            if (bytes > 0) buffer->advance(bytes);
        }
        else if (inError)
        {
            inError = false;
            onTimer(true);
            writeNotif({ "error recovered", " - no data lost" });
        }
    }

    if (!written)
    {
        if (buffer->size * pendingMessages() < 786432000) throw DispatchRetry(); // less than 750 Mb (5 minutes)
        writeNotif({ "buffer overrun", " - discarding data" });
    }
}

template <> void Writer::onMessage(Notif& notif) // errors from other threads
{
    writeNotif(notif);
}

void Writer::writeNotif(const Notif& notif)
{
    auto nowIs = std::chrono::steady_clock::now();
    auto pnotif = notifications.find(notif);
    bool print = pnotif == notifications.end();
    if (!print)
    {
        if (nowIs - pnotif->second > std::chrono::seconds(NOTIF_FREQ))
        {
            notifications.erase(pnotif);
            print = true;
        }
    }
    if (print)
    {
        notifications.emplace(notif, nowIs);
        if (write(STDERR_FILENO, notif.subject.c_str(), notif.subject.length())) {}
        if (write(STDERR_FILENO, notif.msg.c_str(), notif.msg.length())) {}
        if (write(STDERR_FILENO, "\n", 1)) {}
    }
}

void Writer::onTimer(const bool&)
{
    std::size_t undispatched = pendingMessages();

    if (dispatchBusy && (undispatched < 3))
    {
        writeNotif({ "queue ok", " - no data pending to write" });
        dispatchBusy = false;
    }

    if (undispatched > 100)
    {
        dispatchBusy = true;
        writeNotif({ "queue warning", VA_STR(": pending to write (" << undispatched << " messages)") });
    }
}
