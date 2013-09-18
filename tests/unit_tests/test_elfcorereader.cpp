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

#include "test_elfcorereader.h"
#include "CppUnitSignalException.h"
#include <limits.h>
#include <exception>
#include <sys/stat.h>
#include <fcntl.h>

/*****************************************************
  *
  * Register tests with the CPPUNIT framework
  *
  ***************************************************/
//register Test_ElfCoreReader with the CppUnit testFramework
CPPUNIT_TEST_SUITE_REGISTRATION (Test_ElfCoreReader);

void Test_ElfCoreReader::setUp()
{
    coreReader = new ElfCoreReader();
    CPPUNIT_ASSERT(coreReader != NULL);
    CPPUNIT_ASSERT(coreReader->initalize("/bin/bash") == true);
}

void Test_ElfCoreReader::tearDown()
{
    CPPUNIT_ASSERT_NO_SIGNAL_OR_EXCEPTION_MESSAGE("CoreReader Deleted before it should have been", delete(coreReader));
}

void Test_ElfCoreReader::initalizeTest()
{
    ElfCoreReader *reader = new ElfCoreReader();

    //A Null pointer is passed instead of a valid char array defining a file
    CPPUNIT_ASSERT(reader->initalize(NULL) == false);
    //Test for a file that does not exist
    CPPUNIT_ASSERT(reader->initalize("/bin/bash/doesNotExist") == false);
    //Test for a file that exists but is not an elf file.
    CPPUNIT_ASSERT(reader->initalize("test_elfbinaryreader.cpp") == false);
    //Test for a file that exists and is an elf file, does not matter that it is not a core file
    CPPUNIT_ASSERT(reader->initalize("/bin/bash") == true);

    if (reader)
        delete(reader);
}

void Test_ElfCoreReader::elfFileHeader_Uninitalized_Test()
{
    ElfCoreReader *reader = new ElfCoreReader();
    CPPUNIT_ASSERT(coreReader != NULL);
    //The class has not been initalized correctly so a NULL is what is expected
    CPPUNIT_ASSERT(reader->elfFileHeader() == NULL);
    if (reader)
        delete reader;
}

void Test_ElfCoreReader::elfFileHeader_Test()
{
    Ehdr *header1;
    //When initalized correctly we should have a pointer to the correct elf header
    CPPUNIT_ASSERT((header1 = coreReader->elfFileHeader()) != NULL);
}

void Test_ElfCoreReader::programHeader_Test()
{
    const Phdr *progHeader;
    CPPUNIT_ASSERT((progHeader = coreReader->programHeader()) != NULL);
}

void Test_ElfCoreReader::getDataByOffset_Test()
{
    int fd;
    CPPUNIT_ASSERT((fd = open("/bin/bash", O_RDONLY, 0)) >= 0);

    //Calculate the size of the file
    struct stat buf;
    fstat(fd, &buf);
    //Close the file handle
    ::close(fd);

    if (buf.st_size == 0)
        return; // empty file

    const char *data = NULL;
    //get a pointer to the 0'th byte
    CPPUNIT_ASSERT((data = coreReader->getDataByOffset(0)) != NULL);
    data = NULL;
    //get a pointer to a mid value byte
    CPPUNIT_ASSERT((data = coreReader->getDataByOffset(buf.st_size/2)) != NULL);
    data = NULL;
    //get a pointer to the last byte
    CPPUNIT_ASSERT((data = coreReader->getDataByOffset(buf.st_size - 1)) != NULL);
    data = NULL;
    //get a pointer to a byte 1 past the last byte
    CPPUNIT_ASSERT((data = coreReader->getDataByOffset(buf.st_size)) == NULL);
    data = NULL;
    //get a pointer well past the last byte, It should be NULL
    CPPUNIT_ASSERT((data = coreReader->getDataByOffset(UINT_MAX)) == NULL);
    data = NULL;
}

void Test_ElfCoreReader::getSegmentByAddress_Test()
{
    const Phdr *header1;
    const Phdr *header2;

    //Try an address that is well outside the address range for a standard application
    CPPUNIT_ASSERT(coreReader->getSegmentByAddress(0xffffffff) == NULL);
    //Try an address that might actually work
    //use an assert here so that we know whether we have a valid address or not.
    //if this fails it is not a major concern as it only proves that the elf file does not contain
    // a section with that address
    CPPUNIT_ASSERT((header1 = coreReader->getSegmentByAddress(0x0804c740)) != NULL);
    if (header1)
    {
        //if it is a real address check that two calls with the same address return the same data
        header2 = coreReader->getSegmentByAddress(0x0804c740);
        CPPUNIT_ASSERT(header1 == header2);
    }
}

void Test_ElfCoreReader::getSegmentByType_Test()
{
    const Phdr *segment;
    //We are looking at an executable file so we should be able to get an a dynamic section
    CPPUNIT_ASSERT((segment = coreReader->getSegmentByType(PT_DYNAMIC)) != NULL);
    //At the time of writing there is no section 21 so we will use that to prove that for a negative test
    CPPUNIT_ASSERT((segment = coreReader->getSegmentByType(21)) == NULL);
}

void Test_ElfCoreReader::getSegmentByIndex_Test()
{
    const Phdr *segment;
    Ehdr *header;
    //When initalized correctly we should have a pointer to the correct elf header
    CPPUNIT_ASSERT((header = coreReader->elfFileHeader()) != NULL);
    if (header->e_phnum <= 0)
        return;
    //try the 0'th index
    CPPUNIT_ASSERT((segment = coreReader->getSegmentByIndex(0)) != NULL);
    segment = NULL;
    //try the 1st index
    CPPUNIT_ASSERT((segment = coreReader->getSegmentByIndex(1)) != NULL);
    segment = NULL;
    //try a half way index
    CPPUNIT_ASSERT((segment = coreReader->getSegmentByIndex(header->e_phnum/2)) != NULL);
    segment = NULL;
    //try the last index
    CPPUNIT_ASSERT((segment = coreReader->getSegmentByIndex(header->e_phnum - 1)) != NULL);
    segment = NULL;
    //try the last index + 1
    CPPUNIT_ASSERT((segment = coreReader->getSegmentByIndex(header->e_phnum)) == NULL);
    segment = NULL;
    //try and index value that is well outside the range, it should be NULL.
    CPPUNIT_ASSERT((segment = coreReader->getSegmentByIndex(UINT_MAX)) == NULL);
    segment = NULL;
}

