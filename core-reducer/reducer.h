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
  * \file reducer.h
  * \class Reducer
  * \author Brian McGillion <brian.mcgillion@symbio.com>, Denis Mingulov <denis.mingulov@symbio.com>
  *
  * \brief A class that copies the needed sections from a standard core file to a reduced core file.
  *
  * This class facilitates the extracting of inportant information from a standard kernel generated core file,
  * into a a new core file.  The information that is deemed inportant for this impelmentation os the
  * Notes section, The stacks and the Link Map.
  *
  * The Notes section, stores the information for the registers.  Each stack as a series of register
  * structs stred within the notes section.  In addition to the register information the Notes section
  * also includes the applications auxillary vector.  This is the vector that facilitates user space
  * kernel space transfering of information, it can be used to inform the application about the system
  * in which it is running.
  *
  * The stacks, these are the standard stacks, and this application manages to include the stacks
  * for each of the threads that are running at the time of a crash in an application.
  *
  * The link map, this is a section of information that is created by the dynamic linker and can be used
  * by the debugger to determine which libraries were loaded when the application crashed.  This link
  * map is what allows the debugger to display the symbols associated with the stack enteries to be
  * displayed correctly.  In an application that is generated without debugging information included
  * this link map section can be missing.  However with help of the /proc/$PID/maps file that is stored
  * at the time of the application crash it is possible to construct this information in a post
  * processsing stage.
  *
  * All the access to files and all relations to files are specified with regards to the ELF file
  * standard.
  */

#ifndef REDUCER_H
#define REDUCER_H
#include "defines.h"
#include <vector>
#include <string>

//forward declerations
class ElfCoreReader;
class ElfBinaryReader;
class RawElfWriter;

class Reducer
{
public:

    /*!
      * \brief Constructor
      * \param output The name of the file to which the output is to be written
      * \param heap An unused address that can be used to store the link map data into.  Generally the
      * heap address can be safely used for this purpose as it is not going to be store in the reducer
      * core file.
      */
    Reducer(const char *output, ADDRESS heap);

    /*!
      * \brief Destructor
      */
    ~Reducer();

    /*!
      * \brief Initalize the internal structures within the class
      * \param core The name of the core file from which we are going to take information
      * \param binary The name of the executable that has crashed
      * \return true on success false otherwise.
      */
    bool initalize(const char *core, const char *binary);

    /*!
      * \brief Run the algorithm that reduces the input core file and produces a shrunken core
      * that contains only the wanted data.
      * \param stacksOnly Default is to taqke stacks and debug information, but setting this to true only the stacks will be copied to the output file
      * \param mapsFile The file which will be used to get shared objects list. If NULL then original debug data will be used.
      */
    void run(bool stacksOnly=false, const char *mapsFile=NULL);

private:
    /*!
      * \brief Find the note section in the origional core file and store a reference to it
      * \return true on success, false otherwise
      * Gather the ESP (%esp) from the set of registers, this data can be used to
      * generate a list of stacks that are in use within the application at the time it crashed.
      * Also get the process id and executable name of the application at teh time of a crash.
      */
    bool getNotes();

    /*!
      * \brief Find the memory areas in the core file that represent the stacks in the crashed application
      */
    void getStacks();

    /*!
      * \brief Check is heap address setted, if not - try to set up it automatically.
      */
    void checkHeapAddress();

    /*!
      * \brief Copy the memory area fro the core file that contains the dynamic section information
      * \param mapsFile Maps (or smaps etc) file for the process
      * When copying the dynamic section the memory location referenced by DT_DEBUG must be overwritten
      * to point to the r_debug section in our new reduced core file.
      */
    void copyDynamicSectionInformation(const char *mapsFile=NULL);

    /*!
      * \brief Create the dynamic section which overwrites the original data
      * When copying the dynamic section the memory location referenced by DT_DEBUG must be overwritten
      * to point to the r_debug section in our new reduced core file, this function is used when there is no
      * dynamic section data in the core dump
      */
    void generateDynamicSectionInformation();

    /*!
      * \brief Initalize the output file for writing the reduced core file
      * \param stacksOnly Default is to taqke stacks and debug information, but setting this to true only the stacks will be copied to the output file
      * Once the file is created the inital segments are copied to it.
      */
    void copyInitalSegmentsToOutput(bool stacksOnly=false);

    /*!
      * \brief Copy the r_debug and link_map information to the reduced core file
      * \param start The address within the origional core file where we can find the the start of
      * r_debug section
      */
    void copyLinkMapToOutputFile(ADDRESS start);

    /*!
      * \brief Create the r_debug and link_map information to the reduced core file
      * \param start The address within the origional core file where we can find the the start of
      * r_debug section
      * \param mapsFile The maps file name which will be used to generate debug data
      */
    void createLinkMapInOutputFile(ADDRESS start, const char *mapsFile);

    /*!
      * \brief Get a pointer to the data in the origional core file that is referenced by virtual memory address start
      * \return On success a pointer to the buffer containg the data, NULL otherwise
      */
    const char *getBufferAtAddress(ADDRESS start);

private:
    //! A pointer to the class that will handle the reading of the core dump file
    ElfCoreReader *coreReader;
    //! A pointer to the class that will read the executable file associated with the crashed application
    ElfBinaryReader *binaryReader;
    //! A pointer to the class that will be used to write the reduced core file
    RawElfWriter *coreWriter;
    //! A vector that contains a reference to each of the program headers that we want to copy to the reduced core file
    std::vector<const Phdr *> wantedHeaders;
    //! A vector to store the headers that are created, and must be deleted after use.
    std::vector<Phdr *> dynamiclyCreatedHeaders;
    //! The address of the dynamic section as read from the executable file
    ADDRESS dynamicAddressFromExecutable;
    //! The size of the dynamic section as read from the executable file
    size_t dynamicSectionSizeFromExecutable;
    //! The address at which the interpreter is loaded
    ADDRESS interpAddress;
    //! The name of the interpreter that is being used to load the dynamic libraries
    char *interpreter;
    //! The file to which the reduced core data is to be stored
    const char *output;
    //! A virtual memory address into which we can store the link map data.
    ADDRESS heapAddress;
    //! A vector of stack pointers that is used by the process
    std::vector<ADDRESS> stackPointerAddresses;
    //! The id of the process
    int processId;
    //! The name and path of the application that crashed
    char *executableName;
	//! Load address of PHDR
	ADDRESS phdrAddr; 
};

#endif // REDUCER_H
