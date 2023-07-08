#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */
template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    std::unique_lock<std::mutex> unique_lock(_mtx);
	_cond_var.wait(unique_lock, [this]{ return !_queue.empty(); });

	T msg = std::move(_queue.back());
	_queue.pop_back();
	return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // This is basically copied from the lectures
    std::lock_guard<std::mutex> lock(_mtx);
	_queue.push_back(std::move(msg));
	_cond_var.notify_one();
}


/* Implementation of class "TrafficLight" */
TrafficLight::TrafficLight()
{
    _current_tl_phase = TrafficLightPhase::red;
    // Taken from: https://stackoverflow.com/a/5020138/783874
    std::uniform_real_distribution<double> _uniform_dstr(PERIOD_BOUND_SEC[0], PERIOD_BOUND_SEC[1]);
    std::mt19937 _mt;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true) {
        auto tl_phase = _message_queue.receive();
        if (tl_phase == TrafficLightPhase::green)
            return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _current_tl_phase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    // The "threads" attribute comes from the super class (TrafficObject)
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    float period = _uniform_dstr(_mt);

    auto start = std::chrono::high_resolution_clock::now();
    
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        auto end = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

        if (elapsed > period) {
            // Switch the TL state
            if (_current_tl_phase == TrafficLightPhase::green)
                _current_tl_phase = TrafficLightPhase::red;
            else
                _current_tl_phase = TrafficLightPhase::green;

            TrafficLightPhase p = _current_tl_phase;
            _message_queue.send(std::move(p));

            start = std::chrono::high_resolution_clock::now();
        }

        period = _uniform_dstr(_mt);
    }
}
