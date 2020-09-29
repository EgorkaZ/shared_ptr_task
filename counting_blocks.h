#pragma once

#include <cstddef>
#include <memory>

struct counting_block
{
    size_t counter = 0;
    size_t weak_counter = 0;

    counting_block * add_shared() noexcept
    {
        ++counter;
        ++weak_counter;
        return this;
    }

    counting_block * add_weak() noexcept
    {
        ++weak_counter;
        return this;
    }

    void delete_shared()
    {
        --counter;
        --weak_counter;
        if (!counter) {
            delete_object();
        }
    }

    counting_block * delete_weak()
    {
        --weak_counter;
        return this;
    }
    bool should_delete_weak()
    {
        return !weak_counter;
    }

    virtual void delete_object() = 0;

    virtual ~counting_block() = default;
};

template <class T, class Deleter>
struct pointing_block final : counting_block
{
    T * object;
    Deleter deleter;

    pointing_block(T * object, Deleter deleter)
        : object(object)
        , deleter(std::move(deleter))
    {
    }

    void delete_object() final
    {
        deleter(object);
    }
};

template <class T>
struct owning_block final : counting_block
{
    std::aligned_storage_t<sizeof(T), alignof(T)> data;

    template <class... Args>
    explicit owning_block(Args &&... args)
    {
        new (&data) T(std::forward<Args>(args)...);
    }

    T * get_ptr()
    {
        return reinterpret_cast<T *>(&data);
    }

    void delete_object() final
    {
        reinterpret_cast<T &>(data).~T();
    }
};

namespace impl {
counting_block * add_shared_or_null(counting_block * block) noexcept
{
    return block ? block->add_shared() : block;
}

counting_block * add_weak_or_null(counting_block * block) noexcept
{
    return block ? block->add_weak() : block;
}
} // namespace impl
