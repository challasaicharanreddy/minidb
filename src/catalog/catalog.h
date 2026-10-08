#pragma once
#include <list>
#include <string>
#include <vector>
#include "common/types.h"
#include "storage/page_access.h"

struct TableInfo {
    std::string name;
    Schema schema;
    page_id_t first_page_id;
    page_id_t last_page_id;
    page_id_t index_root_page_id;   // -1 until an index exists (later days)
    int pk_column;                  // -1 if the table has no primary key
};

class Catalog {
public:
    explicit Catalog(PageAccess* pages);
    TableInfo* CreateTable(const std::string& name, const std::vector<Column>& columns, int pk_column = -1);
    TableInfo* GetTable(const std::string& name);    // nullptr if missing
    void DropTable(const std::string& name);
    void Save();                                     // write the catalog to page 0

private:
    void Load();
    PageAccess* pages_;
    std::list<TableInfo> tables_;   // a list, so pointers to entries stay valid
};