//////////////////////////////////////////////////////////////////////////////
// Product:  QK/C++ port, H8, Renesas H8 compiler
// Last Updated for Version: 4.4.00
// Date of the Last Update:  Apr 19, 2012
//
//                    Q u a n t u m     L e a P s
//                    ---------------------------
//                    innovating embedded systems
//
// Copyright (C) 2002-2012 Quantum Leaps, LLC. All rights reserved.
//
// This program is open source software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 2 of the License, or
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
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//
// Contact information:
// Quantum Leaps Web sites: http://www.quantum-leaps.com
//                          http://www.state-machine.com
// e-mail:                  info@quantum-leaps.com
//////////////////////////////////////////////////////////////////////////////
#ifndef qk_port_h
#define qk_port_h

                                                       // interrupt entry code
#define QK_ISR_ENTRY()    (++QK_intNest_)

                                                        // interrupt exit code
#define QK_ISR_EXIT()     do { \
    --QK_intNest_; \
    if (QK_intNest_ == (uint8_t)0) { \
        QK_schedule_(0x00); \
    } \
} while (0)


#include "qk.h"                    // QK platform-independent public interface

#endif                                                            // qk_port_h
