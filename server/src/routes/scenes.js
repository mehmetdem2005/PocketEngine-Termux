// ============================================================
//  server/src/routes/scenes.js
//  Scene save/load
// ============================================================
import { Router } from 'express';
import { getDB } from '../db.js';
import { authMiddleware } from './auth.js';

const router = Router();

router.use(authMiddleware);

router.get('/:projectId', (req, res) => {
    const db = getDB();
    const project = db.prepare('SELECT scene_data FROM projects WHERE id = ? AND user_id = ?')
                      .get(req.params.projectId, req.user.id);
    if (!project) return res.status(404).json({ error: 'Not found' });
    res.json({ scene: JSON.parse(project.scene_data || '{}') });
});

router.post('/:projectId', (req, res) => {
    const { scene } = req.body;
    if (!scene) return res.status(400).json({ error: 'Missing scene data' });

    const db = getDB();
    const result = db.prepare(`UPDATE projects
                                SET scene_data = ?,
                                    modified_at = strftime('%s','now')
                                WHERE id = ? AND user_id = ?`)
                     .run(JSON.stringify(scene), req.params.projectId, req.user.id);
    if (result.changes === 0) return res.status(404).json({ error: 'Not found' });
    res.json({ ok: true });
});

export default router;
