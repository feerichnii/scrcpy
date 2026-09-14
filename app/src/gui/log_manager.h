#ifndef SC_GUI_LOG_MANAGER_H
#define SC_GUI_LOG_MANAGER_H

#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace scgui {

class LogManager {
public:
    static constexpr size_t kMaxLinesPerChannel = 2000;

    void set_log_directory(const std::string &dir);

    void append(const std::string &channel, const std::string &line);
    void append_app(const std::string &line);

    std::vector<std::string> channels() const;
    std::vector<std::string> lines(const std::string &channel) const;

    std::string friendly_error_from_logs(const std::string &channel) const;

private:
    mutable std::mutex mutex_;
    std::string log_dir_;
    std::unordered_map<std::string, std::deque<std::string>> buffers_;

    void write_file_locked(const std::string &channel, const std::string &line);
};

} // namespace scgui

#endif
