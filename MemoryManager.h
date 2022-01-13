#pragma once

#ifndef MEMORYMANAGER
#define MEMORYMANAGER

#include <memory>
#include <shared_mutex>
#include <iostream>

#define ME_MEMMAX (100 * 1024)
#define ME_MEMERROR(condition, msg)\
if(!(condition)){\
std::cout << msg << std::endl;\
throw std::bad_alloc();\
}
#define ME_MEMINIT() ME::InitAllocator()
#define ME_MEMCLEAR() ME::DeInitAllocator()

namespace ME
{
    typedef unsigned long long size_t;

    void InitAllocator();
    void DeInitAllocator();

    class UpstreamMemory
    {
    public:
        virtual void* allocate(const size_t& size, const char* msg = nullptr) = 0;
        virtual void* reallocate(void* end_ptr, const size_t& size, const char* msg = nullptr) = 0;
        virtual void deallocate(void* ptr, const size_t& size, const char* msg = nullptr) = 0;
    };
    class malloc_stdfree_UpstreamMemory : public UpstreamMemory
    {
    public:
        virtual void* allocate(const size_t& size, const char* msg = nullptr) override;
        virtual void* reallocate(void* end_ptr, const size_t& size, const char* msg = nullptr) override;
        virtual void deallocate(void* ptr, const size_t& size, const char* msg = nullptr) override;

        static malloc_stdfree_UpstreamMemory* stref;
    };
    class alloc_dealloc_UpstreamMemory : public UpstreamMemory
    {
    public:
        virtual void* allocate(const size_t& size, const char* msg = nullptr) override;
        virtual void* reallocate(void* end_ptr, const size_t& size, const char* msg = nullptr) override;
        virtual void deallocate(void* ptr, const size_t& size, const char* msg = nullptr) override;

        static alloc_dealloc_UpstreamMemory* stref;
    };
    class null_UpstreamMemory : public UpstreamMemory
    {
    public:
        virtual void* allocate(const size_t&, const char* = nullptr) override { ME_MEMERROR(true, "Triggered null_Upstream"); return nullptr; }
        virtual void* reallocate(void* end_ptr, const size_t& size, const char* msg = nullptr) override { return allocate(0); }
        virtual void deallocate(void*, const size_t&, const char* = nullptr) override { allocate(0); }

        static null_UpstreamMemory* stref;
    };

    static UpstreamMemory* set_malloc_stdfree_UpstreamMemory() { return malloc_stdfree_UpstreamMemory::stref; }
    static UpstreamMemory* set_alloc_dealloc_UpstreamMemory() { return alloc_dealloc_UpstreamMemory::stref; }
    static UpstreamMemory* set_null_UpstreamMemory() { return null_UpstreamMemory::stref; }

    class MemoryManager
    {
    public:
        UpstreamMemory* m_UpstreamMemory;

        MemoryManager(UpstreamMemory* upstreammemory = set_alloc_dealloc_UpstreamMemory());
        ~MemoryManager() = default;

        [[nodiscard]] virtual void* allocate(const size_t& size) = 0;
        [[nodiscard]] virtual void* verified_allocate(void* end_ptr, const size_t& size) = 0;
        virtual void deallocate(void* ptr, const size_t& size) noexcept = 0;
        virtual void release() = 0;
        virtual size_t getFreeMemory() const = 0;
        virtual size_t getMaxMemory() const = 0;

        // The global Allocator
        static MemoryManager* Allocator;
    };

    // Faster global Allocators
    // Params Size: Number of variable to be allocated
    template<typename T>
    constexpr static T* alloc(const size_t& size)
    {

        ME_MEMERROR(MemoryManager::Allocator != nullptr, "Allocator not initialized!!");

        if (size)
            return (T*)MemoryManager::Allocator->allocate(sizeof(T) * size);

        return nullptr;
    }
    template<typename T, typename ...Args>
    constexpr static T* allocon(const Args&& ...args)
    {

        ME_MEMERROR(MemoryManager::Allocator != nullptr, "Allocator not initialized!!");

        T* ptr = (T*)MemoryManager::Allocator->allocate(sizeof(T));
        new (ptr) T(args...);
        return ptr;
    }
    template<typename T>
    constexpr static void dealloc(T* ptr, const size_t& size)
    {

        ME_MEMERROR(MemoryManager::Allocator != nullptr, "Allocator not initialized!!");

        MemoryManager::Allocator->deallocate((void*)ptr, size);
    }
    template<typename T>
    constexpr static T* realloc(T* end_ptr, const size_t& size)
    {

        ME_MEMERROR(MemoryManager::Allocator != nullptr, "Allocator not initialized!!");

        return (T*)MemoryManager::Allocator->verified_allocate(end_ptr, sizeof(T) * size);
    }
    static size_t Maxmem() noexcept { return MemoryManager::Allocator->getMaxMemory(); }
    static size_t LeftMem() noexcept { return MemoryManager::Allocator->getFreeMemory(); }
}

#endif // !MEMORYMANAGER