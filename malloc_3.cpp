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
            md_list_end = md;

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
    m_metadata* end = md_list_end;
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
    m_metadata * end = md_list_end;
    if (md_list_end->is_free){
        size_t block_size = md_list_end->size;
        if(!_enlarge_wilderness(size)){
            return nullptr;
        }
        num_free_blocks--;
        num_free_bytes-=block_size;
        end->is_free = false;
        return (void*)++end;
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
    m_metadata* md_block = (m_metadata*)block;
    md_block--; //reached metadata of block
    size_t full_Size = md_block->size;
    if (full_Size-occ_size < 128 + _size_meta_data()){
        return;
    }
    m_metadata* new_metadata = (m_metadata*) ((char*) block + occ_size);
    new_metadata->size = full_Size-occ_size-_size_meta_data();
    new_metadata->is_free = true;
    new_metadata->next = md_block->next;
    new_metadata->prev = md_block;
    if (!md_block->next)
        md_list_end = new_metadata;
    md_block->next = new_metadata;
    md_block->size = occ_size;
    num_free_bytes+=full_Size-occ_size-_size_meta_data();
    num_total_blocks++;
    num_free_blocks++;
    num_total_bytes-=_size_meta_data();

}
void *_mmap_allocation(size_t size) {
    void * space = mmap(nullptr, size + _size_meta_data(),PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS , -1, 0);
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
    if (size > 128*1024){
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
bool combine(m_metadata* first, m_metadata* second){
    if (first->is_free && second->is_free){

        num_free_bytes+= _size_meta_data();
        num_total_bytes+= _size_meta_data();
        num_total_blocks--;
        num_free_blocks--;
        first->size += _size_meta_data()+second->size;
        first->next = second->next;
        m_metadata *_next = second->next;
        m_metadata *_prev = second->prev;
        _prev->next = _next;
        if (_next != nullptr)
            _next->prev = _prev;
        return true;
    }
    return false;
}

void _remove_from_mmap_list(m_metadata* metadata){
    metadata->prev->next = metadata->next;
    if(metadata->next)
        metadata->next->prev = metadata->prev;
    else
        mmap_list_end = metadata->prev;

    num_total_bytes -= metadata->size;
    num_total_blocks--;
    munmap(metadata,metadata->size+_size_meta_data());
}
void sfree(void* p){
    if(!p){
        return;
    }
    m_metadata* listhead = md_list_head.next;
    m_metadata* metadata = (m_metadata*) p;
    metadata--; //reach the start point of the metadata of the block
    if (metadata->is_free){
        return;
    }
    size_t block_size = metadata->size;
    metadata->is_free = true;

    if (metadata->size >= 128*1024){

        _remove_from_mmap_list(metadata);
        return;
    }

    bool combined = combine(metadata->prev,metadata);
    if (combined) {
        metadata = metadata->prev;
        if (!metadata->next)
            md_list_end = metadata;
    }
    if (metadata->next){
        combined = combine(metadata,metadata->next);
        if (combined && !metadata->next)
            md_list_end = metadata;
    }
    num_free_blocks++;
    num_free_bytes += block_size;
}

void *srealloc(void *oldp, size_t size) {
    if (size == 0 || size > 1e8)
        return nullptr;
    m_metadata *metadata = (m_metadata *) oldp;
    metadata--; //reach the start point of the metadata of the block

    if (size >= 128 * 1024) { //mmap realloc
        if (!oldp) {
            void *allocated_mmap = _mmap_allocation(size);
            return allocated_mmap;
        }
        size_t size_diff = metadata->size - size;
        metadata->size = size;
        if (metadata->size >= size) {
            munmap(metadata + _size_meta_data() + size, size_diff);
            num_total_bytes -= size_diff;
            return oldp;
        }
        //allocate new
        void *new_mmap = _mmap_allocation(size);
        //copy
        memcpy(new_mmap, oldp, metadata->size);
        //erase old
        _remove_from_mmap_list(metadata);

        return new_mmap;
    } else { //regular realloc
        if (!oldp) {
            return smalloc(size);
        }
        //check if size is enough
        // if yes - split if needed (and combine?)
        // if no
        // check if enoguh size + available with prev neighbour -> next neighbor -> both neighbors  - if yes, use and split
        // find if there's a different block and use it (split?) or allocate - malloc func does this
        if (metadata->size >= size) {
            size_t size_diff = metadata->size - size;
            if (size_diff >= 128 + _size_meta_data()) { //split
                _split_reused_blocks(oldp, size);
            }
            return oldp;
        }
// #TODO check if needed, currently for test
        if (!metadata->next) {
            if (_enlarge_wilderness(size)) {
                return oldp;
            }
        }
        bool combined = false;
        metadata->is_free = true;
        if (metadata->prev->size && metadata->prev->is_free && metadata->prev->size + metadata->size + _size_meta_data() >= size) { //comb with prev
            size_t e1 = metadata->prev->size + metadata->size + _size_meta_data();
            size_t e2 = metadata->next->size + metadata->size + _size_meta_data();
            bool test = metadata->next && metadata->next->size + metadata->size + _size_meta_data() >= size;
            combined = combine(metadata->prev, metadata);
            if (combined) {
                if (!metadata->next)
                    md_list_end = metadata->prev;
                memcpy(metadata->prev + _size_meta_data(), metadata + _size_meta_data(), metadata->size);
                metadata = metadata->prev;
                metadata->is_free = false;
                m_metadata *to_split = metadata;
                to_split++;
                _split_reused_blocks((void *) to_split, size);
            }
        } else if (metadata->next && metadata->next->is_free && metadata->next->size + metadata->size + _size_meta_data() >= size) { //comb with next
            combined = combine(metadata, metadata->next);
            if (combined) {
                metadata->is_free = false;
                if (!metadata->next)
                    md_list_end = metadata;
                m_metadata *to_split = metadata;
                to_split++;
                _split_reused_blocks((void *) to_split, size);
            }
        } else if (metadata->prev->size && metadata->next &&
                   metadata->prev->size + metadata->size + metadata->next->size + 2 * _size_meta_data() >= size) { // comb both
            if ((metadata->prev->is_free) && (metadata->next->is_free)) {
                combined = combine(metadata->prev, metadata);
                memcpy(metadata->prev + _size_meta_data(), metadata + _size_meta_data(), metadata->size);
                metadata = metadata->prev;
                combine(metadata, metadata->next);
                if (!metadata->next)
                    md_list_end = metadata;
                num_free_bytes -= _size_meta_data();
                metadata->is_free = false;
                m_metadata *to_split = metadata;
                to_split++;
                _split_reused_blocks((void *) to_split, size);
            }
        }
        if (combined) {
            num_free_bytes -= _size_meta_data();
            return ++metadata;
        } else {
            metadata->is_free = false;
            void *new_block = smalloc(size);
            if (!new_block)
                return nullptr;
            memcpy(new_block, oldp, metadata->size);

            sfree(oldp);
            return new_block;

        }
    }
}