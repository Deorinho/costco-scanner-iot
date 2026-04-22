#pragma once
#include <string>

// Events are everything that happens TO the FSM.
//
// Hardware drivers (button ISR, scanner UART reader, WiFi callback, etc.)
// convert physical signals into Events and push them into the FSM.
// Tests push Events in directly — no hardware needed.

enum class EventType {
    WIFI_CONNECTED,
    DB_LOADED,
    BUTTON_PRESS,
    BUTTON_RELEASE,
    BARCODE_RECEIVED,   // data = barcode string
    TOAST_TIMEOUT,
};

struct Event {
    EventType type;
    std::string data;
};
