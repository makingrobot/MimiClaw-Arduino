#include "mimi_cron.h"
#include "mimi_config.h"
#include "mimi_tools.h"
#include <ArduinoJson.h>
#include <time.h>
#include <esp_random.h>
#include "arduino_json_psram.h"
#include "mimi_channel.h"
#include "mimi_application.h"

#define TAG  "cron"

// Global cron instance pointer for tool callbacks
static MimiCron* g_cron = nullptr;

bool tool_cron_add_execute(const char* input_json, char* output, size_t output_size);
bool tool_cron_list_execute(const char* input_json, char* output, size_t output_size);
bool tool_cron_remove_execute(const char* input_json, char* output, size_t output_size);

MimiCron::MimiCron() : _jobCount(0), _bus(nullptr), _taskHandle(nullptr) {
    g_cron = this;
}

bool MimiCron::begin(MimiBus* bus, FileSystem *file_system) {
    _bus = bus;
    _file_system = file_system;

    // cron_add
    static const MimiTool ca = {
        "cron_add",
        "Schedule a recurring or one-shot task. The message will trigger an agent turn when the job fires.",
        "{\"type\":\"object\",\"properties\":{\"name\":{\"type\":\"string\",\"description\":\"Short name for the job\"},\"schedule_type\":{\"type\":\"string\",\"description\":\"'every' for recurring or 'at' for one-shot\"},\"interval_s\":{\"type\":\"integer\",\"description\":\"Interval in seconds (for 'every')\"},\"at_epoch\":{\"type\":\"integer\",\"description\":\"Unix timestamp (for 'at')\"},\"message\":{\"type\":\"string\",\"description\":\"Message to inject when the job fires\"},\"channel\":{\"type\":\"string\",\"description\":\"Reply channel\"},\"chat_id\":{\"type\":\"string\",\"description\":\"Reply chat_id\"}},\"required\":[\"name\",\"schedule_type\",\"message\"]}",
        tool_cron_add_execute
    };
    _tools.push_back(&ca);

    // cron_list
    static const MimiTool cl = {
        "cron_list",
        "List all scheduled cron jobs with their status, schedule, and IDs.",
        "{\"type\":\"object\",\"properties\":{},\"required\":[]}",
        tool_cron_list_execute
    };
    _tools.push_back(&cl);

    // cron_remove
    static const MimiTool cr = {
        "cron_remove",
        "Remove a scheduled cron job by its ID.",
        "{\"type\":\"object\",\"properties\":{\"job_id\":{\"type\":\"string\",\"description\":\"The 8-character job ID to remove\"}},\"required\":[\"job_id\"]}",
        tool_cron_remove_execute
    };
    _tools.push_back(&cr);
    
    return loadJobs();
}

void MimiCron::generateId(char* buf) {
    uint32_t r = esp_random();
    snprintf(buf, 9, "%08x", (unsigned int)r);
}

bool MimiCron::sanitizeDestination(CronJob* job) {
    bool changed = false;
    if (job->channel[0] == '\0') {
        strncpy(job->channel, MIMI_CHAN_SYSTEM, sizeof(job->channel) - 1);
        changed = true;
    }
    MimiApplication *app = (MimiApplication*)(&Application::GetInstance());
    MimiChannel *ch = app->findChannel(job->channel);
    if (ch) {
        if (job->chat_id[0] == '\0' || strcmp(job->chat_id, "cron") == 0) {
            MIMI_LOGW(TAG, "Cron job %s has invalid %s chat_id, fallback to system:cron", job->id, job->channel);
            strncpy(job->channel, MIMI_CHAN_SYSTEM, sizeof(job->channel) - 1);
            strncpy(job->chat_id, "cron", sizeof(job->chat_id) - 1);
            changed = true;
        }
    } else if (job->chat_id[0] == '\0') {
        strncpy(job->chat_id, "cron", sizeof(job->chat_id) - 1);
        changed = true;
    }
    return changed;
}

