#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <ctime>
#include <future>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait()
    // to wait for and receive new messages and pull them from the queue using move semantics.
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> lock(_mu);
    _condition.wait(lock, [this] {
        return !_queue.empty();
    });

    T msg = std::move(_queue.front());
    _queue.pop_front();
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex>
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::unique_lock<std::mutex> lock(_mu);
    _queue.emplace_back(msg);
    _condition.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
    // runs and repeatedly calls the receive function on the message queue.
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        // adding this line to avoid a lot of calls unnecessary:
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto msg = _queue.receive();
        if (msg == TrafficLightPhase::green)
        {
            return;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
    // and toggles the current phase of the traffic light between red and green and sends an update method
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

    // Reference for random numbers in C++11:
    //https://stackoverflow.com/questions/686353/random-float-number-generation
    std::random_device rd;

    // Engines
    //
    std::mt19937 e2(rd());
    //std::knuth_b e2(rd());
    //std::default_random_engine e2(rd()) ;

    //
    // Distribtuions
    //
    std::uniform_int_distribution<int> dist(4000, 6000);
    // std::normal_distribution<> dist(4000, 6000);
    //std::student_t_distribution<> dist(5);
    //std::poisson_distribution<> dist(2);
    //std::extreme_value_distribution<> dist(0,2);

    int randomCycleValue = dist(e2);

    std::chrono::time_point<std::chrono::system_clock> timeNow;

    timeNow = std::chrono::system_clock::now();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        int elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - timeNow).count();

        if (elapsed_seconds >= randomCycleValue)
        {
            if (this->_currentPhase == TrafficLightPhase::red)
            {
                this->_currentPhase = TrafficLightPhase::green;
            }
            else
            {
                this->_currentPhase = TrafficLightPhase::red;
            }

            auto futVar = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, &_queue, std::move(_currentPhase));
            futVar.wait();

            randomCycleValue = dist(e2);
            timeNow = std::chrono::system_clock::now();
        }
    }
}