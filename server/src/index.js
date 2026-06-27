// ============================================================
//  server/src/index.js
//  PocketEngine backend - Render.com deployable
// ============================================================
import 'dotenv/config';
import express    from 'express';
import cors       from 'cors';
import helmet     from 'helmet';
import morgan     from 'morgan';
import rateLimit  from 'express-rate-limit';
import http       from 'http';
import { WebSocketServer } from 'ws';

import { initDB } from './db.js';
import authRoutes      from './routes/auth.js';
import projectRoutes   from './routes/projects.js';
import assetRoutes     from './routes/assets.js';
import sceneRoutes     from './routes/scenes.js';
import multiplayerRoutes, { initWebSocket } from './routes/multiplayer.js';

const PORT = process.env.PORT || 3000;
const app  = express();

// Init SQLite
initDB();

// Security & middleware
app.use(helmet({
    crossOriginResourcePolicy: { policy: 'cross-origin' },
    contentSecurityPolicy: false,
}));
app.use(cors({
    origin: '*',
    methods: ['GET','POST','PUT','DELETE','PATCH'],
    allowedHeaders: ['Content-Type','Authorization'],
}));
app.use(express.json({ limit: '50mb' }));
app.use(express.urlencoded({ extended: true, limit: '50mb' }));
app.use(morgan('tiny'));

// Rate limit
const limiter = rateLimit({
    windowMs: 60 * 1000,
    max: 240,
    standardHeaders: true,
    legacyHeaders: false,
});
app.use('/api', limiter);

// Static asset storage (Render disk mount /uploads)
import fs from 'fs';
if (!fs.existsSync('/data/uploads')) fs.mkdirSync('/data/uploads', { recursive: true });
app.use('/uploads', express.static('/data/uploads'));

// Routes
app.get('/', (req, res) => {
    res.json({
        name:    'PocketEngine Server',
        version: '0.1.0',
        status:  'online',
        docs:    '/api/docs',
        time:    new Date().toISOString(),
    });
});

app.get('/api/docs', (req, res) => {
    res.json({
        endpoints: [
            'POST   /api/auth/register',
            'POST   /api/auth/login',
            'GET    /api/projects',
            'POST   /api/projects',
            'GET    /api/projects/:id',
            'PUT    /api/projects/:id',
            'DELETE /api/projects/:id',
            'POST   /api/assets/upload',
            'GET    /api/assets/:projectId',
            'GET    /api/scenes/:projectId',
            'POST   /api/scenes/:projectId',
            'GET    /api/multiplayer/rooms',
            'POST   /api/multiplayer/rooms',
            'WS     /ws/multiplayer/:roomId',
        ],
    });
});

app.use('/api/auth',         authRoutes);
app.use('/api/projects',     projectRoutes);
app.use('/api/assets',       assetRoutes);
app.use('/api/scenes',       sceneRoutes);
app.use('/api/multiplayer',  multiplayerRoutes);

// 404
app.use((req, res) => {
    res.status(404).json({ error: 'Not found', path: req.path });
});

// Error handler
app.use((err, req, res, next) => {
    console.error('[ERR]', err.message);
    res.status(err.status || 500).json({
        error: err.message || 'Internal server error',
    });
});

// HTTP server + WebSocket
const server = http.createServer(app);
const wss = new WebSocketServer({ server, path: '/ws' });
initWebSocket(wss);

server.listen(PORT, '0.0.0.0', () => {
    console.log(`[PocketEngine Server] Listening on :${PORT}`);
    console.log(`  Environment: ${process.env.NODE_ENV || 'development'}`);
    console.log(`  DB:          ${process.env.DB_PATH || '/data/pocket.db'}`);
});

// Graceful shutdown
process.on('SIGTERM', () => {
    console.log('SIGTERM received, shutting down...');
    server.close(() => process.exit(0));
});
process.on('SIGINT', () => {
    console.log('SIGINT received, shutting down...');
    server.close(() => process.exit(0));
});
