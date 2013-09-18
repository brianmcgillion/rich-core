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
#include "elfcorereader.h"
#include "elfbinaryreader.h"
#include "rawelfwriter.h"
#include "procinterface.h"

#include "../config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/procfs.h>
#include <limits.h>

//add some additional space on the stack
#define STACK_ADDITION 128

// predefined heap address, will be used if an application does not have heap
#define PREDEFINED_HEAP_ADDRESS 4

#ifdef ARM_REGS
/*!
  * \brief The offset pointer in to the registry buffer that holds the value of R13 (aka esp)
  * \sa gdb-7.0/gdb/arm-tdep.c
  */
#define ESP_OFFSET 13
#else
/*!
  * \brief The offset pointer in tothe registry buffer that holds the value of the ESP (%esp)
  * \sa sys/reg.h
  * \sa gdb-7.0/gdb/i386-linux-tdep.c
  */
#define ESP_OFFSET 15
#endif

typedef struct elf_prstatus Status;
typedef struct elf_prpsinfo Info;

#define align_power(address, alignSize) \
(((address) + ((ADDRESS) 1 << (alignSize)) - 1) & ((ADDRESS) -1 << (alignSize)))


Reducer::Reducer(const char *output, ADDRESS heap)
    :   coreReader(NULL),
    binaryReader(NULL),
    coreWriter(NULL),
    dynamicAddressFromExecutable(0),
    dynamicSectionSizeFromExecutable(0),
    interpAddress(0),
    interpreter(0),
    output(output),
    heapAddress(heap),
    processId(INT_MAX),
    executableName(NULL),
	phdrAddr(0)
{
}

Reducer::~Reducer()
{
    if (coreReader)
    {
        delete(coreReader);
        coreReader = NULL;
    }
    if (binaryReader)
    {
        delete(binaryReader);
        binaryReader = NULL;
    }
    if (coreWriter)
    {
        delete(coreWriter);
        coreWriter = NULL;
    }

    while (!dynamiclyCreatedHeaders.empty())
    {
        delete dynamiclyCreatedHeaders.back();
        dynamiclyCreatedHeaders.pop_back();
    }

    //These are only references now.  The objects have been deleted
    wantedHeaders.clear();

    if (interpreter)
        free(interpreter);
}

bool Reducer::initalize(const char *core, const char *binary)
{
    //initalize the elf library
    if (elf_version(EV_CURRENT) == EV_NONE)
        LOG_RETURN(LOG_ERR, false, "Unable to determine the elf version to use");

    coreReader = new ElfCoreReader();
    if (!coreReader->initalize(core))
        return false;

    //read the note section from the core dump as it contains alot of useful information
    //e.g. process id, ESPs for the process and all threads
    if (!getNotes())
        return false;

    binaryReader = new ElfBinaryReader();
    if (!binaryReader->initalize(binary))
        return false;

	Phdr *phdr = binaryReader->getSegmentByType(PT_PHDR);
	ADDRESS loadBias = phdrAddr - phdr->p_vaddr;
    //get the dynamic section from the binary executable
    const CurrentSectionData *binarySectionData = binaryReader->getSectionByType(SHT_DYNAMIC);
    if (binarySectionData)
    {
        dynamicAddressFromExecutable = binarySectionData->sectionHeader->sh_addr + loadBias;
        dynamicSectionSizeFromExecutable = binarySectionData->sectionHeader->sh_size;

        //Now find the address of the INTREP section.  this is the address at which the dynamic linker
        //will be loaded
        binarySectionData = NULL;
        binarySectionData = binaryReader->getSectionByName(".interp");
        if (binarySectionData)
        {
            //Find the address the interpreter is loaded at
            interpAddress = binarySectionData->sectionHeader->sh_addr + loadBias;

            //Find the name of the application that is being used as the interpreter
            Elf_Data *data = NULL;
            if((data = elf_getdata(binarySectionData->section, data)) != NULL)
                interpreter = strdup((char *) data->d_buf);
        }
        else
        {
            LOG(LOG_INFO, "Unable to find '.intrep' section in a dynamic binary.");
        }
    }
    else
    {
        LOG(LOG_INFO, "Unable to find dynamic section in file, it may be a statically linked file!");
    }

    //we have completed all tasks with the binary so close the handle on it.
    delete(binaryReader);
    binaryReader = NULL;

    return true;
}

