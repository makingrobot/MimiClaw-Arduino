
#include "ota.h"
#include "config.h"
#include <Update.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "../app/application.h"

bool Ota::CheckNewVersion() {
    Application& app = Application::GetInstance();

    char buf[128] = {0};
    snprintf("%s/product/checkversion?model=%s&version=%s",
        YUNLC_API_BASE, PRODUCT_MODEL, app.GetAppVersion().c_str());
    String url = String(buf);

    HTTPClient http;
    WiFiClientSecure* client = new WiFiClientSecure();
    client->setInsecure(); // Use root CA bundle in production
    
    int timeout = 15000;
    http.setTimeout(timeout);
    http.setConnectTimeout(timeout);

    if (!http.begin(*client, url)) {
        Log::Error(TAG, __LINE__, "httpclient begin failure.");
        delete client;
        return "";
    }

    http.addHeader("Authorization", String("Bearer ")+PRODUCT_ACCESS_TOKEN);

    String result = "";
    uint8_t retry_count = 0;
    uint8_t retry_times = 3;
    while (1) 
    {
        http.addHeader("Content-Type", "application/json; charset=utf-8");
        int httpCode = http.GET();
       
        if (httpCode > 0) {
            result = http.getString();
            break;  // jump out.
        } else {
            Log::Error(TAG, __LINE__, "HTTP error: %d", httpCode);
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
        return false;
    }

    JsonDocument doc;
    deserializeJson(doc, result);
    
    int errCode = doc["errCode"];
    if (errCode<0) {
        // show msg.
        return false;
    }

    if (errCode==0) {
        // 无新版本
        return true;
    }

    if (errCode==1) {
        // 有新版本
        new_version_ = String(doc["data"]["newVersion"]);
        file_url_ = String(doc["data"]["fileUrl"]);
        file_size_ = doc["data"]["fileSize"];
        file_md5_ = String(doc["data"]["fileMD5"]);
        return true;
    }

    return false;
}
    
bool Ota::Upgrade() {
    if (new_version_.isEmpty() || file_url_.isEmpty()) {
        return false;
    }

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

    http.addHeader("Authorization", String("Bearer ")+PRODUCT_ACCESS_TOKEN);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        uint32_t file_size = http.getSize();

        Update.begin(file_size);
        Update.setMD5(file_md5_.c_str());

        int len = file_size;
        uint8_t buff[1024] = {0};

        NetworkClient *stream = http.getStreamPtr();

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

        if (Update.totalSize == file_size) {
            if (update.end(true)) {
                Log::Info(TAG, "Upgrade completed.");
                return true;
            } else {
                Log::Error(TAG, "Upgrade failed: %s", Update.errorString());
            }
        } else {
            Log::Error(TAG, "File download failed.");
        }
    }

    return false;
}
