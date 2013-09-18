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

#include "signalcatcher.h"
#include <stdlib.h>
#include <cxxabi.h>
#include <memory>

/******************************************************************************
  *
  * SignalException Implementation
  *
  ****************************************************************************/
SignalException::SignalException(int signalId)
    :   signalId(signalId)
{
    //create an array to hold the currently active function calls in the program
    void * addressArray[BACKTRACESIZE];

    //get a list of the backtraces in the application, not greater than BACKTRACESIZE in size
    int actualNumberOfAddresses = backtrace(addressArray, BACKTRACESIZE);

    //convert the addresses to symbolic names and offsets
    char ** symbols = backtrace_symbols(addressArray, actualNumberOfAddresses);

    for (int i = 0; i < actualNumberOfAddresses; i++)
    {
        // Demangling code adapted from (http://www.boxbackup.org/trac/changeset/2178)
        std::string mangled_frame = symbols[i];
        std::string output_frame  = symbols[i]; // default will be mangled unless the correct symbols can be found

        int start = mangled_frame.find('(');
        int end   = mangled_frame.find('+', start);
        std::string mangled_func = mangled_frame.substr(start + 1, end - start - 1);

        size_t len = 256;
        std::auto_ptr<char> output_buf(new char [len]);
        int status;

        if (__cxxabiv1::__cxa_demangle(mangled_func.c_str(), output_buf.get(), &len, &status) != NULL)
        {
            output_frame = mangled_frame.substr(0, start + 1) + output_buf.get() + mangled_frame.substr(end);
        }
        //End of demangling code

        //store the back trace so it can be received in the catch statement
        stackTrace.append(output_frame);
        stackTrace.append("\n");
    }
    //(The strings pointed to by the array of pointers need not and should not be freed.) man 3 backtrace
    delete(symbols);
}

/******************************************************************************
  *
  * SignalCatcher Implementation
  *
  ****************************************************************************/

SignalCatcher* SignalCatcher::Instance()
{
    //create only one instance
    //static vars are automatically freed on program termination
    //this method means that only single threaded use is supported
    static SignalCatcher instance;
    //return pointer to the sole instance
    return &instance;
}

SignalCatcher::SignalCatcher()
{
    addDefaultSignals();
    currentException = NULL;
}

SignalCatcher::~SignalCatcher()
{
    if(currentException != NULL)
        delete(currentException);
    currentException = NULL;
}

void SignalCatcher::addDefaultSignals()
{
    //These are the default signals to monitor
    sigemptyset( &signalsToMonitor );
    //Illegal Instruction (4)
    sigaddset( &signalsToMonitor, SIGILL );
    //grab the signal if it is fired
    listen(SIGILL);
    //Floating point exception (8)
    sigaddset( &signalsToMonitor, SIGFPE );
    //grab the signal if it is fired
    listen(SIGFPE);
    //Invalid memory reference (11)
    sigaddset( &signalsToMonitor, SIGSEGV );
    //grab the signal if it is fired
    listen(SIGSEGV);
    // Bus error (bad memory access) (10,7,10) depending on hardware
    sigaddset( &signalsToMonitor, SIGBUS );
    //grab the signal if it is fired
    listen(SIGBUS);
    //Set the signal mask
    pthread_sigmask( SIG_SETMASK, &signalsToMonitor, NULL );
    //keep the recursiveSignal mask upto date
    recursiveSignalMask = signalsToMonitor;
}

void SignalCatcher::removeAllSignals()
{
    sigemptyset( &signalsToMonitor );
    //Set the signal mask
    pthread_sigmask( SIG_SETMASK, &signalsToMonitor, NULL );
    //keep the recursiveSignal mask upto date
    recursiveSignalMask = signalsToMonitor;
}

