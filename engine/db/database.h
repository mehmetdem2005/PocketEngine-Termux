// ============================================================
//  pocket/engine/db/database.h
//  SQLite wrapper for local storage (project metadata, settings,
//  scene index, recent files, undo history)
// ============================================================
#pragma once

#include "core/types.h"
#include "core/log.h"

#ifdef POCKET_HAS_SQLITE
#include <sqlite3.h>
#else
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
#endif

namespace pk::db {

class Database {
public:
    Database() = default;
    ~Database();

    bool open(const String& path) noexcept;
    void close() noexcept;

    // Execute raw SQL (no result)
    bool exec(const String& sql) noexcept;

    // Prepared statement helper
    struct Stmt {
        sqlite3_stmt* handle{nullptr};
        bool step() noexcept;       // returns true if row available
        void reset() noexcept;
        void finalize() noexcept;
        void bind(int idx, i32 v) noexcept;
        void bind(int idx, i64 v) noexcept;
        void bind(int idx, f32 v) noexcept;
        void bind(int idx, f64 v) noexcept;
        void bind(int idx, const String& v) noexcept;
        void bind(int idx, const char* v) noexcept;
        void bind(int idx) noexcept;  // NULL
        i32  getInt(int idx) const noexcept;
        i64  getInt64(int idx) const noexcept;
        f32  getFloat(int idx) const noexcept;
        String getString(int idx) const noexcept;
    };

    Stmt prepare(const String& sql) noexcept;

    bool isOpen() const noexcept { return m_db != nullptr; }
    const String& path() const noexcept { return m_path; }
    String lastError() const noexcept;

    // Convenience: schema init for the engine's metadata DB
    bool initEngineSchema() noexcept;

private:
    sqlite3* m_db{nullptr};
    String   m_path;
};

Database& engineDB() noexcept;

} // namespace pk::db
