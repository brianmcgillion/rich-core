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

#include "reducer.h"
#include "rawelfwriter.h"
#include "elfcorereader.h"

#include <iostream>
#include <stdlib.h>

#include "../config.h"

void printUsage(char *progName)
{
    std::cout << "\n\nUsage:" << std::endl;
    std::cout << "\t" << progName << " [-options]" << std::endl;
    std::cout << "Options:\n"
            "\t-i input core\n"
            "\t-o output core\n"
            "\t-e executable\n"
            "\t[-a memory address]\n"
            "\t[-m maps file]\n"
            "\t[-s]";
    std::cout << std::endl;
}

int main(int argc, char **argv)
{
    char *progName = argv[0];
    const char *inputFile = NULL;
    char *outFile = NULL;
    char *executable = NULL;
    char *mapsFile = NULL;
    ADDRESS heapAddress = 0;
    bool stacksOnlyMode = false;
    int c;

    while ((c = getopt(argc, argv, "hsi:o:e:a:m:")) != -1)
    {
        switch (c)
        {
        case 'i':
            inputFile = optarg;
            break;
        case 'o':
            outFile = optarg;
            break;
        case 'e':
            executable = optarg;
            break;
        case 'm':
            mapsFile = optarg;
            break;
        case 'a':
            heapAddress = strtol(optarg, NULL, 16);
            break;
        case 's':
            // stacks only mode - copy only the stacks and notes sections from the origional core file
            //so we will have no debug information in the output file
            stacksOnlyMode = true;
            break;
        case 'h':
            printUsage(progName);
            return -1;
            break;
        default:
            printUsage(progName);
            return -1;
            break;
        }
    }

    if (!outFile || !executable || !inputFile)
    {
        //There has been an error parsing some args so assume user error
        printUsage(progName);
        return -1;
    }

    Reducer *reducer = new Reducer(outFile, heapAddress);

    if (!reducer->initalize(inputFile, executable))
    {
        delete(reducer);
        return -1;
    }

    reducer->run(stacksOnlyMode, mapsFile);

    delete(reducer);
    return 0;
}