void SignalCatcher::addSignal(int signalId)
{
    sigaddset( &signalsToMonitor, signalId);
    //update the signal mask
    pthread_sigmask( SIG_SETMASK, &signalsToMonitor, NULL );
    //keep the recursiveSignal mask upto date
    recursiveSignalMask = signalsToMonitor;
    //grab the signal if it is fired
    listen(signalId);
}

void SignalCatcher::removeSignal(int signalId)
{
    sigdelset( &signalsToMonitor, signalId);
    //update teh signal mask
    pthread_sigmask( SIG_SETMASK, &signalsToMonitor, NULL );
    //keep the recursiveSignal mask upto date
    recursiveSignalMask = signalsToMonitor;

    std::map<int, struct sigaction>::iterator iterator = signalActionMap.find(signalId);
    if(iterator == signalActionMap.end())
    {
        //the signal was never mapped to a user defined action so just return
        return;
    }

    //set the origional action back for the given signal
    sigaction(signalId , &iterator->second, NULL);
    //remove our mapping of the signal
    signalActionMap.erase(signalId);
}

void SignalCatcher::listen(int signalId)
{
    std::map<int, struct sigaction>::iterator iterator = signalActionMap.find(signalId);
    if(iterator != signalActionMap.end())
    {
        //the signal is already mapped
        return;
    }

    //create a sigaction struct to hold the preferred action to associate with a Signal
    struct sigaction preferredSignalAction;
    //create a container for the old signal action, this can be used to restore the origional handling
    //for a given signal
    struct sigaction origionalSignalAction;
    //Clear the signal mask
    sigemptyset( &preferredSignalAction.sa_mask );
    //dont set any flags
    preferredSignalAction.sa_flags = 0;
    //define a method that will handle the signal when it is emitted
    preferredSignalAction.sa_handler = SignalCatcher::catcher;
    //assign the signal the new/preferred action
    sigaction(signalId , &preferredSignalAction, &origionalSignalAction);

    //store the signal and the old action so that they can be restored later
    signalActionMap.insert(std::pair<int, struct sigaction>(signalId, origionalSignalAction));
}

void SignalCatcher::ensureSignalSet()
{
    //Make sure that the signal mask is set to the correct default state on recursive calls
    //so that these signals can be restored when a signal is emitted and the stack is rolled back
    //by the siglongjmp.  they are freed in SignalCatcher::prepareToRunUserCode so if not set here on a recursive
    //call TRY the signals would be empty upon return
    pthread_sigmask( SIG_SETMASK, &recursiveSignalMask, NULL );
}

void SignalCatcher::catcher(int signalId)
{
    //store the signal id that has been caught
    SignalCatcher::Instance()->caughtSignalId = signalId;
    //if an old exception has bee created and was not freed do it now
    if(SignalCatcher::Instance()->currentException != NULL)
        delete(SignalCatcher::Instance()->currentException);
    //Take the stacktrace here
    SignalCatcher::Instance()->currentException = new SignalException(signalId);
    //jump all the way back to the point where the stack was saved by the call to sigsetjmp in TRY macro
    siglongjmp( *SignalCatcher::Instance()->jumpLocationStack.top().bufferPointer() , -1 );
}

void SignalCatcher::recover()
{
    //reset the signal mask so that the fatal signals are blocked
    pthread_sigmask( SIG_SETMASK, NULL, &signalsToMonitor );
    //Throw exception to notify the caller
    if(SignalCatcher::Instance()->currentException != NULL)
        throw *SignalCatcher::Instance()->currentException;
}

void SignalCatcher::prepareToRunUserCode(sigjmp_buf_Wrapper &stackMark)
{
    //unlock all signals so that they can be emmitted and caught.
    //handlers have already been assigned to the signals so we need the signals to be emmitted
    //before we can catch them and process them.  see listen to see the catch assignment
    sigemptyset(&signalsToMonitor);
    pthread_sigmask( SIG_SETMASK, &signalsToMonitor, NULL );
    //store the stack jump location
    jumpLocationStack.push(stackMark);
}
