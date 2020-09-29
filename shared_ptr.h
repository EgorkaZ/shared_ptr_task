#pragma once

#include "counting_blocks.h"
#include "weak_ptr.h"

#include <new>
#include <utility>

template <class T>
struct weak_ptr;

template <class T>
struct shared_ptr
{
private:
    counting_block * m_block;
    T * m_ptr;

    template <class Y>
    friend struct weak_ptr;

    template <class Y>
    friend struct shared_ptr;

    template <class Y>
    constexpr shared_ptr(owning_block<Y> * block, Y * ptr)
        : m_block(block)
        , m_ptr(ptr)
    {
    }

public:
    constexpr shared_ptr() noexcept
        : m_block(nullptr)
        , m_ptr(nullptr)
    {
    }

    constexpr shared_ptr(std::nullptr_t) noexcept
        : shared_ptr()
    {
    }

    template <class Y, class Deleter = std::default_delete<Y>>
    explicit shared_ptr(Y * ptr, Deleter d = Deleter{})
    try : m_block((new pointing_block<Y, Deleter>{ptr, d})->add_shared())
        , m_ptr(ptr) {
    }
    catch (...) {
        d(ptr);
    }

    template <class Y>
    shared_ptr(const shared_ptr<Y> & master, T * slave) noexcept
        : m_block(impl::add_shared_or_null(master.m_block))
        , m_ptr(slave)
    {
    }

    shared_ptr(const shared_ptr & other)
        : m_block(impl::add_shared_or_null(other.m_block))
        , m_ptr(other.m_ptr)
    {
    }

    template <class Y>
    shared_ptr(const shared_ptr<Y> & other)
        : m_block(impl::add_shared_or_null(other.m_block))
        , m_ptr(other.m_ptr)
    {
    }

    template <class Y>
    shared_ptr(shared_ptr<Y> && other)
        : m_block(other.m_block)
        , m_ptr(other.m_ptr)
    {
        other.raw_clear();
    }

    shared_ptr(shared_ptr && other) noexcept
        : m_block(other.m_block)
        , m_ptr(other.m_ptr)
    {
        other.raw_clear();
    }

    template <class Y>
    shared_ptr(const weak_ptr<Y> & weak)
        : m_block(impl::add_shared_or_null(weak.m_block))
        , m_ptr(weak.m_ptr)
    {
    }

    shared_ptr(const weak_ptr<T> & weak)
        : m_block(impl::add_shared_or_null(weak.m_block))
        , m_ptr(weak.m_ptr)
    {
    }

    ~shared_ptr()
    {
        if (m_block) {
            m_block->delete_shared();
        }
    }

    shared_ptr & operator=(const shared_ptr & rhs)
    {
        if (&rhs != this) {
            if (m_block) {
                m_block->delete_shared();
            }
            m_block = impl::add_shared_or_null(rhs.m_block);
            m_ptr = rhs.m_ptr;
        }
        return *this;
    }

    shared_ptr & operator=(shared_ptr && other)
    {
        if (&other != this) {
            if (m_block) {
                m_block->delete_shared();
            }
            m_block = other.m_block;
            m_ptr = other.m_ptr;
            other.raw_clear();
        }
        return *this;
    }

    void reset() noexcept
    {
        if (m_block) {
            m_block->delete_shared();
        }
        raw_clear();
    }

    template <class Y, class Deleter = std::default_delete<Y>>
    void reset(Y * ptr, Deleter d = std::default_delete<Y>{})
    {
        if (m_block) {
            m_block->delete_shared();
        }
        try {
            m_block = (new pointing_block(ptr, d))->add_shared();
            m_ptr = ptr;
        }
        catch (...) {
            d(ptr);
            throw std::bad_alloc{};
        }
    }

    T * get() noexcept
    {
        return m_ptr;
    }

    T & operator*() const noexcept
    {
        return *m_ptr;
    }

    T * operator->() const noexcept
    {
        return m_ptr;
    }

    T & operator[](std::ptrdiff_t idx) const
    {
        return m_ptr[idx];
    }

    long use_count() const noexcept
    {
        return m_block ? m_block->counter : 0;
    }

    operator bool() const noexcept
    {
        return m_ptr;
    }

    template <class U>
    bool operator==(const shared_ptr<U> & rhs) const
    {
        return m_ptr == rhs.m_ptr;
    }

    template <class U>
    bool operator!=(const shared_ptr<U> & rhs) const
    {
        return m_ptr != rhs.m_ptr;
    }

    friend bool operator==(const shared_ptr & lhs, const shared_ptr & rhs)
    {
        return lhs.m_ptr == rhs.m_ptr;
    }

    friend bool operator!=(const shared_ptr & lhs, const shared_ptr & rhs)
    {
        return lhs.m_ptr != rhs.m_ptr;
    }

    template <class Y, class... Args>
    friend shared_ptr<Y> make_shared(Args &&... args);

private:
    void raw_clear()
    {
        m_block = nullptr;
        m_ptr = nullptr;
    }
};

template <class Y, class... Args>
inline shared_ptr<Y> make_shared(Args &&... args)
{
    auto block = new owning_block<Y>(std::forward<Args>(args)...);
    block->add_shared();
    return shared_ptr<Y>{block, block->get_ptr()};
}
