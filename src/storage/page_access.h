#pragma once

#include <memory>
#include <unordered_map>

#include "common/config.h"
#include "storage/disk_manager.h"

class PageAccess {
public:
    explicit PageAccess(DiskManager* dm) : dm_(dm) {}

    char* FetchPage(page_id_t id);
    char* NewPage(page_id_t& id);
    void UnpinPage(page_id_t id, bool dirty);

    page_id_t NumPages() const { return dm_->NumPages(); }

    bool AllUnpinned() const { return frames_.empty(); }

private:
    struct Frame {
        std::unique_ptr<char[]> data;
        int pins = 0;
        bool dirty = false;
    };

    DiskManager* dm_;
    std::unordered_map<page_id_t, Frame> frames_;
};