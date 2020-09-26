#pragma once

#include <cstddef>
#include <iostream>
#include <utility>

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

    bool should_delete_weak() // returns if should be deleted
    { return !(--weak_counter); }

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
    {}

    void delete_object() final
    {
        deleter(object);
    }
};

template <class T>
struct owning_block final : counting_block
{
    std::aligned_storage_t<sizeof(T), alignof(T)> data;

    template <class ... Args>
    explicit owning_block(Args && ... args)
    {
        new (&data) T(std::forward<Args>(args)...);
    }

    T * get_ptr() {
        return reinterpret_cast<T *>(&data);
    }

    void delete_object() final
    {
        reinterpret_cast<T &>(data).~T();
    }
};

namespace {
    counting_block * add_shared_or_null(counting_block * block) noexcept
    {
        return block ? block->add_shared() : block;
    }

    counting_block * add_weak_or_null(counting_block * block) noexcept
    {
        return block ? block->add_weak() : block;
    }
}

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
    {}

public:
    constexpr shared_ptr() noexcept
        : m_block(nullptr)
        , m_ptr(nullptr)
    {}

    constexpr shared_ptr(std::nullptr_t) noexcept
        : shared_ptr()
    {}

    template <class Y, class Deleter = std::default_delete<Y>>
    explicit shared_ptr(Y * ptr, Deleter d = Deleter{})
        : m_block(nullptr)
        , m_ptr(ptr)
    {
        try {
            m_block = new pointing_block<Y, Deleter>{ptr, d};
            m_block->add_shared();
        } catch (...) {
            d(ptr);
        }
    }

    template <class Y>
    shared_ptr(const shared_ptr<Y> & master, T * slave) noexcept
        : m_block(add_shared_or_null(master.m_block))
        , m_ptr(slave)
    {}

    shared_ptr(const shared_ptr & other)
        : m_block(add_shared_or_null(other.m_block))
        , m_ptr(other.m_ptr)
    {}

    shared_ptr & operator = (const shared_ptr & rhs)
    {
        if (&rhs != this) {
            if (m_block) {
                m_block->delete_shared();
            }
            m_block = add_shared_or_null(rhs.m_block);
            m_ptr = rhs.m_ptr;
        }
        return *this;
    }

    template <class Y>
    shared_ptr(const shared_ptr<Y> & other)
        : m_block(add_shared_or_null(other.m_block))
        , m_ptr(other.m_ptr)
    {}

    template <class Y>
    shared_ptr(shared_ptr<Y> && other)
        : m_block(other.m_block)
        , m_ptr(other.m_ptr)
    {
        other.m_block = nullptr;
        other.m_ptr = nullptr;
    }

    shared_ptr(shared_ptr && other) noexcept
        : m_block(other.m_block)
        , m_ptr(other.m_ptr)
    {
        other.m_block = nullptr;
        other.m_ptr = nullptr;
    }
    shared_ptr & operator = (shared_ptr && other)
    {
        if (&other != this) {
            if (m_block) {
                m_block->delete_shared();
            }
            m_block = other.m_block;
            m_ptr = other.m_ptr;
            other.m_block = nullptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    template <class Y>
    shared_ptr(const weak_ptr<Y> & weak)
        : m_block(add_shared_or_null(weak.m_block))
        , m_ptr(weak.m_ptr)
    {}

    shared_ptr(const weak_ptr<T> & weak)
        : m_block(add_shared_or_null(weak.m_block))
        , m_ptr(weak.m_ptr)
    {}

    ~shared_ptr()
    {
        if (m_block) {
            m_block->delete_shared();
        }
    }


    void reset() noexcept
    {
        if (m_block) {
            m_block->delete_shared();
        }
        m_block = nullptr;
        m_ptr = nullptr;
    }

    template <class Y, class Deleter = std::default_delete<Y>>
    void reset(Y * ptr, Deleter d = std::default_delete<Y>{})
    {
        if (m_block) {
            m_block->delete_shared();
        }
        try {
            m_block = new pointing_block(ptr, d);
            m_block->add_shared();
            m_ptr = ptr;
        } catch (...) {
            d(ptr);
        }
    }

    T * get() noexcept
    { return m_ptr; }

    T & operator * () const noexcept
    { return *m_ptr; }

    T * operator -> () const noexcept
    { return m_ptr; }

    T & operator[] (std::ptrdiff_t idx) const
    { return m_ptr[idx]; }

    long use_count() const noexcept
    { return m_block ? m_block->counter : 0; }

    operator bool () const noexcept
    { return m_ptr; }


    template <class U>
    bool operator == (const shared_ptr<U> & rhs) const
    {
        return m_ptr == rhs.m_ptr;
    }

    template <class U>
    bool operator != (const shared_ptr<U> & rhs) const
    {
        return m_ptr != rhs.m_ptr;
    }

    friend bool operator == (const shared_ptr & lhs, const shared_ptr & rhs)
    {
        return lhs.m_ptr == rhs.m_ptr;
    }

    friend bool operator != (const shared_ptr & lhs, const shared_ptr & rhs)
    {
        return lhs.m_ptr != rhs.m_ptr;
    }


    template <class Y, class ... Args>
    friend shared_ptr<Y> make_shared(Args && ... args);
};

template <class Y, class ... Args>
inline shared_ptr<Y> make_shared(Args && ... args)
{
    auto block = new owning_block<Y>(std::forward<Args>(args)...);
    block->add_shared();
    return shared_ptr<Y>{block, block->get_ptr()};
}

template <class T>
struct weak_ptr
{
private:
    counting_block * m_block;
    T * m_ptr;

    template <class Y>
    friend struct shared_ptr;

    template <class Y>
    friend struct weak_ptr;
public:

    constexpr weak_ptr() noexcept
        : m_block(nullptr)
        , m_ptr(nullptr)
    {}

    weak_ptr(const weak_ptr & other) noexcept
        : m_block(add_weak_or_null(other.m_block))
        , m_ptr(other.m_ptr)
    {}

    template <class Y>
    weak_ptr(const weak_ptr<Y> & other) noexcept
        : m_block(add_weak_or_null(other.m_block))
        , m_ptr(other.m_ptr)
    {}

    template <class Y>
    weak_ptr(const shared_ptr<Y> & other) noexcept
        : m_block(add_weak_or_null(other.m_block))
        , m_ptr(other.m_ptr)
    {}

    weak_ptr(weak_ptr && other) noexcept
        : m_block(add_weak_or_null(other.m_block))
        , m_ptr(other.m_ptr)
    {
        other.m_block = nullptr;
        other.m_ptr = nullptr;
    }

    template <class Y>
    weak_ptr(weak_ptr<Y> && other) noexcept
        : m_block(other.m_block)
        , m_ptr(other.m_ptr)
    {
        other.m_block = nullptr;
        other.m_ptr = nullptr;
    }

    ~weak_ptr()
    {
        if (m_block && m_block->should_delete_weak()) {
            delete m_block;
        }
    }

    weak_ptr & operator = (const weak_ptr & other) noexcept
    {
        if (&other != this) {
            if (m_block && m_block->should_delete_weak()) {
                delete m_block;
            }
            m_block = add_weak_or_null(other.m_block);
            m_ptr = other.m_ptr;
        }
        return *this;
    }

    template <class Y>
    weak_ptr & operator = (const weak_ptr<Y> & other) noexcept
    {
        if (&other != this) {
            if (m_block && m_block->should_delete_weak()) {
                delete m_block;
            }
            m_block = add_weak_or_null(other.m_block);
            m_ptr = other.m_ptr;
        }
        return *this;
    }

    template <class Y>
    weak_ptr & operator = (const shared_ptr<Y> & other) noexcept
    {
        m_block = add_weak_or_null(other.m_block);
        m_ptr = other.m_ptr;
        return *this;
    }

    weak_ptr & operator = (weak_ptr && other) noexcept
    {
        if (&other != this) {
            if (m_block && m_block->should_delete_weak()) {
                delete m_block;
            }
            m_block = other.m_block;
            m_ptr = other.m_ptr;
            other.m_block = nullptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    template <class Y>
    weak_ptr & operator = (weak_ptr<Y> && other) noexcept
    {
        m_block = add_weak_or_null(other.m_block);
        m_ptr = other.m_ptr;
        return *this;
    }


    shared_ptr<T> lock() const noexcept
    {
        return expired() ? shared_ptr<T>() : shared_ptr<T>(*this);
    }

    bool expired() const noexcept
    {
        return !m_block || !m_block->counter;
    }


};