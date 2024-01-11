/**
 * @file ThreadsafeSTLAdapter.h
 *
 * @brief ThreadsafeSTLAdapter class for creating thread-safe variants of STL adapters
 *
 * @author Hovsep Papoyan
 * Contact: papoyanhovsep93@gmail.com
 * @Date 2024-01-10
 *
 */

#ifndef THREADSAFE_ADAPTER_H
#define THREADSAFE_ADAPTER_H

#include <algorithm>
#include <condition_variable>
#include <mutex>

namespace concurrency
{

struct EmptyAdapter : std::exception
{
    const char* what() const noexcept override { return "Exception: The adapter is empty"; }
};

template<typename Adapter>
constexpr auto detectTopMethodImpl(const Adapter* const p) noexcept -> decltype(p->top(), void(), true) { return true; }

constexpr bool detectTopMethodImpl(const void* const p) noexcept { return false; }

template<typename Adapter>
constexpr bool detectTopMethod() noexcept { return detectTopMethodImpl(static_cast<const Adapter* const>(nullptr)); }

template<typename Adapter>
auto& getCurrent(const Adapter& adapter)
{
    if constexpr (detectTopMethod<Adapter>())
    {
        return adapter.top();
    }
    else
    {
        return adapter.front();
    }
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
class ThreadsafeSTLAdapter;

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
void swap(ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>& lhs,
          ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>& rhs)
{
    if (&lhs != &rhs)
    {
        std::scoped_lock lock(lhs.m_mutex, rhs.m_mutex);
        std::swap(lhs.m_adapter, rhs.m_adapter);
    }
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
class ThreadsafeSTLAdapter
{
private:
    Adapt<AdaptElem, Cont<ContElem, Alloc<AllocElem>>, Ts...> m_adapter;
    std::mutex m_mutex;
    std::condition_variable m_condVar;

    explicit ThreadsafeSTLAdapter(Adapt<AdaptElem, Cont<ContElem, Alloc<AllocElem>>, Ts...>&& adapter);

    template<template<typename...> typename _Adapt,
             typename _AdaptElem, template<typename...> typename _Cont,
                      typename _ContElem, template<typename> typename _Alloc,
                                 typename _AllocElem, typename... _Ts>
    friend auto createThreadsafeSTLAdapterFrom(const _Adapt<_AdaptElem, _Cont<_ContElem, _Alloc<_AllocElem>>, _Ts...>& adapter, _Ts... comparator);
    template<template<typename...> typename _Adapt,
             typename _AdaptElem, template<typename...> typename _Cont,
                      typename _ContElem, template<typename> typename _Alloc,
                                 typename _AllocElem, typename... _Ts>
    friend auto createThreadsafeSTLAdapterFrom(_Adapt<_AdaptElem, _Cont<_ContElem, _Alloc<_AllocElem>>, _Ts...>&& adapter, _Ts... comparator);

public:
    using Elem = typename AdaptElem::element_type;

    ThreadsafeSTLAdapter(const ThreadsafeSTLAdapter& rhs);
    ThreadsafeSTLAdapter(ThreadsafeSTLAdapter&& rhs);
    ThreadsafeSTLAdapter& operator=(const ThreadsafeSTLAdapter& rhs);
    ThreadsafeSTLAdapter& operator=(ThreadsafeSTLAdapter&& rhs);

    void push(Elem value);
    void pushAndNotify(Elem value);

    void waitAndPop(Elem& value);
    std::shared_ptr<Elem> waitAndPop();

    bool tryPop(Elem& value);
    std::shared_ptr<Elem> tryPop();

    void pop(Elem& value);
    std::shared_ptr<Elem> pop();

    friend void swap<>(ThreadsafeSTLAdapter& lhs, ThreadsafeSTLAdapter& rhs);
};

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::
ThreadsafeSTLAdapter(Adapt<AdaptElem, Cont<ContElem, Alloc<AllocElem>>, Ts...>&& adapter)
    : m_adapter(std::move(adapter))
{ }

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::
ThreadsafeSTLAdapter(const ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>& rhs)
{
    std::lock_guard<std::mutex> lock(rhs.m_mutex);
    m_adapter = rhs.m_adapter;
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::
ThreadsafeSTLAdapter(ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>&& rhs)
{
    std::lock_guard<std::mutex> lock(rhs.m_mutex);
    m_adapter = std::move_if_noexcept(rhs.m_adapter);
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>&
ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::
operator=(const ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>& rhs)
{
    if (this != &rhs)
    {
        std::scoped_lock lock(m_mutex, rhs.m_mutex);
        m_adapter = rhs.m_adapter;
    }
    return *this;
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>&
ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::
operator=(ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>&& rhs)
{
    if (this != &rhs)
    {
        std::scoped_lock lock(m_mutex, rhs.m_mutex);
        m_adapter = std::move_if_noexcept(rhs.m_adapter);
    }
    return *this;
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
void ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::push(Elem value)
{
    std::shared_ptr<Elem> item(std::make_shared<Elem>(std::move_if_noexcept(value)));
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_adapter.push(std::move(item));
    }
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
void ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::pushAndNotify(Elem value)
{
    push(std::move_if_noexcept(value));
    m_condVar.notify_one();
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
void ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::waitAndPop(Elem& value)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condVar.wait(lock, [&] { return !m_adapter.empty(); });
    value = std::move_if_noexcept(*getCurrent<Adapt<AdaptElem, Cont<ContElem, Alloc<AllocElem>>, Ts...>>(m_adapter));
    m_adapter.pop();
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
std::shared_ptr<typename ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::Elem>
ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::waitAndPop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condVar.wait(lock, [&] { return !m_adapter.empty(); });
    std::shared_ptr<Elem> res = std::move(getCurrent<Adapt<AdaptElem, Cont<ContElem, Alloc<AllocElem>>, Ts...>>(m_adapter));
    m_adapter.pop();
    return res;
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
bool ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::tryPop(Elem& value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_adapter.empty())
    {
        return false;
    }
    value = std::move_if_noexcept(*getCurrent<Adapt<AdaptElem, Cont<ContElem, Alloc<AllocElem>>, Ts...>>(m_adapter));
    m_adapter.pop();
    return true;
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
std::shared_ptr<typename ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::Elem>
ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::tryPop()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_adapter.empty())
    {
        return std::shared_ptr<Elem>{};
    }
    std::shared_ptr<Elem> res = std::move(getCurrent<Adapt<AdaptElem, Cont<ContElem, Alloc<AllocElem>>, Ts...>>(m_adapter));
    m_adapter.pop();
    return res;
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
void ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::pop(Elem& value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_adapter.empty())
    {
        throw EmptyAdapter{};
    }
    value = std::move_if_noexcept(*getCurrent<Adapt<AdaptElem, Cont<ContElem, Alloc<AllocElem>>, Ts...>>(m_adapter));
    m_adapter.pop();
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
std::shared_ptr<typename ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::Elem>
ThreadsafeSTLAdapter<Adapt, AdaptElem, Cont, ContElem, Alloc, AllocElem, Ts...>::pop()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_adapter.empty())
    {
        throw EmptyAdapter{};
    }
    std::shared_ptr<Elem> res = std::move(getCurrent<Adapt<AdaptElem, Cont<ContElem, Alloc<AllocElem>>, Ts...>>(m_adapter));
    m_adapter.pop();
    return res;
}

template<typename Comparator>
class CustomComparator
{
private:
    Comparator m_comparator;

public:
    explicit CustomComparator(Comparator comparator)
        : m_comparator(std::move(comparator))
    { }

    template<typename Elem>
    bool operator()(const std::shared_ptr<Elem>& x, const std::shared_ptr<Elem>& y) const noexcept(noexcept(m_comparator(*x, *y)))
    {
        return m_comparator(*x, *y);
    }
};

// This function uses a hack to get a reference to the adapter's internal container
template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
const Cont<ContElem, Alloc<AllocElem>>& getProtectedContainer(const Adapt<AdaptElem, Cont<ContElem, Alloc<AllocElem>>, Ts...>& adapter)
{
    struct OpenAdapter : Adapt<AdaptElem, Cont<ContElem, Alloc<AdaptElem>>, Ts...>
    {
        using Adapt<AdaptElem, Cont<ContElem, Alloc<AdaptElem>>, Ts...>::c;
    };
    return static_cast<const OpenAdapter&>(adapter).c;
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
Cont<ContElem, Alloc<AllocElem>>& getProtectedContainer(Adapt<AdaptElem, Cont<ContElem, Alloc<AllocElem>>, Ts...>& adapter)
{
    return const_cast<Cont<ContElem, Alloc<AllocElem>>&>(
           getProtectedContainer(static_cast<const Adapt<AdaptElem, Cont<ContElem, Alloc<AllocElem>>, Ts...>&>(adapter)));
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
auto createThreadsafeSTLAdapterFrom(const Adapt<AdaptElem, Cont<ContElem, Alloc<AllocElem>>, Ts...>& adapter, Ts... comparator)
{
    auto& underlyingContainer = getProtectedContainer(adapter);
    if constexpr(sizeof...(Ts) == 1)
    {
        // std::priority_queue
        Adapt<std::shared_ptr<AdaptElem>, Cont<std::shared_ptr<ContElem>, Alloc<std::shared_ptr<AllocElem>>>, CustomComparator<Ts...>>
        adapterWithSharedPtrElements{ CustomComparator{std::move(comparator...)} };
        auto& underlyingSharedPtrContainer = getProtectedContainer(adapterWithSharedPtrElements);
        std::transform(underlyingContainer.cbegin(), underlyingContainer.cend(), std::back_inserter(underlyingSharedPtrContainer),
                       [](auto& item) { return std::make_shared<AdaptElem>(item); });
        return ThreadsafeSTLAdapter{ std::move(adapterWithSharedPtrElements) };
    }
    if constexpr(sizeof...(Ts) == 0)
    {
        // std::stack / std::queue
        Adapt<std::shared_ptr<AdaptElem>, Cont<std::shared_ptr<ContElem>, Alloc<std::shared_ptr<AllocElem>>>>
        adapterWithSharedPtrElements{ };
        auto& underlyingSharedPtrContainer = getProtectedContainer(adapterWithSharedPtrElements);
        std::transform(underlyingContainer.cbegin(), underlyingContainer.cend(), std::back_inserter(underlyingSharedPtrContainer),
                       [](auto& item) { return std::make_shared<AdaptElem>(item); });
        return ThreadsafeSTLAdapter{ std::move(adapterWithSharedPtrElements) };
    }
}

template<template<typename...> typename Adapt,
         typename AdaptElem, template<typename...> typename Cont,
                  typename ContElem, template<typename> typename Alloc,
                             typename AllocElem, typename... Ts>
auto createThreadsafeSTLAdapterFrom(Adapt<AdaptElem, Cont<ContElem, Alloc<AllocElem>>, Ts...>&& adapter, Ts... comparator)
{
    auto& underlyingContainer = getProtectedContainer(adapter);
    if constexpr(sizeof...(Ts) == 1)
    {
        // std::priority_queue
        Adapt<std::shared_ptr<AdaptElem>, Cont<std::shared_ptr<ContElem>, Alloc<std::shared_ptr<AllocElem>>>, CustomComparator<Ts...>>
        adapterWithSharedPtrElements{ CustomComparator{std::move(comparator...)} };
        auto& underlyingSharedPtrContainer = getProtectedContainer(adapterWithSharedPtrElements);
        std::transform(underlyingContainer.begin(), underlyingContainer.end(), std::back_inserter(underlyingSharedPtrContainer),
                       [](auto& item) { return std::make_shared<AdaptElem>(std::move(item)); });
        return ThreadsafeSTLAdapter{ std::move(adapterWithSharedPtrElements) };
    }
    if constexpr(sizeof...(Ts) == 0)
    {
        // std::stack / std::queue
        Adapt<std::shared_ptr<AdaptElem>, Cont<std::shared_ptr<ContElem>, Alloc<std::shared_ptr<AllocElem>>>>
        adapterWithSharedPtrElements{ };
        auto& underlyingSharedPtrContainer = getProtectedContainer(adapterWithSharedPtrElements);
        std::transform(underlyingContainer.begin(), underlyingContainer.end(), std::back_inserter(underlyingSharedPtrContainer),
                       [](auto& item) { return std::make_shared<AdaptElem>(std::move(item)); });
        return ThreadsafeSTLAdapter{ std::move(adapterWithSharedPtrElements) };
    }
}

} // namespace concurrency

#endif
