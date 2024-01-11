/**
 * @file ProducerConsumerBase.h
 *
 * @brief ProducerConsumerBase base class for Producer and Consumer
 *
 * @author Hovsep Papoyan
 * Contact: papoyanhovsep93@gmail.com
 * @Date 2024-01-10
 *
 */

#ifndef PRODUCER_CONSUMER_BASE_H
#define PRODUCER_CONSUMER_BASE_H

#include "ThreadsafeSTLAdapter.h"

#include <iostream>
#include <queue>
#include <string_view>
#include <thread>

namespace concurrency
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
    decltype(createThreadsafeSTLAdapterFrom(std::queue<Command>{})) m_commandQueue;
    std::unique_ptr<std::thread> m_mainThread;

protected:
    Adapter& m_sharedContainer;
    std::unique_ptr<std::thread> m_workerThread;
    std::mutex m_workerThreadMutex;
    std::condition_variable m_workerThreadCondVar;
    bool m_workerThreadEnabled;
    bool m_isWorkerThreadRunning;
    std::string_view m_name;

public:
    explicit ProducerConsumerBase(const Type type, Adapter& sharedContainer);
    virtual ~ProducerConsumerBase() = default;

    void enableWorkerThread();
    void disableWorkerThread();

protected:
    void runMainThread();
    void shutdownMainThread();

private:
    virtual void workerThreadWork() = 0;
    void disableWorkerThreadIfEnabled();
    void mainThreadWork();
};

template<typename Adapter>
ProducerConsumerBase<Adapter>::ProducerConsumerBase(const Type type, Adapter& sharedContainer)
    : m_commandQueue(createThreadsafeSTLAdapterFrom(std::queue<Command>{}))
    , m_sharedContainer(sharedContainer)
    , m_workerThreadEnabled(false)
    , m_isWorkerThreadRunning(false)
    , m_name(Names[static_cast<unsigned char>(type)])
{ }
    
template<typename Adapter>
void ProducerConsumerBase<Adapter>::enableWorkerThread()
{
    #ifdef DEBUG
    std::cout << m_name << " -> " << __PRETTY_FUNCTION__ << std::endl;
    #endif
    m_commandQueue.pushAndNotify(Command::EnableWorkerThread);
}

template<typename Adapter>
void ProducerConsumerBase<Adapter>::disableWorkerThread()
{
    #ifdef DEBUG
    std::cout << m_name << " -> " << __PRETTY_FUNCTION__ << std::endl;
    #endif
    m_commandQueue.pushAndNotify(Command::DisableWorkerThread);
}

template<typename Adapter>
void ProducerConsumerBase<Adapter>::runMainThread()
{
    try
    {
        m_mainThread = std::make_unique<std::thread>([&]{ mainThreadWork(); });
    }
    catch (const std::exception& ex)
    {
        #ifdef DEBUG
        std::cout << m_name << " -> " << __PRETTY_FUNCTION__ << " -> " << ex.what() << std::endl;
        #endif
    }
    catch (...)
    {
        #ifdef DEBUG
        std::cout << m_name << " -> " << __PRETTY_FUNCTION__ << " -> Unknown exception" << std::endl;
        #endif
    }
}

template<typename Adapter>
void ProducerConsumerBase<Adapter>::shutdownMainThread()
{
    try
    {
        #ifdef DEBUG
        std::cout << m_name << " -> " << __PRETTY_FUNCTION__ << std::endl;
        #endif
        m_commandQueue.pushAndNotify(Command::ShutdownMainThread);
    }
    catch (const std::exception& ex)
    {
        #ifdef DEBUG
        std::cout << m_name << " -> " << __PRETTY_FUNCTION__ << " -> " << ex.what() << std::endl;
        #endif
    }
    catch (...)
    {
        #ifdef DEBUG
        std::cout << m_name << " -> " << __PRETTY_FUNCTION__ << " -> Unknown exception" << std::endl;
        #endif
    }
    if (m_mainThread->joinable())
    {
        m_mainThread->join();
    }
}

template<typename Adapter>
void ProducerConsumerBase<Adapter>::disableWorkerThreadIfEnabled()
{
    std::unique_lock<std::mutex> lock(m_workerThreadMutex);
    if (m_workerThreadEnabled)
    {
        // Disabling worker thread
        m_workerThreadEnabled = false;
        lock.unlock();
        m_workerThreadCondVar.notify_one();
        if (m_workerThread->joinable())
        {
            m_workerThread->join();
        }
    }
    else
    {
        // Nothing to disable, there is no running worker thread
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
                std::unique_lock<std::mutex> lock(m_workerThreadMutex);
                if (!m_workerThreadEnabled && !m_isWorkerThreadRunning)
                {
                    // Enabling worker thread
                    m_workerThreadEnabled = true;
                    lock.unlock();
                    m_workerThread = std::make_unique<std::thread>([&]
                    {
                        try
                        {
                            workerThreadWork();
                        }
                        catch (const std::exception& ex)
                        {
                            #ifdef DEBUG
                            std::cout << m_name << " -> " << __PRETTY_FUNCTION__ << " -> " << ex.what() << std::endl;
                            #endif
                        }
                        catch (...)
                        {
                            #ifdef DEBUG
                            std::cout << m_name << " -> " << __PRETTY_FUNCTION__ << " -> Unknown exception" << std::endl;
                            #endif
                        }
                    });
                }
                else
                {
                    // Nothing to enable, worker thread already enabled
                }
            }
            else if (currentCommand == Command::DisableWorkerThread)
            {
                disableWorkerThreadIfEnabled();
            }
            else if (currentCommand == Command::ShutdownMainThread)
            {
                disableWorkerThreadIfEnabled();
                break;
            }
        }
    }
    catch (const std::exception& ex)
    {
        #ifdef DEBUG
        std::cout << m_name << " -> " << __PRETTY_FUNCTION__ << " -> " << ex.what() << std::endl;
        #endif
    }
    catch (...)
    {
        #ifdef DEBUG
        std::cout << m_name << " -> " << __PRETTY_FUNCTION__ << " -> Unknown exception" << std::endl;
        #endif
    }
}

} // namespace concurrency

#endif
