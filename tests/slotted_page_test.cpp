#include <gtest/gtest.h>
#include <string>
#include "storage/slotted_page.h"

TEST(SlottedPageTest, InsertAndGet) {
    char buf[PAGE_SIZE];
    SlottedPage page(buf);
    page.Init(0);
    auto slot = page.InsertRecord("hello", 5);
    ASSERT_TRUE(slot.has_value());
    uint16_t len;
    const char* rec = page.GetRecord(*slot, len);
    ASSERT_NE(rec, nullptr);
    EXPECT_EQ(std::string(rec, len), "hello");
}

TEST(SlottedPageTest, FillUntilFull) {
    char buf[PAGE_SIZE];
    SlottedPage page(buf);
    page.Init(0);
    char rec[100] = {};
    int count = 0;
    while (page.InsertRecord(rec, 100)) count++;
    EXPECT_EQ(count, 39);   // (4096 - 16) / (100 + 4) = 39
}

TEST(SlottedPageTest, DeleteMakesRecordGone) {
    char buf[PAGE_SIZE];
    SlottedPage page(buf);
    page.Init(0);
    auto a = page.InsertRecord("aaa", 3);
    auto b = page.InsertRecord("bbb", 3);
    EXPECT_TRUE(page.DeleteRecord(*a));
    uint16_t len;
    EXPECT_EQ(page.GetRecord(*a, len), nullptr);
    ASSERT_NE(page.GetRecord(*b, len), nullptr);   // the other record is untouched
    EXPECT_FALSE(page.DeleteRecord(*a));           // already deleted
}