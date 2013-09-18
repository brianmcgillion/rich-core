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

#include "rawelfwriter.h"

#include <elf.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

//add 512 bytes to file buffer each time realloc is called in LM map creation method
#define LM_BUFFER_DATA_SIZE 512

#if __WORDSIZE == 32
/*!
  * \def R_DEBUG_STRUCT_SIZE The size of the r_debug struct
  */
#define R_DEBUG_STRUCT_SIZE 20
#else
/*!
  * \def R_DEBUG_STRUCT_SIZE The size of the r_debug struct
  */
#define R_DEBUG_STRUCT_SIZE 40
#endif

/*!
  * \brief A structure to reference the link map data that we are copying
  */
typedef struct linkMapStruct
{
    ADDRESS addressOffset;         //!< The address offset of the library
    ADDRESS nameOffset;            //!< The address of the string representing the library name
    ADDRESS ldOffset;              //!< The entry point in the library
    ADDRESS nextLinkMapStruct;     //!< A pointer to the next link map struct
    ADDRESS previousLinkMapStruct; //!< A pointer to the previous link map struct
} LinkMap;


RawElfWriter::RawElfWriter()
    : buffer(NULL),
    fd(-1),
    elfHeader(NULL),
    programHeaders(NULL),
    currentBufferSize(0),
    offset(0),
    numProgramHeaders(0),
    currentProgramHeader(0),
    previousLinkAddress(0),
    currentLinkMapSize(0),
    linkMapHeadAddress(0),
    isBufferSorted(false)
{
}

RawElfWriter::~RawElfWriter()
{
    write();
    close();
}

