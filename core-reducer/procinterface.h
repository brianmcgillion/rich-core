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

/*!
  * \file procinterface.h
  * \author Brian McGillion <brian.mcgillion@symbio.com>, Denis Mingulov <denis.mingulov@symbio.com>
  * \class ProcInterface
  * \brief Provide a wrapper around tasks that interact with the /proc file structure
  * \sa man 5 proc
  */

#ifndef PROCINTERFACE_H
#define PROCINTERFACE_H

#include "defines.h"
#include <string>
#include <vector>

class ProcInterface
{
public:
    /*!
      * \brief Default constructor
      * \param pid The pid of the process for which a /proc/$pid mapping is to be created
      * \sa man 5 proc
      */
    ProcInterface(int pid);

    /*!
      * \brief Default destructor
      */
    ~ProcInterface();

    /*!
      * \brief Get a pointer to a heap used by the process
      * \param fileName File name of the maps file, if NULL - by default /proc/[pid]/maps will be used
      * \returns A pointer to the heap.
      * Use the /proc/[pid]/maps file to get [heap] section.
      */
    ADDRESS heapAddress(const char *fileName=NULL) const;

    /*!
      * \brief Structure for any shared object (dynamic load library)
      * Use the /proc/[pid]/maps file to get [heap] section.
      */
    struct SharedObject
    {
        ADDRESS addr;
        std::string name;
    };

    /*!
      * \brief Get a list of shared objects used by the process
      * \param fileName File name of the maps file, if NULL - by default /proc/[pid]/maps will be used
      * \returns A pointer to vector with shared objects
      * Use the /proc/[pid]/maps (or /proc/[pid]/smaps) file to get shared object list
      */
    const std::vector<SharedObject> *getSharedObjects(const char *mapsFile=NULL);

private:
    //! The process id of the Process to monitor
    int m_pid;
    //! A vector of shared objects that are used
    std::vector<SharedObject> m_sharedObjects;
};

#endif // PROCINTERFACE_H
