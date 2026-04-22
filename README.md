# costco-scanner

Handheld ESP32-C3 that scans barcodes and keeps a running Costco subtotal.

## Current state: M1 — FSM + fake scanner

No real hardware wired yet. You can flash this to the ESP32-C3 and drive the
entire state machine over the USB serial monitor:

```
PRESS
SCAN 012345678905
RELEASE
```

The firmware logs `[ACT]` lines showing what actions the FSM emitted. When
real hardware lands in M5, only `src/main.cpp` changes — `fsm.{h,cpp}`, its
tests, and the DB layer are untouched.

## Layout

```
src/
  events.h     things that happen TO the FSM
  actions.h    things the FSM wants done
  item.h       barcode + name + price_cents (integers, no floats for money)
  db.h/.cpp    in-memory barcode → Item lookup (persistence: M2)
  fsm.h/.cpp   pure state machine, zero hardware dependencies
  main.cpp     Arduino entry: converts serial input → Events, runs Actions
test/
  test_fsm/    host-runnable Unity tests for the FSM
platformio.ini
```

## Build & run

### Unit tests (no hardware needed)

```
pio test -e native
```

Expected: 8 passing tests covering boot, idle, scanning, and toast states.

### Firmware on the ESP32-C3

```
pio run -e esp32-c3
pio run -e esp32-c3 -t upload
pio device monitor
```

Then type `PRESS`, `SCAN 012345678905`, `RELEASE` into the monitor.

## Design principles

**Prices are integer cents.** `$4.99` is `499`. Float accumulation silently
breaks cart totals.

**The FSM is hardware-free.** `handle()` takes an `Event`, returns a
`vector<Action>`, no I/O, no globals. That's why the tests run on your
laptop — and why M5 hardware bring-up is a day, not a week.

**Lookup is injected.** The FSM gets a `std::function` at setup time. Tests
pass a lambda over a hand-built map; production passes the real `Db`.

**Unknown barcodes are non-blocking.** They go into a pending queue with a
`$0.00` placeholder, the scanner re-arms immediately, and manual price entry
happens asynchronously from the phone UI. Shopping stays fast.

## Roadmap

- [x] **M1** — FSM + fake scanner over USB serial *(you are here)*
- [ ] **M2** — LittleFS persistence, atomic writes, cart survives reboot
- [ ] **M3** — HTTP API + SSE, driven with curl
- [ ] **M4** — Web UI served from flash (phone browser)
- [ ] **M5** — Hardware: GM65 UART, button, OLED, buzzer
- [ ] **M6** — Display + audio polish
- [ ] **M7** — 3D-printed enclosure
- [ ] **M8** — Field test at Costco
