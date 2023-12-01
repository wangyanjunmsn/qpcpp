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
// <www.state-machine.com>
// <info@state-machine.com>
//============================================================================
//! @date Last updated on: 2023-09-07
//! @version Last updated for: @ref qpcpp_7_3_0
//!
//! @file
//! @brief QP/C++ to MSP40, QUTEST unit test harness, generic C++11 compiler

#ifndef QP_PORT_HPP_
#define QP_PORT_HPP_

#include <cstdint>  // Exact-width types. C++11 Standard

#include "qp_config.hpp" // external QP configuration required in this port

// no-return function specifier (C++11 Standard)
#define Q_NORETURN  [[ noreturn ]] void

// QF configuration for QK -- data members of the QActive class...

// QActive event queue type
#define QACTIVE_EQUEUE_TYPE  QEQueue
// QACTIVE_OS_OBJ_TYPE not used in this port
// QACTIVE_THREAD_TYPE not used in this port

// QF interrupt disable/enable
#define QF_INT_DISABLE()     (++QP::QF::intLock_)
#define QF_INT_ENABLE()      (--QP::QF::intLock_)

// QF critical section
#define QF_CRIT_STAT
#define QF_CRIT_ENTRY()      QF_INT_DISABLE()
#define QF_CRIT_EXIT()       QF_INT_ENABLE()

// QF_LOG2 not defined -- use the internal LOG2() implementation

// include files -------------------------------------------------------------
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
    #include <intrinsics.h>
#elif defined(__GNUC__)
    #include <msp430.h>
    #include "in430.h"
#endif

#include "qequeue.hpp"   // QUTest port uses QEQueue event-queue
#include "qmpool.hpp"    // QUTest port uses QMPool memory-pool
#include "qp.hpp"        // QP platform-independent public interface

//============================================================================
// interface used only inside QF implementation, but not in applications
#ifdef QP_IMPL

    // QUTest scheduler locking (not used)
    #define QF_SCHED_STAT_
    #define QF_SCHED_LOCK_(dummy) ((void)0)
    #define QF_SCHED_UNLOCK_()    ((void)0)

    // native event queue operations
    #define QACTIVE_EQUEUE_WAIT_(me_) \
        Q_ASSERT_INCRIT(110, (me_)->m_eQueue.m_frontEvt != nullptr)
#ifndef Q_UNSAFE
    #define QACTIVE_EQUEUE_SIGNAL_(me_) \
        QF::readySet_.insert(static_cast<std::uint_fast8_t>((me_)->m_prio));
        QF::readySet_.update_(&QF::readySet_dis)
#else
    #define QACTIVE_EQUEUE_SIGNAL_(me_) \
        QF::readySet_.insert(static_cast<std::uint_fast8_t>((me_)->m_prio))
#endif

    // native QF event pool operations
    #define QF_EPOOL_TYPE_  QMPool
    #define QF_EPOOL_INIT_(p_, poolSto_, poolSize_, evtSize_) \
        (p_).init((poolSto_), (poolSize_), (evtSize_))
    #define QF_EPOOL_EVENT_SIZE_(p_)  ((p_).getBlockSize())
    #define QF_EPOOL_GET_(p_, e_, m_, qs_id_) \
        ((e_) = static_cast<QEvt *>((p_).get((m_), (qs_id_))))
    #define QF_EPOOL_PUT_(p_, e_, qs_id_) ((p_).put((e_), (qs_id_)))

#endif // QP_IMPL

#endif // QP_PORT_HPP_

