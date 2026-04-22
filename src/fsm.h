#pragma once
#include "actions.h"
#include "events.h"
#include "item.h"
#include <functional>
#include <optional>
#include <string>
#include <vector>

enum class State {
    BOOT,           // waiting for wifi + db ready
    IDLE,           // cart total on screen, waiting for button
    SCANNING,       // button held, scanner armed, waiting for barcode
    SHOWING_TOAST,  // briefly displaying last-scan result
};

// Lookup is injected so the FSM never depends on storage.
// Production passes a real Db's lookup method.
// Tests pass a hand-built map, a lambda, or a mock that counts calls.
using LookupFn = std::function<std::optional<Item>(const std::string&)>;

class FSM {
public:
    FSM() = default;

    void set_lookup(LookupFn fn) { lookup_ = std::move(fn); }

    // Feed an event, get back the list of actions to perform.
    // Pure with respect to the outside world: no I/O, no globals.
    // This is what makes the FSM trivially unit-testable.
    std::vector<Action> handle(const Event& e);

    State state() const { return state_; }

private:
    State state_{State::BOOT};
    bool wifi_ready_{false};
    bool db_ready_{false};
    LookupFn lookup_;
};