bool MimiCron::loadJobs() {
    File f = _file_system->OpenFile(MIMI_CRON_FILE, FILE_READ);
    if (!f) {
        MIMI_LOGI(TAG, "No cron file found, starting fresh");
        _jobCount = 0;
        return true;
    }

    String content = f.readString();
    f.close();

    JsonDocument doc(&spiram_allocator);
    if (deserializeJson(doc, content)) {
        MIMI_LOGW(TAG, "Failed to parse cron JSON file %s", MIMI_CRON_FILE);
        _jobCount = 0;
        return true;
    }

    JsonArray jobsArr = doc["jobs"].as<JsonArray>();
    if (jobsArr.isNull()) {
        _jobCount = 0;
        return true;
    }

    _jobCount = 0;
    bool repaired = false;
    for (JsonVariant item : jobsArr) {
        if (_jobCount >= MAX_JOBS) break;

        CronJob* job = &_jobs[_jobCount];
        memset(job, 0, sizeof(CronJob));

        strncpy(job->id, item["id"] | "", sizeof(job->id) - 1);
        strncpy(job->name, item["name"] | "", sizeof(job->name) - 1);
        strncpy(job->message, item["message"] | "", sizeof(job->message) - 1);
        strncpy(job->channel, item["channel"] | MIMI_CHAN_SYSTEM, sizeof(job->channel) - 1);
        strncpy(job->chat_id, item["chat_id"] | "cron", sizeof(job->chat_id) - 1);

        job->enabled = item["enabled"] | true;
        job->delete_after_run = item["delete_after_run"] | false;

        const char* kindStr = item["kind"] | "";
        if (strcmp(kindStr, "every") == 0) {
            job->kind = CRON_KIND_EVERY;
            job->interval_s = item["interval_s"] | 0;
        } else if (strcmp(kindStr, "at") == 0) {
            job->kind = CRON_KIND_AT;
            job->at_epoch = (int64_t)(item["at_epoch"] | 0.0);
        } else continue;

        job->last_run = (int64_t)(item["last_run"] | 0.0);
        job->next_run = (int64_t)(item["next_run"] | 0.0);

        if (sanitizeDestination(job)) repaired = true;
        _jobCount++;
    }

    if (repaired) saveJobs();
    MIMI_LOGI(TAG, "Loaded %d cron jobs", _jobCount);
    return true;
}

bool MimiCron::saveJobs() {
    JsonDocument doc(&spiram_allocator);
    JsonArray jobsArr = doc["jobs"].to<JsonArray>();

    for (int i = 0; i < _jobCount; i++) {
        CronJob* job = &_jobs[i];
        JsonObject item = jobsArr.add<JsonObject>();
        item["id"] = job->id;
        item["name"] = job->name;
        item["enabled"] = job->enabled;
        item["kind"] = (job->kind == CRON_KIND_EVERY) ? "every" : "at";
        if (job->kind == CRON_KIND_EVERY) {
            item["interval_s"] = job->interval_s;
        } else {
            item["at_epoch"] = (double)job->at_epoch;
        }
        item["message"] = job->message;
        item["channel"] = job->channel;
        item["chat_id"] = job->chat_id;
        item["last_run"] = (double)job->last_run;
        item["next_run"] = (double)job->next_run;
        item["delete_after_run"] = job->delete_after_run;
    }

    File f = _file_system->OpenFile(MIMI_CRON_FILE, FILE_WRITE);
    if (!f) {
        MIMI_LOGE(TAG, __LINE__, "Failed to open %s for writing", MIMI_CRON_FILE);
        return false;
    }

    serializeJsonPretty(doc, f);
    f.close();
    MIMI_LOGI(TAG, "Cron file %s saved %d cron jobs", MIMI_CRON_FILE, _jobCount);  /* file: /cron.json */
    return true;
}

void MimiCron::processDueJobs() {
    time_t now = time(NULL);
    bool changed = false;

    for (int i = 0; i < _jobCount; i++) {
        CronJob* job = &_jobs[i];
        if (!job->enabled || job->next_run <= 0 || job->next_run > now) continue;

        MIMI_LOGI(TAG, "Cron job firing: %s (%s)", job->name, job->id);

        MimiMsg msg;
        memset(&msg, 0, sizeof(msg));
        strncpy(msg.channel, job->channel, sizeof(msg.channel) - 1);
        strncpy(msg.chat_id, job->chat_id, sizeof(msg.chat_id) - 1);
        msg.content = strdup(job->message);

        if (msg.content && _bus) {
            if (!_bus->pushInbound(&msg)) {
                free(msg.content);
            }
        }

        job->last_run = now;

        if (job->kind == CRON_KIND_AT) {
            if (job->delete_after_run) {
                MIMI_LOGI(TAG, "Deleting one-shot job: %s", job->name);
                for (int j = i; j < _jobCount - 1; j++) {
                    _jobs[j] = _jobs[j + 1];
                }
                _jobCount--;
                i--;
            } else {
                job->enabled = false;
                job->next_run = 0;
            }
        } else {
            job->next_run = now + job->interval_s;
        }
        changed = true;
    }

    if (changed) saveJobs();
}

