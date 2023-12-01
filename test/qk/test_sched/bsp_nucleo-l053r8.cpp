//============================================================================
// Product: BSP for system-testing of QK kernel, NUCLEO-L053R8 board
// Last updated for version 7.2.1
// Last updated on  2023-01-26
//
//                    Q u a n t u m  L e a P s
//                    ------------------------
//                    Modern Embedded Software
//
// Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
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
// along with this program. If not, see <www.gnu.org/licenses>.
//
// Contact information:
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//============================================================================
#include "qpcpp.hpp"
#include "bsp.hpp"

#include "stm32l0xx.h"  // CMSIS-compliant header file for the MCU used
// add other drivers if necessary...

namespace {

Q_DEFINE_THIS_FILE

// Local-scope objects -------------------------------------------------------
// LED pins available on the board (just one user LED LD2--Green on PA.5)
#define LED_LD2  (1U << 5)

// Button pins available on the board (just one user Button B1 on PC.13)
#define BTN_B1   (1U << 13)

#ifdef Q_SPY

    // QSpy source IDs
    static QP::QSpyId const l_SysTick_Handler = { 100U };
    static QP::QSpyId const l_test_ISR = { 101U };

    enum AppRecords { // application-specific trace records
        CONTEXT_SW = QP::QS_USER1,
        TRACE_MSG
    };

#endif

} // unnamed namespace

// ISRs used in this project =================================================
extern "C" {

//............................................................................
void SysTick_Handler(void); // prototype
void SysTick_Handler(void) {
    QK_ISR_ENTRY();   // inform QK about entering an ISR

    QP::QTimeEvt::TICK_X(0U, &l_SysTick_Handler);
    //the_Ticker0->TRIG(&l_SysTick_Handler);

    QK_ISR_EXIT();  // inform QK about exiting an ISR
}
//............................................................................
void EXTI0_1_IRQHandler(void);  // prototype
void EXTI0_1_IRQHandler(void) {
    QK_ISR_ENTRY(); // inform QK about entering an ISR

    // for testing...
    static QP::QEvt const t1(TEST1_SIG);
    QP::QF::PUBLISH(&t1, &l_test_ISR);

    QK_ISR_EXIT();  // inform QK about exiting an ISR
}

} // extern "C"

// BSP functions =============================================================

