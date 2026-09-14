#include "gui/log_manager.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace scgui {

namespace {

bool
contains_ci(const std::string &hay, const char *needle) {
    auto it = std::search(hay.begin(), hay.end(), needle, needle + std::strlen(needle),
                          [](char a, char b) {
                              return std::tolower(static_cast<unsigned char>(a)) ==
                                     std::tolower(static_cast<unsigned char>(b));
                          });
    return it != hay.end();
}

} // namespace

void
LogManager::set_log_directory(const std::string &dir) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_dir_ = dir;
    if (!log_dir_.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(log_dir_, ec);
    }
}

void
LogManager::append_app(const std::string &line) {
    append("Application", line);
}

void
LogManager::append(const std::string &channel, const std::string &line) {
    if (line.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto &buf = buffers_[channel];
    buf.push_back(line);
    while (buf.size() > kMaxLinesPerChannel) {
        buf.pop_front();
    }
    write_file_locked(channel, line);
}

void
LogManager::write_file_locked(const std::string &channel, const std::string &line) {
    if (log_dir_.empty()) {
        return;
    }
    std::string safe = channel;
    for (char &c : safe) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_')) {
            c = '_';
        }
    }
    std::filesystem::path path = std::filesystem::path(log_dir_) / (safe + ".log");
    std::ofstream out(path, std::ios::app);
    if (out) {
        out << line << '\n';
    }
}

std::vector<std::string>
LogManager::channels() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> out;
    out.reserve(buffers_.size());
    for (const auto &kv : buffers_) {
        out.push_back(kv.first);
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<std::string>
LogManager::lines(const std::string &channel) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = buffers_.find(channel);
    if (it == buffers_.end()) {
        return {};
    }
    return std::vector<std::string>(it->second.begin(), it->second.end());
}

std::string
LogManager::friendly_error_from_logs(const std::string &channel) const {
    auto lines = this->lines(channel);
    for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
        const std::string &l = *it;
        if (contains_ci(l, "unauthorized")) {
            return "Device is not authorized.\nUnlock the phone and allow USB debugging.";
        }
        if (contains_ci(l, "offline")) {
            return "Device is offline.\nReconnect the USB cable or restart ADB.";
        }
        if (contains_ci(l, "Could not find") && contains_ci(l, "device")) {
            return "Device was not found.\nCheck the USB connection and ADB authorization.";
        }
        if (contains_ci(l, "server") && contains_ci(l, "failed")) {
            return "scrcpy server failed to start on the device.";
        }
        if (contains_ci(l, "Decoder") && contains_ci(l, "failed")) {
            return "Video decoder failed. Try another codec or lower resolution.";
        }
        if (contains_ci(l, "ERROR:")) {
            return l;
        }
    }
    return "Unable to start mirroring. Check the Logs tab for details.";
}

} // namespace scgui
