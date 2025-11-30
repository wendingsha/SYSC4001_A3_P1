/**
 * @file interrupts.cpp
 * @author wendingsha
 * @brief main.cpp file for EXternal Priority Scheduler
 * 
 */

#include <interrupts_wendingsha_janbeyati.hpp>
#include <map>

//External Priority Scheduler
//smaller PID -> higher priority
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

    //detect arrival
    std::sort(list_processes.begin(), list_processes.end(),
              [](const PCB &a, const PCB &b){
                  return a.arrival_time < b.arrival_time;
              });

    size_t next_arrival = 0;
    
    //record completion time of I/O
    std::map<int, unsigned int> io_finish_time;

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {
        //arrival events
        while (next_arrival < list_processes.size() && list_processes[next_arrival].arrival_time == current_time){
            PCB p = list_processes[next_arrival];

            assign_memory(p);

            execution_status += print_exec_status(current_time,p.PID,NEW,READY);

            p.state = READY;
            ready_queue.push_back(p);
            job_list.push_back(p);

            next_arrival++;
        }

        //handle IO completion
        for (auto it = wait_queue.begin(); it != wait_queue.end();){
            PCB &p = *it;
            if (io_finish_time[p.PID] == current_time){
                execution_status += print_exec_status(current_time, p.PID, WAITING, READY);

                p.state = READY;
                ready_queue.push_back(p);

                sync_queue(job_list, p);
                it = wait_queue.erase(it);
            }else{
                ++it;
            }
        }

        //if CPU idle, schedule new process
        if (running.PID == -1 && !ready_queue.empty()){
            EP_scheduler(ready_queue);

            PCB p = ready_queue.front();
            ready_queue.erase(ready_queue.begin());

            execution_status += print_exec_status(current_time,p.PID, READY, RUNNING);

            p.state = RUNNING;
            p.start_time = current_time;

            running = p;
            sync_queue(job_list, running);
        }

        //condition check
        if(running.PID == -1 && ready_queue.empty() && wait_queue.empty() && next_arrival >= list_processes.size()){
            break;
        }

        //CPU execution
        if(running.PID != -1){
            PCB p = running;

            unsigned int time_to_io = (p.io_freq == 0 ? p.remaining_time : p.io_freq);

            unsigned int cpu_run = std::min(p.remaining_time, time_to_io);

            unsigned int next_time = current_time + cpu_run;

            current_time = next_time;
            p.remaining_time -= cpu_run;

            if(p.io_freq > 0){
                p.io_freq -= cpu_run;
            }

            //case1: process finished
            if(p.remaining_time == 0){
                execution_status += print_exec_status(current_time, p.PID, RUNNING, TERMINATED);

                terminate_process(p, job_list);
                idle_CPU(running);
                continue;
            }

            //case2: io triggered
            if(p.io_freq == 0 && p.io_duration > 0){
                execution_status += print_exec_status(current_time, p.PID, RUNNING, WAITING);

                p.state = WAITING;
                io_finish_time[p.PID] = current_time + p.io_duration;

                wait_queue.push_back(p);
                sync_queue(job_list, p);

                idle_CPU(running);
                continue;
            }

            //contunue running
            running = p;
            sync_queue(job_list, running);
            continue;
        }

        //CPU is idle
        unsigned int next_event = (unsigned int) - 1;

        if(next_arrival < list_processes.size()){
            next_event = list_processes[next_arrival].arrival_time;
        }

        for(auto &p: wait_queue){
            unsigned int t = io_finish_time[p.PID];
            if(next_event == (unsigned int) -1 || t< next_event){
                next_event = t;
            }
        }

        current_time = next_event;
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