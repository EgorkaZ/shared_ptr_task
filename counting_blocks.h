#pragma once

#include <cstddef>

struct counting_block
{
    size_t counter = 1;
    size_t weak_counter = 1;

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

    counting_block * delete_shared() noexcept
    {
        --counter;
        --weak_counter;
        if (counter == 0) {
            delete_object();
        }
        return this;
    }

    counting_block * delete_weak() noexcept
    {
        --weak_counter;
        return this;
    }

    bool should_delete_block() noexcept
    {
        return weak_counter == 0;
    }

    virtual void delete_object() = 0;

    virtual ~counting_block() = default;
};

template <class T>
struct pointing_block : counting_block
{
    T * object;

    pointing_block(T * object) noexcept
        : object(object)
    {
    }

    virtual void delete_object() override
    {
        delete object;
    }
};

template <class T, class Deleter>
struct pointing_deleting_block final : pointing_block<T>
{
    Deleter deleter;

    pointing_deleting_block(T * object, Deleter deleter) noexcept
        : pointing_block<T>(object)
        , deleter(std::move(deleter))
    {
    }

    void delete_object() final
    {
        deleter(pointing_block<T>::object);
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

    T * get_ptr() noexcept
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
