/**
 * @file Producer.h
 *
 * @brief Producer class for pushing elements into shared thread-safe container
 *
 * @author Hovsep Papoyan
 * Contact: papoyanhovsep93@gmail.com
 * @Date 2024-01-10
 *
 */

#ifndef PRODUCER_H
#define PRODUCER_H

#include "ProducerConsumerBase.h"

#include <vector>

namespace concurrency
{

template<typename Adapter>
class Producer : public ProducerConsumerBase<Adapter>
{
private:
    using Super = ProducerConsumerBase<Adapter>;
    using Elem = typename Adapter::Elem;
    decltype(createThreadsafeSTLAdapterFrom(std::queue<std::vector<Elem>>{})) m_vectorItemsQueue;

public:
    explicit Producer(Adapter& sharedContainer);
    ~Producer() override;

    void push(std::vector<Elem> items);

private:
    void workerThreadWork() override;
};

template<typename Adapter>
Producer<Adapter>::Producer(Adapter& sharedContainer)
    : Super(Super::Type::Producer, sharedContainer)
    , m_vectorItemsQueue(createThreadsafeSTLAdapterFrom(std::queue<std::vector<Elem>>{}))
{
    this->runMainThread();
}

template<typename Adapter>
Producer<Adapter>::~Producer()
{
    this->shutdownMainThread();
}

template<typename Adapter>
void Producer<Adapter>::push(std::vector<Elem> items)
{
    m_vectorItemsQueue.push(std::move(items));
}

template<typename Adapter>
void Producer<Adapter>::workerThreadWork()
{
    while (true)
    {
        std::vector<Elem> vectorItem;
        std::unique_lock<std::mutex> lock(this->m_workerThreadMutex);
        this->m_isWorkerThreadRunning = true;
        this->m_workerThreadCondVar.wait(lock, [&] { return !this->m_workerThreadEnabled ? true : m_vectorItemsQueue.tryPop(vectorItem); });
        if (!this->m_workerThreadEnabled)
        {
            this->m_isWorkerThreadRunning = false;
            break;
        }
        for (auto& item : vectorItem)
        {
            #ifdef DEBUG
            std::cout << this->m_name << " -> " << item << std::endl;
            #endif
            this->m_sharedContainer.push(std::move(item));
        }
    }
}

} // namespace concurrency

#endif
