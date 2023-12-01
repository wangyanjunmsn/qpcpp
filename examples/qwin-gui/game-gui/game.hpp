//$file${.::game.hpp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: game.qm
// File:  ${.::game.hpp}
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
//$endhead${.::game.hpp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#ifndef GAME_HPP_
#define GAME_HPP_

namespace GAME {

enum GameSignals : QP::QSignal { // signals used in the game
    TIME_TICK_SIG = QP::Q_USER_SIG, // published from tick ISR
    PLAYER_TRIGGER_SIG, // published by Player (ISR) to trigger the Missile
    PLAYER_QUIT_SIG,    // published by Player (ISR) to quit the game
    GAME_OVER_SIG,      // published by Ship when it finishes exploding

    // insert other published signals here ...
    MAX_PUB_SIG,        // the last published signal

    PLAYER_SHIP_MOVE_SIG, // posted by Player (ISR) to the Ship to move it

    BLINK_TIMEOUT_SIG,  // signal for Tunnel's blink timeout event
    SCREEN_TIMEOUT_SIG, // signal for Tunnel's screen timeout event

    TAKE_OFF_SIG,       // from Tunnel to Ship to grant permission to take off
    HIT_WALL_SIG,       // from Tunnel to Ship when Ship hits the wall
    HIT_MINE_SIG,       // from Mine to Ship or Missile when it hits the mine
    SHIP_IMG_SIG,       // from Ship to the Tunnel to draw and check for hits
    MISSILE_IMG_SIG,    // from Missile the Tunnel to draw and check for hits
    MINE_IMG_SIG,       // sent by Mine to the Tunnel to draw the mine
    MISSILE_FIRE_SIG,   // sent by Ship to the Missile to fire
    DESTROYED_MINE_SIG, // from Missile to Ship when Missile destroyed Mine
    EXPLOSION_SIG,      // from any exploding object to render the explosion
    MINE_PLANT_SIG,     // from Tunnel to the Mine to plant it
    MINE_DISABLED_SIG,  // from Mine to Tunnel when it becomes disabled
    MINE_RECYCLE_SIG,   // sent by Tunnel to Mine to recycle the mine
    SCORE_SIG,          // from Ship to Tunnel to display the score

    MAX_SIG             // the last signal (keep always last)
};

#define GAME_TUNNEL_WIDTH          BSP::SCREEN_WIDTH
#define GAME_TUNNEL_HEIGHT         (BSP::SCREEN_HEIGHT - 10U)
#define GAME_MINES_MAX             5U
#define GAME_MINES_DIST_MIN        10U
#define GAME_SPEED_X               1U
#define GAME_MISSILE_SPEED_X       2U
#define GAME_SHIP_X                10U
#define GAME_SHIP_Y                (GAME_TUNNEL_HEIGHT / 2U)
#define GAME_WALLS_GAP_Y           50U
#define GAME_WALLS_MIN_GAP_Y       20U

enum GameBitmapIds {
    SHIP_BMP,
    MISSILE_BMP,
    MINE1_BMP,
    MINE2_BMP,
    MINE2_MISSILE_BMP,
    EXPLOSION0_BMP,
    EXPLOSION1_BMP,
    EXPLOSION2_BMP,
    EXPLOSION3_BMP,
    MAX_BMP
};

} // namespace GAME

// Shared declarations
//$declare${Shared} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace GAME {

//${Shared::ObjectPosEvt} ....................................................
class ObjectPosEvt : public QP::QEvt {
public:
    std::uint8_t x;
    std::uint8_t y;

public:
    constexpr ObjectPosEvt(
        QP::QSignal sig,
        std::uint8_t const x0,
        std::uint8_t const y0)
      : QEvt(sig),
        x(x0),
        y(y0)
    {}
}; // class ObjectPosEvt

//${Shared::ObjectImageEvt} ..................................................
class ObjectImageEvt : public QP::QEvt {
public:
    std::uint8_t x;
    std::int8_t y;
    std::uint8_t bmp;
}; // class ObjectImageEvt

//${Shared::MineEvt} .........................................................
class MineEvt : public QP::QEvt {
public:
    std::uint8_t id;

public:
    constexpr MineEvt(
        QP::QSignal sig,
        std::uint8_t id_p)
     : QEvt(sig),
       id(id_p)
    {}
}; // class MineEvt

//${Shared::ScoreEvt} ........................................................
class ScoreEvt : public QP::QEvt {
public:
    std::uint16_t score;

public:
    constexpr ScoreEvt(
        QP::QSignal sig,
        std::uint16_t score_p)
     : QEvt(sig),
       score(score_p)
    {}
}; // class ScoreEvt

//${Shared::AO_Tunnel} .......................................................
// opaque pointer
extern QP::QActive * const AO_Tunnel;

//${Shared::AO_Ship} .........................................................
// opaque pointer
extern QP::QActive * const AO_Ship;

//${Shared::AO_Missile} ......................................................
// opaque pointer
extern QP::QActive * const AO_Missile;

//${Shared::Mine1_getInst} ...................................................
QP::QHsm * Mine1_getInst(std::uint8_t id);

//${Shared::Mine2_getInst} ...................................................
QP::QHsm * Mine2_getInst(std::uint8_t id);

} // namespace GAME
//$enddecl${Shared} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#endif  // GAME_HPP_
