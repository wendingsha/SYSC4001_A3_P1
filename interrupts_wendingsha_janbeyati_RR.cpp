/**
 * @file interrupts.cpp
 * @author wendingsha
 * @brief main.cpp file for Round Robin Scheduler
 * 
 */

#include "interrupts_wendingsha_janbeyati.hpp"
#include <map>

const unsigned int QUANTUM = 100;

//main simulator
std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;  
    std::vector<PCB> wait_queue;   
    std::vector<PCB> job_list;     

    unsigned int current_time = 0;
    PCB running;
    idle_CPU(running);

    std::string execution_status;
    execution_status = print_exec_header();

    std::sort(list_processes.begin(), list_processes.end(),
              [](const PCB &a, const PCB &b){
                  return a.arrival_time < b.arrival_time;
              });

    const unsigned int INF = (unsigned int)-1;

    size_t next_arrival = 0;
    const size_t total_processes = list_processes.size();
    size_t terminated_processes = 0;

    std::map<int, unsigned int> io_finish_time;
    std::map<int, unsigned int> io_original_freq;

    for(const auto &p : list_processes) {
        io_original_freq[p.PID] = p.io_freq;
    }

    unsigned int quantum_remaining = 0;

    while (true) {

        //arrival
        while (next_arrival < list_processes.size() &&
               list_processes[next_arrival].arrival_time == current_time)
        {
            PCB p = list_processes[next_arrival];
            assign_memory(p);
            execution_status += print_exec_status(p.arrival_time, p.PID, NEW, READY);

            p.state = READY;
            ready_queue.push_back(p);
            job_list.push_back(p);

            next_arrival++;
        }

        //handle io completion
        for (auto it = wait_queue.begin(); it != wait_queue.end();) {
            PCB &p = *it;
            if (io_finish_time[p.PID] == current_time) {

                execution_status += print_exec_status(current_time, p.PID, WAITING, READY);

                p.state = READY;
                p.io_freq = io_original_freq[p.PID];

                ready_queue.push_back(p);
                sync_queue(job_list, p);
                it = wait_queue.erase(it);

            } else ++it;
        }

        //schedule if CPU idle
        if (running.PID == -1 && !ready_queue.empty()) {

            PCB p = ready_queue.front();
            ready_queue.erase(ready_queue.begin());

            execution_status += print_exec_status(current_time, p.PID, READY, RUNNING);

            p.state = RUNNING;
            if (p.start_time == -1) p.start_time = current_time;

            running = p;
            sync_queue(job_list, running);
            quantum_remaining = QUANTUM;
        }

        if (terminated_processes == total_processes) break;

        //next events
        unsigned int next_arrival_t = INF;
        if (next_arrival < list_processes.size())
            next_arrival_t = list_processes[next_arrival].arrival_time;

        unsigned int next_io_t = INF;
        for (auto &p : wait_queue)
            next_io_t = std::min(next_io_t, io_finish_time[p.PID]);

        unsigned int next_cpu_t = INF;
        if (running.PID != -1) {
            unsigned int cpu_delta = running.remaining_time;
            if (running.io_freq > 0 && running.io_freq < cpu_delta)
                cpu_delta = running.io_freq;
            if (quantum_remaining < cpu_delta)
                cpu_delta = quantum_remaining;
            next_cpu_t = current_time + cpu_delta;
        }

        unsigned int next_time = std::min({next_arrival_t, next_io_t, next_cpu_t});

        if (next_time == INF) break;

        unsigned int delta = next_time - current_time;

        if (running.PID != -1) {
            unsigned int use = delta;
            running.remaining_time -= use;
            quantum_remaining -= use;

            if (running.io_freq > 0) {
                if (use >= running.io_freq) running.io_freq = 0;
                else running.io_freq -= use;
            }
            sync_queue(job_list, running);
        }

        current_time = next_time;

        //CPU execution
        if (running.PID != -1) {

            //finish
            if (running.remaining_time == 0) {
                execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED);
                terminate_process(running, job_list);
                terminated_processes++;
                idle_CPU(running);
                continue;
            }

            //I/O
            if (running.io_freq == 0 && running.io_duration > 0) {
                execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING);

                running.state = WAITING;
                io_finish_time[running.PID] = current_time + running.io_duration;

                wait_queue.push_back(running);
                sync_queue(job_list, running);
                idle_CPU(running);
                continue;
            }

            //Quantum expire
            if (quantum_remaining == 0) {

                execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);

                running.state = READY;
                ready_queue.push_back(running);
                sync_queue(job_list, running);
                idle_CPU(running);
                continue;
            }
        }
    }

    execution_status += print_exec_footer();
    return std::make_tuple(execution_status);
}

int main(int argc, char** argv) {

    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    auto file_name = argv[1];
    std::ifstream input_file(file_name);

    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        list_process.push_back(add_process(input_tokens));
    }

    auto [exec] = run_simulation(list_process);
    write_output(exec, "execution.txt");
    return 0;
}
