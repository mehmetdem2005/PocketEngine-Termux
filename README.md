# 🎮 PocketEngine — Termux:X11 C++ Game Engine

> Unity/Godot benzeri, **telefondan** çalışan, tam teşekküllü C++ oyun motoru.
> Termux:X11 üzerinde OpenGL ES 3.0 + ImGui editor, SQLite yerel DB,
> Render.com bulut backend, GitHub'ta kaynak kod.

![Platform](https://img.shields.io/badge/platform-Android%20%7C%20Termux%3AX11-green)
![Language](https://img.shields.io/badge/language-C%2B%2B20-blue)
![License](https://img.shields.io/badge/license-MIT-blue)

---

## 📋 İçindekiler

1. [Genel Bakış](#genel-bakış)
2. [Mimari](#mimari)
3. [Hızlı Başlangıç](#hızlı-başlangıç)
4. [Kurulum](#kurulum)
5. [Editörü Çalıştırma](#editörü-çalıştırma)
6. [Render Backend](#render-backend)
7. [Klasör Yapısı](#klasör-yapısı)
8. [Özellikler](#özellikler)
9. [Sorun Giderme](#sorun-giderme)
10. [Katkıda Bulunma](#katkıda-bulunma)

---

## Genel Bakış

**PocketEngine**, Android telefonunuzda Termux ve Termux:X11 kullanarak
geliştirebileceğiniz, masaüstü sınıfı bir C++ oyun motorudur. Unity'nin
editor paradigmasından ilham almıştır:

> ✅ **Backend canlı**: https://pocketengine-server.onrender.com
> ✅ **GitHub repo**: https://github.com/mehmetdem2005/PocketEngine-Termux

- 🪟 **Yatay (landscape) editor layout** — Hierarchy, Scene, Inspector, Project, Console, Profiler
- 🧠 **Yüksek performanslı ECS** — Sparse-set tabanlı, cache-friendly
- 🎨 **OpenGL ES 3.0 batch renderer** — 4096 quad/batch, 16 texture slot
- 💾 **SQLite yerel veritabanı** — Proje metadata, recent files, undo history
- ☁️ **Render.com bulut backend** — Asset CDN, proje sync, multiplayer relay
- 🔧 **Tam optimizasyon** — `-O3 -flto -march=native`, frame arena, thread pool
- 🧵 **Multi-threaded** — İşçi thread havuzu, parallel-for, lock-free queues
- 📦 **Hot reload** — Asset mtime takibi ile otomatik yeniden yükleme

---

## Mimari

```
┌────────────────────────────────────────────────────────────────┐
│                        EDITOR (ImGui)                          │
│  Hierarchy  │  Scene View  │  Inspector  │  Console  │  ...    │
├────────────────────────────────────────────────────────────────┤
│                          ENGINE CORE                           │
│  ECS  │  Renderer  │  Assets  │  Physics  │  Audio  │  Script  │
│       │             │          │           │         │          │
│  Allocators  │  Thread Pool  │  Event Bus  │  Scene  │  DB      │
├────────────────────────────────────────────────────────────────┤
│                       PLATFORM LAYER                           │
│         GLFW  │  OpenGL ES 3.0  │  EGL  │  Termux:X11           │
├────────────────────────────────────────────────────────────────┤
│                    Android (Termux proot Debian)               │
└────────────────────────────────────────────────────────────────┘
                              ↕ HTTPS / WS
┌────────────────────────────────────────────────────────────────┐
│                    Render.com (Node.js)                        │
│  Auth (JWT)  │  Projects  │  Assets (CDN)  │  Multiplayer (WS) │
│                  SQLite (Disk) / 1GB                           │
└────────────────────────────────────────────────────────────────┘
```

---

## Hızlı Başlangıç

Telefonunuzda Termux açın ve şunu çalıştırın:

```bash
# 1. PocketEngine reposunu klonla
git clone https://github.com/mehmetdem2005/PocketEngine-Termux.git
cd PocketEngine-Termux

# 2. Setup scriptini çalıştır (Termux'u update eder + Debian proot kurar)
bash scripts/termux/setup_termux.sh

# 3. Editörü başlat
bash scripts/termux/run_editor.sh
```

İlk kurulum ~10-15 dakika sürebilir (Box2D, ImGui vb. derlenecek).

---

## Kurulum

### Gereksinimler

- **Android 9+** telefon
- **Termux** (F-Droid'den yükleyin — Play Store versiyonu eski)
- **Termux:X11** add-on (F-Droid'den)
- ~2GB boş disk alanı
- WiFi bağlantısı (kurulum için)

### Adım Adım Kurulum

#### 1. Termux ve Termux:X11 yükleyin

F-Droid'den:
1. https://f-droid.org/packages/com.termux/
2. https://f-droid.org/packages/termux.x11/ (veya `com.termux.x11`)

Play Store'dan yüklemeyin — oradaki sürümler deprecated.

#### 2. Termux:X11'i Termux ile ilişkilendirin

Termux uygulamasını açın:
```bash
pkg install termux-x11
```

#### 3. PocketEngine'i klonlayın

```bash
git clone https://github.com/mehmetdem2005/PocketEngine-Termux.git
cd PocketEngine-Termux
```

#### 4. Setup scriptini çalıştırın

```bash
bash scripts/termux/setup_termux.sh
```

Bu script şunları yapar:
1. Termux paketlerini günceller (git, cmake, proot-distro, termux-x11, ...)
2. `proot-distro` ile Debian Bookworm kurar
3. Debian içine C++ toolchain + tüm bağımlılıkları yükler (clang, cmake, GLFW, GLM, Box2D, ImGui, sol2, nlohmann/json, SQLite, OpenAL, Bullet3)
4. PocketEngine kaynak kodunu `/opt/pocketengine/src`'e bağlar

#### 5. Motoru derleyin

```bash
# Debian proot içine girip build
proot-distro login debian --user pocket --shared-tmp -- \
    bash /opt/pocketengine/src/scripts/build/build.sh
```

Build çıktısı: `/opt/pocketengine/build/bin/pocketeditor`

---

## Editörü Çalıştırma

### Tek komutla başlatma

```bash
bash scripts/termux/run_editor.sh
```

Bu script:
1. Termux:X11 server'ı başlatır (`:0` ekranında)
2. proot Debian içine girer
3. `DISPLAY=:0` ortam değişkeni ile editörü çalıştırır

### Manuel başlatma

```bash
# Terminal 1 (Termux): X11 server'ı başlat
termux-x11 :0 &

# Terminal 2 (Termux): Editörü çalıştır
proot-distro login debian --user pocket --shared-tmp -- \
    env DISPLAY=:0 /opt/pocketengine/build/bin/pocketeditor
```

### Termux:X11 pencere ayarları

Termux:X11 uygulamasını açın, settings'ten:
- **Display resolution**: Auto veya 1280x720
- **Orientation**: Landscape (zorunlu)
- **Show additional keyboard**: ON (klavye için)

---

## Render Backend

Backend zaten deploy edilmiş: **https://pocketengine-server.onrender.com**

### API Endpoint'leri

| Method | Endpoint | Açıklama |
|--------|----------|----------|
| POST   | `/api/auth/register` | Yeni kullanıcı |
| POST   | `/api/auth/login` | Giriş, JWT döner |
| GET    | `/api/projects` | Kullanıcının projeleri |
| POST   | `/api/projects` | Yeni proje |
| GET    | `/api/projects/:id` | Proje detayı |
| PUT    | `/api/projects/:id` | Güncelle (scene_data) |
| DELETE | `/api/projects/:id` | Sil |
| POST   | `/api/assets/upload/:projectId` | Asset yükle (multipart, max 100MB) |
| GET    | `/api/assets/:projectId` | Proje asset'leri |
| DELETE | `/api/assets/:assetId` | Asset sil |
| GET    | `/api/scenes/:projectId` | Scene JSON al |
| POST   | `/api/scenes/:projectId` | Scene JSON kaydet |
| GET    | `/api/multiplayer/rooms` | Aktif odalar |
| POST   | `/api/multiplayer/rooms` | Yeni oda |
| WS     | `/ws?roomId=xxx&userId=yyy&username=zzz` | Multiplayer WebSocket |

### Test

```bash
# Health check
curl https://pocketengine-server.onrender.com/

# Register
curl -X POST https://pocketengine-server.onrender.com/api/auth/register \
    -H 'Content-Type: application/json' \
    -d '{"username":"test","email":"test@test","password":"secret123"}'
```

### Backend'i lokalde çalıştırma

```bash
cd server
cp .env.example .env
npm install
npm start
# http://localhost:3000
```

### Render'a yeniden deploy

Backend GitHub'a push edildiğinde otomatik deploy olur. Manuel tetikleme için:
```bash
# Render API ile
curl -X POST https://api.render.com/v1/services \
    -H "Authorization: Bearer $RENDER_API_KEY" \
    -H "Content-Type: application/json" \
    -d '{...}'
```

---

## Klasör Yapısı

```
PocketEngine-Termux/
├── CMakeLists.txt              # Ana build yapılandırması
├── Dockerfile                  # Render.com backend image
├── render.yaml                 # Render service tanımı
├── README.md                   # Bu dosya
├── .gitignore
│
├── engine/                     # C++ motor kütüphanesi
│   ├── CMakeLists.txt
│   ├── core/                   # ECS, alloc, log, thread_pool, event, math, scene
│   │   ├── types.h
│   │   ├── log.h/.cpp
│   │   ├── alloc.h/.cpp        # Arena + Pool + Stats
│   │   ├── thread_pool.h/.cpp
│   │   ├── ecs.h               # Sparse-set ECS
│   │   ├── event.h/.cpp
│   │   ├── math.h
│   │   └── scene.h/.cpp
│   ├── renderer/               # OpenGL ES 3.0 batched 2D
│   │   └── renderer.h/.cpp
│   ├── platform/               # GLFW window (Termux:X11)
│   │   └── window.h/.cpp
│   ├── assets/                 # Asset manager (hot reload)
│   │   └── asset_manager.h/.cpp
│   ├── db/                     # SQLite wrapper
│   │   └── database.h/.cpp
│   ├── physics/                # Box2D 2D + Bullet3 3D (TODO)
│   ├── audio/                  # OpenAL (TODO)
│   ├── scripting/              # Lua via sol2 (TODO)
│   ├── ui/                     # ImGui backend
│   │   └── imgui_backend.h/.cpp
│   └── network/                # Render API client (TODO)
│
├── editor/                     # Unity-style editor
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── editor.h/.cpp           # Ana editor sınıfı + dockspace
│   └── panels/                 # Boş panel header'ları (hepsi editor.cpp içinde)
│
├── runtime/                    # Paketlenmiş oyun çalıştırıcısı
│   ├── CMakeLists.txt
│   └── main.cpp
│
├── server/                     # Render.com backend (Node.js + Express)
│   ├── package.json
│   ├── .env.example
│   └── src/
│       ├── index.js            # Express app + WS server
│       ├── db.js               # SQLite init + schema
│       └── routes/
│           ├── auth.js         # JWT auth
│           ├── projects.js     # Project CRUD
│           ├── assets.js       # Multipart upload (Multer)
│           ├── scenes.js       # Scene JSON sync
│           └── multiplayer.js  # WebSocket relay
│
├── scripts/
│   ├── termux/
│   │   ├── setup_termux.sh     # Adım 1: Termux deps + proot Debian
│   │   ├── setup_debian.sh     # Adım 2: Debian içine C++ toolchain
│   │   └── run_editor.sh       # Editörü başlatma helper
│   ├── build/
│   │   └── build.sh            # CMake + Ninja build
│   └── deploy/
│       └── (server deploy via render.yaml)
│
├── third_party/
│   ├── stb/stb_image.h         # Header-only image loader
│   └── imgui/CMakeLists.txt    # ImGui build (vendor /opt/imgui)
│
├── assets/                     # Test asset'leri
│   ├── textures/
│   ├── shaders/
│   ├── audio/
│   ├── models/
│   ├── fonts/
│   └── scripts/
│
├── tests/                      # Unit tests (TODO)
└── docs/                       # Ek dokümanlar (TODO)
```

---

## Özellikler

### ✅ Tamamlandı (v0.1.0)

- [x] **Windowing**: GLFW + EGL + Termux:X11
- [x] **Renderer**: OpenGL ES 3.0 batched 2D (4096 quad/batch)
- [x] **Editor UI**: ImGui dockable panels (Unity-like layout)
- [x] **ECS**: Sparse-set, çoklu component, each() iteration
- [x] **Allocator**: Linear arena + Pool + stats
- [x] **Thread Pool**: Fixed-size, parallel-for
- [x] **Event Bus**: Type-safe pub/sub
- [x] **Asset Manager**: Texture yükleme (stb_image), hot reload
- [x] **Local DB**: SQLite (projects, settings, undo history)
- [x] **Backend**: Render.com (auth, projects, assets, scenes, multiplayer WS)
- [x] **Build system**: CMake + Ninja, clang

### 🚧 Geliştirilecek (v0.2.0+)

- [ ] 3D renderer (forward rendering, depth buffer)
- [ ] Box2D fizik entegrasyonu
- [ ] Bullet3 3D fizik
- [ ] OpenAL audio (3D positional)
- [ ] Lua scripting (sol2 binding)
- [ ] Scene JSON serializer (nlohmann/json)
- [ ] Undo/Redo system
- [ ] Asset browser (file picker)
- [ ] Texture atlas generator
- [ ] Animation system (sprite sheets)
- [ ] Particle system
- [ ] Shader editor (live preview)
- [ ] Touch input (Android)
- [ ] Build & deploy (APK packaging)

---

## Sorun Giderme

### "termux-x11: command not found"

```bash
pkg install termux-x11
```

Eğer F-Droid'den Termux:X11 add-on'u yüklemediyseniz, yükleyin.

### "proot-distro: command not found"

```bash
pkg install proot-distro
```

### GLFW window açılmıyor / "no EGL config"

```bash
# Termux:X11 uygulamasını açın, settings:
# - Display Resolution Mode: "Native" veya "1280x720"
# - Force Landscape: ON
# - Resume on sensor: OFF (manuel yönetim)
```

### OpenGL ES 3.0 desteklenmiyor hatası

Telefonunuzun GPU'su GLES3 desteklemiyor olabilir (eski cihazlar). Test:
```bash
# Debian proot içinde
glxinfo | grep "OpenGL ES"
```

Eğer yoksa, `scripts/build/build.sh` içinde `POCKET_USE_OPENGL_ES=OFF` yapın (desktop GL kullanır).

### Build sırasında "imgui.h not found"

```bash
# ImGui clone edilmemiş - manuel yapın
proot-distro login debian --user root -- \
    git clone --depth 1 https://github.com/ocornut/imgui.git /opt/imgui
```

### Render backend yanıt vermiyor

Backend free tier'da uyur. İlk istek 30-60 saniye sürebilir:
```bash
curl https://pocketengine-server.onrender.com/
# Bekleyin, sonra tekrar deneyin
```

### Disk dolu

proot Debian ~1.5GB yer kaplar. Kontrol:
```bash
du -sh /data/data/com.termux/files/usr/var/lib/proot-distro/installed-rootfs/debian
```

Temizlik:
```bash
proot-distro remove debian  # tamamen sil
# sonra setup'ı tekrar çalıştır
```

---

## Katkıda Bulunma

Bu motor MVP aşamasında. Katkılar memnuniyetle karşılanır:

1. Fork'layın
2. Feature branch açın (`git checkout -b feature/my-feature`)
3. Commit'leyin
4. PR açın

### Kod standartları

- C++20
- clang-format (Google style)
- snake_case dosya isimleri
- `pk::` namespace
- Header guards yerine `#pragma once`
- `[[nodiscard]]` const getter'larda

### Build flags

```bash
# Release (production)
-DCMAKE_BUILD_TYPE=Release -O3 -flto -march=native

# Debug
-DCMAKE_BUILD_TYPE=Debug -O0 -g3 -fsanitize=address,undefined

# RelWithDebInfo (profil)
-DCMAKE_BUILD_TYPE=RelWithDebInfo -O2 -g2
```

---

## Lisans

MIT — `LICENSE` dosyasına bakın.

---

## İletişim

- GitHub Issues: https://github.com/mehmetdem2005/PocketEngine-Termux/issues
- Backend: https://pocketengine-server.onrender.com

---

**PocketEngine** — *Unity'in gücü, telefonun cepte.*
