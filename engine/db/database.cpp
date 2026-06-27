// ============================================================
//  pocket/engine/db/database.cpp
// ============================================================
#include "db/database.h"

namespace pk::db {

Database& engineDB() noexcept {
    static Database db;
    return db;
}

Database::~Database() { close(); }

bool Database::open(const String& path) noexcept {
    close();
    int rc = sqlite3_open(path.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        PK_LOG_ERROR("DB", "Cannot open '%s': %s", path.c_str(), lastError().c_str());
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }
    m_path = path;
    PK_LOG_INFO("DB", "Opened SQLite: %s", path.c_str());
    return true;
}

void Database::close() noexcept {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
        m_path.clear();
    }
}

bool Database::exec(const String& sql) noexcept {
    if (!m_db) return false;
    char* err = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        PK_LOG_ERROR("DB", "SQL error: %s", err ? err : "?");
        if (err) sqlite3_free(err);
        return false;
    }
    return true;
}

bool Database::Stmt::step() noexcept {
    if (!handle) return false;
    int rc = sqlite3_step(handle);
    return rc == SQLITE_ROW;
}
void Database::Stmt::reset() noexcept {
    if (handle) sqlite3_reset(handle);
}
void Database::Stmt::finalize() noexcept {
    if (handle) {
        sqlite3_finalize(handle);
        handle = nullptr;
    }
}
void Database::Stmt::bind(int idx, i32 v) noexcept { sqlite3_bind_int(handle, idx, v); }
void Database::Stmt::bind(int idx, i64 v) noexcept { sqlite3_bind_int64(handle, idx, v); }
void Database::Stmt::bind(int idx, f32 v) noexcept { sqlite3_bind_double(handle, idx, (f64)v); }
void Database::Stmt::bind(int idx, f64 v) noexcept { sqlite3_bind_double(handle, idx, v); }
void Database::Stmt::bind(int idx, const String& v) noexcept { sqlite3_bind_text(handle, idx, v.c_str(), -1, SQLITE_TRANSIENT); }
void Database::Stmt::bind(int idx, const char* v) noexcept { sqlite3_bind_text(handle, idx, v, -1, SQLITE_TRANSIENT); }
void Database::Stmt::bind(int idx) noexcept { sqlite3_bind_null(handle, idx); }

i32 Database::Stmt::getInt(int idx) const noexcept { return sqlite3_column_int(handle, idx); }
i64 Database::Stmt::getInt64(int idx) const noexcept { return sqlite3_column_int64(handle, idx); }
f32 Database::Stmt::getFloat(int idx) const noexcept { return (f32)sqlite3_column_double(handle, idx); }
String Database::Stmt::getString(int idx) const noexcept {
    const unsigned char* s = sqlite3_column_text(handle, idx);
    return s ? String(reinterpret_cast<const char*>(s)) : String{};
}

Database::Stmt Database::prepare(const String& sql) noexcept {
    Stmt s;
    if (!m_db) return s;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &s.handle, nullptr);
    if (rc != SQLITE_OK) {
        PK_LOG_ERROR("DB", "Prepare failed: %s | SQL: %s", lastError().c_str(), sql.c_str());
        s.handle = nullptr;
    }
    return s;
}

String Database::lastError() const noexcept {
    if (!m_db) return "not open";
    return String(sqlite3_errmsg(m_db));
}

bool Database::initEngineSchema() noexcept {
    static const char* kSchema = R"(
        CREATE TABLE IF NOT EXISTS projects (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            path TEXT NOT NULL,
            created INTEGER,
            modified INTEGER
        );
        CREATE TABLE IF NOT EXISTS recent_files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            path TEXT NOT NULL UNIQUE,
            opened INTEGER
        );
        CREATE TABLE IF NOT EXISTS settings (
            key TEXT PRIMARY KEY,
            value TEXT
        );
        CREATE TABLE IF NOT EXISTS undo_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            scene_id INTEGER,
            op TEXT,
            data TEXT,
            timestamp INTEGER
        );
        CREATE TABLE IF NOT EXISTS asset_cache (
            hash INTEGER PRIMARY KEY,
            path TEXT,
            type TEXT,
            size INTEGER,
            cached_at INTEGER
        );
    )";
    return exec(kSchema);
}

} // namespace pk::db
