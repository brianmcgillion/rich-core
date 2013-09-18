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

#ifndef SIGNALCATCHER_H
#define SIGNALCATCHER_H

#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <stdio.h>
#include <stack>
#include <map>
#include <exception>
#include <execinfo.h>
#include <string>
#include <pthread.h>

/*!
  * \brief Define how many function calls at most to retrieve from a backtrace call
  * \details A backtrace is called whenever a fatal signal is caught that would have crashed
  * a production system.  It allows one to see the state of the stack at the time it was called.
  * \sa man 3 backtrace
  */
#define BACKTRACESIZE 100

/*!
 *  \class sigjmp_buf_Wrapper
 *  \brief Wrap a sigjmp_buf that will be used for sigsetjmp.
 *  \sa man 3 sigsetjmp
 */
class sigjmp_buf_Wrapper
{
public:

    /*!
      * \brief Get a poiner to the underlying buffer
      * \returns pointer to a sigjmp_buf instance
      */
    sigjmp_buf * bufferPointer()
    {
        return &jumpLocation;
    }

private:
    //! The wrapped sigjmp_buf
    sigjmp_buf jumpLocation;
};


/*!
 *  \class SignalException
 *  \brief A specialization Exception class
 *  \details This calss is used to notify that a terminal (as in RIP :)) signal was caught.  It provides
 *  the signal id that was emmitted and there is also a backtrace provided to help
 *  with the diagnostics of the issue. see man 3 backtrace for details of the backtrace functionality.
 */
class SignalException
{
public:
    /*!
      * \brief Default constructor
      * \param signalId the id of the signal that was caught.
      * \details takes the signal that was emmitted and generates a backtrace to try
      * and highlight the cause of the problem
      */
    SignalException(int signalId);

    /*!
      * \brief Copy constructor
      * \param other The other SignalException to copy
      */
    SignalException(const SignalException &other)
        :   signalId(other.signalId)
    {
        //copy the old stringstream to the new one
        stackTrace.clear();
        stackTrace.append(other.getStackTrace());
    }

    /*!
      * \brief get the id of the signal that caused this exception
      * \returns the id of the signal.
      * \sa see man 7 signals for definition of signals
      */
    int getSignalId() const
    {
        return signalId;
    }

    /*!
      * \brief get a reference to the stack trace that was created from backtrace
      * \return A reference to the stack trace as a string stream
      * \sa man 3 backtrace
      */
    const std::string & getStackTrace() const
    {
        return stackTrace;
    }

private:
    //! The signal id that was caught
    int signalId;

    //! A string that contains the output from the stack trace
    std::string stackTrace;
};



/*!
 *  \class SignalCatcher
 *  \brief Catch a set of signals that may be emmitted
 *  \details This class handles the creation of a listening mechanism for signals
 *  that may be emmitted.  By default SIGILL, SIGFPE, SIGSEGV, SIGBUS
 *  are being monitored.  These signals are known to cause a core dump if they are unhandled.
 *  This class is designed to be used in a single threaded instance and hence is not thread
 *  safe.  This should only be used in the creation of test cases and is not intended for production level software.
 *  The very nature of the signals being caught signifies that a serious flaw has been found and the code causing these
 *  issues should be fixed.
 */
class SignalCatcher
{
public:
    /*!
      * \brief Get a pointer to the sole instance of the SignalCatcher
      * \return A pointer to the instance of the class
      * \details This class implements a singleton pattern so this method
      * handles brovides access to the instance.
      */
    static SignalCatcher* Instance();

    /*!
      * \brief Add a signal to the list of signals to be monitored
      * \param signalId The Id of the signal to monitor (see man 7 signal)
      * \details Add a new signal and update the signal mask. By default SIGILL, SIGFPE, SIGSEGV, SIGBUS
      * are being monitored.
      * \sa man 7 signal
      */
    void addSignal(int signalId);

    /*!
      * \brief Remove a signal to the list of signals to be monitored
      * \param signalId The Id of the signal being monitored (see man 7 signal)
      * \details By default SIGILL, SIGFPE, SIGSEGV, SIGBUS are being monitored.
      * \sa man 7 signal
      */
    void removeSignal(int SignalId);

    /*!
      * \brief Attempt recover form a fatal signal
      * \details Try to reinstate the previous signal state so that the application can continue.  This method
      * also throws the excepton that was created by catcher.
      */
    void recover();

    /*!
      * \brief This method is called before the users code is run.
      * \param stackMark The marked position on the stack that control will be returned if an error occurs
      * \details Unlock all signals so that they can be emmitted and caught.  Handlers have already been assigned
      * to the signals so we need the signals to be emmitted before we can catch them and process them.
      * see SignalCatcher::listen to see the catch assignment.
      * \sa SignalCatcher::listen
      */
    void prepareToRunUserCode(sigjmp_buf_Wrapper &stackMark);

