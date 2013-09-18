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

#ifndef CPPUNITSIGNALEXCEPTION_H
#define CPPUNITSIGNALEXCEPTION_H
#include <cppunit/extensions/HelperMacros.h>
#include "signalcatcher.h"
#include <signal.h>

namespace CppUnit
{
    /*!
      * \brief  An extension MACRO for the CppUnit framework
      * \param message An Additional user defined message
      * \param expression The line of code that is to be run
      *
      * \details This macro takes a line of code that is expected to throw an exception
      * or emmit a fatal signal. This macro is a wrapper around the SignalCatcher class
      * and its associated TRY macro.  When a fatal signal or unhandled exception are found
      * a stack trace is taken and an warning message is output in the CppUnit log, or xml file
      * if xml hooking is used.
      *
      * Example usage:
      * \code
      * #include CppUnitSignalException.h
      *
      * void Region_test::AreaTest()
      * {
      *     Rect area1(2, 3, 50, 50);
      *     Region region1(area1);
      *     //The following line is expected to throw a signal 11 (SIGSEGV) if NULL is unhandled
      *     CPPUNIT_ASSERT_NO_SIGNAL_OR_EXCEPTION_MESSAGE("No NULL checking performed", region1.ShrinkArea(NULL));
      * }
      * \endcode
      */
# define CPPUNIT_ASSERT_NO_SIGNAL_OR_EXCEPTION_MESSAGE( message, expression )                \
    do{                                                                     \
        CPPUNIT_NS::Message cpputMsg_( "Fatal signal/exception emitted and caught" ); \
        cpputMsg_.addDetail( message );                                     \
        TRY                                                                 \
        {                                                                   \
            expression;                                                     \
        }                                                                   \
        CATCH                                                               \
        {                                                                   \
            cpputMsg_.addDetail( "Caught: " + CPPUNIT_EXTRACT_EXCEPTION_TYPE_( e, "std::exception or derived" ) ); \
            cpputMsg_.addDetail( e.getStackTrace() );                      \
            CPPUNIT_NS::Asserter::fail( cpputMsg_, CPPUNIT_SOURCELINE() );  \
        }                                                                   \
        catch ( const std::exception &e )                                   \
        {                                                                   \
            cpputMsg_.addDetail( "Caught: " +  CPPUNIT_EXTRACT_EXCEPTION_TYPE_( e, "std::exception or derived" ) );    \
            cpputMsg_.addDetail( std::string("What(): ") + e.what() );      \
            CPPUNIT_NS::Asserter::fail( cpputMsg_, CPPUNIT_SOURCELINE() );  \
        }                                                                   \
        catch(...)                                                          \
        {                                                                   \
            cpputMsg_.addDetail( "Caught: unknown. exception" );            \
            CPPUNIT_NS::Asserter::fail( cpputMsg_, CPPUNIT_SOURCELINE() );  \
        }                                                                   \
    }while(false);

    /*!
      * \brief  An extension MACRO for the CppUnit framework
      * \param expression The line of code that is to be run
      *
      * \details This macro takes a line of code that is expected to throw an exception
      * or emmit a fatal signal. This macro is a wrapper around the SignalCatcher class
      * and its associated TRY macro.  When a fatal signal or unhandled exception are found
      * a stack trace is taken and an warning message is output in the CppUnit log, or xml file
      * if xml hooking is used.
      *
      * Example usage:
      * \code
      * #include CppUnitSignalException.h
      *
      * void Region_test::clipCairoTest()
      * {
      *     Rect area1(2, 3, 50, 50);
      *     Region region1(area1);
      *     //The following line is expected to throw a signal 11 (SIGSEGV) if NULL is unhandled
      *     CPPUNIT_ASSERT_NO_SIGNAL_OR_EXCEPTION(region1.ShrinkArea(NULL));
      * }
      * \endcode
      */
# define CPPUNIT_ASSERT_NO_SIGNAL_OR_EXCEPTION( expression )                             \
   CPPUNIT_ASSERT_NO_SIGNAL_OR_EXCEPTION_MESSAGE( CPPUNIT_NS::AdditionalMessage(), expression )

}
#endif // CPPUNITSIGNALEXCEPTION_H
