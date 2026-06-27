// ============================================================
//  pocket/engine/core/log.h
//  Thread-safe leveled logging with colors
// ============================================================
#pragma once

#include "core/types.h"

namespace pk::log {

enum class Level : u8 {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

void setLevel(Level lvl) noexcept;
Level getLevel() noexcept;

void setOutputFile(const String& path);  // empty = stderr only

// Core API
void log(Level lvl, StringView tag, StringView msg) noexcept;

// Convenience macros
#define PK_LOG_TRACE(tag, ...) ::pk::log::log(::pk::log::Level::Trace, tag, ::pk::log::fmt(__VA_ARGS__))
#define PK_LOG_DEBUG(tag, ...) ::pk::log::log(::pk::log::Level::Debug, tag, ::pk::log::fmt(__VA_ARGS__))
#define PK_LOG_INFO(tag, ...)  ::pk::log::log(::pk::log::Level::Info,  tag, ::pk::log::fmt(__VA_ARGS__))
#define PK_LOG_WARN(tag, ...)  ::pk::log::log(::pk::log::Level::Warn,  tag, ::pk::log::fmt(__VA_ARGS__))
#define PK_LOG_ERROR(tag, ...) ::pk::log::log(::pk::log::Level::Error, tag, ::pk::log::fmt(__VA_ARGS__))
#define PK_LOG_FATAL(tag, ...) do { ::pk::log::log(::pk::log::Level::Fatal, tag, ::pk::log::fmt(__VA_ARGS__)); ::abort(); } while(0)

// Tiny sprintf-like helper (no std::format for broader compat)
String fmt(const char* fmtstr, ...) noexcept;

// Tag shortcut
#define PK_TAG "Pocket"

} // namespace pk::log
