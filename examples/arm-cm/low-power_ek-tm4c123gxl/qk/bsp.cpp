//============================================================================
// Product: "Low-Power" example, preemptive QK kernel
// Last Updated for Version: 7.3.0
// Date of the Last Update:  2023-09-01
//
//                    Q u a n t u m  L e a P s
//                    ------------------------
//                    Modern Embedded Software
//
// Copyright (C) 2005-2019 Quantum Leaps, LLC. All rights reserved.
//
// This program is open source software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Alternatively, this program may be distributed and modified under the
// terms of Quantum Leaps commercial licenses, which expressly supersede
// the GNU General Public License and are specifically designed for
// licensees interested in retaining the proprietary status of their code.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <www.gnu.org/licenses/>.
//
// Contact information:
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//============================================================================
#include "qpcpp.hpp"
#include "low_power.hpp"
#include "bsp.hpp"

#include "TM4C123GH6PM.h"  // the device specific header (TI)
#include "rom.h"           // the built-in ROM functions (TI)
#include "sysctl.h"        // system control driver (TI)
#include "gpio.h"          // GPIO driver (TI)
// add other drivers if necessary...

//Q_DEFINE_THIS_FILE

#ifdef Q_SPY
    #error The low-power example does not provide Spy build configuration
#endif

// ISRs defined in this BSP --------------------------------------------------
void SysTick_Handler(void);
void GPIOPortA_IRQHandler(void);

// Local-scope objects and local definitions ---------------------------------

// bitmask of active sub-systems needed for low-power operation.
// NOTE: shared between the idle thread application, see NOTE1
static uint32_t volatile l_activeSet;

// bits in the l_activeSet bitmask
enum {
    SYSTICK_ACTIVE,
    TIMER0_ACTIVE,
    // ...
};

#define LED_RED     (1U << 1)
#define LED_GREEN   (1U << 3)
#define LED_BLUE    (1U << 2)
#define BTN_SW1     (1U << 4)

#define XTAL_HZ     16000000U

// ISRs used in this project =================================================
void SysTick_Handler(void) {
    QK_ISR_ENTRY();   // inform QK about entering an ISR
    QP::QTimeEvt::TICK_X(0U, nullptr); // process time events for rate 0
    QK_ISR_EXIT();  // inform QK about exiting an ISR
}
//............................................................................
void Timer0A_IRQHandler(void) {
    QK_ISR_ENTRY();   // inform QK about entering an ISR
    TIMER0->ICR |= (1U << 0); // clear the Timer0 interrupt source
    QP::QTimeEvt::TICK_X(1U, nullptr); // process time events for rate 1
    QK_ISR_EXIT();  // inform QK about exiting an ISR
}
//............................................................................
void GPIOPortF_IRQHandler(void) {
    QK_ISR_ENTRY();  // inform QK about entering an ISR
    if ((GPIOF->RIS & BTN_SW1) != 0U) { // interrupt caused by SW1?
        static QP::QEvt const pressedEvt(BTN_PRESSED_SIG);
        QP::QF::PUBLISH(&pressedEvt, nullptr);
    }
    GPIOF->ICR = 0xFFU; // clear interrupt sources
    QK_ISR_EXIT(); // inform QK about exiting an ISR
}

// BSP functions =============================================================
void BSP_init(void) {
    // Set the clocking to run directly from the crystal
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);
    SystemCoreClock = XTAL_HZ;

    // NOTE: The VFP (hardware Floating Point) unit is configured by QK

    // enable clock for to the peripherals used by this application...
    SYSCTL->RCGCGPIO  |= (1U << 5);  // enable Run mode for GPIOF
    SYSCTL->RCGCTIMER |= (1U << 0);  // enable Run mode for Timer0

    // configure the Timers
    TIMER0->CTL   &= ~(1U << 0); // disable Timer0 before any changes
    TIMER0->CFG    = 0U;
    TIMER0->TAMR  |= (0x2 << 0); // Timer0A
    TIMER0->TAMR  &= ~(1U << 4);
    TIMER0->TAILR  = XTAL_HZ / BSP_TICKS1_PER_SEC;
    TIMER0->IMR   &= ~(1U << 0); // disable timer interrupt for now
    SYSCTL->RCGCTIMER &= ~(1U << 0); // disable Run mode for Timer0

    // configure the LEDs and push buttons
    SYSCTL->RCGCGPIO  |= (1U << 5);  // enable Run mode for GPIOF
    GPIOF->DIR |= (LED_RED | LED_GREEN | LED_BLUE); // direction: output
    GPIOF->DEN |= (LED_RED | LED_GREEN | LED_BLUE); // digital enable
    GPIOF->DATA_Bits[LED_RED | LED_GREEN | LED_BLUE] = 0U; // turn off

    // configure the button SW1
    GPIOF->DIR &= ~BTN_SW1; /* input */
    GPIOF->DEN |= BTN_SW1; /* digital enable */
    GPIOF->PUR |= BTN_SW1; /* pull-up resistor enable */

    /* configure the GPIO interrupt for SW1 */
    GPIOF->IS  &= ~BTN_SW1; /* edge detect for SW1 */
    GPIOF->IBE &= ~BTN_SW1; /* only one edge generate the interrupt */
    GPIOF->IEV &= ~BTN_SW1; /* a falling edge triggers the interrupt */
    GPIOF->IM  |= BTN_SW1;  /* enable GPIOF interrupt for SW1 */
}
//............................................................................
void BSP_led0_off(void) {
    GPIOF->DATA_Bits[LED_GREEN] = 0U;
}
//............................................................................
void BSP_led0_on(void) {
    GPIOF->DATA_Bits[LED_GREEN] = 0xFFU;
}
//............................................................................
void BSP_led1_off(void) {
    GPIOF->DATA_Bits[LED_BLUE] = 0U;
}
//............................................................................
void BSP_led1_on(void) {
    GPIOF->DATA_Bits[LED_BLUE] = 0xFFU;
}
//............................................................................
void BSP_tickRate0_on(void) {
    // enable SysTick IRQ and Timer
    SysTick->CTRL |= (SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk);
    l_activeSet |= (1U << SYSTICK_ACTIVE);
}
//............................................................................
void BSP_tickRate1_on(void) {
    SYSCTL->RCGCTIMER |= (1U << 0U); // enable Run mode for Timer0
    TIMER0->CTL  &= ~(1U << 0U); // disable Timer0 before any changes
    TIMER0->IMR  |= (1U << 0U);  // enable timer interrupt
    TIMER0->CTL  |= (1U << 0U);  // enable Timer0 after the changes
    l_activeSet  |= (1U << TIMER0_ACTIVE);
}

