//$file${.::philo.cpp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: dpp-comp.qm
// File:  ${.::philo.cpp}
//
// This code has been generated by QM 5.3.0 <www.state-machine.com/qm>.
// DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This generated code is open source software: you can redistribute it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation.
//
// This code is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// NOTE:
// Alternatively, this generated code may be distributed under the terms
// of Quantum Leaps commercial licenses, which expressly supersede the GNU
// General Public License and are specifically designed for licensees
// interested in retaining the proprietary status of their code.
//
// Contact information:
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//
//$endhead${.::philo.cpp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#include "qpcpp.hpp"
#include "dpp.hpp"
#include "bsp.hpp"

//----------------------------------------------------------------------------
namespace { // unnamed namespace for local definitions with internal linkage

Q_DEFINE_THIS_FILE

// helper function to provide a randomized think time for Philos
static inline QP::QTimeEvtCtr think_time() {
    return static_cast<QP::QTimeEvtCtr>((BSP::random() % BSP::TICKS_PER_SEC)
                                        + (BSP::TICKS_PER_SEC/2U));
}

// helper function to provide a randomized eat time for Philos
static inline QP::QTimeEvtCtr eat_time() {
    return static_cast<QP::QTimeEvtCtr>((BSP::random() % BSP::TICKS_PER_SEC)
                                        + BSP::TICKS_PER_SEC);
}

} // unnamed namespace
//----------------------------------------------------------------------------

//$declare${Comp::Philo} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace APP {

//${Comp::Philo} .............................................................
class Philo : public QP::QHsm {
public:
    static Philo inst[N_PHILO];

private:
    CompTimeEvt m_timeEvt;
    friend class Table;

public:
    Philo();
    std::uint8_t getId() {
        return static_cast<std::uint8_t>(this - &APP::Philo::inst[0]);
    }

protected:
    Q_STATE_DECL(initial);
    Q_STATE_DECL(thinking);
    Q_STATE_DECL(hungry);
    Q_STATE_DECL(eating);
}; // class Philo

} // namespace APP
//$enddecl${Comp::Philo} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// definition of the whole "Comp" package
//$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Check for the minimum required QP version
#if (QP_VERSION < 730U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpcpp version 7.3.0 or higher required
#endif
//$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//$define${Comp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace APP {

//${Comp::CompTimeEvt} .......................................................

//${Comp::CompTimeEvt::CompTimeEvt} ..........................................
CompTimeEvt::CompTimeEvt(
    QP::QActive * act,
    std::uint16_t num,
    enum_t const sig,
    std::uint_fast8_t const tickRate)
 : QTimeEvt(act, sig, tickRate),
   compNum(num)
{}

//${Comp::Philo} .............................................................
Philo Philo::inst[N_PHILO];

//${Comp::Philo::Philo} ......................................................
Philo::Philo()
  : QHsm(&initial),
    m_timeEvt(AO_Table, getId(), TIMEOUT_SIG, 0U)
{}

//${Comp::Philo::SM} .........................................................
Q_STATE_DEF(Philo, initial) {
    //${Comp::Philo::SM::initial}
    static_cast<void>(e); // unused parameter

    #ifdef Q_SPY
    std::uint8_t n = getId();
    QS_OBJ_ARR_DICTIONARY(&Philo::inst[n], n);
    QS_OBJ_ARR_DICTIONARY(&Philo::inst[n].m_timeEvt, n);
    #endif

    static bool registered = false;
    if (!registered) {
        registered = true;
        QS_FUN_DICTIONARY(&Philo::initial);
        QS_FUN_DICTIONARY(&Philo::thinking);
        QS_FUN_DICTIONARY(&Philo::hungry);
        QS_FUN_DICTIONARY(&Philo::eating);
    }
    return tran(&thinking);
}

//${Comp::Philo::SM::thinking} ...............................................
Q_STATE_DEF(Philo, thinking) {
    QP::QState status_;
    switch (e->sig) {
        //${Comp::Philo::SM::thinking}
        case Q_ENTRY_SIG: {
            m_timeEvt.armX(think_time(), 0U);
            status_ = Q_RET_HANDLED;
            break;
        }
        //${Comp::Philo::SM::thinking}
        case Q_EXIT_SIG: {
            (void)m_timeEvt.disarm();
            status_ = Q_RET_HANDLED;
            break;
        }
        //${Comp::Philo::SM::thinking::TIMEOUT}
        case TIMEOUT_SIG: {
            status_ = tran(&hungry);
            break;
        }
        //${Comp::Philo::SM::thinking::TEST}
        case TEST_SIG: {
            status_ = Q_RET_HANDLED;
            break;
        }
        default: {
            status_ = super(&top);
            break;
        }
    }
    return status_;
}

//${Comp::Philo::SM::hungry} .................................................
Q_STATE_DEF(Philo, hungry) {
    QP::QState status_;
    switch (e->sig) {
        //${Comp::Philo::SM::hungry}
        case Q_ENTRY_SIG: {
            #ifdef QEVT_DYN_CTOR
            TableEvt const *pe = Q_NEW(TableEvt, HUNGRY_SIG, getId());
            #else
            TableEvt *pe = Q_NEW(TableEvt, HUNGRY_SIG);
            pe->philoId = getId();
            #endif
            AO_Table->postLIFO(pe);
            status_ = Q_RET_HANDLED;
            break;
        }
        //${Comp::Philo::SM::hungry::EAT}
        case EAT_SIG: {
            status_ = tran(&eating);
            break;
        }
        default: {
            status_ = super(&top);
            break;
        }
    }
    return status_;
}

//${Comp::Philo::SM::eating} .................................................
Q_STATE_DEF(Philo, eating) {
    QP::QState status_;
    switch (e->sig) {
        //${Comp::Philo::SM::eating}
        case Q_ENTRY_SIG: {
            m_timeEvt.armX(eat_time(), 0U);
            status_ = Q_RET_HANDLED;
            break;
        }
        //${Comp::Philo::SM::eating}
        case Q_EXIT_SIG: {
            m_timeEvt.disarm();

            // asynchronously post event to the Container
            #ifdef QEVT_DYN_CTOR
            TableEvt const *pe = Q_NEW(TableEvt, DONE_SIG, getId());
            #else
            TableEvt *pe = Q_NEW(TableEvt, DONE_SIG);
            pe->philoId = getId();
            #endif
            AO_Table->postLIFO(pe);
            status_ = Q_RET_HANDLED;
            break;
        }
        //${Comp::Philo::SM::eating::TIMEOUT}
        case TIMEOUT_SIG: {
            status_ = tran(&thinking);
            break;
        }
        default: {
            status_ = super(&top);
            break;
        }
    }
    return status_;
}

//${Comp::SM_Philo[N_PHILO]} .................................................
QP::QAsm * const SM_Philo[N_PHILO] = { // opaque pointers to Philo instances
    &Philo::inst[0],
    &Philo::inst[1],
    &Philo::inst[2],
    &Philo::inst[3],
    &Philo::inst[4]
};

} // namespace APP
//$enddef${Comp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^