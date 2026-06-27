// ============================================================
//  server/src/routes/assets.js
//  Asset upload + listing + download
// ============================================================
import { Router } from 'express';
import multer from 'multer';
import fs from 'fs';
import path from 'path';
import crypto from 'crypto';
import { v4 as uuid } from 'uuid';
import { getDB } from '../db.js';
import { authMiddleware } from './auth.js';

const router = Router();
const UPLOAD_DIR = '/data/uploads';

// Multer config - 100MB max
const upload = multer({
    storage: multer.diskStorage({
        destination: (req, file, cb) => {
            const dir = path.join(UPLOAD_DIR, req.params.projectId || 'misc');
            if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });
            cb(null, dir);
        },
        filename: (req, file, cb) => {
            cb(null, `${uuid()}-${file.originalname}`);
        },
    }),
    limits: { fileSize: 100 * 1024 * 1024 },
});

router.use(authMiddleware);

router.post('/upload/:projectId', upload.single('file'), (req, res) => {
    if (!req.file) return res.status(400).json({ error: 'No file' });

    const { projectId } = req.params;
    const db = getDB();

    // Verify project ownership
    const project = db.prepare('SELECT id FROM projects WHERE id = ? AND user_id = ?')
                      .get(projectId, req.user.id);
    if (!project) {
        fs.unlinkSync(req.file.path);
        return res.status(403).json({ error: 'Not your project' });
    }

    // Compute hash
    const buf = fs.readFileSync(req.file.path);
    const hash = crypto.createHash('sha256').update(buf).digest('hex');

    const id = uuid();
    const relPath = `/uploads/${projectId}/${req.file.filename}`;
    db.prepare(`INSERT INTO assets (id, project_id, filename, mime_type, size, hash, path)
                VALUES (?, ?, ?, ?, ?, ?, ?)`)
      .run(id, projectId, req.file.originalname, req.file.mimetype,
           req.file.size, hash, relPath);

    res.json({
        asset: {
            id,
            filename: req.file.originalname,
            mimeType: req.file.mimetype,
            size: req.file.size,
            hash,
            url: relPath,
        }
    });
});

router.get('/:projectId', (req, res) => {
    const db = getDB();
    const assets = db.prepare('SELECT * FROM assets WHERE project_id = ? ORDER BY created_at DESC')
                     .all(req.params.projectId);
    res.json({ assets });
});

router.delete('/:assetId', (req, res) => {
    const db = getDB();
    const asset = db.prepare('SELECT * FROM assets WHERE id = ?').get(req.params.assetId);
    if (!asset) return res.status(404).json({ error: 'Not found' });

    // Verify ownership via project
    const project = db.prepare('SELECT id FROM projects WHERE id = ? AND user_id = ?')
                      .get(asset.project_id, req.user.id);
    if (!project) return res.status(403).json({ error: 'Not your asset' });

    // Delete file
    const fullPath = path.join('/data', asset.path.replace('/uploads/', 'uploads/'));
    if (fs.existsSync(fullPath)) fs.unlinkSync(fullPath);

    db.prepare('DELETE FROM assets WHERE id = ?').run(req.params.assetId);
    res.json({ ok: true });
});

export default router;
