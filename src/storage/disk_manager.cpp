#include "storage/disk_manager.h"
#include <cstring>
#include <filesystem>
#include <stdexcept>

DiskManager::DiskManager(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        std::ofstream create(path, std::ios::binary);   // create empty file
    }
    file_.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.is_open()) throw std::runtime_error("cannot open " + path);
    num_pages_ = static_cast<page_id_t>(std::filesystem::file_size(path) / PAGE_SIZE);
}

DiskManager::~DiskManager() { file_.close(); }

void DiskManager::ReadPage(page_id_t id, char* out) {
    std::memset(out, 0, PAGE_SIZE);
    if (id >= num_pages_) return;                        // never written: all zeros
    file_.seekg(static_cast<std::streamoff>(id) * PAGE_SIZE);
    file_.read(out, PAGE_SIZE);
    if (!file_) throw std::runtime_error("read failed");
}

void DiskManager::WritePage(page_id_t id, const char* data) {
    file_.seekp(static_cast<std::streamoff>(id) * PAGE_SIZE);
    file_.write(data, PAGE_SIZE);
    file_.flush();
    if (!file_) throw std::runtime_error("write failed");
    if (id >= num_pages_) num_pages_ = id + 1;
}

page_id_t DiskManager::AllocatePage() {
    char zero[PAGE_SIZE] = {};
    page_id_t id = num_pages_;
    WritePage(id, zero);                                 // grows the file by one page
    return id;
}