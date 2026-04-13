/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_FS==1

#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

#include <FS.h>
#include <functional>
#include <string>

enum FileAction {
    Created,
    Updated,
    Deleted,
};

/**
 * 文件系统类
 */
class FileSystem {
public:
    FileSystem(fs::FS& fs) : fs_(fs) { }
    ~FileSystem() { }

    bool CreateDir(const char *path);
    bool RemoveDir(const char *path);

    bool ExistsFile(const char *path); 
    File OpenFile(const char *path, const char *mode = FILE_READ, const bool create = false);
    bool WriteFile(const char *path, const char *content);
    bool AppendFile(const char *path, const char *content);
    bool RenameFile(const char *path, const char *new_path);
    bool DeleteFile(const char *path); 

    void setTotalBytes(uint32_t bytes) { total_bytes_ = bytes; }
    uint32_t totalBytes() const { return total_bytes_; }
    
    void setFreeBytes(uint32_t bytes) { free_bytes_ = bytes; }
    uint32_t freeBytes() const { return free_bytes_; }

    void setType(const std::string& type) { type_ = type; }
    const std::string& type() const { return type_; }

    void SetActionListener(std::function<void(FileAction action, const std::string&)> listener)
    {
        listener_ = listener;
    }

private:
    fs::FS& fs_;
    uint32_t total_bytes_;
    uint32_t free_bytes_;
    std::string type_;

    std::function<void(FileAction action, const std::string&)> listener_;
};

#endif //_FILE_SYSTEM_H

#endif //CONFIG_USE_FS