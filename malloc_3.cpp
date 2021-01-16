//
// Created by student on 1/16/21.
//
#include<cstring>
#include <unistd.h>
#include <sys/mman.h>
typedef struct malloc_metadata {
    size_t size;
    bool is_free;
    malloc_metadata *next;
    malloc_metadata *prev;
} m_metadata;

m_metadata md_list_head = {0,false,nullptr, nullptr}; // dummy
m_metadata* md_list_end = nullptr;

m_metadata mmap_list_head = {0,false,nullptr, nullptr}; // dummy
m_metadata* mmap_list_end = nullptr;

size_t num_free_blocks = 0;
size_t num_free_bytes = 0;
size_t num_total_blocks = 0;
size_t num_total_bytes = 0;

void _insert_to_list_end(m_metadata* md, bool is_mmap){
    if (!is_mmap){
        m_metadata * tmp = md_list_end;
        if(!tmp){ //list is empty
            md_list_head.next = md;
            md_list_end = md;
            md->prev = &md_list_head;
        }
        else{
            md_list_end->next = md;
            md->prev = tmp;
        }
    }
    else{
        m_metadata * tmp = mmap_list_end;
        if(!tmp){ //list is empty
            mmap_list_head.next = md;
            mmap_list_end = md;
            md->prev = &mmap_list_head;
        }
        else{
            mmap_list_end->next = md;
            md->prev = tmp;
        }
    }

}

void* _enlarge_wilderness(size_t size){
    size_t size_to_expand = size-md_list_end->size;
    void* expansion = sbrk(size_to_expand);
    if (expansion == (void*)-1){
        return nullptr;
    }
    md_list_end->size += size_to_expand;
    num_total_bytes+=size_to_expand;
    return md_list_end;
}

void* _find_free_block(size_t size){
    if (!md_list_head.next){
        return nullptr;
    }
    m_metadata * cur_md = md_list_head.next;
    while (cur_md){
        if(cur_md->is_free && cur_md->size >= size){
            cur_md->is_free = false;
            num_free_blocks--;
            num_free_bytes-=cur_md->size;
            return (void*) ++cur_md;
        }
        cur_md = cur_md->next;
    }
    if (md_list_end->is_free){
        if(!_enlarge_wilderness(size)){
            return nullptr;
        }
        num_free_blocks--;
        num_free_bytes-=md_list_end->size;
    }
    return nullptr;
}

size_t _size_meta_data(){
    return sizeof(m_metadata);
}

size_t _num_free_blocks(){
    return num_free_blocks;
}

size_t _num_free_bytes(){
    return num_free_bytes;
}

size_t _num_allocated_blocks(){
    return num_total_blocks;
}

size_t _num_allocated_bytes(){
    return num_total_bytes;
}

size_t _num_meta_data_bytes(){
    return _size_meta_data()*num_total_blocks;
}
void _split_reused_blocks(void* block,size_t occ_size){
    m_metadata * md_block = (m_metadata*)block;
    md_block--; //reached metadata of block
    size_t full_Size = _size_meta_data()+md_block->size;
    if (full_Size-occ_size-_size_meta_data() < 128){
        return;
    }
    m_metadata* new_metadata = (m_metadata*) ((char*) block + occ_size);
    new_metadata->size = full_Size-occ_size-2*_size_meta_data();
    new_metadata->is_free = true;
    new_metadata->next = md_block->next;
    new_metadata->prev = md_block;
    md_block->next = new_metadata;

    num_free_bytes+=full_Size-occ_size-2*_size_meta_data();
    num_free_blocks++;
}
void *_mmap_allocation(size_t size) {
    void *space = mmap(nullptr, size + _size_meta_data(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    if (space == (void*)-1){
        return nullptr;
    }
    m_metadata* mmap_md = (m_metadata*) space;
    mmap_md->size = size;
    mmap_md->is_free = false;
    mmap_md->next = nullptr;
    _insert_to_list_end(mmap_md ,true);
    num_total_bytes+=size;
    num_total_blocks++;
    mmap_md++;
    return (void*) mmap_md;
}
void* smalloc(size_t size){
    if (size == 0 || size > 1e8)
        return nullptr;
    if (size > 128000){
        return _mmap_allocation(size);
    }
    void* found_block = _find_free_block(size);
    if (found_block){
        _split_reused_blocks(found_block,size);
        return found_block;
    }
    void* new_block = sbrk(size+_size_meta_data());
    if (new_block == (void*)-1){
        return nullptr;
    }
    m_metadata* new_metadata = (m_metadata*) new_block;
    new_metadata->size = size;
    new_metadata->is_free = false;
    new_metadata->next = nullptr;
    _insert_to_list_end(new_metadata, false);
    num_total_blocks++;
    num_total_bytes += size;
    new_metadata++;
    return (void*) new_metadata;
}

void* scalloc(size_t num, size_t size){
    void* block = smalloc(num*size);
    if(!block)
        return nullptr;
    return memset(block,0,num*size);
}
void combine(m_metadata* first, m_metadata* second){
    if (first->is_free && second->is_free){
        first->size += _size_meta_data()+second->size;
        first->next = second->next;
        num_free_bytes+= _size_meta_data();
        num_total_blocks--;
        num_free_blocks--;
    }
}
void sfree(void* p){
    if(!p){
        return;
    }
    m_metadata* metadata = (m_metadata*) p;
    metadata--; //reach the start point of the metadata of the block
    if (metadata->is_free){
        return;
    }

    metadata->is_free = true;

    if (metadata->size > 128000){
        munmap(metadata,metadata->size+_size_meta_data());
    }
    combine(metadata->prev,metadata);
    if (metadata->next){
        combine(metadata,metadata->next);
    }
    num_free_blocks++;
    num_free_bytes += metadata->size;

}

void* srealloc(void* oldp, size_t size){
    if (size == 0 || size > 1e8)
        return nullptr;
    void* block = nullptr;
    if (!oldp){
        block = smalloc(size);
    }
    else{
        bool malloced = false;
        m_metadata* metadata = (m_metadata*) oldp;
        metadata--; //reach the start point of the metadata of the block
        if (metadata->size >= size){
            return oldp;
        }
        if (metadata->prev->size + metadata->size +_size_meta_data() >= size){
            combine(metadata->prev,metadata);
        }
        else if(metadata->next && (metadata->next->size + metadata->size +_size_meta_data() >= size)){
            combine(metadata,metadata->next);
        }
        else if (metadata->next && (metadata->next->size + metadata->prev->size + metadata->size +2*_size_meta_data() >= size)){
            combine(metadata->prev,metadata);
            combine(metadata,metadata->next);
        }
        else{
            if ((m_metadata*)oldp == md_list_end){
                block = _enlarge_wilderness(size-((m_metadata*)oldp)->size);
            }
            if(!block){
                block = smalloc(size);
                malloced = true;
                if(!block){
                    return nullptr;
                }
            }
        }
        memcpy(block,oldp,metadata->size);
        if (malloced){
            sfree(oldp);
        }
    }
    return block;
}