void MimiCron::computeInitialNextRun(CronJob* job) {
    time_t now = time(NULL);
    if (job->kind == CRON_KIND_EVERY) {
        job->next_run = now + job->interval_s;
    } else if (job->kind == CRON_KIND_AT) {
        if (job->at_epoch > now) {
            job->next_run = job->at_epoch;
        } else {
            job->next_run = 0;
            job->enabled = false;
        }
    }
}

void MimiCron::cronTask(void* arg) {
    MimiCron* self = (MimiCron*)arg;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(MIMI_CRON_CHECK_INTERVAL_MS));
        self->processDueJobs();
    }
}

bool MimiCron::start() {
    if (_taskHandle) return true;

    // Recompute next_run for enabled jobs
    time_t now = time(NULL);
    for (int i = 0; i < _jobCount; i++) {
        CronJob* job = &_jobs[i];
        if (job->enabled && job->next_run <= 0) {
            if (job->kind == CRON_KIND_EVERY)
                job->next_run = now + job->interval_s;
            else if (job->kind == CRON_KIND_AT && job->at_epoch > now)
                job->next_run = job->at_epoch;
        }
    }

    BaseType_t ok = xTaskCreate(cronTask, "cron", 4096, this, 4, &_taskHandle);
    if (ok != pdPASS) {
        MIMI_LOGE(TAG, __LINE__, "Failed to create cron task");
        return false;
    }

    MIMI_LOGI(TAG, "Cron service started (%d jobs, check every %ds)",
              _jobCount, MIMI_CRON_CHECK_INTERVAL_MS / 1000);
    return true;
}

void MimiCron::stop() {
    if (_taskHandle) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
        MIMI_LOGI(TAG, "Cron service stopped");
    }
}

bool MimiCron::addJob(CronJob* job) {
    if (_jobCount >= MAX_JOBS) return false;
    generateId(job->id);
    sanitizeDestination(job);
    job->enabled = true;
    job->last_run = 0;
    computeInitialNextRun(job);
    _jobs[_jobCount++] = *job;
    saveJobs();
    MIMI_LOGI(TAG, "Added cron job: %s (%s)", job->name, job->id);
    return true;
}

bool MimiCron::removeJob(const char* jobId) {
    for (int i = 0; i < _jobCount; i++) {
        if (strcmp(_jobs[i].id, jobId) == 0) {
            MIMI_LOGI(TAG, "Removing cron job: %s (%s)", _jobs[i].name, jobId);
            for (int j = i; j < _jobCount - 1; j++) _jobs[j] = _jobs[j + 1];
            _jobCount--;
            saveJobs();
            return true;
        }
    }
    return false;
}

void MimiCron::listJobs(const CronJob** jobs, int* count) {
    *jobs = _jobs;
    *count = _jobCount;
}


// ── Tool callbacks (use global cron instance) ──────────────────

