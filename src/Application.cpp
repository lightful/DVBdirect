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

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <ctime>
#include <cstdlib>
#include "Application.h"

int main(int argc, char** argv)
{
    return Application::run(argc, argv);
}

void Application::onStart()
{
    if (argc <= 2)
    {
        std::cout
        << std::endl
        << "  dvbjet v1.0 - Copyright (C) 2016 Ciriaco Garcia de Celis" << std::endl
        << "  Utility to capture multiplexed MPEG-TS files from raw DVB sources" << std::endl
        << std::endl
        << "  Usage: " << argv[0] << " output.mts 3=frequency_value [OPTION=value]..." << std::endl
        << std::endl
        << "    - By default option 5 (DTV_BANDWIDTH_HZ) is set to 8000000 (8 MHz) used in Europe" << std::endl
        << "    - Many cards then only need the option 3 (DTV_FREQUENCY) to tune and receive data" << std::endl
        << "    - Frequencies can be found e.g. in channels.conf (or using the 'w_scan' utility)" << std::endl
        << std::endl
        << "  Device number selection (all default to 0):" << std::endl
        << "      adapter=A  for /dev/dvb/adapterA" << std::endl
        << "      frontend=B for /dev/dvb/adapterA/frontendB" << std::endl
        << "      demux=C    for /dev/dvb/adapterA/demuxC" << std::endl
        << "      dvr=D      for /dev/dvb/adapterA/dvrD" << std::endl
        << "  Recording options:" << std::endl
        << "      start=HH:MM    (current moment if omitted)" << std::endl
        << "      end=HH:MM      (until application is killed if omitted)" << std::endl
        << "      pids=N1,N2,... (using decimal, hexadecimal or octal numbers; by default all PIDs)" << std::endl
        << std::endl
        << "  Terrestrial and Satellite options (always numeric):" << std::endl
        << "      17 (DTV_DELIVERY_SYSTEM), 3 (DTV_FREQUENCY), 6 (DTV_INVERSION), 4 (DTV_MODULATION)" << std::endl
        << "  Terrestrial-only options:" << std::endl
        << "      5 (DTV_BANDWIDTH_HZ), 36 (DTV_CODE_RATE_HP), 37 (DTV_CODE_RATE_LP)," << std::endl
        << "      39 (DTV_TRANSMISSION_MODE), 38 (DTV_GUARD_INTERVAL), 40 (DTV_HIERARCHY)" << std::endl
        << "  Satellite-only options:" << std::endl
        << "      8 (DTV_SYMBOL_RATE), 9 (DTV_INNER_FEC), 12 (DTV_PILOT), 13 (DTV_ROLLOFF)" << std::endl
        << std::endl
        << "  See:" << std::endl
        << "    https://kernel.org/doc/html/latest/media/uapi/dvb/frontend-property-terrestrial-systems.html" << std::endl
        << "    https://kernel.org/doc/html/latest/media/uapi/dvb/frontend-property-satellite-systems.html" << std::endl
        << "    Note: use the enumeration values, never the names (see /usr/include/linux/dvb/frontend.h)" << std::endl
        << "    Note: DVB-T channels.conf (mplayer) indicates (in this order) 3:6:5:36:37:4:39:38:40" << std::endl
        << std::endl;
        stop(1);
    }
    else try
    {
        int32_t start = -1, end = -1;
        config = std::make_shared<Config>();
        config->outputFile = argv[1];
        for (int i = 2; i < argc; i++)
        {
            std::string option(argv[i]);
            auto eq = option.find("=");
            if ((eq == std::string::npos) || (eq == 0)) throw std::runtime_error("bad argument '" + option + "'");
            std::string code = option.substr(0, eq);
            std::string value = option.substr(eq+1);
            if (code == "adapter")
                { if (!(std::stringstream(value) >> config->adapter)) throw std::runtime_error("bad adapter number"); }
            else if (code == "frontend")
                { if (!(std::stringstream(value) >> config->frontend)) throw std::runtime_error("bad frontend number"); }
            else if (code == "demux")
                { if (!(std::stringstream(value) >> config->demux)) throw std::runtime_error("bad demux number"); }
            else if (code == "dvr")
                { if (!(std::stringstream(value) >> config->dvr)) throw std::runtime_error("bad dvr number"); }
            else if ((code == "start") || (code == "end"))
            {
                std::stringstream ph(value);
                int hh, mm;
                char delim = 0;
                if (!(ph >> hh)) hh = -1;
                ph >> delim;
                if (!(ph >> mm)) mm = -1;
                if ((hh < 0) || (hh > 23) || (mm < 0) || (mm > 59) || (delim != ':')) throw std::runtime_error("bad time");
                (code == "start"? start : end) = hh * 3600 + mm * 60;
            }
            else if (code == "pids")
            {
                std::istringstream pids(value);
                std::string pid;
                while (std::getline(pids, pid, ','))
                {
                    char *pend = nullptr;
                    auto pv = std::strtol(pid.c_str(), &pend, 0); // allows any base (std::ios::hex not even sets failbit!)
                    if (*pend || (pv < 0) || (pv > 65535)) throw std::runtime_error("bad pid '" + pid + "'");
                    config->pids.emplace_back(uint16_t(pv));
                }
            }
            else // numeric property
            {
                Config::Property p;
                if (!(std::stringstream(code) >> p.code)) throw std::runtime_error("bad code '" + code + "'");
                if (!(std::stringstream(value) >> p.value)) throw std::runtime_error("bad value '" + value + "'");
                config->properties.emplace_back(p);
            }
        }

        auto seconds = std::time(nullptr);
        struct tm* tmm = std::localtime(&seconds);
        auto nowIs = tmm->tm_hour * 3600 + tmm->tm_min * 60 + tmm->tm_sec;

        if ((start >= 0) && (start < nowIs)) start = nowIs - start > 59? start + 86400 : -1; // likely the user intention
        if ((end >= 0) && (end < nowIs)) end += 86400;
        if ((end >= 0) && (start >= 0) && (end < start)) end += 86400;

        timerStart(true, std::chrono::seconds(start < 0? 0 : start - nowIs));
        if (end >= 0) timerStart(false, std::chrono::seconds(end - nowIs));
    }
    catch (const std::runtime_error& err)
    {
        std::cerr << "ERROR: " << err.what() << std::endl;
        stop(2);
    }
}

template <> void Application::onMessage(DVBFatal&)
{
    dvbReceptor.reset();
    stop(3);
}

template <> void Application::onTimer(const bool& isStart)
{
    if (isStart)
    {
        dvbReceptor = DVBReceptor::create(shared_from_this());
        dvbReceptor->send(config);
    }
    else
    {
        dvbReceptor.reset();
        stop(0);
    }
}
