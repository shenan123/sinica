#pragma once
#include <cstddef>
class Areaallocator
{
private:
    size_t m_bytes;
    void* m_buffer;
    void* m_offset;

public:
    inline Areaallocator(size_t bytes)
    :m_bytes(bytes){
        m_buffer=malloc(m_bytes);
        m_offset=m_buffer;
    }
    template<typename T>
    inline T* alloc(){
        void* offset = m_offset;
        m_offset = static_cast<void*>(static_cast<char*>(m_offset)+sizeof(T));
        return static_cast<T*>(offset);
    }
    inline Areaallocator(const Areaallocator& other)=delete;
    inline Areaallocator operator=(const Areaallocator& other)=delete;

    ~Areaallocator(){
        free(m_buffer);
    }
};


/*
        constexpr size_t align = alignof(T);
        uintptr_t addr=reinterpret_cast<uintptr_t>(m_offset);
        uintptr_t aligned_addr=(addr+align-1)&~(align-1);
        m_offset=reinterpret_cast<void*>(aligned_addr);
        T* ptr =reinterpret_cast<T*>(m_offset);
        m_offset=static_cast<char*>(m_offset)+sizeof(T);
        new(ptr) T{};
        return ptr;

*/