bool tool_cron_add_execute(const char* input_json, char* output, size_t output_size) {
    if (!g_cron) {
        snprintf(output, output_size, "Error: Cron service not initialized");
        return false;
    }

    JsonDocument doc(&spiram_allocator);
    if (deserializeJson(doc, input_json)) {
        snprintf(output, output_size, "Error: invalid JSON input");
        return false;
    }

    const char* name = doc["name"] | "";
    const char* schedType = doc["schedule_type"] | "";
    const char* message = doc["message"] | "";

    if (!name[0] || !schedType[0] || !message[0]) {
        snprintf(output, output_size, "Error: missing required fields");
        return false;
    }

    CronJob job;
    memset(&job, 0, sizeof(job));
    strncpy(job.name, name, sizeof(job.name) - 1);
    strncpy(job.message, message, sizeof(job.message) - 1);

    const char* channel = doc["channel"] | "";
    const char* chatId = doc["chat_id"] | "";
    if (channel[0]) strncpy(job.channel, channel, sizeof(job.channel) - 1);
    if (chatId[0]) strncpy(job.chat_id, chatId, sizeof(job.chat_id) - 1);

    if (strcmp(job.channel, MIMI_CHAN_TELEGRAM) == 0 &&
        (job.chat_id[0] == '\0' || strcmp(job.chat_id, "cron") == 0)) {
        snprintf(output, output_size, "Error: channel='telegram' requires a valid chat_id");
        return false;
    } else if(strcmp(job.channel, MIMI_CHAN_FEISHU) == 0 &&
        (job.chat_id[0] == '\0' || strcmp(job.chat_id, "cron") == 0)) {
        snprintf(output, output_size, "Error: channel='feishu' requires a valid chat_id");
        return false;
    }

    if (strcmp(schedType, "every") == 0) {
        job.kind = CRON_KIND_EVERY;
        int interval = doc["interval_s"] | 0;
        if (interval <= 0) {
            snprintf(output, output_size, "Error: 'every' requires positive interval_s");
            return false;
        }
        job.interval_s = (uint32_t)interval;
    } else if (strcmp(schedType, "at") == 0) {
        job.kind = CRON_KIND_AT;
        int64_t atEpoch = (int64_t)(doc["at_epoch"] | 0.0);
        if (atEpoch <= time(NULL)) {
            snprintf(output, output_size, "Error: at_epoch is in the past");
            return false;
        }
        job.at_epoch = atEpoch;
        job.delete_after_run = doc["delete_after_run"] | true;
    } else {
        snprintf(output, output_size, "Error: schedule_type must be 'every' or 'at'");
        return false;
    }

    if (!g_cron->addJob(&job)) {
        snprintf(output, output_size, "Error: failed to add job (max reached?)");
        return false;
    }

    if (job.kind == CRON_KIND_EVERY) {
        snprintf(output, output_size, "OK: Added recurring job '%s' (id=%s), every %lu sec",
                 job.name, job.id, (unsigned long)job.interval_s);
    } else {
        snprintf(output, output_size, "OK: Added one-shot job '%s' (id=%s), at epoch %lld",
                 job.name, job.id, (long long)job.at_epoch);
    }
    return true;
}

bool tool_cron_list_execute(const char* input_json, char* output, size_t output_size) {
    if (!g_cron) {
        snprintf(output, output_size, "Error: Cron service not initialized");
        return false;
    }

    const CronJob* jobs;
    int count;
    g_cron->listJobs(&jobs, &count);

    if (count == 0) {
        snprintf(output, output_size, "No cron jobs scheduled.");
        return true;
    }

    size_t off = snprintf(output, output_size, "Scheduled jobs (%d):\n", count);
    for (int i = 0; i < count && off < output_size - 1; i++) {
        const CronJob* j = &jobs[i];
        if (j->kind == CRON_KIND_EVERY) {
            off += snprintf(output + off, output_size - off,
                "  %d. [%s] \"%s\" — every %lus, %s, ch=%s:%s\n",
                i + 1, j->id, j->name, (unsigned long)j->interval_s,
                j->enabled ? "enabled" : "disabled", j->channel, j->chat_id);
        } else {
            off += snprintf(output + off, output_size - off,
                "  %d. [%s] \"%s\" — at %lld, %s, ch=%s:%s%s\n",
                i + 1, j->id, j->name, (long long)j->at_epoch,
                j->enabled ? "enabled" : "disabled", j->channel, j->chat_id,
                j->delete_after_run ? " (auto-delete)" : "");
        }
    }
    return true;
}

bool tool_cron_remove_execute(const char* input_json, char* output, size_t output_size) {
    if (!g_cron) {
        snprintf(output, output_size, "Error: Cron service not initialized");
        return false;
    }

    JsonDocument doc(&spiram_allocator);
    if (deserializeJson(doc, input_json)) {
        snprintf(output, output_size, "Error: invalid JSON input");
        return false;
    }

    const char* jobId = doc["job_id"] | "";
    if (!jobId[0]) {
        snprintf(output, output_size, "Error: missing job_id");
        return false;
    }

    if (g_cron->removeJob(jobId)) {
        snprintf(output, output_size, "OK: Removed cron job %s", jobId);
        return true;
    } else {
        snprintf(output, output_size, "Error: job '%s' not found", jobId);
        return false;
    }
}
