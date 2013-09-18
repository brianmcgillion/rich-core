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

#include "elfbinaryreader.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>

/*!
  * \brief A struct used to pass data to the static byName callback method
  */
typedef struct match
{
    const Elf *elfFile;       //!< a pointer to the current elf file
    const char *name;         //!< The name of the section that is being searched for
    const size_t stringIndex; //!< The section index of the .shstrtab
}matchName;

ElfBinaryReader::ElfBinaryReader()
    : fd(-1),
    file(NULL),
    m_elfHeader(NULL),
    m_classSize(0),
    programHeaders(NULL),
    programHeaderNumber(0),
    sectionHeaderStringIndex(0)
{
    current.section = NULL;
    current.sectionHeader = NULL;
    current.index = 0;
}

ElfBinaryReader::~ElfBinaryReader()
{
    close();
}

bool ElfBinaryReader::initalize(const char *fileName)
{
    //initalize the elf library
    if (elf_version(EV_CURRENT) == EV_NONE)
        LOG_RETURN(LOG_ERR, false, "Could not define the elf version to use.");

    if(!fileName)
        LOG_RETURN(LOG_ERR, false, "Uninitalized pointer for fileName");

    if ((fd = open(fileName, O_RDONLY, 0)) < 0)
        LOG_RETURN(LOG_ERR, false, "Opening file '%s' falied.", fileName);

    if ((file = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
        LOG_RETURN(LOG_ERR, false, "elf_begin() failed: %s", elf_errmsg(-1));

    if (elf_kind(file) != ELF_K_ELF)
        LOG_RETURN(LOG_ERR, false, "'%s' does not appear to be an elf file.", fileName);

#if __WORDSIZE == 32
    if ((m_elfHeader = (Ehdr *)elf32_getehdr(file)) == NULL)
        LOG_RETURN(LOG_ERR, false, "Can not read the elf header for '%s'.", elf_errmsg(-1));

    if(!(programHeaders = (Phdr *)elf32_getphdr(file)))
        LOG_RETURN(LOG_ERR, false, "Can not read the elf program headers for '%s'", elf_errmsg(-1));
#else
    if ((m_elfHeader = (Ehdr *)elf64_getehdr(file)) == NULL)
        LOG_RETURN(LOG_ERR, false, "Can not read the elf header for '%s'.", elf_errmsg(-1));

    if(!(programHeaders = (Phdr *)elf64_getphdr(file)))
        LOG_RETURN(LOG_ERR, false, "Can not read the elf program headers for '%s'.", elf_errmsg(-1));
#endif

    if (elf_getphnum(file, &programHeaderNumber) == 0)
        LOG_RETURN(LOG_ERR, false, "getphnum() failed: %s", elf_errmsg(-1));

    if(elf_getshstrndx(file, &sectionHeaderStringIndex) == 0)
        LOG_RETURN(LOG_ERR, false, "elf_getshstrndx() failed: %s", elf_errmsg(-1));

    return true;
}

const CurrentSectionData *ElfBinaryReader::getSectionByIndex(size_t index)
{
    Elf_Scn *section = NULL;

    if((section = elf_getscn(file, index)) == NULL)
        LOG_RETURN(LOG_INFO, NULL, "getscn() failed for section index %d: %s", index, elf_errmsg(-1));

    if(!setCurrent(section))
        return NULL;

    return &current;
}

const CurrentSectionData *ElfBinaryReader::getSectionByAddress(ADDRESS address)
{
    return getSection(&(ElfBinaryReader::byAddress), (void *) &address);
}

const CurrentSectionData *ElfBinaryReader::getSectionByType(Elf_Word type)
{
    return getSection(&(ElfBinaryReader::byType), (void *) &type);
}

const CurrentSectionData *ElfBinaryReader::getSectionByName(const char *name)
{
    if(!name)
        LOG_RETURN(LOG_ERR, NULL, "Uninitalized name string");

    matchName matchNameParams = {file, name, sectionHeaderStringIndex};

    return getSection(&(ElfBinaryReader::byName), (void *) &matchNameParams);
}

const CurrentSectionData *ElfBinaryReader::getSection(bool (*callback) (const Shdr *, void *), void *toMatch)
{
    //test to see if we have a pointer to a section and if it is the section we are looking for
    //many times we are actually looking for the same section as the last search
    if (current.section && current.sectionHeader)
    {
        if((*callback)(current.sectionHeader, toMatch))
            return &current;
    }

    //loop and see if we can find the section
    Elf_Scn *section = NULL;
    while ((section = elf_nextscn(file, section)) != NULL)
    {
        if(!setCurrent(section))
            return NULL;
        if((*callback)(current.sectionHeader, toMatch))
            return &current;
    }

    return NULL;
}

bool ElfBinaryReader::setCurrent(Elf_Scn *section)
{
    if (!section)
        return false;

    Shdr *sectionHeader = NULL;

#if __WORDSIZE == 32
    if (!(sectionHeader = (Shdr *)elf32_getshdr(section)))
        LOG_RETURN(LOG_ERR, false, "getshdr() failed: %s", elf_errmsg(-1));
#else
    if (!(sectionHeader = (Shdr *)elf64_getshdr(section)))
        LOG_RETURN(LOG_ERR, false, "getshdr() failed: %s", elf_errmsg(-1));
#endif

    current.section = section;
    current.sectionHeader = sectionHeader;
    current.index = elf_ndxscn(section);
    return true;
}

bool ElfBinaryReader::byAddress(const Shdr *sectionHeader, void *toMatch)
{
    ADDRESS address = *((ADDRESS *) toMatch);
    if ((sectionHeader->sh_addr <= address) && (address < (sectionHeader->sh_addr + sectionHeader->sh_size)))
        return true;
    return false;
}

bool ElfBinaryReader::byType(const Shdr *sectionHeader, void *toMatch)
{
    if (sectionHeader->sh_type == *((Elf_Word *) toMatch))
        return true;
    return false;
}

bool ElfBinaryReader::byName(const Shdr *sectionHeader, void *toMatch)
{
    matchName matchNameParams = *((matchName *) toMatch);
    char *sectionName;

    if ((sectionName = elf_strptr((Elf *)matchNameParams.elfFile, matchNameParams.stringIndex, sectionHeader->sh_name)) == NULL)
        return false;

    if (strcmp(sectionName, matchNameParams.name) == 0)
        return true;
    return false;
}

Phdr *ElfBinaryReader::getSegmentByType(Elf_Word type)
{
	size_t n;

	if (!elf_getphnum(file, &n))
		return NULL;

	Phdr *phdr = NULL;
#if __WORDSIZE == 32
	if (!(phdr = elf32_getphdr(file)))
		LOG_RETURN(LOG_ERR, false, "getphdr() failed: %s", elf_errmsg(-1)); 
#else
	if (!(phdr = elf64_getphdr(file)))
		LOG_RETURN(LOG_ERR, false, "getphdr() failed: %s", elf_errmsg(-1)); 
#endif

	while (phdr)
	{
		if (phdr->p_type == type)
			return phdr;
		++phdr; 
	}
	return NULL; 
}

void ElfBinaryReader::close()
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