bool RawElfWriter::initalize(const char *fileName, size_t numberOfSegments, size_t initalSizeOfData)
{
    if (!fileName)
        LOG_RETURN(LOG_ERR, false, "File name not initalized ");

    if ((fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0)
        LOG_RETURN(LOG_ERR, false, "Opening file '%s' failed.", fileName);

    currentBufferSize = sizeof(Ehdr) + (numberOfSegments * sizeof(Phdr)) + initalSizeOfData;
    if (!(buffer = (char *)malloc(currentBufferSize * sizeof(char))))
        LOG_RETURN(LOG_ERR, false, "Not enough memory to create output file.");

    numProgramHeaders = numberOfSegments;
    //the elf header starts at byte 0 and takes sizeof(EHdr) bytes
    elfHeader = (Ehdr *)buffer;
    offset += sizeof(Ehdr);
    return true;
}

/*
  *            The structure of a basic elf file
  *
  * ---------------------------------------------------
  * |                                                  |
  * |                  ELF HEADER                      |
  * |                                                  |
  * |--------------------------------------------------|
  * |                Program Header (1)                |
  * |--------------------------------------------------|
  * |                       :                          |
  * |--------------------------------------------------|
  * |                Program Header (N)                |
  * |--------------------------------------------------|
  * |                     DATA                         |
  * |  This can be broken down into smaller blocks     |
  * |  called segments but really a segment is just a  |
  * |  high level definition that relates data to an   |
  * |  offset within the file                          |
  * |                                                  |
  * ----------------------------------------------------
  *
  */
void RawElfWriter::copyElfHeader(const Ehdr *headerToCopy)
{
    if (!headerToCopy)
        LOG_RETURN(LOG_ERR, , "Elf Header error: ");

    memcpy(elfHeader, headerToCopy, sizeof(Ehdr));
    programHeaders = (Phdr *)(buffer + sizeof(Ehdr));

    elfHeader->e_shnum = 0;
    elfHeader->e_shstrndx = 0;
    elfHeader->e_shoff = 0;
    elfHeader->e_phnum = numProgramHeaders;
    elfHeader->e_phoff = sizeof(Ehdr);
    //Zero out the area reserved for the program headers
    memset(programHeaders, 0, (sizeof(Phdr) * numProgramHeaders));

    //Offset should now point to the position where it is safe to start inserting data
    offset += (sizeof(Phdr) * numProgramHeaders);
}

const char *RawElfWriter::copySegment(const Phdr *headerToCopy, const char *data,
                                      const char *overwriteData, size_t overwriteOffset,
                                      size_t overwriteSize)
{
    if (!headerToCopy || !data)
        LOG_RETURN(LOG_ERR, NULL, "No data in this segment/Not a valid segment.");

    //See if we have already used all of the previously assigned
    //Program headers
    if (currentProgramHeader >= numProgramHeaders)
        LOG_RETURN(LOG_ERR, NULL, "Incorrect number of program headers assigned.");

    if ((offset + headerToCopy->p_filesz) > currentBufferSize)
    {
        //we have already assigned space for the Prog header so no more space is required for it
        if (!reallocate(headerToCopy->p_filesz))
            return NULL;
    }

    //We want to save almost all of the Program Header intact
    memcpy(&programHeaders[currentProgramHeader], headerToCopy, sizeof(Phdr));
    //but the offset will have to change to match this files offset
    programHeaders[currentProgramHeader].p_offset = offset;

    //copy the data portion of the segment
    char *writePointer = buffer + offset;
    memcpy(writePointer, data, headerToCopy->p_filesz);

    offset += headerToCopy->p_filesz;

    //finally see if it has been requested to overwrite a portion of the segment that has
    //just been copied.
    if (overwriteData && ((overwriteOffset + overwriteSize) < headerToCopy->p_filesz))
    {
        char *overwritePointer = writePointer + overwriteOffset;
        memcpy(overwritePointer, overwriteData, overwriteSize);
    }

    currentProgramHeader++;
    return writePointer;
}

bool RawElfWriter::reallocate(size_t amountRequired)
{
    currentBufferSize += amountRequired;

    char *tmp = (char *)realloc(buffer, currentBufferSize * sizeof(char));
    if (tmp == NULL)
        LOG_RETURN(LOG_ERR, false, "Insufficient memory for realloc.");

    //rearrange the pointers
    buffer = tmp;
    elfHeader = (Ehdr *)buffer;
    programHeaders = (Phdr *)(buffer + sizeof(Ehdr));

    return true;
}

void RawElfWriter::startLinkMapSegment(ADDRESS heapAddress)
{
    programHeaders[currentProgramHeader].p_type = PT_LOAD;
    programHeaders[currentProgramHeader].p_vaddr = heapAddress;
    programHeaders[currentProgramHeader].p_flags = ( PF_R | PF_W );
    programHeaders[currentProgramHeader].p_offset = offset;
    programHeaders[currentProgramHeader].p_align = 0x1;
}

ADDRESS RawElfWriter::createR_DebugStruct()
{
    // create new r_DebugStruct
    // content is not important because just a link map address is needed - and it will be overwritten
    char rDebugStruct[R_DEBUG_STRUCT_SIZE];
    memset(rDebugStruct, 0, sizeof(rDebugStruct));

    // add it to the file
    return addR_DebugStruct(rDebugStruct);
}

ADDRESS RawElfWriter::addR_DebugStruct(const char *rDebugStart)
{
    if (!rDebugStart)
        return 0;

    if ((R_DEBUG_STRUCT_SIZE + offset) > currentBufferSize)
    {
        if (!reallocate(R_DEBUG_STRUCT_SIZE + offset + LM_BUFFER_DATA_SIZE))
            return 0;
    }

    char *writePointer = buffer + offset;
    memcpy(writePointer, rDebugStart, R_DEBUG_STRUCT_SIZE);

    offset += R_DEBUG_STRUCT_SIZE;

    linkMapHeadAddress = programHeaders[currentProgramHeader].p_vaddr + R_DEBUG_STRUCT_SIZE;
    //set the r_debug::link_map pointer to point to our link map
    memcpy(writePointer + sizeof(ADDRESS), &linkMapHeadAddress, sizeof(ADDRESS));

    //Return the address of the first link in the chain
    return *((ADDRESS *)(rDebugStart + sizeof(ADDRESS)));
}

ADDRESS RawElfWriter::createAndAddLinkMapSegment(ADDRESS memoryAddress, const char *stringStart, bool isLast, bool isFirst)
{
    // empty link map item creation
    LinkMap link;
    link.addressOffset = 0;
    link.ldOffset = 0;
    link.nameOffset = 0;
    link.nextLinkMapStruct = 0;
    link.previousLinkMapStruct = 0;

    // fill it
    link.addressOffset = memoryAddress;
    // content will be overwritten, so important is that it is not 0 only
    if (!isLast)
        link.nextLinkMapStruct = 1;
    if (!isFirst)
        link.previousLinkMapStruct = 1;

    return addLinkMapSegment((char *)&link, stringStart);
}


ADDRESS RawElfWriter::addLinkMapSegment(const char *linkMapStart, const char *stringStart)
{
    if (!linkMapStart)
        return 0;

    LinkMap *LM_To_Copy = (LinkMap *)linkMapStart;

    //at the very least the string will have a null byte
    size_t stringSize = 1;

    if (stringStart)
        stringSize += strlen(stringStart);

    if ((sizeof(LinkMap) + stringSize + offset) > currentBufferSize)
    {
        if (!reallocate(sizeof(LinkMap) + stringSize + offset + LM_BUFFER_DATA_SIZE))
            return 0;
    }

    //write the link map information
    LinkMap *LM_Writer = (LinkMap *)(buffer + offset);

    //The addresses that gdb is concerned with are the Virtual memory addresses
    //where each part of the Link map can be found
    LM_Writer->addressOffset = LM_To_Copy->addressOffset;
    //the address where gdb can read the library string name from
    LM_Writer->nameOffset = linkMapHeadAddress + currentLinkMapSize + sizeof(LinkMap);
    LM_Writer->ldOffset = LM_To_Copy->ldOffset;

    //Add the address of the next link in the chain
    if (LM_To_Copy->nextLinkMapStruct != 0)
    {
        LM_Writer->nextLinkMapStruct = linkMapHeadAddress
                                       + currentLinkMapSize + sizeof(LinkMap) + stringSize;
    }
    else
    {
        //This is the final link in the chain
        LM_Writer->nextLinkMapStruct = 0;
    }

    LM_Writer->previousLinkMapStruct = previousLinkAddress;

    offset += sizeof(LinkMap);

    //copy the string containing the library name
    char *writePointer = buffer + offset;
    for (unsigned int i = 0; i < (stringSize - 1); i++)
        *writePointer++ = *stringStart++;

    //Always write the null character ourselves, to account for cases where the origional string size is 0
    *writePointer++ = '\0';

    offset += stringSize;

    previousLinkAddress = linkMapHeadAddress + currentLinkMapSize;
    currentLinkMapSize += (sizeof(LinkMap) + stringSize);
    return LM_To_Copy->nextLinkMapStruct;
}

void RawElfWriter::finalizeLinkMapSegment()
{
    //finish writing the headers
    programHeaders[currentProgramHeader].p_filesz = offset - programHeaders[currentProgramHeader].p_offset;
    programHeaders[currentProgramHeader].p_memsz = offset - programHeaders[currentProgramHeader].p_offset;
}

/*!
  * \brief Compare two program header virtual memory address values to determine which is the smaller
  * \param first The first variable to comapre against
  * \param second The second variable to compare to
  * \returns A negative value if a < b, a positive value if b > a or 0 if a == b
  * Provided as a callback method that can be used with qsort to sort the program headers of the output
  * file into ascending order based on virtual memory address.
  */
int compareProgramHeaders(const void *first, const void *second)
{
    if (((Phdr *)first)->p_vaddr < ((Phdr *)second)->p_vaddr)
        return -1;
    else if (((Phdr *)first)->p_vaddr > ((Phdr *)second)->p_vaddr)
        return 1;
    else
        return 0;
}

void RawElfWriter::sortBuffer()
{
    if (isBufferSorted || !buffer || !programHeaders)
        return;

    qsort(programHeaders, numProgramHeaders, sizeof(Phdr), compareProgramHeaders);

    isBufferSorted = true;
}

bool RawElfWriter::write()
{
    if (buffer)
    {
        if (!isBufferSorted)
            sortBuffer();

        if (::write(fd, buffer, offset * sizeof(char)) != (ssize_t)offset)
            LOG_RETURN(LOG_ERR, false, "Error writing file to disk");

        free(buffer);
        buffer = NULL;
    }

    return true;
}

void RawElfWriter::close()
{
    if (fd)
    {   
        ::close(fd);
        fd = -1;
    }
}
