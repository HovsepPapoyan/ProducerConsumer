/**
 * @file main.cpp
 *
 * @brief For demonstrating some usage of Producer and Consumer.
 *
 * @author Hovsep Papoyan
 * Contact: papoyanhovsep93@gmail.com
 * @Date 2024-01-10
 *
 */

#include "Consumer.h"
#include "Producer.h"

void consumerCallable(const int item)
{
    std::cout << "CONSUMER -> " << item << std::endl;
}

int main()
{
    auto sharedContainer = mt::createThreadSafeSTLAdapterFrom(std::priority_queue<int, std::vector<int>>{}, {});
    mt::Producer producer(sharedContainer);
    mt::Consumer consumer(sharedContainer, consumerCallable);
    producer.enableWorkerThread();
    consumer.enableWorkerThread();
    producer.push({ 1,2,3,4,5,6 });
    producer.disableWorkerThread();
    consumer.disableWorkerThread();
    producer.enableWorkerThread();
    consumer.enableWorkerThread();
    producer.push({ 1,2,3,4,5,6 });
    producer.disableWorkerThread();
    consumer.disableWorkerThread();
    producer.push({ 1,2,3,4,5,6 });
    producer.enableWorkerThread();
    consumer.enableWorkerThread();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}
