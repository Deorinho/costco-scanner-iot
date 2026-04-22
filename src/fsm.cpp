#include "fsm.h"
#include <cstdio>

namespace {
constexpr int TOAST_MS = 1500;

std::string format_known(const Item& item) {
    char buf[96];
    // "NAME  $X.XX" — OLED row is ~21 chars at a comfortable font size,
    // so keep DB names short or let the display code truncate.
    std::snprintf(buf, sizeof(buf), "%s  $%d.%02d",
                  item.name.c_str(),
                  item.price_cents / 100,
                  item.price_cents % 100);
    return std::string(buf);
}
}  // namespace

std::vector<Action> FSM::handle(const Event& e) {
    std::vector<Action> out;

    switch (state_) {
        case State::BOOT: {
            if (e.type == EventType::WIFI_CONNECTED) wifi_ready_ = true;
            if (e.type == EventType::DB_LOADED)      db_ready_   = true;
            if (wifi_ready_ && db_ready_) {
                state_ = State::IDLE;
                out.push_back({ActionType::DISPLAY_READY, ""});
            }
            break;
        }

        case State::IDLE: {
            if (e.type == EventType::BUTTON_PRESS) {
                state_ = State::SCANNING;
                out.push_back({ActionType::ARM_SCANNER, ""});
            }
            break;
        }

        case State::SCANNING: {
            if (e.type == EventType::BUTTON_RELEASE) {
                // User let go without a scan — back to idle.
                state_ = State::IDLE;
                out.push_back({ActionType::DISARM_SCANNER, ""});
                out.push_back({ActionType::DISPLAY_READY, ""});
                break;
            }
            if (e.type == EventType::BARCODE_RECEIVED) {
                out.push_back({ActionType::DISARM_SCANNER, ""});

                std::optional<Item> item;
                if (lookup_) item = lookup_(e.data);

                if (item.has_value()) {
                    out.push_back({ActionType::ADD_TO_CART, e.data});
                    out.push_back({ActionType::BEEP_SUCCESS, ""});
                    out.push_back({ActionType::DISPLAY_TOAST, format_known(*item)});
                } else {
                    // Unknown barcode: non-blocking. Drop a placeholder
                    // into the cart and let the phone UI resolve it later.
                    out.push_back({ActionType::ADD_PENDING, e.data});
                    out.push_back({ActionType::BEEP_ERROR, ""});
                    out.push_back({ActionType::DISPLAY_TOAST, "UNKNOWN: " + e.data});
                }

                state_ = State::SHOWING_TOAST;
                out.push_back({ActionType::START_TOAST_TIMER, std::to_string(TOAST_MS)});
            }
            break;
        }

        case State::SHOWING_TOAST: {
            if (e.type == EventType::TOAST_TIMEOUT) {
                state_ = State::IDLE;
                out.push_back({ActionType::DISPLAY_READY, ""});
            } else if (e.type == EventType::BUTTON_PRESS) {
                // Let the user re-trigger without waiting for toast to finish.
                state_ = State::SCANNING;
                out.push_back({ActionType::ARM_SCANNER, ""});
            }
            // BUTTON_RELEASE here is ignored (it's the trailing edge of the
            // hold that just produced a scan). Same with other events.
            break;
        }
    }

    return out;
}
