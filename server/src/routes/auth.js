// ============================================================
//  server/src/routes/auth.js
//  User registration + JWT login
// ============================================================
import { Router } from 'express';
import bcrypt from 'bcryptjs';
import jwt from 'jsonwebtoken';
import { getDB } from '../db.js';

const router = Router();
const JWT_SECRET = process.env.JWT_SECRET || 'pocket-dev-secret-change-me';
const JWT_EXPIRY = process.env.JWT_EXPIRY || '7d';

router.post('/register', (req, res) => {
    const { username, email, password } = req.body;
    if (!username || !email || !password) {
        return res.status(400).json({ error: 'Missing fields' });
    }
    if (password.length < 6) {
        return res.status(400).json({ error: 'Password must be at least 6 chars' });
    }

    const db = getDB();
    const existing = db.prepare('SELECT id FROM users WHERE username = ? OR email = ?')
                       .get(username, email);
    if (existing) {
        return res.status(409).json({ error: 'User already exists' });
    }

    const hash = bcrypt.hashSync(password, 10);
    const info = db.prepare('INSERT INTO users (username, email, password) VALUES (?, ?, ?)')
                   .run(username, email, hash);
    const token = jwt.sign({ id: info.lastInsertRowid, username }, JWT_SECRET,
                            { expiresIn: JWT_EXPIRY });
    res.json({ token, user: { id: info.lastInsertRowid, username, email } });
});

router.post('/login', (req, res) => {
    const { username, password } = req.body;
    if (!username || !password) {
        return res.status(400).json({ error: 'Missing credentials' });
    }

    const db = getDB();
    const user = db.prepare('SELECT * FROM users WHERE username = ? OR email = ?')
                   .get(username, username);
    if (!user || !bcrypt.compareSync(password, user.password)) {
        return res.status(401).json({ error: 'Invalid credentials' });
    }

    const token = jwt.sign({ id: user.id, username: user.username }, JWT_SECRET,
                            { expiresIn: JWT_EXPIRY });
    res.json({ token, user: { id: user.id, username: user.username, email: user.email } });
});

// Auth middleware
export function authMiddleware(req, res, next) {
    const header = req.headers.authorization;
    if (!header || !header.startsWith('Bearer ')) {
        return res.status(401).json({ error: 'No token' });
    }
    const token = header.substring(7);
    try {
        req.user = jwt.verify(token, JWT_SECRET);
        next();
    } catch (e) {
        return res.status(401).json({ error: 'Invalid token' });
    }
}

export default router;
export { JWT_SECRET };
