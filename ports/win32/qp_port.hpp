//============================================================================
// QP/C++ Real-Time Embedded Framework (RTEF)
//
//                   Q u a n t u m  L e a P s
//                   ------------------------
//                   Modern Embedded Software
//
// Copyright (C) 2005 Quantum Leaps, LLC <state-machine.com>.
//
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
//
// This software is dual-licensed under the terms of the open source GNU
// General Public License version 3 (or any later version), or alternatively,
// under the terms of one of the closed source Quantum Leaps commercial
// licenses.
//
// The terms of the open source GNU General Public License version 3
// can be found at: <www.gnu.org/licenses/gpl-3.0>
//
// The terms of the closed source Quantum Leaps commercial licenses
// can be found at: <www.state-machine.com/licensing>
//
// Redistributions in source code must retain this top-level comment block.
// Plagiarizing this software to sidestep the license obligations is illegal.
//
// Contact information:
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//============================================================================
//! @date Last updated on: 2023-09-07
//! @version Last updated for: @ref qpcpp_7_3_0
//!
//! @file
//! @brief QP/C++ port to Win32 (multithreaded), generic C++11

#ifndef QP_PORT_HPP_
#define QP_PORT_HPP_

#include <cstdint>    // Exact-width types. C++11 Standard

#ifdef QP_CONFIG
#include "qp_config.hpp" // external QP configuration
#endif

// no-return function specifier (C++11 Standard)
#define Q_NORETURN  [[ noreturn ]] void


// QActive event queue, os-type, and thread types
#define QACTIVE_EQUEUE_TYPE  QEQueue
#define QACTIVE_OS_OBJ_TYPE  void*
#define QACTIVE_THREAD_TYPE  void*

// QF critical section for Win32, see NOTE1
#define QF_CRIT_STAT
#define QF_CRIT_ENTRY()      QP::QF::enterCriticalSection_()
#define QF_CRIT_EXIT()       QP::QF::leaveCriticalSection_()

// QF_LOG2 not defined -- use the internal LOG2() implementation

namespace QP {
namespace QF {

// internal functions for critical section management
void enterCriticalSection_();
void leaveCriticalSection_();

// set clock tick rate and priority
void setTickRate(uint32_t ticksPerSec, int tickPrio);

// application-level clock tick callback
void onClockTick();

// abstractions for console access...
void consoleSetup();
void consoleCleanup();
int consoleGetKey();
int consoleWaitForKey();

} // namespace QF
} // namespace QP

// special adaptations for QWIN GUI applications
#ifdef QWIN_GUI
    // replace main() with main_gui() as the entry point to a GUI app.
    #define main() main_gui()
    int main_gui(); // prototype of the GUI application entry point
#endif

// include files -------------------------------------------------------------
#include "qequeue.hpp"   // Win32-QV needs the native event-queue
#include "qmpool.hpp"    // Win32-QV needs the native memory-pool
#include "qp.hpp"        // QP platform-independent public interface

//============================================================================
// interface used only inside QF implementation, but not in applications

#ifdef QP_IMPL

    // QF scheduler locking for Win32, see NOTE2
    #define QF_SCHED_STAT_
    #define QF_SCHED_LOCK_(dummy) QF::enterCriticalSection_()
    #define QF_SCHED_UNLOCK_()    QF::leaveCriticalSection_()

    // QF event queue customization for Win32...
    #define QACTIVE_EQUEUE_WAIT_(me_) \
        while ((me_)->m_eQueue.m_frontEvt == nullptr) { \
            QF_CRIT_EXIT(); \
            static_cast<void>(WaitForSingleObject((me_)->m_osObject, INFINITE)); \
            QF_CRIT_ENTRY(); \
        }

    #define QACTIVE_EQUEUE_SIGNAL_(me_) \
        static_cast<void>(SetEvent((me_)->m_osObject))

    // native QF event pool operations
    #define QF_EPOOL_TYPE_  QMPool
    #define QF_EPOOL_INIT_(p_, poolSto_, poolSize_, evtSize_) \
        (p_).init((poolSto_), (poolSize_), (evtSize_))
    #define QF_EPOOL_EVENT_SIZE_(p_)  ((p_).getBlockSize())
    #define QF_EPOOL_GET_(p_, e_, m_, qs_id_) \
        ((e_) = static_cast<QEvt *>((p_).get((m_), (qs_id_))))
    #define QF_EPOOL_PUT_(p_, e_, qs_id_)  ((p_).put((e_), (qs_id_)))

    // Minimum required Windows version is Windows-XP or newer (0x0501)
    #ifdef WINVER
    #undef WINVER
    #endif
    #ifdef _WIN32_WINNT
    #undef _WIN32_WINNT
    #endif

    #define WINVER _WIN32_WINNT_WINXP
    #define _WIN32_WINNT _WIN32_WINNT_WINXP

    #define WIN32_LEAN_AND_MEAN
    #include <windows.h> // Win32 API

#endif // QP_IMPL

//============================================================================
// NOTE1:
// QP, like all real-time frameworks, needs to execute certain sections of
// code exclusively, meaning that only one thread can execute the code at
// the time. Such sections of code are called "critical sections".
//
// This port uses a pair of functions QF::enterCriticalSection_() /
// QF::leaveCriticalSection_() to enter/leave the cirtical section,
// respectively.
//
// These functions are implemented in the qf_port.cpp module, where they
// manipulate the file-scope Win32 critical section object l_win32CritSect
// to protect all critical sections. Using the single critical section
// object for all critical section guarantees that only one thread at a time
// can execute inside a critical section. This prevents race conditions and
// data corruption.
//
// Please note, however, that the Win32 critical section implementation
// behaves differently than interrupt disabling. A common Win32 critical
// section ensures that only one thread at a time can execute a critical
// section, but it does not guarantee that a context switch cannot occur
// within the critical section. In fact, such context switches probably
// will happen, but they should not cause concurrency hazards because the
// critical section eliminates all race conditions.
//
// Unlike simply disabling and enabling interrupts, the critical section
// approach is also subject to priority inversions. Various versions of
// Windows handle priority inversions differently, but it seems that most of
// them recognize priority inversions and dynamically adjust the priorities of
// threads to prevent it. Please refer to the MSN articles for more
// information.
//
// NOTE2:
// Scheduler locking (used inside QActive::publish_()) is implemented in this
// port with the main critical section. This means that event multicasting
// will appear atomic, in the sense that no thread will be able to post
// events during multicasting.
//

#endif // QP_PORT_HPP_

