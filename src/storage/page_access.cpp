#include "storage/page_access.h"
#include <stdexcept>

char* PageAccess::FetchPage(page_id_t id) {
    auto it=frames_.find(id);
    if(it!=frames_.end()){
        it->second.pins++;
        return it->second.data.get();
    }
    Frame f;
    f.data=std::make_unique<char[]>(PAGE_SIZE);
    dm_->ReadPage(id,f.data.get());
    f.pins=1;
    char* ptr=f.data.get();
    frames_[id]=std::move(f);
    return ptr;
}

char* PageAccess::NewPage(page_id_t& id) {
    id=dm_->AllocatePage();
    return FetchPage(id);
}

void PageAccess::UnpinPage(page_id_t id, bool dirty) {
    auto it=frames_.find(id);
    if(it==frames_.end())throw std::runtime_error("cannot unpin of a page that is not pinned");
    it->second.pins--;
    it->second.dirty=dirty||it->second.dirty;
    if(it->second.pins==0){
        if(it->second.dirty){
            dm_->WritePage(id, it->second.data.get());
        }
        frames_.erase(id);
    }
}



