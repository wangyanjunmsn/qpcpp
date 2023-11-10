//============================================================================
// Product: DPP example, NUCLEO-H743ZI board, embOS RTOS kernel
// Last updated for version 7.3.1
// Last updated on  2023-11-15
//
//                   Q u a n t u m  L e a P s
//                   ------------------------
//                   Modern Embedded Software
//
// Copyright (C) 2005 Quantum Leaps, LLC. <state-machine.com>
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
#include "qpcpp.hpp"             // QP/C++ real-time embedded framework
#include "dpp.hpp"               // DPP Application interface
#include "bsp.hpp"               // Board Support Package

// STM32CubeH7 include files
//#include "stm32h7xx_it.h"

#include "stm32h7xx_hal.h"
#include "stm32h7xx_nucleo_144.h"
// add other drivers if necessary...

//============================================================================
namespace { // unnamed namespace for local stuff with internal linkage

Q_DEFINE_THIS_FILE

static OS_TICK_HOOK l_tick_hook;
static std::uint32_t l_rndSeed;

#ifdef Q_SPY

    // QS source IDs
    static QP::QSpyId const l_embos_ticker = { 0U };

    static UART_HandleTypeDef l_uartHandle;
    QP::QSTimeCtr QS_tickTime_;
    QP::QSTimeCtr QS_tickPeriod_;

    enum AppRecords { // application-specific trace records
        PHILO_STAT = QP::QS_USER,
        PAUSED_STAT,
    };

#endif

} // unnamed namespace