void Reducer::run(bool stacksOnly, const char *mapsFile)
{
    checkHeapAddress();
    getStacks();
    copyInitalSegmentsToOutput(stacksOnly);
    if (!stacksOnly)
        copyDynamicSectionInformation(mapsFile);

    //Finish writing the file to disk
    if (coreWriter)
        coreWriter->write();
}

void Reducer::checkHeapAddress()
{
    if (heapAddress)
        return;

    // try to get it from /proc/[pid] (maps file)
    ProcInterface iface(processId);
    heapAddress = iface.heapAddress();

    if (heapAddress)
        return;

    // if not - use predefined value
    heapAddress = PREDEFINED_HEAP_ADDRESS;
}

bool Reducer::getNotes()
{
    const Phdr *noteSegment = coreReader->getSegmentByType(PT_NOTE);
    if (!noteSegment)
        LOG_RETURN(LOG_ERR, false, "There does not appear to be a notes segment in the core file.");

    wantedHeaders.push_back(noteSegment);
    /*
      * FROM GDB
      *
      *  Supported register note sections.
      *  static struct core_regset_section i386_linux_regset_sections[] =
      *  {
      *  { ".reg", 144 },
      *  { ".reg2", 108 },
      *  { ".reg-xfp", 512 },
      *  { NULL, 0 }
      *  };
      *
      *  Therefore assume, the following names used in elf.h:
      *  .reg == NT_PRSTATUS
      *  .reg2 == NT_FPREGSET
      *  .reg-xfp == NT_PRXFPREG
      */
    Nhdr *current = (Nhdr *)((char *)coreReader->elfFileHeader() + noteSegment->p_offset);
    Nhdr *end = (Nhdr *)((char *)current + noteSegment->p_filesz);

    while (current < end)
    {
        if (current->n_type == NT_PRSTATUS)
        {
            Status *status = (Status *)((char *)(current + 1) + align_power(current->n_namesz, 2));
            stackPointerAddresses.push_back((ADDRESS)status->pr_reg[ESP_OFFSET]);
            //The main process should have the lowest pid.  All the threads that are created
            //from it should have a higher process id
            if (status->pr_pid < processId)
                processId = status->pr_pid;
        }
        else if (current->n_type == NT_PRPSINFO)
        {
            Info *info = (Info *)((char *)(current + 1) + align_power(current->n_namesz, 2));
            //The first part of a programs arguments "argv[0]" should be the applications name
            //but not just the name it should include its path
            executableName = info->pr_psargs;
        }
		else if (current->n_type == NT_AUXV)
		{
			Auxv *aux = (Auxv *)((char *)(current + 1) + align_power(current->n_namesz, 2)); 
			while (aux->a_type != AT_NULL)
			{
				if (aux->a_type == AT_PHDR)
				{
					phdrAddr = (ADDRESS)aux->a_un.a_val;
					break; 
				}
				++aux;
			}
		}

        current = (Nhdr *)((char *)(current + 1) + align_power (current->n_namesz, 2)
                           + align_power (current->n_descsz, 2));
    }

    //without these pieces of information we have to assume that the core file may be corrupt
    //if this is the case then even gdb will not be able to parse it correctly,
    //so there is no point continue working on the core.
    if (!executableName || (processId == INT_MAX))
        LOG_RETURN(LOG_ERR, false, "Unable to determine file information");

    return true;
}

