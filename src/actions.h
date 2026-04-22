#pragma once
#include <string>

// Actions are what the FSM wants the outside world to do.
//
// The driver code in main.cpp translates each Action into calls on real
// hardware (GPIO, UART, I2C display) and on service objects (cart, DB).
// Tests assert on the returned action list directly — they never run any
// of the side effects.

enum class ActionType {
    ARM_SCANNER,
    DISARM_SCANNER,
    ADD_TO_CART,         // data = barcode; price resolved via DB
    ADD_PENDING,         // data = barcode; unknown, needs manual entry
    BEEP_SUCCESS,
    BEEP_ERROR,
    DISPLAY_READY,       // idle screen: cart total, pending count
    DISPLAY_TOAST,       // data = formatted line, e.g. "Milk 2L  $4.99"
    START_TOAST_TIMER,   // data = duration in ms (as string)
};

struct Action {
    ActionType type;
    std::string data;
};
