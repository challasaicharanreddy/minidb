#pragma once
#include "catalog/catalog.h"
#include "storage/page_access.h"

// Walks every live row of a table: follows the page chain and skips deleted slots.
class TableIterator {
public:
    TableIterator(PageAccess* pages, const Schema* schema, page_id_t first)
        : pages_(pages), schema_(schema), page_id_(first) {}
    bool Next(Row& row, RID& rid);   // false when there are no more rows

private:
    PageAccess* pages_;
    const Schema* schema_;
    page_id_t page_id_;
    uint16_t slot_ = 0;
};

class TableHeap {
public:
    TableHeap(PageAccess* pages, Catalog* catalog, TableInfo* info)
        : pages_(pages), catalog_(catalog), info_(info) {}
    RID Insert(const Row& row);
    bool Get(RID rid, Row& out);
    bool Delete(RID rid);
    TableIterator Begin() { return TableIterator(pages_, &info_->schema, info_->first_page_id); }

private:
    PageAccess* pages_;
    Catalog* catalog_;
    TableInfo* info_;
};