static void STM32L053R8_MPU_setup(void) {
    // The following MPU configuration contains the general STM32 memory model
    // as described in the ST AppNote AN4838 "Managing memory protection unit
    // in STM32 MCUs", Figure 2. Cortex-M0+/M3/M4/M7 processor memory map.
    //
    // Please note that the actual STM32 MCUs provide much less Flash and SRAM
    // than the maximums configured here. This means that actual MCUs have
    // unmapped memory regions (e.g., beyond the actual SRAM). Attempts to
    // access these regions causes the HardFault exception, which is the
    // desired behavior.
    //
    static struct {
        uint32_t rbar;
        uint32_t rasr;
    } const mpu_setup[] = {

        { // region #0: Flash: base=0x0000'0000, size=512M=2^(28+1)
          0x00000000U                       // base address
              | MPU_RBAR_VALID_Msk          // valid region
              | (MPU_RBAR_REGION_Msk & 0U), // region #0
          (28U << MPU_RASR_SIZE_Pos)        // 2^(18+1) region
              | (0x6U << MPU_RASR_AP_Pos)   // PA:ro/UA:ro
              | (1U << MPU_RASR_C_Pos)      // C=1
              | MPU_RASR_ENABLE_Msk         // region enable
        },

        { // region #1: SRAM: base=0x2000'0000, size=512M=2^(28+1)
          0x20000000U                       // base address
              | MPU_RBAR_VALID_Msk          // valid region
              | (MPU_RBAR_REGION_Msk & 1U), // region #1
          (28U << MPU_RASR_SIZE_Pos)        // 2^(28+1) region
              | (0x3U << MPU_RASR_AP_Pos)   // PA:rw/UA:rw
              | (1U << MPU_RASR_XN_Pos)     // XN=1
              | (1U << MPU_RASR_S_Pos)      // S=1
              | (1U << MPU_RASR_C_Pos)      // C=1
              | MPU_RASR_ENABLE_Msk         // region enable
        },

        // region #3: (not configured)
        { MPU_RBAR_VALID_Msk | (MPU_RBAR_REGION_Msk & 2U), 0U },

        { // region #3: Peripherals: base=0x4000'0000, size=512M=2^(28+1)
          0x40000000U                       // base address
              | MPU_RBAR_VALID_Msk          // valid region
              | (MPU_RBAR_REGION_Msk & 3U), // region #3
          (28U << MPU_RASR_SIZE_Pos)        // 2^(28+1) region
              | (0x3U << MPU_RASR_AP_Pos)   // PA:rw/UA:rw
              | (1U << MPU_RASR_XN_Pos)     // XN=1
              | (1U << MPU_RASR_S_Pos)      // S=1
              | (1U << MPU_RASR_B_Pos)      // B=1
              | MPU_RASR_ENABLE_Msk         // region enable
        },

        { // region #4: Priv. Periph: base=0xE000'0000, size=512M=2^(28+1)
          0xE0000000U                       // base address
              | MPU_RBAR_VALID_Msk          // valid region
              | (MPU_RBAR_REGION_Msk & 4U), // region #4
          (28U << MPU_RASR_SIZE_Pos)        // 2^(28+1) region
              | (0x3U << MPU_RASR_AP_Pos)   // PA:rw/UA:rw
              | (1U << MPU_RASR_XN_Pos)     // XN=1
              | (1U << MPU_RASR_S_Pos)      // S=1
              | (1U << MPU_RASR_B_Pos)      // B=1
              | MPU_RASR_ENABLE_Msk         // region enable
        },

        { // region #5: Ext RAM: base=0x6000'0000, size=1G=2^(29+1)
          0x60000000U                       // base address
              | MPU_RBAR_VALID_Msk          // valid region
              | (MPU_RBAR_REGION_Msk & 5U), // region #5
          (29U << MPU_RASR_SIZE_Pos)        // 2^(28+1) region
              | (0x3U << MPU_RASR_AP_Pos)   // PA:rw/UA:rw
              | (1U << MPU_RASR_XN_Pos)     // XN=1
              | (1U << MPU_RASR_S_Pos)      // S=1
              | (1U << MPU_RASR_B_Pos)      // B=1
              | MPU_RASR_ENABLE_Msk         // region enable
        },

        { // region #6: Ext Dev: base=0xA000'0000, size=1G=2^(29+1)
          0xA0000000U                       // base address
              | MPU_RBAR_VALID_Msk          // valid region
              | (MPU_RBAR_REGION_Msk & 6U), // region #6
          (29U << MPU_RASR_SIZE_Pos)        // 2^(28+1) region
              | (0x3U << MPU_RASR_AP_Pos)   // PA:rw/UA:rw
              | (1U << MPU_RASR_XN_Pos)     // XN=1
              | (1U << MPU_RASR_S_Pos)      // S=1
              | (1U << MPU_RASR_B_Pos)      // B=1
              | MPU_RASR_ENABLE_Msk         // region enable
        },

        { // region #7: NULL-pointer: base=0x000'0000, size=128M=2^(26+1)
          // NOTE: this region extends to  0x080'0000, which is where
          // the ROM is re-mapped by STM32

          0x00000000U                       // base address
              | MPU_RBAR_VALID_Msk          // valid region
              | (MPU_RBAR_REGION_Msk & 7U), // region #7
          (26U << MPU_RASR_SIZE_Pos)        // 2^(26+1)=128M region
              | (0x0U << MPU_RASR_AP_Pos)   // PA:na/UA:na
              | (1U << MPU_RASR_XN_Pos)     // XN=1
              | MPU_RASR_ENABLE_Msk         // region enable
        },

    };

    __DSB();
    MPU->CTRL = 0U; // disable the MPU
    for (uint_fast8_t n = 0U; n < Q_DIM(mpu_setup); ++n) {
        MPU->RBAR = mpu_setup[n].rbar;
        MPU->RASR = mpu_setup[n].rasr;
    }
    MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk     // enable background region
                | MPU_CTRL_ENABLE_Msk;      // enable the MPU
    __ISB();
    __DSB();
}
//............................................................................
void BSP::init(void) {
    // setup the MPU...
    STM32L053R8_MPU_setup();

    // NOTE: SystemInit() already called from the startup code
    //  but SystemCoreClock needs to be updated
    //
    SystemCoreClockUpdate();

    // enable GPIOA clock port for the LED LD2
    RCC->IOPENR |= (1U << 0);

    // configure LED (PA.5) pin as push-pull output, no pull-up, pull-down
    GPIOA->MODER   &= ~((3U << 2*5));
    GPIOA->MODER   |=  ((1U << 2*5));
    GPIOA->OTYPER  &= ~((1U <<   5));
    GPIOA->OSPEEDR &= ~((3U << 2*5));
    GPIOA->OSPEEDR |=  ((1U << 2*5));
    GPIOA->PUPDR   &= ~((3U << 2*5));

    // enable GPIOC clock port for the Button B1
    RCC->IOPENR |=  (1U << 2);

    // configure Button (PC.13) pins as input, no pull-up, pull-down
    GPIOC->MODER   &= ~(3U << 2*13);
    GPIOC->OSPEEDR &= ~(3U << 2*13);
    GPIOC->OSPEEDR |=  (1U << 2*13);
    GPIOC->PUPDR   &= ~(3U << 2*13);

    // initialize the QS software tracing...
    if (!QS_INIT(nullptr)) { // initialize the QS software tracing
        Q_ERROR();
    }

    // dictionaries...
    QS_OBJ_DICTIONARY(&l_SysTick_Handler);
    QS_OBJ_DICTIONARY(&l_test_ISR);

    QS_USR_DICTIONARY(CONTEXT_SW);
    QS_USR_DICTIONARY(TRACE_MSG);
}
//............................................................................
void BSP::terminate(int16_t result) {
    Q_UNUSED_PAR(result);
}
//............................................................................
void BSP::ledOn(void) {
    GPIOA->BSRR = (LED_LD2); // turn LED2 on
}
//............................................................................
void BSP::ledOff(void) {
    GPIOA->BSRR = (LED_LD2 << 16); // turn LED2 off
}
//............................................................................
void BSP::trigISR(void) {
    NVIC_SetPendingIRQ(EXTI0_1_IRQn);
}
//............................................................................
void BSP::trace(QP::QActive const *thr, char const *msg) {
    QS_BEGIN_ID(TRACE_MSG, 0U)
        QS_OBJ(thr);
        QS_STR(msg);
    QS_END()
}
//............................................................................
uint32_t BSP::romRead(int32_t offset, uint32_t fromEnd) {
    int32_t const rom_base = (fromEnd == 0U)
                             ? 0x08000000
                             : 0x08010000 - 4;
    return *(uint32_t volatile *)(rom_base + offset);
}
//............................................................................
void BSP::romWrite(int32_t offset, uint32_t fromEnd, uint32_t value) {
    int32_t const rom_base = (fromEnd == 0U)
                             ? 0x08000000
                             : 0x08010000 - 4;
    *(uint32_t volatile *)(rom_base + offset) = value;
}

