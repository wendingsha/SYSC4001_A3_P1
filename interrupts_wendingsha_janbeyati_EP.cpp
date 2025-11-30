/**
 * @file interrupts.cpp
 * @author wendingsha
 * @brief main.cpp file for EXternal Priority Scheduler
 * 
 */

#include "interrupts_wendingsha_janbeyati.hpp"
#include <map>

void EP_scheduler(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &a, const PCB &b ){
                    return a.PID < b.PID; 
                } 
            );
}

//main simulator
std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Sort input processes by arrival time
    std::sort(list_processes.begin(), list_processes.end(),
              [](const PCB &a, const PCB &b){
                  return a.arrival_time < b.arrival_time;
              });

    const unsigned int INF = (unsigned int)-1;

    size_t next_arrival = 0;
    const size_t total_processes = list_processes.size();
    size_t terminated_processes = 0;
    
    //record completion time
    std::map<int, unsigned int> io_finish_time;

    //store original io_freq for each process
    std::map<int, unsigned int> io_original_freq;
    for(const auto &p : list_processes) {
        io_original_freq[p.PID] = p.io_freq;
    }

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
            } else {
                ++it;
            }
        }

        //if CPU idle, schedule new process
        if (running.PID == -1 && !ready_queue.empty()) {
            EP_scheduler(ready_queue);

            PCB p = ready_queue.front();
            ready_queue.erase(ready_queue.begin());

            execution_status += print_exec_status(current_time, p.PID, READY, RUNNING);

            p.state = RUNNING;
            if (p.start_time == -1) {
                p.start_time = current_time;
            }

            running = p;
            sync_queue(job_list, running);
        }

        if (terminated_processes == total_processes) {
            break;
        }

        unsigned int next_arrival_time = INF;
        if (next_arrival < list_processes.size()) {
            next_arrival_time = list_processes[next_arrival].arrival_time;
        }

        unsigned int next_io_time = INF;
        for (auto &p : wait_queue) {
            unsigned int t = io_finish_time[p.PID];
            if (t < next_io_time) {
                next_io_time = t;
            }
        }

        unsigned int next_cpu_time = INF;
        if (running.PID != -1) {
            unsigned int cpu_delta = running.remaining_time;
            if (running.io_freq > 0 && running.io_freq < cpu_delta) {
                cpu_delta = running.io_freq;
            }
            next_cpu_time = current_time + cpu_delta;
        }

        unsigned int next_time = INF;
        if (next_arrival_time < next_time) next_time = next_arrival_time;
        if (next_io_time      < next_time) next_time = next_io_time;
        if (next_cpu_time     < next_time) next_time = next_cpu_time;

        if (next_time == INF) {
            break;
        }

        //CPU execution
        unsigned int delta = next_time - current_time;
        if (running.PID != -1 && delta > 0) {

            if (delta > running.remaining_time) {
                delta = running.remaining_time;
            }

            running.remaining_time -= delta;

            if (running.io_freq > 0) {
                if (delta >= running.io_freq) {
                    running.io_freq = 0;
                } else {
                    running.io_freq -= delta;
                }
            }

            sync_queue(job_list, running);
        }

        current_time = next_time;


        if (running.PID != -1) {

            //case1: process finished
            if (running.remaining_time == 0) {
                execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED);

                terminate_process(running, job_list);
                terminated_processes++;
                idle_CPU(running);
            }
            //case2: io triggered
            else if (running.io_freq == 0 && running.io_duration > 0) {
                execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING);

                running.state = WAITING;
                io_finish_time[running.PID] = current_time + running.io_duration;

                wait_queue.push_back(running);
                sync_queue(job_list, running);
                idle_CPU(running);
            }
        }

        if (terminated_processes == total_processes) {
            break;
        }
    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}
