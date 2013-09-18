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

#include "test_elfbinaryreader.h"
#include "CppUnitSignalException.h"
#include <limits.h>
#include <exception>
/*****************************************************
  *
  * Register tests with the CPPUNIT framework
  *
  ***************************************************/

//register Test_ElfBinaryReader with the CppUnit testFramework
CPPUNIT_TEST_SUITE_REGISTRATION (Test_ElfBinaryReader);

void Test_ElfBinaryReader::setUp()
{
    binaryReader = new ElfBinaryReader();
    CPPUNIT_ASSERT(binaryReader != NULL);

    CPPUNIT_ASSERT(binaryReader->initalize("/bin/bash") == true);
}

void Test_ElfBinaryReader::tearDown()
{
    CPPUNIT_ASSERT_NO_SIGNAL_OR_EXCEPTION_MESSAGE("BinaryReader Deleted before it should have been", delete(binaryReader));
}

void Test_ElfBinaryReader::initalizeTest()
{
    ElfBinaryReader *reader = new ElfBinaryReader();

    //A Null pointer is passed instead of a valid char array defining a file
    CPPUNIT_ASSERT(reader->initalize(NULL) == false);
    //Test for a file that does not exist
    CPPUNIT_ASSERT(reader->initalize("/bin/bash/doesNotExist") == false);
    //Test for a file that exists but is not an elf file.
    CPPUNIT_ASSERT(reader->initalize("test_elfbinaryreader.cpp") == false);
    //Test for a binary(executable) file that exists
    CPPUNIT_ASSERT(reader->initalize("/bin/bash") == true);

    if (reader)
        delete(reader);
}

void Test_ElfBinaryReader::getSectionByName_Test()
{
    //test that we return properly if NULL is passed to method
    CPPUNIT_ASSERT(binaryReader->getSectionByName(NULL) == NULL);
    //Test that the given section actually exists,  All executables should have a .text section
    CPPUNIT_ASSERT(binaryReader->getSectionByName(".text") != NULL);
}

void Test_ElfBinaryReader::getSectionByIndex_Test()
{
    const CurrentSectionData *sectionData1;
    const CurrentSectionData *sectionData2;

    //try to get an out of range index and hopefully it will return correctly
    CPPUNIT_ASSERT(binaryReader->getSectionByIndex(ULONG_MAX) == NULL);
    //try an index that is in range
    CPPUNIT_ASSERT((sectionData1 = binaryReader->getSectionByIndex(1)) != NULL);
    //try an index that is in range again, this will cover the code in ElfBinaryReader::getSection()
    //that tests if we have already found the current section, so that it does not repeat the search
    //for data that is already current
    CPPUNIT_ASSERT((sectionData2 = binaryReader->getSectionByIndex(1)) != NULL);

    //assert that sections returned are the same
    CPPUNIT_ASSERT(sectionData1 == sectionData2);
}

void Test_ElfBinaryReader::getSectionByAddress_Test()
{
    const CurrentSectionData *sectionData1;
    const CurrentSectionData *sectionData2;

    //Try an address that is well outside the address range for a standard application
    CPPUNIT_ASSERT(binaryReader->getSectionByAddress(0xffffffff) == NULL);
    //Try an address that might actually work
    //use an assert here so that we know whether we have a valid address or not.
    //if this fails it is not a major concern as it only proves that the elf file does not contain
    // a section with that address
    CPPUNIT_ASSERT((sectionData1 = binaryReader->getSectionByAddress(0x0804c740)) != NULL);
    if (sectionData1)
    {
        //if it is a real address check that two calls with the same address return the same data
        sectionData2 = binaryReader->getSectionByAddress(0x0804c740);
        CPPUNIT_ASSERT(sectionData1 == sectionData2);
    }
}

void Test_ElfBinaryReader::getSectionByType_Test()
{
    const CurrentSectionData *binarySectionData;
    //We are looking at an executable file so we should be able to get an a dynamic section
    CPPUNIT_ASSERT((binarySectionData = binaryReader->getSectionByType(SHT_DYNAMIC)) != NULL);
    //At the time of writing there is no section 21 so we will use that to prove that for a negative test
    CPPUNIT_ASSERT((binarySectionData = binaryReader->getSectionByType(21)) == NULL);
}

void Test_ElfBinaryReader::close_Test()
{
    //attempt to close the underlying file handles in the biary reader
    CPPUNIT_ASSERT_NO_SIGNAL_OR_EXCEPTION_MESSAGE("Error closing the ElfBinaryReader", binaryReader->close());
    //and do it again to ensure no negative effect
    CPPUNIT_ASSERT_NO_SIGNAL_OR_EXCEPTION_MESSAGE("Error closing the ElfBinaryReader", binaryReader->close());
}
