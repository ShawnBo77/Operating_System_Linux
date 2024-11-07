#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <vector>
#include <chrono>
#include <time.h>
#include <sched.h>

using namespace std;

#define SCHED_NORMAL 0 
#define SCHED_FIFO 1

typedef struct {
    pthread_t thread_id; //typedef unsigned long pthread_t
    int thread_num;
    float time_wait;
    int thread_sched_policy;
    int thread_sched_priority;
} thread_info_t;

pthread_barrier_t barrier;

int get_policy_num(string policy){
    if(policy == "NORMAL"){
        return SCHED_NORMAL;
    }
    else{
        return SCHED_FIFO;
    }
}

void *thread_func(void *arg)
{
    thread_info_t *thread_info = (thread_info_t *)arg;
    // cout << "thread : " << thread_info->thread_num << " " << thread_info->thread_sched_policy << " " << thread_info->thread_sched_priority << "\n";

    /* 1. Wait until all threads are ready */
    // printf("Thread %d sched_getcpu = %d\n", thread_info->thread_num, sched_getcpu());
    pthread_barrier_wait(&barrier);

    /* 2. Do the task */ 
    for (int i = 0; i < 3; i++) {
        printf("Thread %d is starting\n", thread_info->thread_num);
        /* Busy for <time_wait> seconds */
        auto start_time = chrono::high_resolution_clock::now();
        auto end_time = start_time + chrono::milliseconds(int(thread_info->time_wait*1000));
        auto time = chrono::high_resolution_clock::now();
        while (time < end_time) {
            time = chrono::high_resolution_clock::now();
            // printf("Thread %d is running\n", thread_info->thread_num);
            // auto now = chrono::high_resolution_clock::to_time_t(time);
            // cout << ctime(&now);  // This will print a readable time format
        }
    }

    /* 3. Exit the function  */
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    int opt;
    int num_threads = 0;
    float time_wait = 0;
    vector<string> policies;
    vector<int> priorities;

    /* 1. Parse program arguments */  
    while ((opt = getopt(argc, argv, "n:t:s:p:")) != -1) {
        switch (opt) {
            case 'n':{
                num_threads = atoi(optarg);
                break;
            }
            case 't':{
                time_wait = atof(optarg);
                break;
            }
            case 's':{
                char *policies_str = optarg;
                char *policy;

                policy = strtok(policies_str, ",");

                int i = 0;
                while (policy != NULL && i < num_threads) {
                    // printf("%s\n", policy);
                    policies.push_back(string(policy));
                    policy = strtok(NULL, ",");	
                    i++;
                }
                
                // for(int j = 0; j < policies.size(); j++){
                //     cout << policies[j] << "\n";
                // }
                break;
            }
            case 'p':{
                char* priorities_str = optarg;
                char *priority;

                priority = strtok(priorities_str, ",");

                int i = 0;
                while (priority != NULL && i < num_threads) {
                    // printf("%s\n", priority);
                    priorities.push_back(stoi(string(priority)));
                    priority = strtok(NULL, ",");	
                    i++;
                }

                // for(int j = 0; j < priorities.size(); j++){
                //     cout << priorities[j] << "\n";
                // }
                break;
            }
            default:
                cout << "Please follow the pattern below:" << "\n";
                cout << argv[0] << " -n <num_threads> -t <time_wait> -s <policies> -p <priorities>\n" << "\n";
                return -1;
        }
    }

    /* 2. Create <num_threads> worker threads (create thread info) */
    // cout << "pthread_barrier_init" << "\n";
    pthread_barrier_init(&barrier, NULL, num_threads + 1); // the created threads and main thread
    thread_info_t thread_info[num_threads];

    /* 3. Set CPU affinity */
    // cout << "Set CPU affinity" << "\n";
    int cpu_id = 0;
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) != 0) {
        cout << "Error: sched_setscheduler\n";
    }
    

    /* 4. Set the attributes to each thread (and create thread) */
    // cout << "Set the attributes to each thread" << "\n";
    for (int i = 0; i < num_threads; i++) {
        thread_info[i].thread_num = i;
        thread_info[i].time_wait = time_wait;
        thread_info[i].thread_sched_policy = get_policy_num(policies[i]);
        thread_info[i].thread_sched_priority = priorities[i];

        if (thread_info[i].thread_sched_policy == SCHED_FIFO) {
            pthread_attr_t pthread_attr;
            struct sched_param param;

            pthread_attr_init(&pthread_attr);
            // do not inherit the parent's scheduling policy to set the thread's policy.
            pthread_attr_setinheritsched(&pthread_attr, PTHREAD_EXPLICIT_SCHED);

            // set the scheduling policy
            if (pthread_attr_setschedpolicy(&pthread_attr, SCHED_FIFO) != 0) {
                cout << "Error: SCHED_FIFO pthread_attr_setschedpolicy\n";
            }

            param.sched_priority = thread_info[i].thread_sched_priority; // set priority

            // set the scheduling paramters
            if (pthread_attr_setschedparam(&pthread_attr, &param) != 0) {
                cout << "Error: SCHED_FIFO pthread_attr_setschedparam\n";
            }

            if (pthread_create(&thread_info[i].thread_id, &pthread_attr, thread_func, &thread_info[i]) != 0) {
                cout << "Error: Thread " << thread_info[i].thread_num << "SCHED_FIFO pthread_create\n";
            }

        } 
        else if (thread_info[i].thread_sched_policy == SCHED_NORMAL) {
            if (pthread_create(&thread_info[i].thread_id, NULL, thread_func, &thread_info[i]) != 0){
                cout << "Error: Thread " << thread_info[i].thread_num << "SCHED_NORMAL pthread_create\n";
            }
        } 
        else {
            cout << "The policy supports only NORMAL and FIFO!\n";
        }
    }

    /* 5. Start all threads at once */
    pthread_barrier_wait(&barrier);

    /* 6. Wait for all threads to finish  */ 
    for (int i = 0; i < num_threads; i++) {
        pthread_join(thread_info[i].thread_id, NULL);
    }
    // cout << "thread finish" << "\n";
    pthread_barrier_destroy(&barrier);

    return 0;
}
