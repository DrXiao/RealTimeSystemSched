#ifndef __TASK_H__
#define __TASK_H__
#include "list.h"
#define DEBUG 0
#define MAX_TASK 16

extern int clock;

struct task{
    int tid;
    int phase;
    int period;
    int WCET;
    int relative_deadline;
    // double utilization;
};

struct job {
    int release_time;
    int remain_exec_time;
    int abs_deadlne;
    int tid;
};

struct job_queue_node {
    struct job job;
    struct list_head link;
};

int init_tasks_info(char *);

void sched_ability_test(int (*)(struct job *, struct job *));

void check_jobs_miss_deadline(struct list_head *);

void check_new_jobs_release(struct list_head *);

void execute_job(struct list_head *, int (*)(struct job *, struct job *));

void job_queue_add_tail(struct list_head *, struct job *);

void destroy_job_queue(struct list_head *);

int rm_cmp_jobs(struct job *, struct job *);

int edf_cmp_jobs(struct job *, struct job *);

int strict_lst_cmp_jobs(struct job *, struct job *);

#endif