    /*!
      * \brief Makes sure that the default signals are set.
      * \details For recursuve calls to TRY make sure that the correct signals have been set, so that they can
      * be reset after a fatal signal has been caught.  This is required if the previous TRY/CATCH did not encounter
      * a error situation, because the first call to TRY/CATCH will have stopped blocking the signals and unless
      * an error occured they will still be unblocked, so if the subsequent call encounters an error situation it
      * will try to block no signals.
      */
    void ensureSignalSet();

    /*!
      * \brief Default Destructor
      */
    ~SignalCatcher();

protected:

    /*!
      * \brief Default constructor
      * \details Defined as non-public to support the singleton pattern
      */
    SignalCatcher();

    /*!
      * \brief Add all the default signals to be monitored and update the signal mask
      * \details By default SIGILL, SIGFPE, SIGSEGV, SIGBUS are being monitored. (see man 7 signal)
      * \sa man 7 signal
      */
    void addDefaultSignals();

    /*!
      * \brief Clear out the signal mask so that no signals are bing monitored
      */
    void removeAllSignals();

    /*!
      * \brief Used by sigaction (see man 2 sigaction) to handle a signal that is emmitted
      * \param signalId The id of the signal emmitted
      * \details sigaction will pass the signal id of the signal that has been caught to this method
      * \sa man 2 sigaction
      */
    static void catcher (int signalId);

    /*!
      * \brief Start listening for signals and change the default action (see man 2 sigaction)
      * \param signalId The Id of the signal to intercept (see man 7 signal)
      * \details By default SIGILL, SIGFPE, SIGSEGV, SIGBUS are being monitored.
      * \sa man 2 sigaction
      * \sa man 7 signal
      */
    void listen(int signalId);

private:

    //! create a mapping of a signal to the origional action that this signal resulted in so that we can
    //! stop listening for a signal when it is no longer required
    std::map<int, struct sigaction> signalActionMap;

    //! store the jump positions created by sigsetjmp so that we can roll back one at a time
    std::stack<sigjmp_buf_Wrapper> jumpLocationStack;

    //! A signal mask that can be used to block signals
    sigset_t signalsToMonitor;

    //! A signal mask that is used for recursive calls to set mask
    sigset_t recursiveSignalMask; 

    //! The id of the error signal that was caught
    int caughtSignalId;

    //! The current exception that is created when a fatal signal is encountered
    SignalException *currentException;
};

/*!
  * \brief  A macro that is used to start the encapsulation of potentially unsafe code
  * \details This Macro uses the SignalCatcher class to mark the current position on the stack,
  * and keep track of the state of the signals.  sigsetjmp(...) is the point that siglongjmp call see
  * (SignalCatcher::catcher()) will return to if a fatal signal is encountered.  If the vaule returned by sigsetjmp
  * is zero then it means that no fatal signals were encountered, otherwise one was and the stack has bee rolled back
  * by catcher. So we must throw an exception here to notify the caller. On the first attempt the return value of sigsetjmp
  * should be 0, it is only after a call to siglongjmp that the value will be non zero. This should be use for all multiline purposes.
  * if a one line catch is required try and use CPPUNIT_ASSERT_NO_SIGNAL_OR_EXCEPION().
  * \sa SignalCatcher::recover()
  * \sa SignalCatcher::catcher()
  * \sa SignalCatcher::listen()
  * \sa SignalException
  *
  * Example of use
  * \code
  *
  * #include "signalcatcher.h"
  *
  * myTester::testMethod()
  *{
  *
  * TRY
    {
        method1();
        method2();
            :
            :
            :
        methodn()
    }
    CATCH
    {
        //The default catch variable available to the user here is e
        // it provides std::string e.getStackTrace()
        // and int e.getSignalId()
        std::cout << e.getStackTrace() << std::endl;
        std::cout << e.getSignalId() << std::endl;
        std::cout << "Catch SignalException " << std::endl;
    }
    catch(const std::exception &e)
    {
        std::cout << std::string("What(): ") + e.what() << std::endl;
        std::cout << "Catch standard exception or exception derived from std::exception" << std::endl;
    }
    catch(...)
    {
        std::cout << "Catch everything else" << std::endl;
    }

    }
  * \endcode
  */
#define TRY try{ \
sigjmp_buf_Wrapper stackMark; \
SignalCatcher::Instance()->ensureSignalSet(); \
if( sigsetjmp( *(stackMark.bufferPointer()), 1 ) != 0 ) \
{ \
  SignalCatcher::Instance()->recover(); \
} \
else \
{ \
  SignalCatcher::Instance()->prepareToRunUserCode(stackMark); \
}

/*!
  * \brief A macro used to surround user code
  * \details when used with TRY this combination of
  * macros will help to encapsulate potentially unsafe code
  * and deal with any SignalExceptions that may be thrown
  * \sa TRY
  */
#define CATCH } catch(const SignalException &e)

#endif // SIGNALCATCHER_H
