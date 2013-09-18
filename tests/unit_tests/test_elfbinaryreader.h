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
  * \file test_elfbinaryreader.h
  * \author Brian McGillion <brian.mcgillion@symbio.com>, Denis Mingulov <denis.mingulov@symbio.com>
  * \class Test_ElfBinaryReader
  * \brief Contains the functionality for testing ElfBinaryReader
  */

#ifndef TEST_ELFBINARYREADER_H
#define TEST_ELFBINARYREADER_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "elfbinaryreader.h"

namespace CppUnit
{
    /*!
      * \brief A specialization of the CppUnit::assertion_traits struct
      * \details This struct allows for CurrentSectionData structure to to be tested for Equality
      * by using the CPPUNIT_ASSERT_*() methods
      */
    template<> struct assertion_traits<CurrentSectionData>
    {
        /*!
          * \brief Test for equality between two instances of CurrentSectionData
          * \param data1 First value to compare
          * \param data2 The value to compare against
          * \return Returns true if the values match and false otherwise
          * \details Compare the coontents of two CurrentSectionData instances to see if they are equal
          */
        static bool equal( const CurrentSectionData& data1, const CurrentSectionData& data2 )
        {
            //Test to see that the section index's are the same.  Then test the sectionHeader
            //sub-structures if these are the same then it can be assumed that the sections are the same.
            if (data1.index != data2.index)
                return false;
            if (data1.sectionHeader->sh_addr != data2.sectionHeader->sh_addr)
                return false;
            if (data1.sectionHeader->sh_addralign != data2.sectionHeader->sh_addralign)
                return false;
            if (data1.sectionHeader->sh_entsize != data2.sectionHeader->sh_entsize)
                return false;
            if (data1.sectionHeader->sh_flags != data2.sectionHeader->sh_flags)
                return false;
            if (data1.sectionHeader->sh_info != data2.sectionHeader->sh_info)
                return false;
            if (data1.sectionHeader->sh_link != data2.sectionHeader->sh_link)
                return false;
            if (data1.sectionHeader->sh_name != data2.sectionHeader->sh_name)
                return false;
            if (data1.sectionHeader->sh_offset != data2.sectionHeader->sh_offset)
                return false;
            if (data1.sectionHeader->sh_size != data2.sectionHeader->sh_size)
                return false;
            if (data1.sectionHeader->sh_type != data2.sectionHeader->sh_type)
                return false;

            return true;
        }

        /*!
          * \brief Create a string representation of a CurrentSectionData instance
          * \param data The CurrentSectionData instance to write out
          * \return The string that is created from the object
          * \details This method is used to get a prepresentation of what was expected and what
          * was the actual result of an equality comparison between two CurrentSectionData instances
          */
        static std::string toString( const CurrentSectionData& data )
        {
            OStringStream ost;
            ost <<  "Index =  " << data.index << "  address =  " << data.sectionHeader->sh_addr <<
                    "  address Alignment = " << data.sectionHeader->sh_addralign << "  Entry size = " <<
                    data.sectionHeader->sh_entsize << "  Flags = " << data.sectionHeader->sh_flags <<
                    "  Info = " << data.sectionHeader->sh_info << "  Link = " << data.sectionHeader->sh_link <<
                    "  Name = " << data.sectionHeader->sh_name << "  Offset " << data.sectionHeader->sh_offset <<
                    "  Size = " << data.sectionHeader->sh_size  << "  Type = "  << data.sectionHeader->sh_type;
            return ost.str();
        }
    };
}


//TODO Add tests for the private members of ElfBinaryReader
//this will require making this class a friend class.
class Test_ElfBinaryReader : public CppUnit::TestFixture
{
    //Declare A test suite and the methods that are going to be called from it.
    CPPUNIT_TEST_SUITE (Test_ElfBinaryReader);
    CPPUNIT_TEST (initalizeTest);
    CPPUNIT_TEST (getSectionByName_Test);
    CPPUNIT_TEST (getSectionByIndex_Test);
    CPPUNIT_TEST (getSectionByAddress_Test);
    CPPUNIT_TEST (getSectionByType_Test);
    CPPUNIT_TEST (close_Test);
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
      * \brief Test ElfBinaryReader::initalize()
      */
    void initalizeTest();
    /*!
      * \brief Test ElfBinaryReader::getSectionByName()
      */
    void getSectionByName_Test();
    /*!
      * \brief Test ElfBinaryReader::getSectionByIndex()
      */
    void getSectionByIndex_Test();
    /*!
      * \brief Test ElfBinaryReader::getSectionByAddress()
      */
    void getSectionByAddress_Test();
    /*!
      * \brief Test ElfBinaryReader::getSectionByType()
      */
    void getSectionByType_Test();
    /*!
      * \brief Test ElfBinaryReader::close()
      */
    void close_Test();

private:
    ElfBinaryReader *binaryReader;
};

#endif // TEST_ELFBINARYREADER_H
