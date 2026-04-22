#pragma once
#include "item.h"
#include <optional>
#include <string>
#include <unordered_map>

// In-memory barcode → Item store.
//
// Kept as its own class so the FSM can be handed a std::function lookup
// and stay unaware of where the data lives. In M2 this class grows a
// load()/save() pair that serializes to LittleFS, but the interface the
// FSM sees won't change.
class Db {
public:
    void upsert(const Item& item);
    std::optional<Item> lookup(const std::string& barcode) const;
    size_t size() const { return items_.size(); }
    void clear() { items_.clear(); }

private:
    std::unordered_map<std::string, Item> items_;
};