//............................................................................
uint32_t BSP::ramRead(int32_t offset, uint32_t fromEnd) {
    int32_t const ram_base = (fromEnd == 0U)
                             ? 0x20000000
                             : 0x20002000 - 4;
    return *(uint32_t volatile *)(ram_base + offset);
}
//............................................................................
void BSP::ramWrite(int32_t offset, uint32_t fromEnd, uint32_t value) {
    int32_t const ram_base = (fromEnd == 0U)
                             ? 0x20000000
                             : 0x20002000 - 4;
    *(uint32_t volatile *)(ram_base + offset) = value;
}

// namespace QP ==============================================================
namespace QP {

// QF callbacks --------------------------------------------------------------
void QF::onStartup(void) {
    // assign all priority bits for preemption-prio. and none to sub-prio.
    // NOTE: this might have been changed by STM32Cube.
    NVIC_SetPriorityGrouping(0U);

    // set priorities of ALL ISRs used in the system
    NVIC_SetPriority(USART2_IRQn, 0U);
    NVIC_SetPriority(SysTick_IRQn, QF_AWARE_ISR_CMSIS_PRI + 0U);
    NVIC_SetPriority(EXTI0_1_IRQn, QF_AWARE_ISR_CMSIS_PRI + 1U);
    // ...

    // enable IRQs...
    NVIC_EnableIRQ(EXTI0_1_IRQn);
}
//............................................................................
void QF::onCleanup(void) {
}
//............................................................................
void QK::onIdle(void) {
#ifdef Q_SPY
    QS::rxParse();  // parse all the received bytes
    QS::doOutput();
#elif defined NDEBUG
    // Put the CPU and peripherals to the low-power mode.
    // you might need to customize the clock management for your application,
    // see the datasheet for your particular Cortex-M3 MCU.
    //
    __WFI(); // Wait-For-Interrupt
#endif
}

// QS callbacks ==============================================================
//............................................................................
void QTimeEvt::tick1_(
    uint_fast8_t const tickRate,
    void const * const sender)
{
    QF_INT_DISABLE();
    // TODO pend the SysTick
    *Q_UINT2PTR_CAST(uint32_t, 0xE000ED04U) = (1U << 26U);
    QF_INT_ENABLE();
}

//----------------------------------------------------------------------------

} // namespace QP

extern "C" {
//............................................................................
#ifdef QF_ON_CONTEXT_SW
// NOTE: the context-switch callback is called with interrupts DISABLED
void QF_onContextSw(QP::QActive *prev, QP::QActive *next) {
    QS_BEGIN_INCRIT(CONTEXT_SW, 0U) // no critical section!
        QS_OBJ(prev);
        QS_OBJ(next);
    QS_END_INCRIT()
}
#endif // QF_ON_CONTEXT_SW

} // extern "C"

//============================================================================
// NOTE0:
// The MPU protection against NULL-pointer dereferencing sets up a no-access
// MPU region #7 around the NULL address (0x0). The size of this region is set
// to 2^(26+1)==0x0800'0000, because that is the address of Flash in STM32.
//
// REMARK: STM32 MCUs automatically relocate the Flash memory and the Vector
// Table in it to address 0x0800'0000 at startup. However, even though the
// region 0..0x0800'0000 is un-mapped after the relocation, the read access
// is still allowed and causes no CPU exception. Therefore setting up the MPU
// to protect that region is necessary.
//
