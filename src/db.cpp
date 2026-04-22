#include "db.h"

void Db::upsert(const Item& item) {
    items_[item.barcode] = item;
}

std::optional<Item> Db::lookup(const std::string& barcode) const {
    auto it = items_.find(barcode);
    if (it == items_.end()) return std::nullopt;
    return it->second;
}
