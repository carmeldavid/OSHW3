//
// Created by student on 1/16/21.
//
#include <unistd.h>

void* smalloc(size_t size) {
    if (size == 0 || size > 1e8)
        return nullptr;
    void* ret = sbrk(size);
    if (ret == (void*)-1){
        return nullptr;
    }
    return ret;

}
