#ifndef TIME_COUNTER_HPP
# define TIME_COUNTER_HPP

#include <chrono>
#include <iostream>

class TimeCounter {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    std::chrono::high_resolution_clock::time_point current_time;
    bool is_running;

public:
    TimeCounter() : is_running(false) {}

    void start() {
        start_time = std::chrono::high_resolution_clock::now();
        is_running = true;
    }

    void elapsed_time(std::string part_name) {
        if (is_running) {
            current_time = std::chrono::high_resolution_clock::now();
        }
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time);
        std::cout << part_name <<": " << (double) duration.count() << " ms" << std::endl;
        start_time = std::chrono::high_resolution_clock::now();
    }

    void end_counter() {
        end_time = std::chrono::high_resolution_clock::now();
        is_running = false;
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        std::cout << "Counter ended. Total time passed: " << (double)duration.count() << " ms" << std::endl;
    }
};

#endif