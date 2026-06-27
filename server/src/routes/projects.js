// ============================================================
//  server/src/routes/projects.js
//  CRUD for projects
// ============================================================
import { Router } from 'express';
import { v4 as uuid } from 'uuid';
import { getDB } from '../db.js';
import { authMiddleware } from './auth.js';

const router = Router();

router.use(authMiddleware);

router.get('/', (req, res) => {
    const db = getDB();
    const projects = db.prepare('SELECT id, name, description, created_at, modified_at FROM projects WHERE user_id = ? ORDER BY modified_at DESC')
                       .all(req.user.id);
    res.json({ projects });
});

router.post('/', (req, res) => {
    const { name, description = '' } = req.body;
    if (!name) return res.status(400).json({ error: 'Name required' });

    const id = uuid();
    const db = getDB();
    db.prepare('INSERT INTO projects (id, user_id, name, description, scene_data) VALUES (?, ?, ?, ?, ?)')
      .run(id, req.user.id, name, description, '{}');

    const project = db.prepare('SELECT * FROM projects WHERE id = ?').get(id);
    res.json({ project });
});

router.get('/:id', (req, res) => {
    const db = getDB();
    const project = db.prepare('SELECT * FROM projects WHERE id = ? AND user_id = ?')
                      .get(req.params.id, req.user.id);
    if (!project) return res.status(404).json({ error: 'Not found' });
    res.json({ project });
});

router.put('/:id', (req, res) => {
    const { name, description, scene_data } = req.body;
    const db = getDB();
    const result = db.prepare(`UPDATE projects
                                SET name = COALESCE(?, name),
                                    description = COALESCE(?, description),
                                    scene_data = COALESCE(?, scene_data),
                                    modified_at = strftime('%s','now')
                                WHERE id = ? AND user_id = ?`)
                     .run(name, description, scene_data, req.params.id, req.user.id);
    if (result.changes === 0) return res.status(404).json({ error: 'Not found' });
    res.json({ ok: true });
});

router.delete('/:id', (req, res) => {
    const db = getDB();
    const result = db.prepare('DELETE FROM projects WHERE id = ? AND user_id = ?')
                     .run(req.params.id, req.user.id);
    if (result.changes === 0) return res.status(404).json({ error: 'Not found' });
    res.json({ ok: true });
});

export default router;
