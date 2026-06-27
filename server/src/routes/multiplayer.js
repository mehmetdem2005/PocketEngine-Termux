// ============================================================
//  server/src/routes/multiplayer.js
//  Realtime multiplayer relay over WebSocket
// ============================================================
import { Router } from 'express';
import { v4 as uuid } from 'uuid';
import { getDB } from '../db.js';
import { authMiddleware } from './auth.js';

const router = Router();
router.use(authMiddleware);

// In-memory room state
const rooms = new Map(); // roomId -> { name, ownerId, clients: Set<ws> }

router.get('/rooms', (req, res) => {
    const out = [];
    for (const [id, r] of rooms.entries()) {
        out.push({
            id, name: r.name, ownerId: r.ownerId,
            playerCount: r.clients.size,
        });
    }
    res.json({ rooms: out });
});

router.post('/rooms', (req, res) => {
    const { name, isPublic = true, maxPlayers = 8 } = req.body;
    const id = uuid();
    rooms.set(id, {
        name: name || 'Unnamed Room',
        ownerId: req.user.id,
        maxPlayers,
        isPublic,
        clients: new Set(),
    });
    res.json({ roomId: id });
});

// WebSocket initialization - called from index.js
export function initWebSocket(wss) {
    wss.on('connection', (ws, req) => {
        // Parse URL: /ws?roomId=xxx&userId=yyy
        const url = new URL(req.url, 'http://localhost');
        const roomId = url.searchParams.get('roomId');
        const userId = url.searchParams.get('userId');
        const username = url.searchParams.get('username') || 'Anon';

        if (!roomId || !rooms.has(roomId)) {
            ws.send(JSON.stringify({ type: 'error', message: 'Invalid room' }));
            ws.close();
            return;
        }

        const room = rooms.get(roomId);
        if (room.clients.size >= room.maxPlayers) {
            ws.send(JSON.stringify({ type: 'error', message: 'Room full' }));
            ws.close();
            return;
        }

        // Register
        ws.userId = userId;
        ws.username = username;
        ws.roomId = roomId;
        room.clients.add(ws);

        console.log(`[WS] ${username} joined room ${roomId} (${room.clients.size}/${room.maxPlayers})`);

        // Notify others
        broadcast(room, {
            type: 'player_joined',
            userId, username,
            playerCount: room.clients.size,
        }, ws);

        // Send current state to new client
        ws.send(JSON.stringify({
            type: 'joined',
            roomId,
            playerCount: room.clients.size,
        }));

        ws.on('message', (data) => {
            try {
                const msg = JSON.parse(data.toString());
                // Relay to others in room
                broadcast(room, {
                    type: 'message',
                    from: userId,
                    username,
                    payload: msg,
                    ts: Date.now(),
                }, ws);
            } catch (e) {
                console.warn('[WS] bad message:', e.message);
            }
        });

        ws.on('close', () => {
            room.clients.delete(ws);
            console.log(`[WS] ${username} left room ${roomId} (${room.clients.size})`);
            if (room.clients.size === 0) {
                rooms.delete(roomId);
                console.log(`[WS] Room ${roomId} destroyed (empty)`);
            } else {
                broadcast(room, {
                    type: 'player_left',
                    userId, username,
                    playerCount: room.clients.size,
                });
            }
        });
    });
}

function broadcast(room, msg, excludeWs) {
    const data = JSON.stringify(msg);
    for (const client of room.clients) {
        if (client === excludeWs) continue;
        if (client.readyState === 1) client.send(data);
    }
}

export default router;
