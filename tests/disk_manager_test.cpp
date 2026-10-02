#include <gtest/gtest.h>
#include <cstring>
#include <filesystem>
#include "storage/disk_manager.h"

class DiskManagerTest : public ::testing::Test {
protected:
    std::string path = "test.db";
    void SetUp() override { std::filesystem::remove(path); }
    void TearDown() override { std::filesystem::remove(path); }
};

TEST_F(DiskManagerTest, WriteThenRead) {
    DiskManager dm(path);
    char in[PAGE_SIZE], out[PAGE_SIZE];
    std::memset(in, 'A', PAGE_SIZE);
    dm.WritePage(3, in);
    dm.ReadPage(3, out);
    EXPECT_EQ(std::memcmp(in, out, PAGE_SIZE), 0);
}

TEST_F(DiskManagerTest, PersistsAfterReopen) {
    char in[PAGE_SIZE], out[PAGE_SIZE];
    std::memset(in, 'B', PAGE_SIZE);
    { DiskManager dm(path); dm.WritePage(0, in); }
    DiskManager dm2(path);
    dm2.ReadPage(0, out);
    EXPECT_EQ(std::memcmp(in, out, PAGE_SIZE), 0);
}

TEST_F(DiskManagerTest, UnwrittenPageIsZero) {
    DiskManager dm(path);
    char out[PAGE_SIZE];
    dm.ReadPage(5, out);
    for (char c : out) EXPECT_EQ(c, 0);
}

TEST_F(DiskManagerTest, AllocateIsSequential) {
    DiskManager dm(path);
    EXPECT_EQ(dm.AllocatePage(), 0);
    EXPECT_EQ(dm.AllocatePage(), 1);
    EXPECT_EQ(dm.NumPages(), 2);
}