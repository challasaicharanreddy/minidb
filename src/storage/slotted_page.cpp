#include "storage/slotted_page.h"
#include <iostream>

void SlottedPage::Init(page_id_t id) {
    std::memset(data_, 0, PAGE_SIZE);

    Set<page_id_t>(0, id);
    Set<page_id_t>(4, INVALID_PAGE_ID);
    Set<uint16_t>(8, 0);
    Set<uint16_t>(10, PAGE_SIZE);
}

uint16_t SlottedPage::FreeSpace() const {
    return static_cast<uint16_t>(
        Get<uint16_t>(10) -
        (HEADER_SIZE + NumSlots() * SLOT_SIZE)
    );
}

std::optional<uint16_t> SlottedPage::InsertRecord(
    const char* rec,
    uint16_t len
) {
    if (len == 0 || len + SLOT_SIZE > FreeSpace()) {
        return std::nullopt;
    }

    uint16_t slot = NumSlots();

    uint16_t offset =
        static_cast<uint16_t>(Get<uint16_t>(10) - len);

    std::memcpy(data_ + offset, rec, len);

    Set<uint16_t>(SlotPos(slot), offset);
    Set<uint16_t>(SlotPos(slot) + 2, len);

    Set<uint16_t>(8, slot + 1);
    Set<uint16_t>(10, offset);

    return slot;
}

const char* SlottedPage::GetRecord(
    uint16_t slot,
    uint16_t& len
) const {
    if (slot >= NumSlots()) {
        return nullptr;
    }

    len = Get<uint16_t>(SlotPos(slot) + 2);

    if (len == 0) {
        return nullptr;
    }

    return data_ + Get<uint16_t>(SlotPos(slot));
}

bool SlottedPage::DeleteRecord(uint16_t slot) {
    if (slot >= NumSlots() ||
        Get<uint16_t>(SlotPos(slot) + 2) == 0) {
        return false;
    }

    Set<uint16_t>(SlotPos(slot) + 2, 0);

    return true;
}

void SlottedPage::Dump() const {
    std::cout << "page " << Get<page_id_t>(0)
              << " next " << NextPageId()
              << " slots " << NumSlots()
              << " free " << FreeSpace()
              << "\n";

    for (uint16_t i = 0; i < NumSlots(); i++) {
        std::cout << "  slot " << i
                  << ": offset "
                  << Get<uint16_t>(SlotPos(i))
                  << " len "
                  << Get<uint16_t>(SlotPos(i) + 2)
                  << "\n";
    }
}