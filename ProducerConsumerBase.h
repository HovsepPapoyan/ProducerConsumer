/**
 * @file ProducerConsumerBase.h
 *
 * @brief ProducerConsumerBase base class for Producer and Consumer.
 *
 * @author Hovsep Papoyan
 * Contact: papoyanhovsep93@gmail.com
 * @Date 2024-01-10
 *
 */

#ifndef PRODUCER_CONSUMER_BASE_H
#define PRODUCER_CONSUMER_BASE_H

#include "ThreadSafeSTLAdapter.h"

#include <iostream>
#include <queue>

namespace mt
{
    template<typename Adapter>
    class ProducerConsumerBase
    {
    protected:
        static constexpr std::string_view Names[] = { "PRODUCER", "CONSUMER" };
        enum class Command : unsigned char
        {
            EnableWorkerThread,
            DisableWorkerThread,
            ShutdownMainThread
        };
        enum class Type : unsigned char
        {
            Producer,
            Consumer
        };

    private:
        decltype(createThreadSafeSTLAdapterFrom(std::queue<Command>{})) m_commandQueue;
        std::unique_ptr<std::jthread> m_mainThread;

    protected:
        Adapter& m_sharedContainer;
        std::unique_ptr<std::jthread> m_workerThread;
        std::mutex m_workerThreadMutex;
        std::atomic<bool> m_workerThreadEnabled;
        std::string_view m_name;

    public:
        explicit ProducerConsumerBase(const Type type, Adapter& sharedContainer);
        ProducerConsumerBase(const ProducerConsumerBase&) = default;
        ProducerConsumerBase(ProducerConsumerBase&&) = default;
        ProducerConsumerBase& operator=(const ProducerConsumerBase&) = default;
        ProducerConsumerBase& operator=(ProducerConsumerBase&) = default;
        virtual ~ProducerConsumerBase() = default;

        void enableWorkerThread();
        void disableWorkerThread();

    protected:
        void runMainThread();
        void shutdownMainThread();

    private:
        virtual void workerThreadWork() = 0;
        void interruptWorkerThread();
        void mainThreadWork();
    };

    template<typename Adapter>
    ProducerConsumerBase<Adapter>::ProducerConsumerBase(const Type type, Adapter& sharedContainer)
        : m_commandQueue(createThreadSafeSTLAdapterFrom(std::queue<Command>{}))
        , m_sharedContainer(sharedContainer)
        , m_workerThreadEnabled(false)
        , m_name(Names[static_cast<unsigned char>(type)])
    { }

    template<typename Adapter>
    void ProducerConsumerBase<Adapter>::enableWorkerThread()
    {
        m_commandQueue.pushAndNotify(Command::EnableWorkerThread);
    }

    template<typename Adapter>
    void ProducerConsumerBase<Adapter>::disableWorkerThread()
    {
        m_commandQueue.pushAndNotify(Command::DisableWorkerThread);
    }

    template<typename Adapter>
    void ProducerConsumerBase<Adapter>::runMainThread()
    {
        try
        {
            m_mainThread = std::make_unique<std::jthread>([&] { mainThreadWork(); });
        }
        catch (const std::exception& ex)
        {
            std::cerr << m_name << " -> " << ex.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << m_name << " -> Unknown exception" << std::endl;
        }
    }

    template<typename Adapter>
    void ProducerConsumerBase<Adapter>::shutdownMainThread()
    {
        try
        {
            m_commandQueue.pushAndNotify(Command::ShutdownMainThread);
        }
        catch (const std::exception& ex)
        {
            std::cerr << m_name << " -> " << ex.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << m_name << " -> Unknown exception" << std::endl;
        }
        if (m_mainThread->joinable())
        {
            m_mainThread->join();
        }
    }

    template<typename Adapter>
    void ProducerConsumerBase<Adapter>::interruptWorkerThread()
    {
        std::lock_guard<std::mutex> lock(m_workerThreadMutex);
        m_workerThreadEnabled = false;
        if (m_workerThread->joinable())
        {
            m_workerThread->join();
        }
    }

    template<typename Adapter>
    void ProducerConsumerBase<Adapter>::mainThreadWork()
    {
        try
        {
            while (true)
            {
                Command currentCommand;
                m_commandQueue.waitAndPop(currentCommand);

                if (currentCommand == Command::EnableWorkerThread)
                {
                    if (!m_workerThreadEnabled)
                    {
                        std::lock_guard<std::mutex> lock(m_workerThreadMutex);
                        m_workerThreadEnabled = true;
                        m_workerThread = std::make_unique<std::jthread>([&]
                            {
                                try
                                {
                                    workerThreadWork();
                                }
                                catch (const std::exception& ex)
                                {
                                    std::cerr << m_name << " -> " << ex.what() << std::endl;
                                }
                                catch (...)
                                {
                                    std::cerr << m_name << " -> Unknown exception" << std::endl;
                                }
                            });
                    }
                }
                else if (currentCommand == Command::DisableWorkerThread)
                {
                    interruptWorkerThread();
                }
                else if (currentCommand == Command::ShutdownMainThread)
                {
                    interruptWorkerThread();
                    break;
                }
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << m_name << " -> " << ex.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << m_name << " -> Unknown exception" << std::endl;
        }
    }
} // namespace mt

#endif
