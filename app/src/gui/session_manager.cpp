#include "gui/session_manager.h"

namespace scgui {

SessionManager::SessionManager(ProcessManager *processes,
                               SettingsManager *settings,
                               LayoutManager *layout,
                               LogManager *logs)
    : processes_(processes)
    , settings_(settings)
    , layout_(layout)
    , logs_(logs) {
}

ProcessScrcpySession *
SessionManager::ensure_session(const std::string &serial) {
    auto it = sessions_.find(serial);
    if (it != sessions_.end()) {
        return it->second.get();
    }
    auto session = std::make_unique<ProcessScrcpySession>(serial, processes_, logs_);
    auto *ptr = session.get();
    sessions_[serial] = std::move(session);
    return ptr;
}

bool
SessionManager::start_now(const std::string &serial) {
    auto *session = ensure_session(serial);
    DeviceSettings ds = settings_->effective(serial);
    GlobalSettings gs = settings_->global();
    layout_->set_enabled(gs.auto_layout);
    layout_->set_columns(gs.layout_cols);

    WindowRect rect;
    if (gs.auto_layout) {
        rect = layout_->allocate(layout_index_++, std::max<size_t>(layout_total_, 1));
    }
    return session->start(ds, rect);
}

void
SessionManager::enqueue(const std::vector<std::string> &serials) {
    GlobalSettings gs = settings_->global();
    auto now = std::chrono::steady_clock::now();
    int delay = std::max(0, gs.stagger_ms);
    layout_index_ = 0;
    layout_total_ = serials.size();

    size_t i = 0;
    for (const auto &serial : serials) {
        PendingStart p;
        p.serial = serial;
        p.when = now + std::chrono::milliseconds(delay * static_cast<int>(i));
        queue_.push_back(p);
        ++i;
    }
}

bool
SessionManager::start(const std::string &serial) {
    std::lock_guard<std::mutex> lock(mutex_);
    layout_index_ = 0;
    layout_total_ = 1;
    return start_now(serial);
}

bool
SessionManager::stop(const std::string &serial) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(serial);
    if (it == sessions_.end()) {
        return false;
    }
    it->second->stop();
    return true;
}

bool
SessionManager::restart(const std::string &serial) {
    stop(serial);
    return start(serial);
}

void
SessionManager::start_selected(const std::vector<std::string> &serials) {
    std::lock_guard<std::mutex> lock(mutex_);
    enqueue(serials);
}

void
SessionManager::start_all(const std::vector<std::string> &serials) {
    start_selected(serials);
}

void
SessionManager::stop_selected(const std::vector<std::string> &serials) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &s : serials) {
        auto it = sessions_.find(s);
        if (it != sessions_.end()) {
            it->second->stop();
        }
    }
    // remove queued starts for these serials
    std::deque<PendingStart> kept;
    for (const auto &p : queue_) {
        bool drop = false;
        for (const auto &s : serials) {
            if (p.serial == s) {
                drop = true;
                break;
            }
        }
        if (!drop) {
            kept.push_back(p);
        }
    }
    queue_ = std::move(kept);
}

void
SessionManager::stop_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
    for (auto &kv : sessions_) {
        kv.second->stop();
    }
}

SessionState
SessionManager::state(const std::string &serial) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(serial);
    if (it == sessions_.end()) {
        return SessionState::Stopped;
    }
    return it->second->state();
}

std::string
SessionManager::error_message(const std::string &serial) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(serial);
    if (it == sessions_.end()) {
        return {};
    }
    return it->second->error_message();
}

void
SessionManager::on_device_removed(const std::string &serial) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(serial);
    if (it != sessions_.end()) {
        it->second->set_disconnected();
    }
}

void
SessionManager::tick() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    GlobalSettings gs = settings_->global();
    int inflight = 0;
    for (const auto &kv : sessions_) {
        auto st = kv.second->state();
        if (st == SessionState::Starting || st == SessionState::Running) {
            // count only starting for concurrency limit of startups
        }
        if (st == SessionState::Starting) {
            ++inflight;
        }
    }

    while (!queue_.empty()) {
        if (inflight >= std::max(1, gs.max_simultaneous_startups)) {
            break;
        }
        if (queue_.front().when > now) {
            break;
        }
        PendingStart p = queue_.front();
        queue_.pop_front();
        if (start_now(p.serial)) {
            ++inflight;
        }
    }
}

std::vector<std::string>
SessionManager::active_serials() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> out;
    for (const auto &kv : sessions_) {
        auto st = kv.second->state();
        if (st == SessionState::Running || st == SessionState::Starting ||
            st == SessionState::Stopping || st == SessionState::Failed) {
            out.push_back(kv.first);
        }
    }
    return out;
}

} // namespace scgui
