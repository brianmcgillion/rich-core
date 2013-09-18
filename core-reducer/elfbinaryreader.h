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
  * \file elfbinaryreader.h
  * \author Brian McGillion <brian.mcgillion@symbio.com>, Denis Mingulov <denis.mingulov@symbio.com>
  * \class ElfBinaryReader
  * \brief Contains the functionality for reading from a executable Elf file
  * Read an elf file that represents an executable.  This requires working on the file with
  * reference to sections and section headers.
  */


#ifndef ELFBINARYREADER_H
#define ELFBINARYREADER_H

#include "defines.h"
#include <string>
#include <libelf.h>

/*!
  * \brief A structure to store the most recently found section that matched certian criteria
  * This is done because it is common to request the same section multiple times in a row.  And under
  * these cases it will save looping needlessly over the sections to find the required section.
  */
typedef struct currentPointers
{
    Shdr *sectionHeader; //!< A Pointer to the most recently found section header
    Elf_Scn *section;    //!< A pointer to the most recently found section
    size_t index;        //!< The index of the current section
}CurrentSectionData;


class ElfBinaryReader
{

public:
    /*!
      * \brief Default Constructor
      */
    ElfBinaryReader();

    /*!
      * \brief Default destructor
      */
    virtual ~ElfBinaryReader();

    /*!
      * \brief Get a pointer to the elf header of the underlying elf file
      * \return A pointer tot he header or NULL if the header is not present
      */
    Ehdr *elfHeader() const { return m_elfHeader; }

    /*!
      * \brief Return a point to the program headers section
      * \returns A pointer to the program headers if they have been initalized.
      */
    const Phdr *programHeader() const { return programHeaders; }

    /*!
      * \brief Get the bit size of the underlying elf file.
      * \returns Either ELFCLASS32 or ELFCLASS64
      */
    int classSize() const { return m_classSize; }

    /*!
      * \brief Initalize the elf file.
      * \returns True on success False otherwise.
      * Handles the initalization of a elf file.  Open the file and perform some basic
      * checks that the file is correctly formatted.
      */
    bool initalize(const char *fileName);

    /*!
      * \brief Get a pointer to the first section that has the name name.
      * \param name The name of the section to find.
      * \returns A pointer to the section if it exists and no errors were encountered, NULL otherwise
      */
    const CurrentSectionData *getSectionByName(const char *name);

    /*!
      * \brief Given an index get a pointer to that section.
      * \param index The zero-based index of the section to find
      * \returns A pointer to the section if it exists and no errors were encountered, NULL otherwise
      */
    const CurrentSectionData *getSectionByIndex(size_t index);

    /*!
      * \brief Given an address find the section that contains that address
      * \param address An address with in a section that is required
      * \returns A pointer to the section if it exists or NULL if it does not exist or an error occurs.
      */
    const CurrentSectionData *getSectionByAddress(ADDRESS address);

    /*!
      * \brief Using section type (sh_type) find the first section that matches a particular type.
      * \param type The sh_type of the section that should be found
      * \returns A pointer to the section if it exists or NULL otherwise
      */
    const CurrentSectionData *getSectionByType(Elf_Word type);

	Phdr *getSegmentByType(Elf_Word type);

    /*!
      * \brief Close the underlying file handles
      */
    void close();

private:
    /*!
      * \brief Set an internal structure to keep a track of the currently used section and get the section header for that section.
      * \param section The section to track
      * \returns true if the section header could be found and false otherwise
      */
    bool setCurrent(Elf_Scn *section);

    /*!
      * \brief Get a pointer to a section
      * \param callback A pointer to a method that can be used as a callback to find a section based on different criteria
      * \param toMatch A pointer to the data that is to be matched in order to determine the correct section to find
      * \returns A pointer tot he section if it exists and there were no errors or NULL otherwise.
      */
    const CurrentSectionData *getSection(bool (*callback) (const Shdr *, void *), void *toMatch);

    /*!
      * \brief A callback method that is used to find the section based on an address parameter
      * \param sectionHeader the section header to examine
      * \param toMatch an address to match
      */
    static bool byAddress(const Shdr *sectionHeader, void *toMatch);

    /*!
      * \brief A callback method that is used to find the section based on a sh_type parameter
      * \param sectionHeader the section header to examine
      * \param toMatch a type to match
      */
    static bool byType(const Shdr *sectionHeader, void *toMatch);

    /*!
      * \brief A callback method that is used to find the section based on a name
      * \param sectionHeader the section header to examine
      * \param toMatch a matchName structure containing the relevant data
      */
    static bool byName(const Shdr *sectionHeader, void *toMatch);

private:

    //! struct containing the current pointers to the section and related header that have been just found
    CurrentSectionData current;

    //! A file descriptor that points to the OS level file is beinf written or read
    int fd;

    //! A pointer to the Elf structured file that we are reading or writing
    Elf *file;

    //! A pointer tot he header to the underlying Elf file header
    Ehdr *m_elfHeader;

    //! The elf Class size either ELFCLASS32 or ELFCLASS64
    int m_classSize;

    //! A pointer to the start of the program headers
    Phdr *programHeaders;

    //! The number of program headers in an elf file.
    size_t programHeaderNumber;

    //! The section index of the section header strings (.shstrtab)
    size_t sectionHeaderStringIndex;
};

#endif // ELFBINARYREADER_H
