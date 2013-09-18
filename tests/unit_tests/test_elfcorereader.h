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
  * \file test_elfcorereader.h
  * \author Brian McGillion <brian.mcgillion@symbio.com>, Denis Mingulov <denis.mingulov@symbio.com>
  * \class Test_ElfCoreReader
  * \brief Contains the functionality for testing ElfCoreReader
  */

#ifndef TEST_ELFCOREREADER_H
#define TEST_ELFCOREREADER_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "elfcorereader.h"

namespace CppUnit
{
    /*!
      * \brief A specialization of the CppUnit::assertion_traits struct
      * \details This struct allows for Phdr structure to to be tested for Equality
      * by using the CPPUNIT_ASSERT_*() methods
      */
    template<> struct assertion_traits<Phdr>
    {
        /*!
          * \brief Test for equality between two instances of Phdr
          * \param header1 First value to compare
          * \param header2 The value to compare against
          * \return Returns true if the values match and false otherwise
          * \details Compare the contents of two Phdr (program Header) instances to see if they are equal
          */
        static bool equal( const Phdr& header1, const Phdr& header2 )
        {
            if (header1.p_align != header2.p_align)
                return false;
            if (header1.p_filesz != header2.p_filesz)
                return false;
            if (header1.p_flags != header2.p_flags)
                return false;
            if (header1.p_memsz != header2.p_memsz)
                return false;
            if (header1.p_offset != header2.p_offset)
                return false;
            if (header1.p_paddr != header2.p_paddr)
                return false;
            if (header1.p_type != header2.p_type)
                return false;
            if (header1.p_vaddr != header2.p_vaddr)
                return false;

            return true;
        }

        /*!
          * \brief Create a string representation of a Phdr instance
          * \param header The Phdr instance to write out
          * \return The string that is created from the object
          * \details This method is used to get a prepresentation of what was expected and what
          * was the actual result of an equality comparison between two Phdr instances
          */
        static std::string toString( const Phdr& header )
        {
            OStringStream ost;
            ost <<  "Align =  " << header.p_align << "  File Size =  " << header.p_filesz <<
                    "  Flags = " << header.p_flags << "  Memory Size = " << header.p_memsz <<
                    "  Offset = " << header.p_offset << "  Program Address = " << header.p_paddr <<
                    "  Type  = " << header.p_type << "  Virtual Memory Address = " << header.p_vaddr;
            return ost.str();
        }
    };
}

class Test_ElfCoreReader : public CppUnit::TestFixture
{

    //Declare A test suite and the methods that are going to be called from it.
    CPPUNIT_TEST_SUITE (Test_ElfCoreReader);
    CPPUNIT_TEST (initalizeTest);
    CPPUNIT_TEST (elfFileHeader_Uninitalized_Test);
    CPPUNIT_TEST (elfFileHeader_Test);
    CPPUNIT_TEST (programHeader_Test);
    CPPUNIT_TEST (getDataByOffset_Test);
    CPPUNIT_TEST (getSegmentByAddress_Test);
    CPPUNIT_TEST (getSegmentByType_Test);
    CPPUNIT_TEST (getSegmentByIndex_Test);
    CPPUNIT_TEST_SUITE_END ();

public:
    /*!
      * \brief Initalize data for the test cases
      * \details Create data for use in the test cases
      */
    void setUp();

    /*!
      * \brief Clean up after the tests have been run
      */
    void tearDown();

protected:
    /*!
      * \brief Test ElfCoreReader::initalize()
      */
    void initalizeTest();

    /*!
      * \brief Test ElfCoreReader::elfFileHeader()
      * Test that a NULL value is returned if the class has not been initalized correctly
      */
    void elfFileHeader_Uninitalized_Test();

    /*!
      * \brief Test ElfCoreReader::elfFileHeader()
      */
    void elfFileHeader_Test();

    /*!
      * \brief Test ElfCoreReader::programHeader()
      */
    void programHeader_Test();

    /*!
      * \brief Test ElfCoreReader::getDataByOffset()
      */
    void getDataByOffset_Test();

    /*!
      * \brief Test ElfCoreReader::getDataByAddress()
      */
    void getSegmentByAddress_Test();

    /*!
      * \brief Test ElfCoreReader::getDataByType()
      */
    void getSegmentByType_Test();

    /*!
      * \brief Test ElfCoreReader::getDataByIndex()
      */
    void getSegmentByIndex_Test();

private:
    ElfCoreReader *coreReader;
};

#endif // TEST_ELFCOREREADER_H
