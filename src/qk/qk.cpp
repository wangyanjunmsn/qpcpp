//$file${src::qk::qk.cpp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: qpcpp.qm
// File:  ${src::qk::qk.cpp}
//
// This code has been generated by QM 5.3.0 <www.state-machine.com/qm>.
// DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
//
// This code is covered by the following QP license:
// License #    : LicenseRef-QL-dual
// Issued to    : Any user of the QP/C++ real-time embedded framework
// Framework(s) : qpcpp
// Support ends : 2024-12-31
// License scope:
//
// Copyright (C) 2005 Quantum Leaps, LLC <state-machine.com>.
//
//                    Q u a n t u m  L e a P s
//                    ------------------------
//                    Modern Embedded Software
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
//
//$endhead${src::qk::qk.cpp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define QP_IMPL             // this is QP implementation
#include "qp_port.hpp"      // QP port
#include "qp_pkg.hpp"       // QP package-scope interface
#include "qsafe.h"          // QP Functional Safety (FuSa) Subsystem
#ifdef Q_SPY                // QS software tracing enabled?
    #include "qs_port.hpp"  // QS port
    #include "qs_pkg.hpp"   // QS facilities for pre-defined trace records
#else
    #include "qs_dummy.hpp" // disable the QS software tracing
#endif // Q_SPY

// protection against including this source file in a wrong project
#ifndef QK_HPP_
    #error "Source file included in a project NOT based on the QK kernel"
#endif // QK_HPP_

// unnamed namespace for local definitions with internal linkage
namespace {
Q_DEFINE_THIS_MODULE("qk")
} // unnamed namespace

//$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Check for the minimum required QP version
#if (QP_VERSION < 730U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpcpp version 7.3.0 or higher required
#endif
//$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//$define${QK::QK-base} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {
namespace QK {

//${QK::QK-base::schedLock} ..................................................
QSchedStatus schedLock(std::uint_fast8_t const ceiling) noexcept {
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    Q_REQUIRE_INCRIT(100, !QK_ISR_CONTEXT_());

    // first store the previous lock prio
    QSchedStatus stat;
    if (ceiling > QK_priv_.lockCeil) { // raising the lock ceiling?
        QS_BEGIN_PRE_(QS_SCHED_LOCK, 0U)
            QS_TIME_PRE_();   // timestamp
            // the previous lock ceiling & new lock ceiling
            QS_2U8_PRE_(static_cast<std::uint8_t>(QK_priv_.lockCeil),
                        static_cast<std::uint8_t>(ceiling));
        QS_END_PRE_()

        // previous status of the lock
        stat = static_cast<QSchedStatus>(QK_priv_.lockHolder);
        stat = stat | (static_cast<QSchedStatus>(QK_priv_.lockCeil) << 8U);

        // new status of the lock
        QK_priv_.lockHolder = QK_priv_.actPrio;
        QK_priv_.lockCeil   = ceiling;
    }
    else {
        stat = 0xFFU; // scheduler not locked
    }

    QF_MEM_APP();
    QF_CRIT_EXIT();

    return stat; // return the status to be saved in a stack variable
}

//${QK::QK-base::schedUnlock} ................................................
void schedUnlock(QSchedStatus const stat) noexcept {
    // has the scheduler been actually locked by the last QK_schedLock()?
    if (stat != 0xFFU) {
        QF_CRIT_STAT
        QF_CRIT_ENTRY();
        QF_MEM_SYS();

        std::uint_fast8_t const lockCeil = QK_priv_.lockCeil;
        std::uint_fast8_t const prevCeil = (stat >> 8U);
        Q_REQUIRE_INCRIT(200, (!QK_ISR_CONTEXT_())
                          && (lockCeil > prevCeil));

        QS_BEGIN_PRE_(QS_SCHED_UNLOCK, 0U)
            QS_TIME_PRE_(); // timestamp
            // current lock ceiling (old), previous lock ceiling (new)
            QS_2U8_PRE_(static_cast<std::uint8_t>(lockCeil),
                        static_cast<std::uint8_t>(prevCeil));
        QS_END_PRE_()

        // restore the previous lock ceiling and lock holder
        QK_priv_.lockCeil   = prevCeil;
        QK_priv_.lockHolder = (stat & 0xFFU);

        // find if any AOs should be run after unlocking the scheduler
        if (QK_sched_() != 0U) { // preemption needed?
            QK_activate_(); // activate any unlocked AOs
        }

        QF_MEM_APP();
        QF_CRIT_EXIT();
    }
}

} // namespace QK
} // namespace QP
//$enddef${QK::QK-base} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

