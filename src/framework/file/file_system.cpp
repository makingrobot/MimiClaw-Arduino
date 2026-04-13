/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_FS==1

#include "file_system.h"
#include "../sys/log.h"

#define TAG "FileSystem"

bool FileSystem::CreateDir(const char *path) {
    bool ret = fs_.mkdir(path);
    if (!ret) {
        Log::Warn(TAG, "Dir %s create failed", path);
        return false;
    }
    if (listener_) {
        listener_(FileAction::Created, std::string(path));
    }
    return ret;
}

bool FileSystem::RemoveDir(const char *path) {
    bool ret = fs_.rmdir(path);
    if (!ret) {
        Log::Warn(TAG, "Dir %s remove failed", path);
    }
    return ret;
}

bool FileSystem::ExistsFile(const char *path) {
    return fs_.exists(path);
}

File FileSystem::OpenFile(const char *path,  const char *mode, const bool create) {
    return fs_.open(path, mode, create);
}

bool FileSystem::WriteFile(const char *path, const char *content) {
    File file = fs_.open(path, FILE_WRITE);
    if (!file) {
        Log::Warn(TAG, "File %s to open failed", path);
        return false;
    }

    file.print(content);
    
    if (listener_) {
        listener_(FileAction::Updated, std::string(path));
    }
    return true;
}

bool FileSystem::AppendFile(const char *path, const char *content) {
    File file = fs_.open(path, FILE_APPEND);
    if (!file) {
        Log::Warn(TAG, "File %s to open failed", path);
        return false;
    }

    file.print(content);

    if (listener_) {
        listener_(FileAction::Updated, std::string(path));
    }
    return true;
}

bool FileSystem::RenameFile(const char *path, const char *new_path) {
    bool ret = fs_.rename(path, new_path);
    if (!ret) {
        Log::Warn(TAG, "Rename %s to %s failed", path, new_path);
    }
    return ret;
}

bool FileSystem::DeleteFile(const char *path) {
    bool ret = fs_.remove(path);
    if (!ret) {
      Log::Warn(TAG, "File %s delete failed", path);
      return false;
    }
    
    if (listener_) {
        listener_(FileAction::Deleted, std::string(path));
    }
    return ret;
}

#endif //CONFIG_USE_FS