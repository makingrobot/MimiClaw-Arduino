#include "mimi_heartbeat.h"
#include "mimi_config.h"
#include <SPIFFS.h>

static const char* TAG MIMI_TAG_UNUSED = "heartbeat";

#define HEARTBEAT_PROMPT \
    "Read " MIMI_HEARTBEAT_FILE " and follow any instructions or tasks listed there. " \
    "If nothing needs attention, reply with just: HEARTBEAT_OK"

static MimiHeartbeat* g_hb = nullptr;

MimiHeartbeat::MimiHeartbeat() : _bus(nullptr), _timer(nullptr) {
    g_hb = this;
}

bool MimiHeartbeat::begin(MimiBus* bus) {
    _bus = bus;
    MIMI_LOGI(TAG, "Heartbeat initialized (file: %s, interval: %ds)",
              MIMI_HEARTBEAT_FILE, MIMI_HEARTBEAT_INTERVAL_MS / 1000);
    return true;
}

bool MimiHeartbeat::hasTasks() {
    File f = SPIFFS.open(MIMI_HEARTBEAT_FILE, "r");
    if (!f) return false;

    bool found = false;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        if (line.startsWith("#")) continue;

        // Skip completed checkboxes: "- [x]" or "* [x]"
        if ((line.startsWith("- [x]") || line.startsWith("- [X]") ||
             line.startsWith("* [x]") || line.startsWith("* [X]"))) {
            continue;
        }

        found = true;
        break;
    }

    f.close();
    return found;
}

bool MimiHeartbeat::sendHeartbeat() {
    if (!hasTasks()) {
        MIMI_LOGD(TAG, "No actionable tasks in HEARTBEAT.md");
        return false;
    }

    MimiMsg msg;
    memset(&msg, 0, sizeof(msg));
    strncpy(msg.channel, MIMI_CHAN_SYSTEM, sizeof(msg.channel) - 1);
    strncpy(msg.chat_id, "heartbeat", sizeof(msg.chat_id) - 1);
    msg.content = strdup(HEARTBEAT_PROMPT);

    if (!msg.content) {
        MIMI_LOGE(TAG, "Failed to allocate heartbeat prompt");
        return false;
    }

    if (!_bus->pushInbound(&msg)) {
        MIMI_LOGW(TAG, "Failed to push heartbeat message");
        free(msg.content);
        return false;
    }

    MIMI_LOGI(TAG, "Triggered agent check");
    return true;
}

void MimiHeartbeat::timerCallback(TimerHandle_t xTimer) {
    if (g_hb) g_hb->sendHeartbeat();
}

bool MimiHeartbeat::start() {
    if (_timer) {
        MIMI_LOGW(TAG, "Heartbeat timer already running");
        return true;
    }

    _timer = xTimerCreate("heartbeat",
                          pdMS_TO_TICKS(MIMI_HEARTBEAT_INTERVAL_MS),
                          pdTRUE, nullptr, timerCallback);
    if (!_timer) {
        MIMI_LOGE(TAG, "Failed to create heartbeat timer");
        return false;
    }

    if (xTimerStart(_timer, pdMS_TO_TICKS(1000)) != pdPASS) {
        MIMI_LOGE(TAG, "Failed to start heartbeat timer");
        return false;
    }

    MIMI_LOGI(TAG, "Heartbeat started (every %d min)", MIMI_HEARTBEAT_INTERVAL_MS / 60000);
    return true;
}

void MimiHeartbeat::stop() {
    if (_timer) {
        xTimerStop(_timer, pdMS_TO_TICKS(1000));
        xTimerDelete(_timer, pdMS_TO_TICKS(1000));
        _timer = nullptr;
        MIMI_LOGI(TAG, "Heartbeat stopped");
    }
}

bool MimiHeartbeat::trigger() {
    return sendHeartbeat();
}
