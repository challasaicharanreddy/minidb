#include "catalog/catalog.h"
#include <cstring>
#include <stdexcept>
#include "storage/slotted_page.h"

namespace {

    constexpr uint32_t MAGIC = 0x4D494E49; // "MINI"
    constexpr int MAX_TABLES = 8;
    constexpr int MAX_COLUMNS = 8;
    constexpr int NAME_LEN = 32;
    
    struct DiskColumn {
        char name[NAME_LEN];
        int32_t type;
        uint16_t length;
    };
    
    struct DiskTable {
        char name[NAME_LEN];
        int32_t num_columns;
        DiskColumn columns[MAX_COLUMNS];
    
        page_id_t first_page_id;
        page_id_t last_page_id;
        page_id_t index_root_page_id;
    
        int32_t pk_column;
    };
    
    constexpr size_t TABLES_POS = 8;
    
    static_assert(
        TABLES_POS + MAX_TABLES * sizeof(DiskTable) <= PAGE_SIZE,
        "catalog must fit in one page"
    );
    
}

Catalog::Catalog(PageAccess* pages) : pages_(pages) {
    if(pages_->NumPages()==0){
        page_id_t id;
        pages_->NewPage(id);
        pages_->UnpinPage(id,true);
        Save();
    }
    else {
        Load();
    }
}

void Catalog::Save() {
    char* page = pages_->FetchPage(0);
    std::memset(page, 0, PAGE_SIZE);
    uint32_t magic = MAGIC;
    int32_t n = static_cast<int32_t>(tables_.size());
    std::memcpy(page, &magic, 4);
    std::memcpy(page + 4, &n, 4);
    size_t pos = TABLES_POS;
    for (const TableInfo& t : tables_) {
        DiskTable d;
        std::memset(&d, 0, sizeof d);
        std::strncpy(d.name, t.name.c_str(), NAME_LEN - 1);
        d.num_columns = static_cast<int32_t>(t.schema.columns.size());
        for (int i = 0; i < d.num_columns; i++) {
            const Column& c = t.schema.columns[i];
            std::strncpy(d.columns[i].name, c.name.c_str(), NAME_LEN - 1);
            d.columns[i].type = static_cast<int32_t>(c.type);
            d.columns[i].length = c.length;
        }
        d.first_page_id = t.first_page_id;
        d.last_page_id = t.last_page_id;
        d.index_root_page_id = t.index_root_page_id;
        d.pk_column = t.pk_column;
        std::memcpy(page + pos, &d, sizeof d);
        pos += sizeof d;
    }
    pages_->UnpinPage(0, true);
}

void Catalog::Load() {
    char* page = pages_->FetchPage(0);
    uint32_t magic;
    int32_t n;
    std::memcpy(&magic, page, 4);
    std::memcpy(&n, page + 4, 4);
    if (magic != MAGIC) {
        pages_->UnpinPage(0, false);
        throw std::runtime_error("not a MiniDB file");
    }
    size_t pos = TABLES_POS;
    for (int i = 0; i < n; i++) {
        DiskTable d;
        std::memcpy(&d, page + pos, sizeof d);
        pos += sizeof d;
        TableInfo t;
        t.name = d.name;
        for (int c = 0; c < d.num_columns; c++)
            t.schema.columns.push_back({d.columns[c].name, static_cast<ColumnType>(d.columns[c].type),
                                        d.columns[c].length});
        t.first_page_id = d.first_page_id;
        t.last_page_id = d.last_page_id;
        t.index_root_page_id = d.index_root_page_id;
        t.pk_column = d.pk_column;
        tables_.push_back(t);
    }
    pages_->UnpinPage(0, false);
}

TableInfo* Catalog::CreateTable(const std::string& name, const std::vector<Column>& columns, int pk_column) {
    if (name.empty() || name.size() >= NAME_LEN) throw std::runtime_error("invalid table name");
    if (GetTable(name)) throw std::runtime_error("table already exists: " + name);
    if (tables_.size() >= MAX_TABLES) throw std::runtime_error("too many tables (max 8)");
    if (columns.empty() || columns.size() > MAX_COLUMNS) throw std::runtime_error("a table needs 1 to 8 columns");
    for (const Column& c : columns)
        if (c.name.empty() || c.name.size() >= NAME_LEN) throw std::runtime_error("invalid column name");
    Schema schema{columns};
    if (schema.RecordSize() > 4000) throw std::runtime_error("row too large");
    if (pk_column >= 0 && (pk_column >= static_cast<int>(columns.size()) ||
                           columns[pk_column].type != ColumnType::INT))
        throw std::runtime_error("primary key must be an INT column");

    page_id_t first;
    SlottedPage heap_page(pages_->NewPage(first));   // first page of the table's heap
    heap_page.Init(first);
    pages_->UnpinPage(first, true);

    tables_.push_back({name, schema, first, first, INVALID_PAGE_ID, pk_column});
    Save();
    return &tables_.back();
}

TableInfo* Catalog::GetTable(const std::string& name) {
    for (TableInfo& t : tables_)
        if (t.name == name) return &t;
    return nullptr;
}

void Catalog::DropTable(const std::string& name) {
    for (auto it = tables_.begin(); it != tables_.end(); ++it) {
        if (it->name == name) {
            tables_.erase(it);
            Save();
            return;
        }
    }
    throw std::runtime_error("no such table: " + name);
}

