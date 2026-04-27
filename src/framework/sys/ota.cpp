
#include "ota.h"
#include "config.h"
#include <Update.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "system_info.h"
#include "../app/application.h"

#define TAG "ota"

bool Ota::CheckNewVersion() {
    Log::Info(TAG, "Check new version.");

    Application& app = Application::GetInstance();

    char buf[128] = {0};
    snprintf(buf, sizeof(buf)-1, "%s/product/checkversion?model=%s&chipid=%s&&version=%s",
        YUNLC_API_BASE, PRODUCT_MODEL, SystemInfo::GetChipId().c_str(), app.GetAppVersion().c_str());
    String url = String(buf);

    HTTPClient http;
    WiFiClientSecure* client = new WiFiClientSecure();
    client->setInsecure(); // Use root CA bundle in production
    
    int timeout = 10000;
    http.setTimeout(timeout);
    http.setConnectTimeout(timeout);

    if (!http.begin(*client, url)) {
        Log::Error(TAG, __LINE__, "httpclient begin failure.");
        delete client;
        return false;
    }

    http.addHeader("Authorization", String("Bearer ")+PRODUCT_TOKEN);
    http.addHeader("Content-Type", "application/json; charset=utf-8");

    String result = "";
    uint8_t retry_count = 0;
    uint8_t retry_times = 3;
    while (1) 
    {
        int httpCode = http.GET();
       
        if (httpCode > 0) {
            result = http.getString();
            break;  // jump out.
        } else {
            Log::Error(TAG, __LINE__, "GET %s error: %d", url.c_str(), httpCode);
            if (retry_count++ > retry_times) { 
                break; //jump out.
            }

            delay(1000 * retry_count);
            Log::Info(TAG, "trying GET again...");
        }
    }

    http.end();
    
    if (result.isEmpty()) {
        // 检查失败
        Log::Warn(TAG, "response no content.");
        return false;
    }

    JsonDocument doc;
    deserializeJson(doc, result);
    
    int errCode = doc["errCode"];
    if (errCode<0) {
        // show msg.
        const char *msg = doc["errMsg"] | "";
        Log::Warn(TAG, "response message: %s", msg);
        return false;
    }

    if (errCode==0) {
        // 无新版本
        Log::Info(TAG, "No new version.");
        return true;
    }

    if (errCode==1) {
        // 有新版本
        new_version_ = String(doc["data"]["newVersion"]);
        file_url_ = String(doc["data"]["fileUrl"]);
        file_size_ = doc["data"]["fileSize"];
        file_md5_ = String(doc["data"]["fileMD5"]);
        
        Log::Info(TAG, "Found new version: %s", new_version_.c_str());
        return true;
    }

    return false;
}
    
bool Ota::Upgrade() {
    if (new_version_.isEmpty() || file_url_.isEmpty()) {
        return false;
    }

    Log::Info(TAG, "Found new version. Upgrading...");

    HTTPClient http;
    WiFiClientSecure* client = new WiFiClientSecure();
    client->setInsecure(); // Use root CA bundle in production
    
    int timeout = 15000;
    http.setTimeout(timeout);
    http.setConnectTimeout(timeout);

    if (!http.begin(*client, file_url_)) {
        Log::Error(TAG, __LINE__, "httpclient begin failure.");
        return "";
    }

    http.addHeader("Authorization", String("Bearer ")+PRODUCT_TOKEN);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Log::Error(TAG, "File request failed, status: %d", httpCode);
        return false;
    }

    uint32_t file_size = http.getSize();

    // 1.开始更新
    Update.begin(file_size);
    if (md5_verify_) {
        Update.setMD5(file_md5_.c_str());
    }

    int len = file_size;
    uint8_t buff[1024] = {0};

    NetworkClient *stream = http.getStreamPtr();

    // 2.写入数据
    while (http.connected() && (len > 0 || len == -1)) {
        size_t size = stream->available();

        if (size) {
            int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
            
            Update.write(buff, c);

            if (len > 0) {
                len -= c;
            }
        }
        delay(1);
    }

    // 3.校验写入
    if (Update.size() == file_size) {
        if (Update.end(true)) {
            Log::Info(TAG, "Upgrade completed.");
            return true;
        } else {
            Log::Error(TAG, "Upgrade failed: %s", Update.errorString());
        }
    } else {
        Log::Error(TAG, "File download failed.");
    }

    return false;
}
