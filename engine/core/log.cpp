// ============================================================
//  pocket/engine/core/log.cpp
// ============================================================
#include "core/log.h"

#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <mutex>
#include <fstream>

#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace pk::log {

static std::atomic<Level> g_level{Level::Info};
static std::mutex         g_mutex;
static std::ofstream      g_file;

static const char* levelStr(Level l) noexcept {
    switch(l) {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO ";
        case Level::Warn:  return "WARN ";
        case Level::Error: return "ERROR";
        case Level::Fatal: return "FATAL";
    }
    return "?????";
}

static const char* levelColor(Level l) noexcept {
    switch(l) {
        case Level::Trace: return "\033[90m";
        case Level::Debug: return "\033[37m";
        case Level::Info:  return "\033[32m";
        case Level::Warn:  return "\033[33m";
        case Level::Error: return "\033[31m";
        case Level::Fatal: return "\033[1;31m";
    }
    return "";
}

static const char* kReset = "\033[0m";

void setLevel(Level lvl) noexcept { g_level.store(lvl, std::memory_order_relaxed); }
Level getLevel() noexcept { return g_level.load(std::memory_order_relaxed); }

void setOutputFile(const String& path) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (path.empty()) {
        g_file.close();
    } else {
        g_file.open(path, std::ios::out | std::ios::app);
    }
}

void log(Level lvl, StringView tag, StringView msg) noexcept {
    if (static_cast<u8>(lvl) < static_cast<u8>(g_level.load(std::memory_order_relaxed)))
        return;

    // Timestamp
    auto now       = Clock::now();
    auto t         = std::chrono::system_clock::to_time_t(now);
    auto ms        = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::tm tmv{};
    ::localtime_r(&t, &tmv);
    char ts[24];
    std::snprintf(ts, sizeof(ts), "%02d:%02d:%02d.%03lld",
                  tmv.tm_hour, tmv.tm_min, tmv.tm_sec, (long long)ms.count());

    std::lock_guard<std::mutex> lk(g_mutex);

    // stderr (with color)
    std::fprintf(stderr, "%s[%s]%s [%.*s] %.*s\n",
                 levelColor(lvl), ts, kReset,
                 static_cast<int>(tag.size()), tag.data(),
                 static_cast<int>(msg.size()), msg.data());

    // File (no color)
    if (g_file.is_open()) {
        g_file << '[' << ts << "] [" << levelStr(lvl) << "] ["
               << tag << "] " << msg << '\n';
        g_file.flush();
    }

#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "PocketEngine", "[%.*s] %.*s",
                        static_cast<int>(tag.size()), tag.data(),
                        static_cast<int>(msg.size()), msg.data());
#endif
}

String fmt(const char* fmtstr, ...) noexcept {
    char buf[1024];
    va_list args;
    va_start(args, fmtstr);
    int n = std::vsnprintf(buf, sizeof(buf), fmtstr, args);
    va_end(args);
    if (n < 0) return {};
    if (n < static_cast<int>(sizeof(buf))) return String(buf, n);

    // Long string: heap
    String out(n, '\0');
    va_start(args, fmtstr);
    std::vsnprintf(out.data(), out.size() + 1, fmtstr, args);
    va_end(args);
    return out;
}

} // namespace pk::log
