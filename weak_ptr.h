#pragma once

#include <new>
#include <utility>

#include "counting_blocks.h"

template <class T>
struct shared_ptr;

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
        : m_block(impl::add_weak_or_null(other.m_block))
        , m_ptr(other.m_ptr)
    {}

    template <class Y>
    weak_ptr(const weak_ptr<Y> & other) noexcept
        : m_block(impl::add_weak_or_null(other.m_block))
        , m_ptr(other.m_ptr)
    {}

    template <class Y>
    weak_ptr(const shared_ptr<Y> & other) noexcept
        : m_block(impl::add_weak_or_null(other.m_block))
        , m_ptr(other.m_ptr)
    {}

    weak_ptr(weak_ptr && other) noexcept
        : m_block(impl::add_weak_or_null(other.m_block))
        , m_ptr(other.m_ptr)
    { other.raw_clear(); }

    template <class Y>
    weak_ptr(weak_ptr<Y> && other) noexcept
        : m_block(other.m_block)
        , m_ptr(other.m_ptr)
    { other.raw_clear(); }

    ~weak_ptr()
    {
        if (m_block && m_block->delete_weak()->should_delete_weak()) {
            delete m_block;
        }
    }

    weak_ptr & operator = (const weak_ptr & other) noexcept
    { return assign(other); }

    template <class Y>
    weak_ptr & operator = (const weak_ptr<Y> & other) noexcept
    { return assign(other); }

    template <class Y>
    weak_ptr & operator = (const shared_ptr<Y> & other) noexcept
    {
        if (m_block != other.m_block) {
            if (m_block && m_block->delete_weak()->should_delete_weak()) {
                delete m_block;
            }
            m_block = impl::add_weak_or_null(other.m_block);
        }
        m_ptr = other.m_ptr;
        return *this;
    }

    weak_ptr & operator = (weak_ptr && other) noexcept
    { return assign(std::move(other)); }

    template <class Y>
    weak_ptr & operator = (weak_ptr<Y> && other) noexcept
    { return assign(std::move(other)); }

    shared_ptr<T> lock() const noexcept
    {
        return expired() ? shared_ptr<T>() : shared_ptr<T>(*this);
    }

    bool expired() const noexcept
    {
        return !m_block || !m_block->counter;
    }

private:
    template<class Y>
    weak_ptr & assign(const weak_ptr<Y> & other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        if (other.m_block != m_block) {
            if (m_block && m_block->delete_weak()->should_delete_weak()) {
                delete m_block;
            }
            m_block = impl::add_weak_or_null(other.m_block);
        }
        m_ptr = other.m_ptr;
        return * this;
    }

    template <class Y>
    weak_ptr & assign(weak_ptr<Y> && other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        if (other.m_block != m_block) {
            if (m_block && m_block->delete_weak()->should_delete_weak()) {
                delete m_block;
            }
            m_block = other.m_block;
        }
        m_ptr = other.m_ptr;
        other.raw_clear();
        return *this;
    }

    void raw_clear() noexcept
    {
        m_block = nullptr;
        m_ptr = nullptr;
    }

};