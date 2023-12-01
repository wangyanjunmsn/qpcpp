//============================================================================
// QP/C++ Real-Time Embedded Framework (RTEF)
//
//                   Q u a n t u m  L e a P s
//                   ------------------------
//                   Modern Embedded Software
//
// Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
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
//! @brief QP/C++ port to uC-OS2 RTOS, generic C++11 compiler

#ifndef QP_PORT_HPP_
#define QP_PORT_HPP_

#include <cstdint>  // Exact-width types. C++11 Standard

#ifdef QP_CONFIG
#include "qp_config.hpp" // external QP configuration
#endif

// no-return function specifier (C++11 Standard)
#define Q_NORETURN  [[ noreturn ]] void

// QActive customization for uC-OS2
#define QACTIVE_EQUEUE_TYPE     OS_EVENT *
#define QACTIVE_THREAD_TYPE     std::uint32_t

// include files -------------------------------------------------------------
#include "ucos_ii.h"     // uC-OS2 API, port and compile-time configuration

#include "qequeue.hpp"   // QP native queue for event deferral
#include "qmpool.hpp"    // QP native QF memory pool
#include "qp.hpp"        // QP platform-independent public interface

// QF interrupt disable/enable for uC-OS2, NOTE1
#if (OS_CRITICAL_METHOD == 1U)
    #define QF_CRIT_STAT
    #define QF_CRIT_ENTRY()  OS_ENTER_CRITICAL()
    #define QF_CRIT_EXIT()   OS_EXIT_CRITICAL()
#elif (OS_CRITICAL_METHOD == 3U)
    #define QF_CRIT_STAT     OS_CPU_SR cpu_sr;
    #define QF_CRIT_ENTRY()  OS_ENTER_CRITICAL()
    #define QF_CRIT_EXIT()   OS_EXIT_CRITICAL()
#else
    #error Unsupported uC-OS2 critical section type
#endif // OS_CRITICAL_METHOD

namespace QP {

enum UCOS2_TaskAttrs {
    TASK_NAME_ATTR
};

} // namespace QP

//============================================================================
// interface used only inside QF implementation, but not in applications
#ifdef QP_IMPL

    // uC-OS2-specific scheduler locking, see NOTE2
    #define QF_SCHED_STAT_
    #define QF_SCHED_LOCK_(dummy) do { \
        if (OSIntNesting == 0U) {      \
            OSSchedLock();             \
        }                              \
    } while (false)

    #define QF_SCHED_UNLOCK_() do { \
        if (OSIntNesting == 0U) {   \
            OSSchedUnlock();        \
        }                           \
    } while (false)

    // native QF event pool customization
    #define QF_EPOOL_TYPE_        QMPool
    #define QF_EPOOL_INIT_(p_, poolSto_, poolSize_, evtSize_) \
        (p_).init((poolSto_), (poolSize_), (evtSize_))
    #define QF_EPOOL_EVENT_SIZE_(p_)  ((p_).getBlockSize())
    #define QF_EPOOL_GET_(p_, e_, m_, qs_id_) \
        ((e_) = static_cast<QEvt *>((p_).get((m_), (qs_id_))))
    #define QF_EPOOL_PUT_(p_, e_, qs_id_) ((p_).put((e_), (qs_id_)))

#endif // ifdef QP_IMPL

//============================================================================
// NOTE1:
// This QP port to uC-OS2 re-uses the exact same critical section mechanism
// as uC-OS2. The goal is to make this port independent on the CPU or the
// toolchain by employing only the official uC-OS2 API. That way, all CPU
// and toolchain dependencies are handled internally by uC-OS2.
//
// NOTE2:
// uC-OS2 provides only global scheduler locking for all thread priorities
// by means of OSSchedLock() and OSSchedUnlock(). Therefore, locking the
// scheduler only up to the specified lock priority is not supported.
//

#endif // QP_PORT_HPP_

