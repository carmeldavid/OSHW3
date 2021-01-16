//
// Created by student on 1/16/21.
//
#include<cstring>
#include <unistd.h>

typedef struct malloc_metadata {
    size_t size;
    bool is_free;
    malloc_metadata *next;
    malloc_metadata *prev;
} m_metadata;

m_metadata md_list_head = {0,false,nullptr, nullptr}; // dummy
m_metadata* md_list_end = nullptr;
size_t num_free_blocks = 0;
size_t num_free_bytes = 0;
size_t num_total_blocks = 0;
size_t num_total_bytes = 0;

void _insert_to_list_end(m_metadata* md){
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
            return (void*) cur_md;
        }
        cur_md = cur_md->next;
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

void* smalloc(size_t size){
    if (size == 0 || size > 1e8)
        return nullptr;
    void* found_block = _find_free_block(size);
    if (found_block){
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
    _insert_to_list_end(new_metadata);
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
    num_free_blocks++;
    num_free_bytes += metadata->size;
}

void* srealloc(void* oldp, size_t size){
    if (size == 0 || size > 1e8)
        return nullptr;
    void* block;
    if (!oldp){
        block = smalloc(size);
    }
    else{
        m_metadata* metadata = (m_metadata*) oldp;
        metadata--; //reach the start point of the metadata of the block
        if (metadata->size >= size){
            return oldp;
        }
        block = smalloc(size);
        if(!block){
            return nullptr;
        }
        memcpy(block,oldp,metadata->size);
        sfree(oldp);
    }
    return block;
}








