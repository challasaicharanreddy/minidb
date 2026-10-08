#include "storage/table_heap.h"
#include "storage/row_serializer.h"
#include "storage/slotted_page.h"

RID TableHeap::Insert(const Row& row) {
    std::string bytes = RowSerializer::Serialize(info_->schema, row);
    uint16_t len = static_cast<uint16_t>(bytes.size());

    SlottedPage last(pages_->FetchPage(info_->last_page_id));
    auto slot = last.InsertRecord(bytes.data(), len);
    if (slot) {
        pages_->UnpinPage(info_->last_page_id, true);
        return {info_->last_page_id, *slot};
    }

    // The last page is full: add a new page and link it into the chain.
    page_id_t new_id;
    SlottedPage fresh(pages_->NewPage(new_id));
    fresh.Init(new_id);
    last.SetNextPageId(new_id);
    pages_->UnpinPage(info_->last_page_id, true);
    slot = fresh.InsertRecord(bytes.data(), len);
    pages_->UnpinPage(new_id, true);
    info_->last_page_id = new_id;
    catalog_->Save();   // remember the new last page
    return {new_id, *slot};
}

bool TableHeap::Get(RID rid, Row& out) {
    SlottedPage page(pages_->FetchPage(rid.page_id));
    uint16_t len;
    const char* rec = page.GetRecord(rid.slot_id, len);
    if (rec) out = RowSerializer::Deserialize(info_->schema, rec);
    pages_->UnpinPage(rid.page_id, false);
    return rec != nullptr;
}

bool TableHeap::Delete(RID rid) {
    SlottedPage page(pages_->FetchPage(rid.page_id));
    bool ok = page.DeleteRecord(rid.slot_id);
    pages_->UnpinPage(rid.page_id, ok);
    return ok;
}

bool TableIterator::Next(Row& row, RID& rid) {
    while (page_id_ != INVALID_PAGE_ID) {
        SlottedPage page(pages_->FetchPage(page_id_));
        while (slot_ < page.NumSlots()) {
            uint16_t len;
            const char* rec = page.GetRecord(slot_, len);
            RID current{page_id_, slot_};
            slot_++;
            if (rec) {                                   // skip deleted slots
                row = RowSerializer::Deserialize(*schema_, rec);
                rid = current;
                pages_->UnpinPage(page_id_, false);
                return true;
            }
        }
        page_id_t next = page.NextPageId();              // end of this page: follow the chain
        pages_->UnpinPage(page_id_, false);
        page_id_ = next;
        slot_ = 0;
    }
    return false;
}