#ifndef TIME_COUNTER_HPP
#define TIME_COUNTER_HPP

#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <numeric>

class TimeCounter {
private:
    // Helper to manage the 10-measure rolling average
    struct RollingAverage {
        double history[10] = {0.0};
        int index = 0;
        int count = 0;

        void add(double value) {
            history[index] = value;
            index = (index + 1) % 10;
            if (count < 10) count++;
        }

        double get_avg() const {
            if (count == 0) return 0.0;
            double sum = 0;
            for (int i = 0; i < count; ++i) sum += history[i];
            return sum / count;
        }
    };

    // Timing points for each category
    std::chrono::high_resolution_clock::time_point t_food_start, t_render_start, t_advance_start,
        t_evaporation_start, t_mpi_start;
    
    // Average storage
    RollingAverage avg_food, avg_render, avg_advance, avg_evaporation, avg_mpi_comm;
    // Per-iteration accumulator for MPI communication (summed across multiple MPI calls)
    double mpi_accum_us = 0.0;

public:
    TimeCounter() = default;

    // --- Food/Display Timing ---
    void start_food() { t_food_start = std::chrono::high_resolution_clock::now(); }
    void end_food() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - t_food_start);
        avg_food.add(duration.count() / 1000.0);
    }

    // --- Render/Event Timing ---
    void start_render() { t_render_start = std::chrono::high_resolution_clock::now(); }
    void end_render() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - t_render_start);
        avg_render.add(duration.count() / 1000.0);
    }

    // --- Advance (pure ant computation, no MPI) ---
    void start_advance() { t_advance_start = std::chrono::high_resolution_clock::now(); }
    void end_advance() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - t_advance_start);
        avg_advance.add(duration.count() / 1000.0);
    }

    // --- Evaporation (pure local computation, no MPI) ---
    void start_evaporation() { t_evaporation_start = std::chrono::high_resolution_clock::now(); }
    void end_evaporation() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - t_evaporation_start);
        avg_evaporation.add(duration.count() / 1000.0);
    }

    // --- MPI Communication (accumulated across all MPI calls in one iteration) ---
    void start_mpi() { t_mpi_start = std::chrono::high_resolution_clock::now(); }
    void end_mpi() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - t_mpi_start);
        mpi_accum_us += duration.count();
    }
    // Call at the end of each iteration to flush accumulated MPI time
    void flush_mpi() {
        avg_mpi_comm.add(mpi_accum_us / 1000.0);
        mpi_accum_us = 0.0;
    }

    // --- Display Results ---
    void print_averages() {
        std::cout << "\r[Avg MS] Adv: " << avg_advance.get_avg() 
                  << " | Evap: " << avg_evaporation.get_avg()
                  << " | MPI: " << avg_mpi_comm.get_avg()
                  << " | Render: " << avg_render.get_avg() 
                  << " | Food: " << avg_food.get_avg() << "          " << std::flush;
    }
};

#endif