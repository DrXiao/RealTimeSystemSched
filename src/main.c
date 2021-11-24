#include <stdio.h>
#include <stdlib.h>
#include "task.h"

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("./main input_file.txt\n");
        exit(0);
    }

    char *input_task = argv[1];
    

    int (*algo[3])(struct job *, struct job *) = {
        rm_cmp_jobs, edf_cmp_jobs, strict_lst_cmp_jobs
    };

    char *algo_name[3] = {
        "Rate Monotonic", "Earliest Deadline First", 
        "strict LST"
    };


    for(int algo_method_idx = 0; algo_method_idx < 3; algo_method_idx++) {
        clock = 0;
        printf("** %s **\n", algo_name[algo_method_idx]);
        fprintf(stderr, "** %s **\n", algo_name[algo_method_idx]);
        int max_clock = init_tasks_info(input_task);
        struct list_head job_queue = INIT_LIST_HEAD(job_queue);
        sched_ability_test(algo[algo_method_idx]);
        while(clock < max_clock) {
            check_jobs_miss_deadline(&job_queue);
            check_new_jobs_release(&job_queue);
            execute_job(&job_queue, algo[algo_method_idx]);
            clock++;
        }
        printf("%d\n\n", clock);
        fprintf(stderr, "\n");
        destroy_job_queue(&job_queue);
    }

    

    return 0;
}