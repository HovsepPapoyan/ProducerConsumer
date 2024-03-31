/**
 * @file Consumer.h
 *
 * @brief Consumer class for popping elements from shared thread-safe container.
 *
 * @author Hovsep Papoyan
 * Contact: papoyanhovsep93@gmail.com
 * @Date 2024-01-10
 *
 */

#ifndef CONSUMER_H
#define CONSUMER_H

#include "ProducerConsumerBase.h"

namespace mt
{
    template<typename Adapter, typename Callable>
    class Consumer : public ProducerConsumerBase<Adapter>
    {
    private:
        using Super = ProducerConsumerBase<Adapter>;
        using Elem = typename Adapter::Elem;

        Callable m_callable;

    public:
        explicit Consumer(Adapter& sharedContainer, Callable callable);
        Consumer(const Consumer&) = default;
        Consumer(Consumer&&) = default;
        Consumer& operator=(const Consumer&) = default;
        Consumer& operator=(Consumer&) = default;
        ~Consumer() override;

    private:
        void workerThreadWork() override;
    };

    template<typename Adapter, typename Callable>
    Consumer<Adapter, Callable>::Consumer(Adapter& sharedContainer, Callable callable)
        : Super(Super::Type::Consumer, sharedContainer)
        , m_callable(std::move(callable))
    {
        this->runMainThread();
    }

    template<typename Adapter, typename Callable>
    Consumer<Adapter, Callable>::~Consumer()
    {
        this->shutdownMainThread();
    }

    template<typename Adapter, typename Callable>
    void Consumer<Adapter, Callable>::workerThreadWork()
    {
        while (this->m_workerThreadEnabled)
        {
            if (Elem item; this->m_sharedContainer.tryPop(item))
            {
                m_callable(std::move_if_noexcept(item));
            }
            else
            {
                std::this_thread::yield();
            }
        }
    }
} // namespace mt

#endif
