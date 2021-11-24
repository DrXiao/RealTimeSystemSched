#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include "task.h"
#include "list.h"
#include <math.h>

static int total_completed_job_num = 0;
static int miss_deadline_job_num = 0;
static struct task tasks[MAX_TASK] = {0};
static int num_of_tasks = 0;
static bool has_new_job = false;
static struct job_queue_node *exec_job = NULL;
static bool empty = false;
int clock = 0;

int gcd(int a, int b) {
    while (a % b) {
        int temp = a % b;
        a = b;
        b = temp;
    }
    return b;
}

int init_tasks_info(char *filename) {
    FILE *input = fopen(filename, "r");
    if (input == NULL) {
        printf("File %s Not Found\n", filename);
        exit(0);
    }
    miss_deadline_job_num = 0;
    total_completed_job_num = 0;
    num_of_tasks = 0;
    while (fscanf(input, "%d, %d, %d, %d", &tasks[num_of_tasks].phase,
                  &tasks[num_of_tasks].period,
                  &tasks[num_of_tasks].relative_deadline,
                  &tasks[num_of_tasks].WCET) != EOF) {
        tasks[num_of_tasks].tid = num_of_tasks + 1;
        num_of_tasks++;
    }
    fclose(input);
#if DEBUG == 1
    for (int i = 0; i < num_of_tasks; i++) {
        printf("tid-%d: %d, %d, %d, %d\n", tasks[i].tid, tasks[i].phase,
               tasks[i].period, tasks[i].relative_deadline, tasks[i].WCET);
    }
#endif
    int max_phase = tasks[0].phase;
    int lcm = tasks[0].period;
    exec_job = NULL;
    empty = false;
    for (int task_idx = 1; task_idx < num_of_tasks; task_idx++) {
        lcm = lcm * tasks[task_idx].period / gcd(lcm, tasks[task_idx].period);
        max_phase = max_phase < tasks[task_idx].phase ? tasks[task_idx].phase
                                                      : max_phase;
    }
    return lcm + max_phase;
}

void sched_ability_test(int (*sched_algo)(struct job *, struct job *)) {
    double sched_ability = 0;
    if (sched_algo == edf_cmp_jobs) {
        for (int task_idx = 0; task_idx < num_of_tasks; task_idx++) {
            sched_ability +=
                (double)tasks[task_idx].WCET /
                ((double)(tasks[task_idx].period <
                                  tasks[task_idx].relative_deadline
                              ? tasks[task_idx].period
                              : tasks[task_idx].relative_deadline));
        }
        printf("Schedulability test for RM: %lf, sched_num: %d\n",
               sched_ability, 1);
        printf("Schedulable: %s\n", sched_ability <= 1 ? "true" : "false");
    }
    else if (sched_algo == rm_cmp_jobs) {
        for (int task_idx = 0; task_idx < num_of_tasks; task_idx++) {
            sched_ability +=
                (double)tasks[task_idx].WCET /
                ((double)(tasks[task_idx].period <
                                  tasks[task_idx].relative_deadline
                              ? tasks[task_idx].period
                              : tasks[task_idx].relative_deadline));
        }
        double sched_num =
            (double)num_of_tasks * (pow(2.0, 1.0 / num_of_tasks) - 1);
        printf("Schedulability test for EDF: %lf, sched_num: %lf\n",
               sched_ability, sched_num);
        printf("Schedulable: %s\n",
               sched_ability <= (sched_num) ? "true" : "false");
    }
    else
        return;
}

void check_jobs_miss_deadline(struct list_head *job_queue) {
    struct list_head *cur = NULL;
    struct job_queue_node *accesser = NULL;
    for (cur = job_queue->next; cur != job_queue; cur = cur->next) {
        accesser = (struct job_queue_node *)(CONTAINER_OF(
            cur, struct job_queue_node, link));
        int abs_deadline = accesser->job.abs_deadlne;
        int remain_exec_time = accesser->job.remain_exec_time;
        if (abs_deadline - clock - remain_exec_time < 0) {
            fprintf(stderr,
                    "At clock %d: Job %d misses deadline (abs_deadline: %d, "
                    "remain_exec_time: %d)\n",
                    clock, accesser->job.tid, accesser->job.abs_deadlne,
                    accesser->job.remain_exec_time);
            miss_deadline_job_num += 1;
            cur = cur->next;
            list_del(&accesser->link);
            free(accesser);
            cur = cur->prev;
        }
    }
}

