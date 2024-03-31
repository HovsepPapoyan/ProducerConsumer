/**
 * @file Producer.h
 *
 * @brief Producer class for pushing elements into shared thread-safe container.
 *
 * @author Hovsep Papoyan
 * Contact: papoyanhovsep93@gmail.com
 * @Date 2024-01-10
 *
 */

#ifndef PRODUCER_H
#define PRODUCER_H

#include "ProducerConsumerBase.h"

namespace mt
{
    template<typename Adapter>
    class Producer : public ProducerConsumerBase<Adapter>
    {
    private:
        using Super = ProducerConsumerBase<Adapter>;
        using Elem = typename Adapter::Elem;
        decltype(createThreadSafeSTLAdapterFrom(std::queue<std::vector<Elem>>{})) m_vectorItemsQueue;

    public:
        explicit Producer(Adapter& sharedContainer);
        Producer(const Producer&) = default;
        Producer(Producer&&) = default;
        Producer& operator=(const Producer&) = default;
        Producer& operator=(Producer&) = default;
        ~Producer() override;

        void push(std::vector<Elem> items);

    private:
        void workerThreadWork() override;
    };

    template<typename Adapter>
    Producer<Adapter>::Producer(Adapter& sharedContainer)
        : Super(Super::Type::Producer, sharedContainer)
        , m_vectorItemsQueue(createThreadSafeSTLAdapterFrom(std::queue<std::vector<Elem>>{}))
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
        while (this->m_workerThreadEnabled)
        {
            std::vector<Elem> vectorItem;
            if (m_vectorItemsQueue.tryPop(vectorItem))
            {
                for (auto& item : vectorItem)
                {
                    this->m_sharedContainer.push(std::move(item));
                }
            }
            else
            {
                std::this_thread::yield();
            }
        }
    }
} // namespace mt

#endif
