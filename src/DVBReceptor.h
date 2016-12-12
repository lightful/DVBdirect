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

#ifndef DVBRECEPTOR_H
#define DVBRECEPTOR_H

#include <cerrno>
#include <list>
#include <sys++/ActorThread.hpp>
#include "Writer.h"

struct DVBFatal {};

class DVBReceptor : public ActorThread<DVBReceptor>
{
    friend ActorThread<DVBReceptor>;

    DVBReceptor(const std::shared_ptr<class Application>& parent) : app(parent), writer(Writer::create()) {}

    template <typename Any> void onMessage(Any&);
    template <typename Any> void onTimer(const Any&);

    void onStop();

    void fatalProblem(const char* subject, int unknownError = EINVAL);

    ActorThread<class Application>::ptr app;

    Writer::ptr writer;

    int frontend_fd;
    std::list<int> demux_fds;
    int dvr_fd;
};

#endif /* DVBRECEPTOR_H */
