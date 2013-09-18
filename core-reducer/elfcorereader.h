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
  * \file elfcorereader.h
  * \author Brian McGillion <brian.mcgillion@symbio.com>, Denis Mingulov <denis.mingulov@symbio.com>
  * \class ElfCoreReader
  * \brief Contains the functionality for reading from a core file
  * Read an elf file that has been created by a core dump.  This requires working on the file with
  * reference to program headers and segments. As opposed to sections and section headers that would be used
  * if accessing an executable Elf file.
  */

#ifndef ELFCOREREADER_H
#define ELFCOREREADER_H

#include <libelf.h>
#include "defines.h"
#include <string>

class ElfCoreReader
{
public:
    /*!
      * \brief Constructor
      */
    ElfCoreReader();

    /*!
      * \brief Destructor
      */
    virtual ~ElfCoreReader();

    /*!
      * \brief Initalize the instance to work with a specific core file
      * \param fileName The path and filename of the core file to read
      * \return true on success, false otherwise
      */
    bool initalize(const char *fileName);

    /*!
      * \brief Get a pointer to the elf header of the underlying elf file
      * \return A pointer tot he header or NULL if the header is not present
      */
    inline Ehdr *elfFileHeader() const { return elfHeader; }

    /*!
      * \brief Return a point to the program headers section
      * \returns A pointer to the program headers if they have been initalized.
      */
    inline const Phdr *programHeader() const { return programHeaders; }

    /*!
      * \brief get a pointer to a sections data represented by an offset from the begining of the file
      * \param offset The start of the buffer from the beginning of the file
      * \return A pointer to the data segment if it is within the bounds of the file, NULL otherwise
      */
    const char *getDataByOffset(size_t offset);

    /*!
      * \brief get a pointer to the program header that contains the address \a toMatch
      * \param toMatch The address within a segment for which we want the program header
      * \return a pointer tothe header if it exists, NULL otherwise
      * This method implements a binary search algorithm and as a result it is necessary for the input
      * core file to have its segments sorted by virtual memory address.  This is generally the case
      * for system core files.
      */
    const Phdr *getSegmentByAddress(ADDRESS toMatch);

    /*!
      * \brief Get a pointer to a program header based on the type of the segment
      * \param toMatch The type of the segment that is to be matched
      * \return A pointer to the program header if it exists, false otherwise.
      * This method returns the first matching segment that has type equal to toMatch
      */
    const Phdr *getSegmentByType(Elf_Word toMatch);

    /*!
      * \brief Get a pointer to a program header based on its index within the program header array
      * \param index The index of hte header to find
      * \return A pointer to the program header if it exists, NULL otherwise.
      */
    const Phdr *getSegmentByIndex(size_t index);

private:
    /*!
      * \brief close the uderlying core file
      */
    void close();

private:
    //! A file descritor to the underlying core file
    int fd;
    //! a pointer to the elf file as returned by libelf
    Elf *file;
    //! A pointer to the start of the program header array
    Phdr *programHeaders;
    //! a pointer to the elf header struct
    Ehdr *elfHeader;
    //! The size of the core file that we are dealing with
    size_t fileSize;
};

#endif // ELFCOREREADER_H
