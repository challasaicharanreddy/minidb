#pragma once
#include <fstream>
#include <string>
#include "common/config.h"

class DiskManager {
public:
    explicit DiskManager(const std::string& path);
    ~DiskManager();
    DiskManager(const DiskManager&) = delete;
    DiskManager& operator=(const DiskManager&) = delete;

    void ReadPage(page_id_t id, char* out);
    void WritePage(page_id_t id, const char* data);
    page_id_t AllocatePage();
    page_id_t NumPages() const { return num_pages_; }

private:
    std::fstream file_;
    page_id_t num_pages_;
};