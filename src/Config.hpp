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

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <memory>
#include <string>
#include <cstdint>
#include <list>

struct Config
{
    struct Property
    {
        uint32_t code;
        uint32_t value;
    };

    Config() : adapter(0), frontend(0), demux(0), dvr(0) {}
    uint32_t adapter;
    uint32_t frontend;
    uint32_t demux;
    uint32_t dvr;
    std::string outputFile;
    std::list<Property> properties;
    std::list<uint16_t> pids;
};

#endif /* CONFIG_HPP */