extern "C" {
//$define${QK-extern-C} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

//${QK-extern-C::QK_priv_} ...................................................
QK_Attr QK_priv_;

//${QK-extern-C::QK_sched_} ..................................................
std::uint_fast8_t QK_sched_() noexcept {
    Q_REQUIRE_INCRIT(400, QK_priv_.readySet.verify_(
                                   &QK_priv_.readySet_dis));

    std::uint_fast8_t p;
    if (QK_priv_.readySet.isEmpty()) {
        p = 0U; // no activation needed
    }
    else {
        // find the highest-prio AO with non-empty event queue
        p = QK_priv_.readySet.findMax();

        // is the AO's prio. below the active preemption-threshold?
        if (p <= QK_priv_.actThre) {
            p = 0U; // no activation needed
        }
        // is the AO's prio. below the lock-ceiling?
        else if (p <= QK_priv_.lockCeil) {
            p = 0U; // no activation needed
        }
        else {
            QK_priv_.nextPrio = p;
        }
    }

    return p;
}

//${QK-extern-C::QK_activate_} ...............................................
void QK_activate_() noexcept {
    std::uint_fast8_t const prio_in = QK_priv_.actPrio; // save initial prio.
    std::uint_fast8_t p = QK_priv_.nextPrio; // next prio to run
    QK_priv_.nextPrio = 0U; // clear for the next time

    Q_REQUIRE_INCRIT(500, (prio_in <= QF_MAX_ACTIVE)
                      && (0U < p) && (p <= QF_MAX_ACTIVE));

    #if (defined QF_ON_CONTEXT_SW) || (defined Q_SPY)
    std::uint_fast8_t pprev = prio_in;
    #endif // QF_ON_CONTEXT_SW || Q_SPY

    // loop until no more ready-to-run AOs of higher prio than the initial
    QP::QActive *a;
    do  {
        a = QP::QActive::registry_[p]; // obtain the pointer to the AO
        Q_ASSERT_INCRIT(505, a != nullptr); // the AO must be registered

        // set new active prio. and preemption-ceiling
        QK_priv_.actPrio = p;
        QK_priv_.actThre = a->getPThre();

    #if (defined QF_ON_CONTEXT_SW) || (defined Q_SPY)
        if (p != pprev) { // changing threads?

            QS_BEGIN_PRE_(QP::QS_SCHED_NEXT, p)
                QS_TIME_PRE_();     // timestamp
                QS_2U8_PRE_(p,      // prio. of the scheduled AO
                            pprev); // previous prio.
            QS_END_PRE_()

    #ifdef QF_ON_CONTEXT_SW
            QF_onContextSw(((pprev != 0U)
                            ? QP::QActive::registry_[pprev]
                            : nullptr), a);
    #endif // QF_ON_CONTEXT_SW

            pprev = p; // update previous prio.
        }
    #endif // QF_ON_CONTEXT_SW || Q_SPY

        QF_INT_ENABLE(); // unconditionally enable interrupts

        QP::QEvt const * const e = a->get_();
        // NOTE QActive_get_() performs QF_MEM_APP() before return

        // dispatch event (virtual call)
        a->dispatch(e, a->getPrio());
    #if (QF_MAX_EPOOL > 0U)
        QP::QF::gc(e);
    #endif

        // determine the next highest-prio. AO ready to run...
        QF_INT_DISABLE(); // unconditionally disable interrupts
        QF_MEM_SYS();

        // internal integrity check (duplicate inverse storage)
        Q_ASSERT_INCRIT(502, QK_priv_.readySet.verify_(
                                      &QK_priv_.readySet_dis));

        if (a->getEQueue().isEmpty()) { // empty queue?
            QK_priv_.readySet.remove(p);
    #ifndef Q_UNSAFE
            QK_priv_.readySet.update_(&QK_priv_.readySet_dis);
    #endif
        }

        if (QK_priv_.readySet.isEmpty()) {
            p = 0U; // no activation needed
        }
        else {
            // find new highest-prio AO ready to run...
            p = static_cast<std::uint8_t>(QK_priv_.readySet.findMax());

            // is the new prio. below the initial preemption-threshold?
            if (p <= QP::QActive::registry_[prio_in]->getPThre()) {
                p = 0U; // no activation needed
            }
            // is the AO's prio. below the lock preemption-threshold?
            else if (p <= QK_priv_.lockCeil) {
                p = 0U; // no activation needed
            }
            else {
                Q_ASSERT_INCRIT(510, p <= QF_MAX_ACTIVE);
            }
        }
    } while (p != 0U);

    // restore the active prio. and preemption-threshold
    QK_priv_.actPrio = prio_in;
    QK_priv_.actThre = QP::QActive::registry_[prio_in]->getPThre();

    #if (defined QF_ON_CONTEXT_SW) || (defined Q_SPY)
    if (prio_in != 0U) { // resuming an active object?
        a = QP::QActive::registry_[prio_in]; // pointer to preempted AO

        QS_BEGIN_PRE_(QP::QS_SCHED_NEXT, prio_in)
            QS_TIME_PRE_();     // timestamp
            // prio. of the resumed AO, previous prio.
            QS_2U8_PRE_(prio_in, pprev);
        QS_END_PRE_()
    }
    else {  // resuming prio.==0 --> idle
        a = nullptr; // QK idle loop

        QS_BEGIN_PRE_(QP::QS_SCHED_IDLE, pprev)
            QS_TIME_PRE_();     // timestamp
            QS_U8_PRE_(pprev);  // previous prio.
        QS_END_PRE_()
    }

    #ifdef QF_ON_CONTEXT_SW
    QF_onContextSw(QP::QActive::registry_[pprev], a);
    #endif // QF_ON_CONTEXT_SW

    #endif // QF_ON_CONTEXT_SW || Q_SPY
}
//$enddef${QK-extern-C} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
} // extern "C"

