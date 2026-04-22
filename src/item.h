#pragma once
#include <string>

// IMPORTANT: prices are integer cents. Never floats for money.
//   $4.99 is stored as 499.
// Rationale: repeated addition of floating-point prices accumulates rounding
// error ($0.01–0.10 off a full cart), and integers match how point-of-sale
// systems do it. Display-time division handles the decimal.
struct Item {
    std::string barcode;
    std::string name;
    int price_cents;
};
