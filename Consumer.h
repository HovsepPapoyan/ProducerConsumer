/**
 * @file Consumer.h
 *
 * @brief Consumer class for popping elements from shared thread-safe container
 *
 * @author Hovsep Papoyan
 * Contact: papoyanhovsep93@gmail.com
 * @Date 2024-01-10
 *
 */

#ifndef CONSUMER_H
#define CONSUMER_H

#include "ProducerConsumerBase.h"

namespace concurrency
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
    while (true)
    {
        Elem item;
        std::unique_lock<std::mutex> lock(this->m_workerThreadMutex);
        this->m_isWorkerThreadRunning = true;
        this->m_workerThreadCondVar.wait(lock, [&] { return !this->m_workerThreadEnabled ? true : this->m_sharedContainer.tryPop(item); });
        if (!this->m_workerThreadEnabled)
        {
            this->m_isWorkerThreadRunning = false;
            break;
        }
        #ifdef DEBUG
        std::cout << this->m_name << " -> " << item << std::endl;
        #endif
        m_callable(std::move_if_noexcept(item));
    }
}

} // namespace concurrency

#endif
