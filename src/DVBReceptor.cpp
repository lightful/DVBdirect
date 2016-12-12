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

#include <linux/ioctl.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <stropts.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <sys++/String.hpp>
#include "Config.hpp"
#include "Application.h"
#include "DVBReceptor.h"

#define BUFFER_SIZE 65536  // about 26 milliseconds worth of data

template <> void DVBReceptor::onMessage(std::shared_ptr<Config>& config)
{
    writer->send(config);

    frontend_fd = open(VA_STR("/dev/dvb/adapter" << config->adapter << "/frontend" << config->frontend).c_str(), O_RDWR);

    if (frontend_fd < 0)
    {
        fatalProblem("FATAL: error opening frontend");
        return;
    }

    struct dtv_property cmdv;
    memset(&cmdv, 0, sizeof(dtv_property));
    cmdv.cmd = DTV_API_VERSION;
    struct dtv_properties cmdvseq = { 1, &cmdv };
    if (ioctl(frontend_fd, FE_GET_PROPERTY, &cmdvseq) < 0)
    {
        fatalProblem("FATAL: DVB driver doesn't support DVB API v5");
        return;
    }

    auto avoidable = [](const Config::Property& x) { return (x.code == DTV_CLEAR) || (x.code == DTV_TUNE); };
    std::remove_if(config->properties.begin(), config->properties.end(), avoidable);

    auto findbw = [](const Config::Property& x) { return (x.code == DTV_BANDWIDTH_HZ); };

    if (std::find_if(config->properties.begin(), config->properties.end(), findbw) == config->properties.end())
        config->properties.push_back({ DTV_BANDWIDTH_HZ, 8000000 }); // default value

    config->properties.push_front({ DTV_CLEAR, DTV_UNDEFINED }); // ensured only at the beginning
    config->properties.push_back({ DTV_TUNE, DTV_UNDEFINED });   // ensured only at the end

    struct dtv_property* cmds = new dtv_property[config->properties.size()];
    memset(cmds, 0, sizeof(dtv_property) * config->properties.size());
    struct dtv_properties cmdseq = { 0, cmds };

    for (const auto& property : config->properties)
    {
        cmds[cmdseq.num].cmd = property.code;
        cmds[cmdseq.num].u.data = property.value;
        cmdseq.num++;
    }

    bool badConfigure = ioctl(frontend_fd, FE_SET_PROPERTY, &cmdseq) < 0;
    delete []cmds;

    if (badConfigure)
    {
        fatalProblem("FATAL: error configuring frontend");
        return;
    }

    fe_status_t fe_status;
    for (int i = 1; i < 120; i++)
    {
        ioctl(frontend_fd, FE_READ_STATUS, &fe_status );
        if (fe_status & FE_HAS_LOCK) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    if (!(fe_status & FE_HAS_LOCK))
    {
        fatalProblem("FATAL: could not tune", ETIMEDOUT);
        return;
    }

    if (config->pids.empty()) config->pids.emplace_back(0x2000); // "wildcard" PID (full MPEG stream)

    for (auto pid : config->pids)
    {
        int demux_fd = open(VA_STR("/dev/dvb/adapter" << config->adapter << "/demux" << config->demux).c_str(), O_RDWR);

        if (demux_fd < 0)
        {
            fatalProblem("FATAL: error opening demux");
            return;
        }

        demux_fds.emplace_back(demux_fd);

        dmx_pes_filter_params pesfilter;
        pesfilter.pid = pid;
        pesfilter.input = DMX_IN_FRONTEND;
        pesfilter.output = DMX_OUT_TS_TAP;
        pesfilter.pes_type = DMX_PES_OTHER;
        pesfilter.flags = DMX_IMMEDIATE_START;

        if (ioctl(demux_fd, DMX_SET_PES_FILTER, &pesfilter) < 0)
        {
            fatalProblem("FATAL: error configuring demux");
            return;
        }
    }

    dvr_fd = open(VA_STR("/dev/dvb/adapter" << config->adapter << "/dvr" << config->dvr).c_str(), O_RDONLY);

    if (dvr_fd < 0)
    {
        fatalProblem("FATAL: error opening dvr");
        return;
    }

    timerStart(true, std::chrono::seconds(0), TimerCycle::Periodic); // indefinite reception loop
}

template <> void DVBReceptor::onTimer(const bool&)
{
    auto buffer = std::make_shared<Buffer>(BUFFER_SIZE);
    auto bytes = read(dvr_fd, buffer->data, buffer->size);

    if (bytes <= 0) fatalProblem("error receiving data");
    else
    {
        buffer->setLength(bytes);
        writer->send(buffer);
    }
}

void DVBReceptor::onStop()
{
    writer->waitIdle();
    app.reset();
}

void DVBReceptor::fatalProblem(const char* subject, int unknownError)
{
    std::string problem = strerror(errno? errno : unknownError);
    writer->send<true>(Notif { subject, VA_STR(": " << problem) });
    app->send(DVBFatal{});
    timerStop(true);
}