//============================================================================
// Error handler
extern "C" {

Q_NORETURN Q_onError(char const * const module, int const id) {
    // NOTE: this implementation of the error handler is intended only
    // for debugging and MUST be changed for deployment of the application
    // (assuming that you ship your production code with assertions enabled).
    Q_UNUSED_PAR(module);
    Q_UNUSED_PAR(id);
    QS_ASSERTION(module, id, 10000U); // report assertion to QS

#ifndef NDEBUG
    // light up LED
    BSP_LED_On(LED1);
    // for debugging, hang on in an endless loop...
    for (;;) {
    }
#else
    NVIC_SystemReset();
    for (;;) { // explicitly "no-return"
    }
#endif
}
//............................................................................
void assert_failed(char const * const module, int const id); // prototype
void assert_failed(char const * const module, int const id) {
    Q_onError(module, id);
}

// ISRs used in the application ==============================================
//............................................................................
#ifdef Q_SPY

// ISR for receiving bytes from the QSPY Back-End
// NOTE: This ISR is "kernel-unaware" meaning that it does not interact with
// the FreeRTOS or QP and is not disabled. Such ISRs don't need to call
// portEND_SWITCHING_ISR(() at the end, but they also cannot call any
// embOS or QP APIs.
void USART3_IRQHandler(void); // prototype
void USART3_IRQHandler(void) {
    // is RX register NOT empty?
    if ((l_uartHandle.Instance->ISR & USART_ISR_RXNE_RXFNE) != 0U) {
        std::uint32_t b = l_uartHandle.Instance->RDR;
        QP::QS::rxPut(b);
        l_uartHandle.Instance->ISR &= ~USART_ISR_RXNE_RXFNE; // clear int.
    }
}
#endif // Q_SPY

// Application hooks used in this project ====================================

// embOS application hooks =================================================
static void tick_handler(void) { // signature of embOS tick hook routine

    // process time events at rate 0
    QP::QTimeEvt::TICK_X(0U, &l_embos_ticker);

    // Perform the debouncing of buttons. The algorithm for debouncing
    // adapted from the book "Embedded Systems Dictionary" by Jack Ganssle
    // and Michael Barr, page 71.
    static struct {
        std::uint32_t depressed;
        std::uint32_t previous;
    } buttons = { 0U, 0U };

    std::uint32_t current = BSP_PB_GetState(BUTTON_KEY); // read the Key button
    std::uint32_t tmp = buttons.depressed; // save debounced depressed buttons
    buttons.depressed |= (buttons.previous & current); // set depressed
    buttons.depressed &= (buttons.previous | current); // clear released
    buttons.previous   = current; // update the history
    tmp ^= buttons.depressed;     // changed debounced depressed
    current = buttons.depressed;

    if (tmp != 0U) {  // debounced Key button state changed?
        if (current != 0U) { // is PB0 depressed?
            static QP::QEvt const pauseEvt(APP::PAUSE_SIG);
            QP::QActive::PUBLISH(&pauseEvt, &l_embos_ticker);
        }
        else { // the button is released
            static QP::QEvt const serveEvt(APP::SERVE_SIG);
            QP::QActive::PUBLISH(&serveEvt, &l_embos_ticker);
        }
    }

#ifdef Q_SPY
    tmp = SysTick->CTRL; // clear SysTick_CTRL_COUNTFLAG
    QS_tickTime_ += QS_tickPeriod_; // account for the clock rollover
#endif
}

//............................................................................
// OS_Idle() function overridden from RTOSInit_STM32F4x_CMSIS.c
//
// Function description
//   This is basically the "core" of the embOS idle loop.
//   This core loop can be changed, but:
//   The idle loop does not have a stack of its own, therefore no
//   functionality should be implemented that relies on the stack
//   to be preserved. However, a simple program loop can be programmed
//   (like toggling an output or incrementing a counter)
//
void OS_Idle(void) {
    while (1) {

        // toggle the User LED on and then off, see NOTE2
        QF_INT_DISABLE();
        BSP_LED_On(LED3);
        QF_INT_ENABLE();

#ifdef Q_SPY
        QF_INT_DISABLE();
        QP::QS::rxParse();  // parse all the received bytes
        QF_INT_ENABLE();

        if ((l_uartHandle.Instance->ISR & UART_FLAG_TXE) != 0U) { // TXE empty?
            QF_INT_DISABLE();
            uint16_t b = QP::QS::getByte();
            QF_INT_ENABLE();

            if (b != QS_EOD) { // not End-Of-Data?
                l_uartHandle.Instance->TDR = b; // put into TDR
            }
        }
#elif defined NDEBUG
        // Put the CPU and peripherals to the low-power mode.
        // you might need to customize the clock management for your application,
        // see the datasheet for your particular Cortex-M MCU.
        //
        // !!!CAUTION!!!
        // The WFI instruction stops the CPU clock, which unfortunately disables
        // the JTAG port, so the ST-Link debugger can no longer connect to the
        // board. For that reason, the call to __WFI() has to be used with CAUTION.
        //
        // NOTE: If you find your board "frozen" like this, strap BOOT0 to VDD and
        // reset the board, then connect with ST-Link Utilities and erase the part.
        // The trick with BOOT(0) is it gets the part to run the System Loader
        // instead of your broken code. When done disconnect BOOT0, and start over.
        //
        //__WFI(); // Wait-For-Interrupt
#endif
    }
}

} // extern "C"

// BSP functions =============================================================
namespace BSP {

void init() {
    // Configure the MPU to prevent NULL-pointer dereferencing ...
    MPU->RBAR = 0x0U                          // base address (NULL)
                | MPU_RBAR_VALID_Msk          // valid region
                | (MPU_RBAR_REGION_Msk & 7U); // region #7
    MPU->RASR = (7U << MPU_RASR_SIZE_Pos)     // 2^(7+1) region
                | (0x0U << MPU_RASR_AP_Pos)   // no-access region
                | MPU_RASR_ENABLE_Msk;        // region enable
    MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk       // enable background region
                | MPU_CTRL_ENABLE_Msk;        // enable the MPU
    __ISB();
    __DSB();

    // enable the MemManage_Handler for MPU exception
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;

    // NOTE: SystemInit() has been already called from the startup code
    // but SystemCoreClock needs to be updated
    SystemCoreClockUpdate();

    // NOTE: VFP (hardware Floating Point) unit is configured by embOS

    // enable clock for to the peripherals used by this application...
    SCB_EnableICache(); // enable I-Cache
    SCB_EnableDCache(); // enable D-Cache

    // configure Flash prefetch and Instr. cache through ART accelerator
#if (ART_ACCLERATOR_ENABLE != 0)
    __HAL_FLASH_ART_ENABLE();
#endif // ART_ACCLERATOR_ENABLE

    // Configure the LEDs
    BSP_LED_Init(LED1);
    BSP_LED_Init(LED2);
    BSP_LED_Init(LED3);

    // configure the User Button in GPIO Mode
    BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_GPIO);

