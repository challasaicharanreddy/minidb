#include <gtest/gtest.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>
#include "catalog/catalog.h"
#include "storage/table_heap.h"

namespace {
const char* kPath = "heap_test.db";

struct Db {   // everything needed to open a database file
    DiskManager dm;
    PageAccess pages;
    Catalog catalog;
    explicit Db(const std::string& path) : dm(path), pages(&dm), catalog(&pages) {}
};

std::vector<Column> UserColumns() {
    return {{"id", ColumnType::INT, 4}, {"name", ColumnType::VARCHAR, 20}};
}

int CountRows(TableHeap& heap) {
    TableIterator it = heap.Begin();
    Row row;
    RID rid;
    int n = 0;
    while (it.Next(row, rid)) n++;
    return n;
}
}  // namespace

class TableHeapTest : public ::testing::Test {
protected:
    void SetUp() override { std::filesystem::remove(kPath); }
    void TearDown() override { std::filesystem::remove(kPath); }
};

TEST_F(TableHeapTest, InsertAndScanTenThousandRows) {
    Db db(kPath);
    TableInfo* t = db.catalog.CreateTable("users", UserColumns());
    TableHeap heap(&db.pages, &db.catalog, t);
    for (int i = 0; i < 10000; i++)
        heap.Insert({int32_t(i), std::string("user") + std::to_string(i)});

    TableIterator it = heap.Begin();
    Row row;
    RID rid;
    int expected = 0;
    while (it.Next(row, rid)) {
        EXPECT_EQ(std::get<int32_t>(row[0]), expected);
        EXPECT_EQ(std::get<std::string>(row[1]), "user" + std::to_string(expected));
        expected++;
    }
    EXPECT_EQ(expected, 10000);
    EXPECT_TRUE(db.pages.AllUnpinned());   // no pin leaks
}

TEST_F(TableHeapTest, DeleteEveryThirdRow) {
    Db db(kPath);
    TableInfo* t = db.catalog.CreateTable("users", UserColumns());
    TableHeap heap(&db.pages, &db.catalog, t);
    std::vector<RID> rids;
    for (int i = 0; i < 10000; i++)
        rids.push_back(heap.Insert({int32_t(i), std::string("u")}));
    for (int i = 0; i < 10000; i += 3) EXPECT_TRUE(heap.Delete(rids[i]));

    EXPECT_EQ(CountRows(heap), 6666);
    Row row;
    EXPECT_FALSE(heap.Get(rids[0], row));   // deleted
    EXPECT_TRUE(heap.Get(rids[1], row));    // still there
    EXPECT_TRUE(db.pages.AllUnpinned());
}

TEST_F(TableHeapTest, SurvivesRestart) {
    {
        Db db(kPath);
        TableInfo* t = db.catalog.CreateTable("users", UserColumns());
        TableHeap heap(&db.pages, &db.catalog, t);
        for (int i = 0; i < 1000; i++) heap.Insert({int32_t(i), std::string("u")});
    }   // everything closed here

    Db db(kPath);
    TableInfo* t = db.catalog.GetTable("users");
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->schema.columns.size(), 2u);
    TableHeap heap(&db.pages, &db.catalog, t);
    EXPECT_EQ(CountRows(heap), 1000);
    heap.Insert({int32_t(1000), std::string("after")});   // inserting after a restart works
    EXPECT_EQ(CountRows(heap), 1001);
}

TEST_F(TableHeapTest, TwoTablesDoNotInterfere) {
    Db db(kPath);
    TableInfo* users = db.catalog.CreateTable("users", UserColumns());
    TableInfo* orders = db.catalog.CreateTable("orders", std::vector<Column>{{"order_id", ColumnType::INT, 4}});
    TableHeap uh(&db.pages, &db.catalog, users);
    TableHeap oh(&db.pages, &db.catalog, orders);
    for (int i = 0; i < 100; i++) {
        uh.Insert({int32_t(i), std::string("u")});
        oh.Insert({int32_t(i * 10)});
    }
    EXPECT_EQ(CountRows(uh), 100);
    EXPECT_EQ(CountRows(oh), 100);
    TableIterator it = oh.Begin();
    Row row;
    RID rid;
    ASSERT_TRUE(it.Next(row, rid));
    EXPECT_EQ(row.size(), 1u);
}

TEST_F(TableHeapTest, CatalogRejectsBadRequests) {
    Db db(kPath);
    db.catalog.CreateTable("t0", UserColumns());
    EXPECT_THROW(db.catalog.CreateTable("t0", UserColumns()), std::runtime_error);   // duplicate name
    for (int i = 1; i < 8; i++) db.catalog.CreateTable("t" + std::to_string(i), UserColumns());
    EXPECT_THROW(db.catalog.CreateTable("t8", UserColumns()), std::runtime_error);   // more than 8 tables
}