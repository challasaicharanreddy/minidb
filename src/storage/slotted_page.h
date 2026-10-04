#pragma once
#include <cstdint>
#include <cstring>
#include <optional>
#include "common/config.h"

class SlottedPage {
public:
    explicit SlottedPage(char* data) : data_(data) {}

    void Init(page_id_t id);

    std::optional<uint16_t> InsertRecord(
        const char* rec,
        uint16_t len
    );

    const char* GetRecord(
        uint16_t slot,
        uint16_t& len
    ) const;

    bool DeleteRecord(uint16_t slot);

    uint16_t NumSlots() const {
        return Get<uint16_t>(8);
    }

    page_id_t NextPageId() const {
        return Get<page_id_t>(4);
    }

    void SetNextPageId(page_id_t id) {
        Set<page_id_t>(4, id);
    }

    uint16_t FreeSpace() const;

    void Dump() const;

private:
    static constexpr size_t HEADER_SIZE = 16;
    static constexpr size_t SLOT_SIZE = 4;

    static size_t SlotPos(uint16_t i) {
        return HEADER_SIZE + i * SLOT_SIZE;
    }

    template <typename T>
    T Get(size_t pos) const {
        T v;
        std::memcpy(&v, data_ + pos, sizeof v);
        return v;
    }

    template <typename T>
    void Set(size_t pos, T v) {
        std::memcpy(data_ + pos, &v, sizeof v);
    }

    char* data_;
};