// namespace QP **************************************************************
namespace QP {

// QF callbacks ==============================================================
void QF::onStartup(void) {
    // set up the SysTick timer to fire at BSP_TICKS0_PER_SEC rate
    SysTick_Config(SystemCoreClock / BSP_TICKS0_PER_SEC);

    // assing all priority bits for preemption-prio. and none to sub-prio.
    NVIC_SetPriorityGrouping(0U);

    // set priorities of ALL ISRs used in the system, see NOTE00
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!! CAUTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Assign a priority to EVERY ISR explicitly by calling NVIC_SetPriority().
    // DO NOT LEAVE THE ISR PRIORITIES AT THE DEFAULT VALUE!
    //
    NVIC_SetPriority(TIMER0A_IRQn,   QF_AWARE_ISR_CMSIS_PRI);
    NVIC_SetPriority(SysTick_IRQn,   QF_AWARE_ISR_CMSIS_PRI + 1U);
    NVIC_SetPriority(GPIOF_IRQn,     QF_AWARE_ISR_CMSIS_PRI + 1U);
    // ...

    // enable IRQs in the NVIC...
    NVIC_EnableIRQ(GPIOF_IRQn);
    NVIC_EnableIRQ(TIMER0A_IRQn);
}
//............................................................................
void QF::onCleanup(void) {
}
//............................................................................
void QK::onIdle(void) {
    QF_INT_DISABLE();
    if (((l_activeSet & (1U << SYSTICK_ACTIVE)) != 0U) // rate-0 enabled?
        && QP::QTimeEvt::noActive(0U))  // no time events at rate-0?
    {
        // safe to disable SysTick and interrupt
        SysTick->CTRL &= ~(SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk);
        l_activeSet &= ~(1U << SYSTICK_ACTIVE); // mark rate-0 as disabled
    }
    if (((l_activeSet & (1U << TIMER0_ACTIVE)) != 0U) // rate-1 enabled?
        && QP::QTimeEvt::noActive(1U))  // no time events at rate-1?
    {
        // safe to disable Timer0 and interrupt
        TIMER0->CTL  &= ~(1U << 0U); // disable Timer0
        TIMER0->IMR  &= ~(1U << 0U); // disable timer interrupt
        l_activeSet &= ~(1U << TIMER0_ACTIVE); // mark rate-1 as disabled
    }
    QF_INT_ENABLE();

    GPIOF->DATA_Bits[LED_RED] = 0xFFU; // turn LED on, see NOTE2
    __WFI(); // wait for interrupt
    GPIOF->DATA_Bits[LED_RED] = 0x00U; // turn LED off, see NOTE2
}

} // namespace QP

//............................................................................
extern "C" {

Q_NORETURN Q_onError(char const * const module, int_t const id) {
    // NOTE: add here your application-specific error handling
    //
    (void)module;
    (void)id;
#ifndef NDEBUG
    // for debugging, hang on in an endless loop toggling the RED LED...
    while (GPIOF->DATA_Bits[BTN_SW1] != 0) {
        GPIOF->DATA = LED_RED;
        GPIOF->DATA = 0U;
    }
#endif
    NVIC_SystemReset();
}
//............................................................................
void assert_failed(char const * const module, int_t const id); // prototype
void assert_failed(char const * const module, int_t const id) {
    Q_onError(module, id);
}

} // extern "C"

//============================================================================
// NOTE1:
// The bitmask l_activeSet is **shared** between the QK idle thread and
// the application-level threads. Therefore this variable needs to be
// always accessed in a critical section.
//
// NOTE2:
// One of the LEDs is used to visualize the CPU sleep mode. The LED is turned
// on before entering the sleep mode and turned off after waking up, so that
// the LED stays ON when the CPU sleeps.
//
// NOTE3:
// The additional polling-delay is added to artificially extend the CPU
// activity after the sleep mode is exited, so that the time the idle-LED
// stays off for a bit longer and can be observed on the slower logic analyzer.
//