    BSP::randomSeed(1234U);

    // initialize the QS software tracing...
    if (!QS_INIT(nullptr)) {
        Q_ERROR();
    }

    // dictionaries...
    QS_OBJ_DICTIONARY(&l_embos_ticker);
    QS_USR_DICTIONARY(PHILO_STAT);
    QS_USR_DICTIONARY(PAUSED_STAT);

    QS_ONLY(APP::produce_sig_dict());

    // setup the QS filters...
    QS_GLB_FILTER(QP::QS_ALL_RECORDS); // all records
    QS_GLB_FILTER(-QP::QS_QF_TICK);    // exclude the clock tick
}
//............................................................................
void start() {
    // initialize event pools
    static QF_MPOOL_EL(APP::TableEvt) smlPoolSto[2*APP::N_PHILO];
    QP::QF::poolInit(smlPoolSto, sizeof(smlPoolSto), sizeof(smlPoolSto[0]));

    // initialize publish-subscribe
    static QP::QSubscrList subscrSto[APP::MAX_PUB_SIG];
    QP::QActive::psInit(subscrSto, Q_DIM(subscrSto));

    // start AOs/threads...
    static QP::QEvt const *philoQueueSto[APP::N_PHILO][APP::N_PHILO];
    static OS_STACKPTR int philoStack[APP::N_PHILO][128];
    for (std::uint8_t n = 0U; n < APP::N_PHILO; ++n) {
        APP::AO_Philo[n]->start(
            Q_PRIO(n + 3U, 3U),      // QP-prio., embOS prio.
            philoQueueSto[n],        // event queue storage
            Q_DIM(philoQueueSto[n]), // queue length [events]
            philoStack[n],           // stack storage
            sizeof(philoStack[n]));  // stack size [bytes]
    }

    static QP::QEvt const *tableQueueSto[APP::N_PHILO];
    static OS_STACKPTR int tableStack[128];
    APP::AO_Table->start(
        Q_PRIO(APP::N_PHILO + 7U, 7U),// QP-prio., embOS prio.
        tableQueueSto,           // event queue storage
        Q_DIM(tableQueueSto),    // queue length [events]
        tableStack,              // stack storage
        sizeof(tableStack));     // stack size [bytes]
}
//............................................................................
void displayPhilStat(std::uint8_t n, char const *stat) {
    Q_UNUSED_PAR(n);

    if (stat[0] == 'e') {
        BSP_LED_On(LED1);
    }
    else {
        BSP_LED_Off(LED1);
    }

    // app-specific trace record...
    QS_BEGIN_ID(PHILO_STAT, APP::AO_Table->getPrio())
        QS_U8(1, n);  // Philosopher number
        QS_STR(stat); // Philosopher status
    QS_END()
}
//............................................................................
void displayPaused(std::uint8_t const paused) {
    if (paused != 0U) {
        BSP_LED_On(LED2);
    }
    else {
        BSP_LED_Off(LED2);
    }

    // application-specific trace record
    QS_BEGIN_ID(PAUSED_STAT, APP::AO_Table->getPrio())
        QS_U8(1, paused);  // Paused status
    QS_END()
}
//............................................................................
void randomSeed(std::uint32_t const seed) {
    l_rndSeed = seed;
}
//............................................................................
std::uint32_t random() { // a very cheap pseudo-random-number generator
    // Some floating point code is to exercise the VFP...
    double volatile x = 3.1415926;
    x = x + 2.7182818;

    OS_TASK_EnterRegion(); // lock embOS scheduler
    // "Super-Duper" Linear Congruential Generator (LCG)
    // LCG(2^32, 3*7*11*13*23, 0, seed)
    std::uint32_t rnd = l_rndSeed * (3U*7U*11U*13U*23U);
    l_rndSeed = rnd; // set for the next time
    OS_TASK_LeaveRegion(); // unlock embOS scheduler

    return (rnd >> 8);
}
//............................................................................
void ledOn() {
    BSP_LED_On(LED3);
}
//............................................................................
void ledOff() {
    BSP_LED_Off(LED3);
}
//............................................................................
void terminate(int16_t result) {
    Q_UNUSED_PAR(result);
}

} // namespace BSP

