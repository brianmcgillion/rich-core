/*
 * This file is part of sp-rich-core
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Brian McGillion <brian.mcgillion@symbio.com>, Denis Mingulov <denis.mingulov@symbio.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */
#include "procinterface.h"

#include <cstdio>
#include <errno.h>
#include <string.h>

ProcInterface::ProcInterface(int pid)
    :   m_pid(pid)
{
}

ProcInterface::~ProcInterface()
{
}

ADDRESS ProcInterface::heapAddress(const char *fileName) const
{
    const char *useFileName = NULL;
    char generatedFileName[128] = {0};
    if (fileName)
    {
        useFileName = fileName;
    }
    else
    {
        // filename is missing - use /proc/[pid]/maps
        sprintf(generatedFileName, "/proc/%d/maps", m_pid);
        useFileName = generatedFileName;
    }

    FILE *fd = fopen(useFileName, "rt");
    if (fd)
    {
        // TODO: remove precoded size
        char buf[256];
        while (!feof(fd))
        {
            if (fgets(buf, sizeof(buf), fd))
            {
                if(strstr(buf, "[heap]"))
                {
                    // correct line is found
                    ADDRESS addr = 0;
                    if(sscanf(buf, "%x", &addr)>0)
                    {
                        fclose(fd);
                        return addr;
                    }
                    break;
                }
            }
        }
        fclose(fd);
    }

    return 0;
}

const std::vector<ProcInterface::SharedObject> *ProcInterface::getSharedObjects(const char *fileName)
{
    // clean everything
    m_sharedObjects.clear();

    const char *useFileName = NULL;
    char generatedFileName[256] = {0};
    if (fileName)
    {
        useFileName = fileName;
    }
    else
    {
        // filename is missing - use /proc/pid/maps
        sprintf(generatedFileName, "/proc/%d/maps", m_pid);
        useFileName = generatedFileName;
    }

    FILE *fd = fopen(useFileName, "rt");
    if (fd)
    {
        char buf[256];
        while (!feof(fd))
        {
            // go through the file line by line
            if (fgets(buf, sizeof(buf), fd))
            {
                // check - does a line contain share object information?
                if (strstr(buf, "r-xp") && strstr(buf, ".so") && !strstr(buf, "(deleted)"))
                {
                    // correct line is found - get needed data
                    ADDRESS addr = 0;
                    sscanf(buf, "%x", &addr);
                    int len = strlen(buf);
                    if (len > 0)
                    {
                        // linefeed is the latest character
                        if (buf[len-1] > 0 && buf[len-1] < ' ')
                        {
                            buf[len-1] = 0;
                            len--;
                        }

                        int begin = 0;
                        for (int i = len-1; i >= 0; i--)
                        {
                            if (buf[i] > 0 && buf[i] <= ' ')
                            {
                                begin = i+1;
                                break;
                            }
                        }
                        if (begin > 0)
                        {
                            SharedObject so;
                            so.addr = addr;
                            so.name = buf+begin;

                            m_sharedObjects.push_back(so);
                        }
                    }
                }
            }
        }
        fclose(fd);
    }

    return &m_sharedObjects;
}