void Reducer::getStacks()
{
    for (unsigned int i = 0; i < stackPointerAddresses.size(); i++)
    {
        const Phdr *coreSegment = coreReader->getSegmentByAddress(stackPointerAddresses.at(i));
        if (!coreSegment)
            continue;

        Phdr *toStore = new Phdr;
        memcpy(toStore, coreSegment, sizeof(Phdr));

        //stacks grow downwards so the data between the top of the stack (esp) and the base of the
        //memory section is just junk data !! (hopefully :))
        if (stackPointerAddresses.at(i) - STACK_ADDITION > toStore->p_vaddr)
            toStore->p_vaddr = stackPointerAddresses.at(i) - STACK_ADDITION;
        //The size of the stack that we are interested in is the area between the the high level
        //memory address of the section and the esp
        toStore->p_filesz = (coreSegment->p_vaddr + coreSegment->p_filesz) - toStore->p_vaddr;
        toStore->p_memsz = toStore->p_filesz;
        //The offset into the file from where we want to copy the data is the esp.
        toStore->p_offset += (toStore->p_vaddr - coreSegment->p_vaddr);
        //store so it can be copied to the output core file later
        wantedHeaders.push_back(toStore);
		//These headers are are created and as such must be deleted correctly;
        dynamiclyCreatedHeaders.push_back(toStore);
    }
}

void Reducer::generateDynamicSectionInformation()
{
    // prepare new program header
    Phdr newHeader;
    newHeader.p_align = 1;
    newHeader.p_flags = PF_R;
    newHeader.p_memsz = newHeader.p_filesz = dynamicSectionSizeFromExecutable;
    newHeader.p_vaddr = dynamicAddressFromExecutable;
    newHeader.p_offset = 0;
    newHeader.p_type = PT_LOAD;
    newHeader.p_paddr = 0;

    int size = newHeader.p_filesz/sizeof(Elf_Dyn);

    Elf_Dyn *dyn = new Elf_Dyn[size];
    memset(dyn, 0, newHeader.p_filesz);
    //ensure that only the last element in the array is a pointer to null aka DT_NULL
    // overwrite the content
    // GDB firstly read the executable file to find the location of DT_DEBUG and after that it
    // is read from the coredump, so to speed-up we can just set every value to heapAddress
    for (int i = 0; i < size-1; i++)
        dyn[i].d_un.d_val = heapAddress;

    // add it to the target
    coreWriter->copySegment(&newHeader, (char *)dyn);

    delete [] dyn;
}

void Reducer::copyDynamicSectionInformation(const char *mapsFile)
{
    const Phdr *coreSegment = coreReader->getSegmentByAddress(dynamicAddressFromExecutable);
    if (!coreSegment)
    {
        // if maps file has to be used - generate dynamic section
        // even if information in the coredump is missing, DT_DEBUG section should be recreated
        if (mapsFile)
        {
            generateDynamicSectionInformation();
            createLinkMapInOutputFile(heapAddress, mapsFile);
        }
        return;
    }

    //Try to find the link map from the dynamic section
    Elf_Dyn *current = (Elf_Dyn *)((char *)coreReader->elfFileHeader() + coreSegment->p_offset
                                   + (dynamicAddressFromExecutable - coreSegment->p_vaddr));

    //a var used to calculate the offset of the DT_DEBUG dynamic section's address pointer
    //This address pointer has to be overwritten to make it point to the start of our r_debug
    //section.  Once this is overwritten then gdb can follow the link map correctly
    size_t offset = dynamicAddressFromExecutable - coreSegment->p_vaddr;

    while (current->d_tag != DT_NULL)
    {
        if (current->d_tag == DT_DEBUG)
        {
            //the offset should be shifted to point to the second variable of the struct
            //see elf.h
            offset += sizeof(Elf_SWord);
            coreWriter->copySegment(coreSegment, coreReader->getDataByOffset(coreSegment->p_offset),
                                    (char *)&heapAddress, offset, sizeof(ADDRESS));
            if(mapsFile)
                // maps file is given, use it to generate new debug information
                createLinkMapInOutputFile(current->d_un.d_ptr, mapsFile);
            else
                // else use original debug info
                copyLinkMapToOutputFile(current->d_un.d_ptr);
            break;
        }
        offset += sizeof(Elf_Dyn);
        current++;
    }
}