//============================================================================

namespace QP {

// QF callbacks --------------------------------------------------------------

void QF::onStartup() {
    OS_TICK_AddHook(&l_tick_hook, &tick_handler);

#ifdef Q_SPY
    NVIC_SetPriority(USART3_IRQn,  0U); // kernel unaware interrupt
    NVIC_EnableIRQ(USART3_IRQn); // UART interrupt used for QS-RX
#endif
}
//............................................................................
void QF::onCleanup() {
}

// QS callbacks --------------------------------------------------------------
#ifdef Q_SPY
namespace QS {

//............................................................................
bool onStartup(void const *arg) {
    Q_UNUSED_PAR(arg);

    static std::uint8_t qsTxBuf[2*1024]; // buffer for QS-TX channel
    initBuf(qsTxBuf, sizeof(qsTxBuf));

    static std::uint8_t qsRxBuf[100];    // buffer for QS-RX channel
    rxInitBuf(qsRxBuf, sizeof(qsRxBuf));

    l_uartHandle.Instance        = USART3;
    l_uartHandle.Init.BaudRate   = 115200;
    l_uartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    l_uartHandle.Init.StopBits   = UART_STOPBITS_1;
    l_uartHandle.Init.Parity     = UART_PARITY_NONE;
    l_uartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    l_uartHandle.Init.Mode       = UART_MODE_TX_RX;
    l_uartHandle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&l_uartHandle) != HAL_OK) {
        return 0U; // return failure
    }

    // Set UART to receive 1 byte at a time via interrupt
    HAL_UART_Receive_IT(&l_uartHandle, (uint8_t *)qsRxBuf, 1);
    // NOTE: wait till QF::onStartup() to enable UART interrupt in NVIC

    QS_tickPeriod_ = SystemCoreClock / BSP::TICKS_PER_SEC;
    QS_tickTime_ = QS_tickPeriod_; // to start the timestamp at zero

    return true; // return success
}
//............................................................................
void onCleanup() {
}
//............................................................................
QSTimeCtr onGetTime() { // NOTE: invoked with interrupts DISABLED
    if ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0) { // not set?
        return QS_tickTime_ - (QSTimeCtr)SysTick->VAL;
    }
    else { // the rollover occured, but the SysTick_ISR did not run yet
        return QS_tickTime_ + QS_tickPeriod_ - (QSTimeCtr)SysTick->VAL;
    }
}
//............................................................................
void onFlush() {
    for (;;) {
        QF_INT_DISABLE();
        std::uint16_t b = getByte();
        QF_INT_ENABLE();

        if (b != QS_EOD) { // NOT end-of-data
            // busy-wait as long as TX FIFO has data to transmit
            while ((l_uartHandle.Instance->ISR & UART_FLAG_TXE) == 0U) {
                QF_INT_ENABLE();
                QF_CRIT_EXIT_NOP();

                QF_INT_DISABLE();
            }
            // place the byte in the UART TDR register
            l_uartHandle.Instance->TDR = b;
            QF_INT_ENABLE();
        }
        else {
            QF_INT_ENABLE();
            break; // break out of the loop
        }
    }
}
//............................................................................
//! callback function to reset the target (to be implemented in the BSP)
void onReset() {
    NVIC_SystemReset();
}
//............................................................................
// callback function to execute a user command
void onCommand(std::uint8_t cmdId, std::uint32_t param1,
               std::uint32_t param2, std::uint32_t param3)
{
    Q_UNUSED_PAR(cmdId);
    Q_UNUSED_PAR(param1);
    Q_UNUSED_PAR(param2);
    Q_UNUSED_PAR(param3);
}

} // namespace QS
#endif // Q_SPY
//----------------------------------------------------------------------------

} // namespace QP

//============================================================================
// NOTE1:
// The User LED is used to visualize the idle loop activity. The brightness
// of the LED is proportional to the frequency of invcations of the idle loop.
// Please note that the LED is toggled with interrupts locked, so no interrupt
// execution time contributes to the brightness of the User LED.

