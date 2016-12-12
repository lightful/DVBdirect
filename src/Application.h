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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <sys++/ActorThread.hpp>
#include "Config.hpp"
#include "DVBReceptor.h"

class Application : public ActorThread<Application>
{
    friend ActorThread<Application>;

    Application(int cmdArgc, char** cmdArgv) : argc(cmdArgc), argv(cmdArgv) {}

    void onStart();

    template <typename Any> void onMessage(Any&);
    template <typename Any> void onTimer(const Any&);

    const int argc;
    char** const argv;

    std::shared_ptr<Config> config;
    DVBReceptor::ptr dvbReceptor;
};

#endif /* APPLICATION_H */