void check_new_jobs_release(struct list_head *job_queue) {
    has_new_job = false;
    for (int task_idx = 0; task_idx < num_of_tasks; task_idx++) {
        if ((clock - tasks[task_idx].phase) >= 0 && (clock - tasks[task_idx].phase) % tasks[task_idx].period == 0) {
            struct job new_job = (struct job){
                .abs_deadlne = clock + tasks[task_idx].relative_deadline,
                .release_time = clock,
                .remain_exec_time = tasks[task_idx].WCET,
                .tid = tasks[task_idx].tid};
            job_queue_add_tail(job_queue, &new_job);
            has_new_job = true;
        }
    }
}

void execute_job(struct list_head *job_queue,
                 int (*sched_algo)(struct job *, struct job *)) {
    bool change_job = false;
    if (has_new_job || !exec_job) {
        struct list_head *cur = NULL;
        LIST_FOR_EACH(cur, job_queue) {
            struct job_queue_node *accesser =
                (struct job_queue_node *)(CONTAINER_OF(
                    cur, struct job_queue_node, link));
            if (!exec_job || sched_algo(&exec_job->job, &accesser->job) == 1) {
                exec_job = accesser;
                change_job = true;
            }
        }
    }
    if (change_job) {
        printf("%d T%d\n", clock, exec_job->job.tid);
    }
    if (exec_job) {
        exec_job->job.remain_exec_time -= 1;
        if (exec_job->job.remain_exec_time == 0) {
            list_del(&exec_job->link);
            free(exec_job);
            exec_job = NULL;
        }
        empty = false;
    }
    else if(!empty){
        printf("%d %p\n", clock, exec_job);
        empty = true;
    }
}

void job_queue_add_tail(struct list_head *job_queue, struct job *job) {
    struct list_head *cur = NULL;
    for (cur = job_queue; cur->next != job_queue; cur = cur->next)
        ;
    struct job_queue_node *new_job = calloc(1, sizeof(struct job_queue_node));
    new_job->job = *job;
    list_add_tail(&new_job->link, cur);
}

int rm_cmp_jobs(struct job *job1, struct job *job2) {
    if (!job1) {
        return 1;
    }
    if (!job2) {
        return -1;
    }
    int task1_idx = job1->tid - 1;
    int task2_idx = job2->tid - 1;
    if (tasks[task1_idx].period < tasks[task2_idx].period)
        return -1;
    else if (tasks[task2_idx].period == tasks[task2_idx].period &&
             job1->tid <= job2->tid)
        return -1;
    else
        return 1;
}

int edf_cmp_jobs(struct job *job1, struct job *job2) {
    if (!job1) {
        return 1;
    }
    if (!job2) {
        return -1;
    }
    if (job1->abs_deadlne < job2->abs_deadlne)
        return -1;
    else if (job1->abs_deadlne == job2->abs_deadlne && job1->tid <= job2->tid)
        return -1;
    else
        return 1;
}

int strict_lst_cmp_jobs(struct job *job1, struct job *job2) {
    if (!job1) {
        return 1;
    }
    if (!job2) {
        return -1;
    }
    int job1_slack = job1->abs_deadlne - clock - job1->remain_exec_time;
    int job2_slack = job2->abs_deadlne - clock - job2->remain_exec_time;

    if (job1_slack < job2_slack)
        return -1;
    else if (job1_slack == job2_slack && job1->tid <= job2->tid)
        return -1;
    else
        return 1;
}

void destroy_job_queue(struct list_head *job_queue) {
    struct list_head *cur = NULL;
    for (cur = job_queue->next; cur != job_queue; cur = job_queue->next) {
        struct job_queue_node *del_ptr = (struct job_queue_node *)(CONTAINER_OF(
            cur, struct job_queue_node, link));
        list_del(cur);
        free(del_ptr);
    }
}