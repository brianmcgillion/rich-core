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

#include "elfcorereader.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

ElfCoreReader::ElfCoreReader()
    : fd(-1),
    file(NULL),
    programHeaders(NULL),
    elfHeader(NULL),
    fileSize(0)
{
}

ElfCoreReader::~ElfCoreReader()
{
    close();
}

bool ElfCoreReader::initalize(const char *fileName)
{
    //initalize the elf library
    if (elf_version(EV_CURRENT) == EV_NONE)
        LOG_RETURN(LOG_ERR, false, "Could not define the elf version to use.");

    if (!fileName)
        LOG_RETURN(LOG_ERR, false, "Uninitialized pointer for fileName");

    if ((fd = open(fileName, O_RDONLY, 0)) < 0)
        LOG_RETURN(LOG_ERR, false, "Opening file '%s' failed.", fileName);

    if ((file = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
        LOG_RETURN(LOG_ERR, false, "elf_begin() failed: %s", elf_errmsg(-1));

    if (elf_kind(file) != ELF_K_ELF)
        LOG_RETURN(LOG_ERR, false, "'%s' does not appear to be an elf file.", fileName);

    if ((elfHeader = (Ehdr *)elf_rawfile(file, NULL)) == NULL)
        LOG_RETURN(LOG_ERR, false, "Can not read the elf header for '%s'.", elf_errmsg(-1));

    if ((programHeaders = (Phdr *)((char *)elfHeader + elfHeader->e_phoff)) == NULL)
        LOG_RETURN(LOG_ERR, false, "Can't access Program headers for '%s'", fileName);

    //determine the size of the file that we are working on
    struct stat buf;
    fstat(fd, &buf);
    fileSize = buf.st_size;

    return true;
}

const Phdr *ElfCoreReader::getSegmentByAddress(ADDRESS toMatch)
{
    int first = 0;
    int last = elfHeader->e_phnum;
    int mid = 0;

    while (first <= last)
    {
        mid = (first + last) / 2;
        if (toMatch < programHeaders[mid].p_vaddr)
            last = mid - 1; //search the lower half
        else if ((programHeaders[mid].p_vaddr + programHeaders[mid].p_filesz) <= toMatch)
            first = mid + 1; //search the upper half
        else
            return &programHeaders[mid]; //found our match
    }
    //failed to find a match
    return NULL;
}

const Phdr *ElfCoreReader::getSegmentByType(Elf_Word toMatch)
{
    for (int i = 0; i < elfHeader->e_phnum; i++)
        if (programHeaders[i].p_type == toMatch)
            return &programHeaders[i];

    return NULL;
}

const Phdr *ElfCoreReader::getSegmentByIndex(size_t index)
{
    if (0 <= index  && index < elfHeader->e_phnum)
        return &programHeaders[index];

    return NULL;
}

const char *ElfCoreReader::getDataByOffset(size_t offset)
{
    if (offset < fileSize)
        return ((char *)elfHeader + offset);

    return NULL;
}

void ElfCoreReader::close()
{
    if (file)
    {
        elf_end(file);
        file = NULL;
    }
    if (fd)
    {
        ::close(fd);
        fd = -1;
    }
}
