/*
 * MimiClaw - Cron Scheduler
 */
#ifndef MIMI_CRON_H
#define MIMI_CRON_H

#include <Arduino.h>
#include "mimi_bus.h"

enum CronKind {
    CRON_KIND_EVERY = 0,
    CRON_KIND_AT    = 1,
};

struct CronJob {
    char id[9];
    char name[32];
    bool enabled;
    CronKind kind;
    uint32_t interval_s;
    int64_t at_epoch;
    char message[256];
    char channel[16];
    char chat_id[32];
    int64_t last_run;
    int64_t next_run;
    bool delete_after_run;
};

class MimiCron {
public:
    MimiCron();
    bool begin(MimiBus* bus);
    bool start();
    void stop();

    bool addJob(CronJob* job);
    bool removeJob(const char* jobId);
    void listJobs(const CronJob** jobs, int* count);

    static const int MAX_JOBS = 16;

private:
    CronJob _jobs[MAX_JOBS];
    int _jobCount;
    MimiBus* _bus;
    TaskHandle_t _taskHandle;

    bool loadJobs();
    bool saveJobs();
    void processDueJobs();
    void generateId(char* buf);
    bool sanitizeDestination(CronJob* job);
    void computeInitialNextRun(CronJob* job);

    static void cronTask(void* arg);
};

#endif // MIMI_CRON_H
