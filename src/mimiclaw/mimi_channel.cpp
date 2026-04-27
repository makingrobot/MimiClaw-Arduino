
#include "mimi_channel.h"
#include "mimi_config.h"

#define TAG "channel"

bool MimiChannel::onMessage(const char* chat_id, const char* text) {
    MimiMsg msg;
    memset(&msg, 0, sizeof(msg));
    strncpy(msg.channel, name().c_str(), sizeof(msg.channel) - 1);
    strncpy(msg.chat_id, chat_id, sizeof(msg.chat_id) - 1);
    msg.content = strdup(text);
    if (msg.content && _bus) {
        // Push to inbound bus
        if (!_bus->pushInbound(&msg)) {
            MIMI_LOGE(TAG, __LINE__, "Inbound queue full, dropping message");
            free(msg.content);
            return false;
        }
        return true;
    }
    return false;
}

/******************* MimiLocal ************************/

MimiLocal::MimiLocal() {

}

bool MimiLocal::sendMessage(const char* chat_id, const char* text)
{
    MIMI_LOGI(TAG, "System message [%s]: %.128s", chat_id, text);
}