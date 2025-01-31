//$file${qxk::xthread2.cpp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: dpp_mpu.qm
// File:  ${qxk::xthread2.cpp}
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
//$endhead${qxk::xthread2.cpp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#include "qpcpp.hpp"             // QP/C++ real-time embedded framework
#include "dpp.hpp"               // DPP Application interface
#include "bsp.hpp"               // Board Support Package

//----------------------------------------------------------------------------
// unnamed namespace for local definitions with internal linkage
namespace {
Q_DEFINE_THIS_FILE
} // unnamed namespace

//----------------------------------------------------------------------------
//$declare${XThreads::XThread2} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace APP {

//${XThreads::XThread2} ......................................................
class XThread2 : public QP::QXThread {
private:

    // NOTE: data needed by this thread should be members of
    // the thread class. That way they are in the memory region
    // accessible from this thread.
    std::uint8_t m_foo;

public:
    XThread2();

private:
    static void run(QP::QXThread * const thr);
}; // class XThread2

} // namespace APP
//$enddecl${XThreads::XThread2} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Check for the minimum required QP version
#if (QP_VERSION < 730U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpcpp version 7.3.0 or higher required
#endif
//$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//$define${Shared-TH::XThread2_ctor} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace APP {

//${Shared-TH::XThread2_ctor} ................................................
void XThread2_ctor(
    std::uint8_t * const sto,
    std::uint32_t const size,
    void const * const mpu)
{
    Q_REQUIRE(sizeof(XThread2) <= size);

    // run the constructor through placement new()
    auto me = new(sto) XThread2();
    me->setThread(mpu);
}

} // namespace APP
//$enddef${Shared-TH::XThread2_ctor} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//$define${XThreads::XThread2} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace APP {

//${XThreads::XThread2} ......................................................

//${XThreads::XThread2::XThread2} ............................................
XThread2::XThread2()
  : QXThread(&run)
{}

//${XThreads::XThread2::run} .................................................
void XThread2::run(QP::QXThread * const thr) {
    // downcast the generic thr pointer to the specific thread
    //auto me = static_cast<XThread2 *>(thr);

    QS_OBJ_DICTIONARY(TH_XThread2);
    QS_OBJ_DICTIONARY(TH_XThread2->getTimeEvt());
    QS_OBJ_DICTIONARY(&TH_sema);
    QS_OBJ_DICTIONARY(&TH_mutex);

    // initialize the semaphore before using it
    // NOTE: Here the semaphore is initialized in the highest-priority thread
    // that uses it. Alternatively, the semaphore can be initialized
    // before any thread runs.
    TH_sema.init(0U,  // count==0 (signaling semaphore)
                1U); // max_count==1 (binary semaphore)

    // initialize the mutex before using it
    // NOTE: Here the mutex is initialized in the highest-priority thread
    // that uses it. Alternatively, the mutex can be initialized
    // before any thread runs.
    TH_mutex.init(APP::N_PHILO + 6U); // priority-ceiling mutex
    //l_mutex.init(0U); // alternatively: priority-ceiling NOT used

    for (;;) {
        // wait on a semaphore (BLOCK indefinitely)
        TH_sema.wait();

        TH_mutex.lock(QP::QXTHREAD_NO_TIMEOUT); // lock the mutex
        QP::QXThread::delay(5U);  // wait more (BLOCK)
        TH_mutex.unlock();
    }
}

} // namespace APP
//$enddef${XThreads::XThread2} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
