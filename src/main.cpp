// ESP32-C3 firmware entry point.
//
// M1 milestone: FSM + fake scanner driven over USB serial.
// No real hardware is wired yet — you can flash this to the ESP32-C3 and
// exercise the whole state machine by typing commands into the serial
// monitor:
//
//     PRESS
//     SCAN 012345678905
//     RELEASE
//
// The FSM responds by emitting Actions, which are logged as [ACT] lines.
// When we add real hardware in M5, only this file changes — the FSM,
// its tests, and the DB layer stay put.

#include <Arduino.h>
#include "db.h"
#include "fsm.h"

namespace {
FSM fsm;
Db  db;

// Cart state. Lives in RAM for M1; M2 persists to LittleFS.
struct CartLine {
    std::string barcode;
    int price_cents;
    bool pending;   // true while waiting for manual price entry
};
std::vector<CartLine> cart;

// Non-zero when a toast is on screen; fires TOAST_TIMEOUT when millis()
// passes this value.
unsigned long toast_deadline_ms = 0;

// ---- action execution -----------------------------------------------

void execute(const Action& a) {
    switch (a.type) {
        case ActionType::ARM_SCANNER:
            Serial.println("[ACT] arm scanner");
            // M5: send GM65 "trigger on" command over UART.
            break;

        case ActionType::DISARM_SCANNER:
            Serial.println("[ACT] disarm scanner");
            // M5: send GM65 "trigger off" command over UART.
            break;

        case ActionType::ADD_TO_CART: {
            auto item = db.lookup(a.data);
            if (item) {
                cart.push_back({a.data, item->price_cents, false});
                Serial.printf("[ACT] cart += %s ($%d.%02d)\n",
                              item->name.c_str(),
                              item->price_cents / 100,
                              item->price_cents % 100);
            }
            break;
        }

        case ActionType::ADD_PENDING:
            cart.push_back({a.data, 0, true});
            Serial.printf("[ACT] pending += %s\n", a.data.c_str());
            break;

        case ActionType::BEEP_SUCCESS:
            Serial.println("[ACT] beep: success");
            // M6: short high tone on the piezo.
            break;

        case ActionType::BEEP_ERROR:
            Serial.println("[ACT] beep: error");
            // M6: two low tones on the piezo.
            break;

        case ActionType::DISPLAY_READY: {
            int total = 0;
            int pending_n = 0;
            for (const auto& line : cart) {
                total += line.price_cents;
                if (line.pending) pending_n++;
            }
            Serial.printf("[DISPLAY] $%d.%02d  (pending: %d)\n",
                          total / 100, total % 100, pending_n);
            // M6: draw the idle screen on the SSD1306.
            break;
        }

        case ActionType::DISPLAY_TOAST:
            Serial.printf("[DISPLAY] %s\n", a.data.c_str());
            // M6: draw a centered message on the SSD1306.
            break;

        case ActionType::START_TOAST_TIMER:
            toast_deadline_ms = millis() + std::stoul(a.data);
            break;
    }
}

void dispatch(const Event& e) {
    for (const auto& a : fsm.handle(e)) execute(a);
}

// ---- fake scanner over USB serial -----------------------------------

void handle_serial_command(const String& line) {
    if (line == "PRESS") {
        dispatch({EventType::BUTTON_PRESS, ""});
    } else if (line == "RELEASE") {
        dispatch({EventType::BUTTON_RELEASE, ""});
    } else if (line.startsWith("SCAN ")) {
        dispatch({EventType::BARCODE_RECEIVED, line.substring(5).c_str()});
    } else {
        Serial.printf("unknown command: %s\n", line.c_str());
        Serial.println("valid: PRESS | RELEASE | SCAN <barcode>");
    }
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\ncostco-scanner starting (M1 stub)");

    // Seed a few test items so there's something to scan.
    db.upsert({"012345678905", "Milk 2L",           499});
    db.upsert({"098765432109", "Bread",             389});
    db.upsert({"076808000146", "Kirkland Eggs 24",  899});

    fsm.set_lookup([](const std::string& bc) { return db.lookup(bc); });

    // Fake boot-complete events. Real WiFi + DB-load come in M2/M3.
    dispatch({EventType::WIFI_CONNECTED, ""});
    dispatch({EventType::DB_LOADED, ""});

    Serial.println("Ready. Commands: PRESS | RELEASE | SCAN <barcode>");
}

void loop() {
    // Accumulate serial input into lines.
    static String buf;
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (buf.length() > 0) {
                handle_serial_command(buf);
                buf = "";
            }
        } else {
            buf += c;
        }
    }

    // Fire TOAST_TIMEOUT when the toast duration elapses.
    if (toast_deadline_ms != 0 && millis() >= toast_deadline_ms) {
        toast_deadline_ms = 0;
        dispatch({EventType::TOAST_TIMEOUT, ""});
    }
}
