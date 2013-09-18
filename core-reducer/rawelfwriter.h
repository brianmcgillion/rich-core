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
  * \file rawelfwriter.h
  * \author Brian McGillion <brian.mcgillion@symbio.com>, Denis Mingulov <denis.mingulov@symbio.com>
  * \class RawElfWriter
  * \brief Dispence with libelf functions for writing a core file.
  * The core file is treated as a malloc'd buffer that grows to accomodate the reduced core file.
  * it is only written to the output file when the file structure is complete.
  */

#ifndef RAWELFWRITER_H
#define RAWELFWRITER_H

#include "defines.h"
#include <string>


class RawElfWriter
{
public:
    /*!
      * \brief Constructor
      */
    RawElfWriter();

    /*!
      * \brief destructor
      */
    ~RawElfWriter();

    /*!
      * \brief initalize the class so that we can start to create the new core file.
      * \param fileName The path of the file to which we are going to write the finished core file
      * \param numberOfSegments The number of segments that are going to be created.
      * \param initalSizeOfData The inital amount of space to reserve for the file.
      * \return true on success, false otherwise
      */
    bool initalize(const char *fileName, size_t numberOfSegments, size_t initalSizeOfData);

    /*!
      * \brief A convenience method to copy the elf header from one core file to our reduced core file
      * \param header A pointer to the header file that is to be copied
      */
    void copyElfHeader(const Ehdr *header);

    /*!
      * \brief copy a segment from the one core file to this file
      * When the data is being copied it is possible to over write a portion of the new segment with
      * some specified data.
      * \param programHeader The program header that contains the information about the segment
      * \param data A pointer to the data for the segment that is to be copied
      * \param overwriteData The data to replace some existing data in the segment with
      * \param overwriteOffset The position relative to the start of the segment into which \a overwriteData is to be copied
      * \param overwriteSize The amount of the segment that is going to be overwritten by \a overwriteData
      * \returns On success a pointer to the start of the data segment in the new file, similar to strcpy, NULL otherwise
      */
    const char *copySegment(const Phdr *programHeader, const char *data,
                            const char *overwriteData = NULL, size_t overwriteOffset = 0,
                            size_t overwriteSize = 0);

    /*!
      * \brief Start the creation of a segment that will contain the link map data
      * \param heapAddress The Virtual memory address that will represent the start of the r_debug struct
      * This method must be called before \a addR_DebugStruct() and \a addlinkMapSegment()
      */
    void startLinkMapSegment(ADDRESS heapAddress);

    /*!
      * \brief Create a new r_debug segment to our new file.
      * \return The ADDRESS of the start of the link map in the original core dump file
      * Copy the structure to the new file and set the start address of the link map in the new file
      * to point to the area of memory that has been reserved for the link map.  The address that is returned is
      * the virtual memory address of the start of the link map in the origional core file
      */
    ADDRESS createR_DebugStruct();

    /*!
      * \brief Copy an r_debug segment to our new file.
      * \param rDebugStart A pointer to the start of a buffer that can be cast to r_debug
      * \return The ADDRESS of the start of the link map in the original core dump file
      * Copy the structure to the new file and set the start address of the link map in the new file
      * to point to the area of memory that has been reserved for the link map.  The address that is returned is
      * the virtual memory address of the start of the link map in the origional core file
      */
    ADDRESS addR_DebugStruct(const char *rDebugStart);

    /*!
      * \brief create a link map struct and add it and corresponding library name string to the new file
      * \param memoryAddress A pointer to the start of the shared object
      * \param stringStart A pointer to the start of the library name string
      * \param isLast True if it is last item
      * \param isFirst True if it is first item
      * \return The address of hte next link map entry in the origional core file. It returns NULL when there is no more links.
      */
    ADDRESS createAndAddLinkMapSegment(ADDRESS memoryAddress, const char *stringStart, bool isLast, bool isFirst);

    /*!
      * \brief add a link map struct and corresponding library name string to the new file
      * \param linkMapStart A pointer to the start of the link map struct
      * \param stringStart A pointer to the start of the library name string
      * \return The address of the next link map entry in the origional core file. It returns NULL when there is no more links.
      */
    ADDRESS addLinkMapSegment(const char *linkMapStart, const char *stringStart);

    /*!
      * \brief This method must be called when all data has been added to the link map segment.
      * It is important that this is called after directly after all the link map data has been added
      * to the new segment.  This method updates the segment size and file size within hte respective headers.
      */
    void finalizeLinkMapSegment();

    /*!
      * \brief This method writes the memory buffer to file and should be called once at the end of processing
      * \return true on success false otherwise.
      */
    bool write();

private:
    /*!
      * \brief close the underlying file handle
      */
    void close();

    /*!
      * \brief allocate more space to the internal buffer if required
      * \param amountRequired The amount of new space to allocate to the buffer
      * \return true on success false otherwise
      */
    bool reallocate(size_t amountRequired);

    /*!
      * \brief Sort the Program headers so that the lowest Virtual memory address segments come first
      * Only the Program headers have to be sorted.  The data to which they point does not have to be moved
      * because these data segments are referenced by a file offset that is contained within the program header.
      * The headers should be sorted to allow for more efficient search algorithms to be used when trying to
      * locate a segment by virtual memory address.
      */
    void sortBuffer();

private:
    //! The buffer that is used to create the elf file
    char *buffer;
    //! descriptor for system file
    int fd;
    //! A pointer to the elf header structure in the the core file
    Ehdr *elfHeader;
    //! A pointer to the start of the array program headers
    Phdr *programHeaders;
    //! The current size of the elf file
    size_t currentBufferSize;
    //! The offset point in the address where we are writing to
    size_t offset;
    //! The number of program headers used in teh new core file
    size_t numProgramHeaders;
    //! The current Program header that is being worked on
    size_t currentProgramHeader;
    //! The address of the previous link map entry
    ADDRESS previousLinkAddress;
    //! The size that is currnetly taken up by the link map
    size_t currentLinkMapSize;
    //! The address of the start of the link map within the new core file
    ADDRESS linkMapHeadAddress;
    //! Determine if the buffer is sorted. Only the Program Headers addresss' have to be sorted
    bool isBufferSorted;
};

#endif // RAWELFWRITER_H
