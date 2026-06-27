// ============================================================
//  server/src/db.js
//  SQLite persistence on Render disk
// ============================================================
import Database from 'better-sqlite3';
import fs from 'fs';

const DB_PATH = process.env.DB_PATH || '/data/pocket.db';

let db;

export function initDB() {
    // Ensure dir exists
    const dir = DB_PATH.substring(0, DB_PATH.lastIndexOf('/'));
    if (dir && !fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });

    db = new Database(DB_PATH);
    db.pragma('journal_mode = WAL');
    db.pragma('foreign_keys = ON');

    db.exec(`
        CREATE TABLE IF NOT EXISTS users (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            username    TEXT UNIQUE NOT NULL,
            email       TEXT UNIQUE NOT NULL,
            password    TEXT NOT NULL,
            created_at  INTEGER DEFAULT (strftime('%s','now'))
        );

        CREATE TABLE IF NOT EXISTS projects (
            id          TEXT PRIMARY KEY,
            user_id     INTEGER NOT NULL,
            name        TEXT NOT NULL,
            description TEXT,
            scene_data  TEXT,
            created_at  INTEGER DEFAULT (strftime('%s','now')),
            modified_at INTEGER DEFAULT (strftime('%s','now')),
            FOREIGN KEY (user_id) REFERENCES users(id)
        );

        CREATE TABLE IF NOT EXISTS assets (
            id          TEXT PRIMARY KEY,
            project_id  TEXT NOT NULL,
            filename    TEXT NOT NULL,
            mime_type   TEXT,
            size        INTEGER,
            hash        TEXT,
            path        TEXT NOT NULL,
            created_at  INTEGER DEFAULT (strftime('%s','now')),
            FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS rooms (
            id          TEXT PRIMARY KEY,
            name        TEXT NOT NULL,
            owner_id    INTEGER,
            max_players INTEGER DEFAULT 8,
            is_public   INTEGER DEFAULT 1,
            created_at  INTEGER DEFAULT (strftime('%s','now'))
        );

        CREATE INDEX IF NOT EXISTS idx_projects_user  ON projects(user_id);
        CREATE INDEX IF NOT EXISTS idx_assets_project ON assets(project_id);
    `);

    console.log(`[DB] SQLite ready at ${DB_PATH}`);
    return db;
}

export function getDB() {
    if (!db) initDB();
    return db;
}