//$define${QK::QF-cust} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {
namespace QF {

//${QK::QF-cust::init} .......................................................
void init() {
    bzero_(&QF::priv_,                 sizeof(QF::priv_));
    bzero_(&QK_priv_,                  sizeof(QK_priv_));
    bzero_(&QActive::registry_[0],     sizeof(QActive::registry_));

    #ifndef Q_UNSAFE
    QK_priv_.readySet.update_(&QK_priv_.readySet_dis);
    #endif

    // setup the QK scheduler as initially locked and not running
    QK_priv_.lockCeil = (QF_MAX_ACTIVE + 1U); // scheduler locked

    // storage capable for holding a blank QActive object (const in ROM)
    static void* const
        idle_ao[((sizeof(QActive) + sizeof(void*)) - 1U) / sizeof(void*)]
            = { nullptr };

    // register the blank QActive object as the idle-AO (cast 'const' away)
    QActive::registry_[0] = QF_CONST_CAST_(QActive*,
        reinterpret_cast<QActive const*>(idle_ao));

    #ifdef QK_INIT
    QK_INIT(); // port-specific initialization of the QK kernel
    #endif
}

//${QK::QF-cust::stop} .......................................................
void stop() {
    onCleanup();  // cleanup callback
    // nothing else to do for the QK preemptive kernel
}

//${QK::QF-cust::run} ........................................................
int_t run() {
    #ifdef Q_SPY
    // produce the QS_QF_RUN trace record
    QF_INT_DISABLE();
    QF_MEM_SYS();
    QS::beginRec_(QS_REC_NUM_(QS_QF_RUN));
    QS::endRec_();
    QF_MEM_APP();
    QF_INT_ENABLE();
    #endif // Q_SPY

    onStartup(); // application-specific startup callback

    QF_INT_DISABLE();
    QF_MEM_SYS();

    QK_priv_.lockCeil = 0U; // unlock the QK scheduler

    // activate AOs to process events posted so far
    if (QK_sched_() != 0U) {
        QK_activate_();
    }

    #ifdef QK_START
    QK_START(); // port-specific startup of the QK kernel
    #endif

    QF_MEM_APP();
    QF_INT_ENABLE();

    for (;;) { // QK idle loop...
        QK::onIdle(); // application-specific QK on-idle callback
    }

    #ifdef __GNUC__  // GNU compiler?
    return 0;
    #endif
}

} // namespace QF
} // namespace QP
//$enddef${QK::QF-cust} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//$define${QK::QActive} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {

//${QK::QActive} .............................................................

//${QK::QActive::start} ......................................................
void QActive::start(
    QPrioSpec const prioSpec,
    QEvt const * * const qSto,
    std::uint_fast16_t const qLen,
    void * const stkSto,
    std::uint_fast16_t const stkSize,
    void const * const par)
{
    Q_UNUSED_PAR(stkSto);  // not needed in QK
    Q_UNUSED_PAR(stkSize); // not needed in QK

    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    Q_REQUIRE_INCRIT(300, (!QK_ISR_CONTEXT_())
                      && (stkSto == nullptr));
    QF_MEM_APP();
    QF_CRIT_EXIT();

    m_prio  = static_cast<std::uint8_t>(prioSpec & 0xFFU); //  QF-prio.
    m_pthre = static_cast<std::uint8_t>(prioSpec >> 8U); // preemption-thre.
    register_(); // make QF aware of this AO

    m_eQueue.init(qSto, qLen); // initialize the built-in queue

    this->init(par, m_prio); // top-most initial tran. (virtual call)
    QS_FLUSH(); // flush the trace buffer to the host

    // See if this AO needs to be scheduled in case QK is already running
    QF_CRIT_ENTRY();
    QF_MEM_SYS();
    if (QK_sched_() != 0U) { // synchronous preemption needed?
        QK_activate_(); // synchronously activate AOs
    }
    QF_MEM_APP();
    QF_CRIT_EXIT();
}

} // namespace QP
//$enddef${QK::QActive} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
