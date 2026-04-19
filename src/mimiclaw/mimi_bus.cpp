#include "mimi_bus.h"
#include "mimi_config.h"

static const char* TAG MIMI_TAG_UNUSED = "bus";

MimiBus::MimiBus() : _inQueue(nullptr), _outQueue(nullptr) {}

bool MimiBus::begin() {
    _inQueue = xQueueCreate(MIMI_BUS_QUEUE_LEN, sizeof(MimiMsg));
    _outQueue = xQueueCreate(MIMI_BUS_QUEUE_LEN, sizeof(MimiMsg));
    if (!_inQueue || !_outQueue) {
        MIMI_LOGE(TAG, __LINE__, "Failed to create message queues");
        return false;
    }
    MIMI_LOGI(TAG, "Message bus initialized (depth %d)", MIMI_BUS_QUEUE_LEN);
    return true;
}

bool MimiBus::pushInbound(const MimiMsg* msg) {
    if (xQueueSend(_inQueue, msg, pdMS_TO_TICKS(1000)) != pdTRUE) {
        MIMI_LOGW(TAG, "Inbound queue full, dropping message");
        return false;
    }
    return true;
}

bool MimiBus::popInbound(MimiMsg* msg, uint32_t timeout_ms) {
    TickType_t ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xQueueReceive(_inQueue, msg, ticks) == pdTRUE;
}

bool MimiBus::pushOutbound(const MimiMsg* msg) {
    if (xQueueSend(_outQueue, msg, pdMS_TO_TICKS(1000)) != pdTRUE) {
        MIMI_LOGW(TAG, "Outbound queue full, dropping message");
        return false;
    }
    return true;
}

bool MimiBus::popOutbound(MimiMsg* msg, uint32_t timeout_ms) {
    TickType_t ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xQueueReceive(_outQueue, msg, ticks) == pdTRUE;
}