void Reducer::copyLinkMapToOutputFile(ADDRESS start)
{
    //The structure for DT_DEBUG may exist but it's pointer to r_debug info may be 0x0 meaning that
    //no debug information has been created or it has been stripped
    if (!start)
        return;

    //The output file is not initalized yet ??
    if (!coreWriter)
        return;

    //Initalize a segment for the link map
    coreWriter->startLinkMapSegment(heapAddress);

    const char *r_debugBuffer = getBufferAtAddress(start);
    //copy the r_debug structure to the new segment
    //and find the actual start of the link_map structure
    start = coreWriter->addR_DebugStruct(r_debugBuffer);
    ADDRESS stringAddress = 0;

    while (start)
    {
        //get a pointer to the structure that contains the link map header
        const char *linkMapBuffer = getBufferAtAddress(start);
        //find the library name string that the linkmap heder references
        stringAddress = *((ADDRESS *)(linkMapBuffer + LM_NAME));
        const char *stringBuffer = getBufferAtAddress(stringAddress);
        //The section that contains the refernece to the interpreter may be readonly in the origional
        //binary file, hence when the core dump occurs this data is not written to the core file.
        //We have already collected this data during the initalization phase (above)
        //so we can use it here so that the correct interpreter will be used, and hence gdb will load
        //the symbols correctly
        if (!stringBuffer && (stringAddress == interpAddress))
            stringBuffer = interpreter;

        //write the Link map structure and address to the output file
        //And get the address of the next link in the LM_LINK_MAP returned
        start = coreWriter->addLinkMapSegment(linkMapBuffer, stringBuffer);
    }

    //finish the segment for the link map
    coreWriter->finalizeLinkMapSegment();
}

void Reducer::createLinkMapInOutputFile(ADDRESS start, const char *mapsFile)
{
    //The structure for DT_DEBUG may exist but it's pointer to r_debug info may be 0x0 meaning that
    //no debug information has been created or it has been stripped
    if (!start || !mapsFile)
        return;

    //The output file is not initalized yet ??
    if (!coreWriter)
        return;

    // generate list of shared objects
    ProcInterface iface(processId);
    const std::vector<ProcInterface::SharedObject> *soList = iface.getSharedObjects(mapsFile);

    // if it is empty - do nothing
    if(soList->size()<=0)
        return;

    //Initalize a segment for the link map
    coreWriter->startLinkMapSegment(heapAddress);

    //create the r_debug structure in the new segment
    start = coreWriter->createR_DebugStruct();

    //add empty first link map item (should be so by GDB)
    coreWriter->createAndAddLinkMapSegment(0, 0, false, true);

    int size = soList->size();
    for (int i = 0; i < size; i++)
    {
        coreWriter->createAndAddLinkMapSegment(soList->at(i).addr, soList->at(i).name.c_str(),
                                               (i==(size-1)), false);
    }

    //finish the segment for the link map
    coreWriter->finalizeLinkMapSegment();
}

const char *Reducer::getBufferAtAddress(ADDRESS start)
{
    const Phdr *coreSegment = coreReader->getSegmentByAddress(start);
    if (!coreSegment)
        return NULL;

    char *buf = (char *)coreReader->elfFileHeader() + coreSegment->p_offset;
    if (!buf)
        return NULL;

    buf += (start - coreSegment->p_vaddr);
    return buf;
}

void Reducer::copyInitalSegmentsToOutput(bool stacksOnly)
{
    int fileSize = 0;
    for (unsigned int i = 0; i < wantedHeaders.size(); i++)
        fileSize += ((Phdr *)wantedHeaders.at(i))->p_filesz;

    //Setup a writer to store the newly created core file
    coreWriter = new RawElfWriter();

    int additionalHeaders = 0;
    //In addition to the Notes and stacks we want 2 headers reserved for the the dynamic section
    //information and the heap segment (by default)
    if (!stacksOnly)
        additionalHeaders = 2;
    if (!coreWriter->initalize(output, wantedHeaders.size() + additionalHeaders , fileSize))
        return;

    coreWriter->copyElfHeader(coreReader->elfFileHeader());
    for (unsigned int i = 0; i < wantedHeaders.size(); i++)
    {
        coreWriter->copySegment(wantedHeaders.at(i),
                                coreReader->getDataByOffset(((Phdr *)wantedHeaders.at(i))->p_offset));
    }
}



