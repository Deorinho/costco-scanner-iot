// Host-side unit tests for the FSM.
// Run with:   pio test -e native
//
// These tests never touch hardware. They drive the FSM directly by feeding
// Events and asserting on the returned list of Actions.

#include "db.h"
#include "fsm.h"
#include <unity.h>

namespace {
FSM* fsm;
Db*  db;

// Drive the FSM from BOOT to IDLE for tests that don't care about boot.
void boot_to_idle() {
    fsm->handle({EventType::WIFI_CONNECTED, ""});
    fsm->handle({EventType::DB_LOADED, ""});
}

bool has_action(const std::vector<Action>& actions, ActionType t) {
    for (const auto& a : actions) if (a.type == t) return true;
    return false;
}

const Action* find_action(const std::vector<Action>& actions, ActionType t) {
    for (const auto& a : actions) if (a.type == t) return &a;
    return nullptr;
}
}  // namespace

void setUp(void) {
    fsm = new FSM();
    db  = new Db();
    db->upsert({"012345678905", "Milk 2L", 499});
    db->upsert({"098765432109", "Bread",   389});
    fsm->set_lookup([](const std::string& bc) { return db->lookup(bc); });
}

void tearDown(void) {
    delete fsm;
    delete db;
}

// ---- BOOT ------------------------------------------------------------

void test_boot_needs_both_wifi_and_db(void) {
    auto out = fsm->handle({EventType::WIFI_CONNECTED, ""});
    TEST_ASSERT_EQUAL(static_cast<int>(State::BOOT), static_cast<int>(fsm->state()));
    TEST_ASSERT_EQUAL(0, out.size());

    out = fsm->handle({EventType::DB_LOADED, ""});
    TEST_ASSERT_EQUAL(static_cast<int>(State::IDLE), static_cast<int>(fsm->state()));
    TEST_ASSERT_TRUE(has_action(out, ActionType::DISPLAY_READY));
}

void test_boot_order_does_not_matter(void) {
    fsm->handle({EventType::DB_LOADED, ""});
    TEST_ASSERT_EQUAL(static_cast<int>(State::BOOT), static_cast<int>(fsm->state()));
    fsm->handle({EventType::WIFI_CONNECTED, ""});
    TEST_ASSERT_EQUAL(static_cast<int>(State::IDLE), static_cast<int>(fsm->state()));
}

// ---- IDLE ------------------------------------------------------------

void test_button_press_arms_scanner(void) {
    boot_to_idle();
    auto out = fsm->handle({EventType::BUTTON_PRESS, ""});
    TEST_ASSERT_EQUAL(static_cast<int>(State::SCANNING), static_cast<int>(fsm->state()));
    TEST_ASSERT_TRUE(has_action(out, ActionType::ARM_SCANNER));
}

// ---- SCANNING --------------------------------------------------------

void test_button_release_without_scan_returns_to_idle(void) {
    boot_to_idle();
    fsm->handle({EventType::BUTTON_PRESS, ""});
    auto out = fsm->handle({EventType::BUTTON_RELEASE, ""});
    TEST_ASSERT_EQUAL(static_cast<int>(State::IDLE), static_cast<int>(fsm->state()));
    TEST_ASSERT_TRUE(has_action(out, ActionType::DISARM_SCANNER));
    TEST_ASSERT_TRUE(has_action(out, ActionType::DISPLAY_READY));
}

void test_known_barcode_adds_to_cart(void) {
    boot_to_idle();
    fsm->handle({EventType::BUTTON_PRESS, ""});
    auto out = fsm->handle({EventType::BARCODE_RECEIVED, "012345678905"});

    TEST_ASSERT_EQUAL(static_cast<int>(State::SHOWING_TOAST), static_cast<int>(fsm->state()));
    TEST_ASSERT_TRUE(has_action(out, ActionType::ADD_TO_CART));
    TEST_ASSERT_TRUE(has_action(out, ActionType::BEEP_SUCCESS));
    TEST_ASSERT_TRUE(has_action(out, ActionType::START_TOAST_TIMER));
    TEST_ASSERT_FALSE(has_action(out, ActionType::ADD_PENDING));

    const Action* toast = find_action(out, ActionType::DISPLAY_TOAST);
    TEST_ASSERT_NOT_NULL(toast);
    TEST_ASSERT_TRUE(toast->data.find("Milk 2L") != std::string::npos);
    TEST_ASSERT_TRUE(toast->data.find("$4.99") != std::string::npos);
}

void test_unknown_barcode_goes_to_pending(void) {
    boot_to_idle();
    fsm->handle({EventType::BUTTON_PRESS, ""});
    auto out = fsm->handle({EventType::BARCODE_RECEIVED, "999999999999"});

    TEST_ASSERT_EQUAL(static_cast<int>(State::SHOWING_TOAST), static_cast<int>(fsm->state()));
    TEST_ASSERT_TRUE(has_action(out, ActionType::ADD_PENDING));
    TEST_ASSERT_TRUE(has_action(out, ActionType::BEEP_ERROR));
    TEST_ASSERT_FALSE(has_action(out, ActionType::ADD_TO_CART));
}

// ---- SHOWING_TOAST ---------------------------------------------------

void test_toast_timeout_returns_to_idle(void) {
    boot_to_idle();
    fsm->handle({EventType::BUTTON_PRESS, ""});
    fsm->handle({EventType::BARCODE_RECEIVED, "012345678905"});
    auto out = fsm->handle({EventType::TOAST_TIMEOUT, ""});
    TEST_ASSERT_EQUAL(static_cast<int>(State::IDLE), static_cast<int>(fsm->state()));
    TEST_ASSERT_TRUE(has_action(out, ActionType::DISPLAY_READY));
}

void test_button_press_during_toast_re_arms(void) {
    boot_to_idle();
    fsm->handle({EventType::BUTTON_PRESS, ""});
    fsm->handle({EventType::BARCODE_RECEIVED, "012345678905"});
    // Still in SHOWING_TOAST. User is fast and wants the next scan now.
    auto out = fsm->handle({EventType::BUTTON_PRESS, ""});
    TEST_ASSERT_EQUAL(static_cast<int>(State::SCANNING), static_cast<int>(fsm->state()));
    TEST_ASSERT_TRUE(has_action(out, ActionType::ARM_SCANNER));
}

// ---- entry -----------------------------------------------------------

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_boot_needs_both_wifi_and_db);
    RUN_TEST(test_boot_order_does_not_matter);
    RUN_TEST(test_button_press_arms_scanner);
    RUN_TEST(test_button_release_without_scan_returns_to_idle);
    RUN_TEST(test_known_barcode_adds_to_cart);
    RUN_TEST(test_unknown_barcode_goes_to_pending);
    RUN_TEST(test_toast_timeout_returns_to_idle);
    RUN_TEST(test_button_press_during_toast_re_arms);
    return UNITY_END();
}
