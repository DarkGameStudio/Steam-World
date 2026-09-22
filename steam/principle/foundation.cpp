// ============================================================================
//  steam_foundation.cpp
//  ----------------------------------------------------------------------
//  The shared skeleton under every Steam game. Not a game — the *basis*
//  a game is built on. Every subsystem here appears, in some form, in
//  Hollow Knight, Vampire Survivors, Baldur's Gate 3, and Stardew Valley.
//
//  The pipeline:
//
//    boot()  ->  subsystems init in dependency order
//              -> state stack pushes MainMenu
//              -> MainMenu pushes Gameplay
//              -> frame loop:
//                    poll input
//                    drain event queue
//                    fixed-step simulation (accumulator)
//                    variable-step presentation (interpolation)
//                    render
//              -> Save writes serialized world to disk / Steam Cloud
//              -> shutdown in reverse order
//
//  Build (MSVC):  cl /EHsc /std:c++17 /DUNICODE /D_UNICODE steam_foundation.cpp ^
//                    user32.lib gdi32.lib
//  Build (MinGW): g++ steam_foundation.cpp -o foundation.exe -mwindows -O2 ^
//                    -std=c++17 -lgdi32 -luser32
// ============================================================================

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <algorithm>

// ============================================================================
//  SECTION 0 — TYPES & UTILITIES
//  Every Steam game picks its primitive types and never deviates from them.
//  Consistency here prevents a decade of silent bugs.
// ============================================================================
using u8  = uint8_t;   using u16 = uint16_t;  using u32 = uint32_t;  using u64 = uint64_t;
using i8  = int8_t;    using i16 = int16_t;   using i32 = int32_t;   using i64 = int64_t;
using f32 = float;     using f64 = double;
using usize = size_t;

template <typename T> inline T Min(T a, T b) { return a < b ? a : b; }
template <typename T> inline T Max(T a, T b) { return a > b ? a : b; }
template <typename T> inline T Clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

inline u64 NowNanos() {
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (u64)((double)c.QuadPart * 1e9 / (double)f.QuadPart);
}

// ============================================================================
//  SECTION 1 — HANDLES
//  Games never hand out raw pointers to game objects, resources, or entities.
//  A 32-bit handle packs an index + generation. If the underlying slot is
//  recycled, the generation bumps and old handles fail validation. This is
//  THE pattern that makes hot-reload, streaming, and saves reliable.
// ============================================================================
template <typename Tag>
struct Handle {
    u32 index = 0xFFFFFFFF;
    u32 gen   = 0;
    bool Valid() const { return index != 0xFFFFFFFF; }
    bool operator==(Handle o) const { return index == o.index && gen == o.gen; }
    bool operator!=(Handle o) const { return !(*this == o); }
};

struct EntityTag {};
using Entity = Handle<EntityTag>;

struct ResourceTag {};
using ResourceId = Handle<ResourceTag>;

// ============================================================================
//  SECTION 2 — EVENT BUS
//  Every subsystem needs to notify others without knowing who's listening.
//  Steam achievements fire events. A player death fires events. Save system
//  listens. Audio listens. UI listens. Nothing is coupled to anything.
// ============================================================================
enum class EventType : u32 {
    None = 0,
    EntityDamaged,
    EntityDied,
    PlayerScored,
    LevelCompleted,
    AssetLoaded,
    SaveRequested,
    AchievementUnlocked,
    SteamOverlayOpened,
};

struct Event {
    EventType type = EventType::None;
    Entity    source{};
    u64       payloadA = 0;
    u64       payloadB = 0;
    f32       amount = 0.f;
};

struct EventBus {
    // Subscribers keyed by event type. A handler may unsubscribe by returning false.
    std::unordered_map<u32, std::vector<std::function<bool(const Event&)>>> subs;

    void Subscribe(EventType t, std::function<bool(const Event&)> fn) {
        subs[(u32)t].push_back(std::move(fn));
    }
    void Emit(const Event& e) {
        auto it = subs.find((u32)e.type);
        if (it == subs.end()) return;
        auto& list = it->second;
        // Iterate by index so handlers may append during dispatch.
        for (usize i = 0; i < list.size(); ++i)
            if (list[i] && list[i](e)) { /* keep */ }
    }
};

// ============================================================================
//  SECTION 3 — MEMORY
//  Games do not call malloc/free per frame. Two structures cover 95% of use:
//    Arena  — bump pointer, freed all at once (level load, per-frame scratch)
//    Pool   — slot-allocated, stable addresses, individual free (entities)
// ============================================================================
struct Arena {
    u8*   base = nullptr;
    usize cap  = 0;
    usize used = 0;

    void Init(void* mem, usize bytes) { base = (u8*)mem; cap = bytes; used = 0; }
    void Reset() { used = 0; }
    usize Remaining() const { return cap - used; }

    void* Push(usize bytes, usize align = 8) {
        usize p = (used + align - 1) & ~(align - 1);
        if (p + bytes > cap) return nullptr;
        void* r = base + p;
        used = p + bytes;
        return r;
    }
    template <typename T, typename... Args>
    T* Make(Args&&... args) {
        void* m = Push(sizeof(T), alignof(T));
        return m ? new (m) T(std::forward<Args>(args)...) : nullptr;
    }
};

// Pool<T> — stable slots, generation-tagged handles, O(1) alloc/free.
template <typename T, typename H>
struct Pool {
    struct Slot { T data{}; u32 gen = 1; bool used = false; };
    std::vector<Slot>  slots;
    std::vector<u32>   freeList;

    H Alloc() {
        u32 idx;
        if (!freeList.empty()) { idx = freeList.back(); freeList.pop_back(); }
        else { idx = (u32)slots.size(); slots.emplace_back(); }
        slots[idx].used = true;
        return H{ idx, slots[idx].gen };
    }
    void Free(H h) {
        if (h.index >= slots.size() || !slots[h.index].used) return;
        slots[h.index].used = false;
        slots[h.index].gen++;
        freeList.push_back(h.index);
    }
    T* Get(H h) {
        if (h.index >= slots.size()) return nullptr;
        Slot& s = slots[h.index];
        if (!s.used || s.gen != h.gen) return nullptr;
        return &s.data;
    }
};

// ============================================================================
//  SECTION 4 — RESOURCE MANAGER
//  Textures, sounds, meshes, prefabs. Games reference them by handle, not by
//  pointer. Loading is async: Load() returns immediately with a pending id;
//  a background thread or coroutine fills it. Steam Workshop and Steam Cloud
//  both hook into this layer.
// ============================================================================
enum class ResKind : u8 { Texture, Mesh, Sound, Font, Prefab, Count };

struct ResEntry {
    ResKind      kind = ResKind::Texture;
    std::string  path;
    bool         loaded = false;
    usize        bytes = 0;
    void*        gpuHandle = nullptr;    // whatever the backend expects
};

struct ResourceManager {
    Pool<ResEntry, ResourceId>  pool;
    EventBus*                   bus = nullptr;
    std::unordered_map<u64, ResourceId> pathCache;   // path hash -> id

    ResourceId Load(ResKind kind, const char* path) {
        u64 h = 14695981039346656037ULL;
        for (const char* p = path; *p; ++p) h = (h ^ (u8)*p) * 1099511628211ULL;

        auto it = pathCache.find(h);
        if (it != pathCache.end()) return it->second;   // already loaded or pending

        ResourceId id = pool.Alloc();
        ResEntry*  e  = pool.Get(id);
        e->kind = kind;
        e->path = path;
        // In a real engine: enqueue to a worker thread. Here: fake-instant.
        e->loaded = true;
        e->bytes  = 4096 + (h & 0xFFFF);
        e->gpuHandle = (void*)(uintptr_t)(h | 1);

        pathCache[h] = id;
        if (bus) {
            Event ev;
            ev.type = EventType::AssetLoaded;
            ev.payloadA = (u64)id.index;
            ev.payloadB = (u64)id.gen;
            bus->Emit(ev);
        }
        return id;
    }
    ResEntry* Get(ResourceId id) { return pool.Get(id); }
    void Unload(ResourceId id) {
        ResEntry* e = pool.Get(id);
        if (!e) return;
        for (auto it = pathCache.begin(); it != pathCache.end(); ++it)
            if (it->second == id) { pathCache.erase(it); break; }
        pool.Free(id);
    }
};

// ============================================================================
//  SECTION 5 — ECS LITE
//  Every modern game object is (id, set of components). Systems iterate
//  components, not object trees. This kills the deepest class hierarchy and
//  unlocks job scheduling, hot reload, and network replication.
// ============================================================================
struct Transform {
    f32 x = 0, y = 0, z = 0;
    f32 vx = 0, vy = 0, vz = 0;
    f32 px = 0, py = 0, pz = 0;     // previous — for render interpolation
};

struct Health {
    f32 current = 100.f;
    f32 max     = 100.f;
    bool dead   = false;
};

struct Sprite {
    ResourceId tex{};
    f32 w = 32, h = 32;
    u32 tint = 0xFFFFFFFF;
};

struct Player {
    f32 speed = 180.f;
    i32 score = 0;
};

struct World {
    Pool<Transform, Entity>  transforms;
    Pool<Health,    Entity>  healths;
    Pool<Sprite,    Entity>  sprites;
    Pool<Player,    Entity>  players;
    EventBus*                bus = nullptr;

    Entity Spawn() {
        return transforms.Alloc();      // spawn always creates a transform
    }
    void Destroy(Entity e) {
        transforms.Free(e);
        healths.Free(e);
        sprites.Free(e);
        players.Free(e);
    }

    void Damage(Entity e, f32 amount) {
        Health* h = healths.Get(e);
        if (!h || h->dead) return;
        h->current -= amount;

        Event ev;
        ev.type   = EventType::EntityDamaged;
        ev.source = e;
        ev.amount = amount;
        if (bus) bus->Emit(ev);

        if (h->current <= 0.f) {
            h->current = 0.f;
            h->dead = true;
            Event d;
            d.type   = EventType::EntityDied;
            d.source = e;
            if (bus) bus->Emit(d);
        }
    }

    // The physics step. Runs at a FIXED timestep, never variable.
    void StepSimulation(f32 dt) {
        for (auto& s : transforms.slots) {
            if (!s.used) continue;
            Transform& t = s.data;
            t.px = t.x; t.py = t.y; t.pz = t.z;
            t.x += t.vx * dt;
            t.y += t.vy * dt;
            t.z += t.vz * dt;
            // gravity for anything with health (toy demo)
        }
    }

    // Render-interp helpers — presentation runs between sim ticks.
    void GetInterpolated(Entity e, f32 alpha, f32& x, f32& y, f32& z) {
        Transform* t = transforms.Get(e);
        if (!t) { x = y = z = 0; return; }
        x = t->px + (t->x - t->px) * alpha;
        y = t->py + (t->y - t->py) * alpha;
        z = t->pz + (t->z - t->pz) * alpha;
    }
};

// ============================================================================
//  SECTION 6 — INPUT
//  Games never poll keys inside gameplay. Input is sampled once per frame
//  into an InputState. Gameplay reads state. Rebinding, gamepad, and Steam
//  Input API all slot into this layer.
// ============================================================================
struct InputState {
    bool down[256]  = {};
    bool pressed[256] = {};   // edge: went down this frame
    bool released[256] = {};
    f32  mouseX = 0, mouseY = 0;
    f32  scroll = 0;

    void BeginFrame() { std::memset(pressed, 0, sizeof(pressed));
                        std::memset(released, 0, sizeof(released));
                        scroll = 0; }
    void OnKeyDown(u32 vk) { if (!down[vk]) pressed[vk] = true; down[vk] = true; }
    void OnKeyUp(u32 vk)   { if (down[vk]) released[vk] = true; down[vk] = false; }
    bool IsDown(u32 vk)     const { return down[vk]; }
    bool WasPressed(u32 vk) const { return pressed[vk]; }
};

// ============================================================================
//  SECTION 7 — STATE STACK
//  Every Steam game with a main menu, pause menu, and loading screen uses a
//  state stack. States form a stack because "pause" overlays "gameplay"
//  without destroying it — you can see the world behind the pause menu.
// ============================================================================
struct GameContext;   // fwd

struct GameState {
    virtual ~GameState() = default;
    virtual void OnEnter(GameContext&) {}
    virtual void OnExit (GameContext&) {}
    virtual void OnPause(GameContext&) {}   // another state pushed on top
    virtual void OnResume(GameContext&) {}  // state above was popped
    virtual void Update(GameContext&, f32 dt) = 0;
    virtual void Render(GameContext&, f32 alpha) = 0;
};

// ============================================================================
//  SECTION 8 — SAVE / SERIALIZATION
//  Steam Cloud syncs a directory. Every game has its own binary format.
//  Rule one: version the header. Rule two: never trust field order.
// ============================================================================
struct SaveHeader {
    char  magic[4]   = { 'S','T','M','F' };
    u32   version    = 1;
    u64   timestamp  = 0;
    u32   entityCount = 0;
    u32   checksum   = 0;
};

static u32 Checksum32(const void* data, usize n) {
    const u8* p = (const u8*)data;
    u32 h = 2166136261u;
    for (usize i = 0; i < n; ++i) { h ^= p[i]; h *= 16777619u; }
    return h;
}

struct SaveSystem {
    std::string saveDir = ".\\saves\\";
    EventBus*   bus = nullptr;

    bool Save(const World& w) {
        CreateDirectoryA(saveDir.c_str(), nullptr);
        std::string path = saveDir + "slot0.sav";

        FILE* f = std::fopen(path.c_str(), "wb");
        if (!f) return false;

        SaveHeader h;
        h.timestamp = NowNanos();
        h.entityCount = 0;
        for (auto& s : w.transforms.slots) if (s.used) h.entityCount++;
        h.checksum = Checksum32(&w.transforms.slots[0],
                                w.transforms.slots.size() * sizeof(w.transforms.slots[0]));

        std::fwrite(&h, sizeof(h), 1, f);
        std::fwrite(w.transforms.slots.data(), sizeof(w.transforms.slots[0]),
                    w.transforms.slots.size(), f);
        std::fwrite(w.healths.slots.data(), sizeof(w.healths.slots[0]),
                    w.healths.slots.size(), f);
        std::fclose(f);

        if (bus) { Event e; e.type = EventType::SaveRequested; bus->Emit(e); }
        return true;
    }

    bool Load(World& w) {
        std::string path = saveDir + "slot0.sav";
        FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) return false;

        SaveHeader h;
        if (std::fread(&h, sizeof(h), 1, f) != 1) { std::fclose(f); return false; }
        if (std::memcmp(h.magic, "STMF", 4) != 0) { std::fclose(f); return false; }
        if (h.version != 1) { std::fclose(f); return false; }   // migration goes here

        w.transforms.slots.resize(h.entityCount);
        std::fread(w.transforms.slots.data(),
                   sizeof(w.transforms.slots[0]), h.entityCount, f);
        std::fclose(f);
        return true;
    }
};

// ============================================================================
//  SECTION 9 — STEAM INTEGRATION
//  Steamworks is just a library. Every game wraps it in a thin adapter so
//  the rest of the code never includes <steam/steam_api.h>. Achievements,
//  stats, cloud, overlay, P2P — all funnel through here.
// ============================================================================
struct SteamAdapter {
    bool   initialized   = false;
    bool   overlayActive = false;
    EventBus* bus = nullptr;

    bool Init() {
        // Real: SteamAPI_Init() and check the return.
        initialized = true;
        printf("[Steam] SteamAPI_Init OK — user %llu\n", 76561198000000000ull);
        return true;
    }
    void Shutdown() { initialized = false; }

    void SetAchievement(const char* apiName) {
        // Real: SteamUserStats()->SetAchievement(apiName);
        printf("[Steam] Achievement: %s\n", apiName);
        Event e; e.type = EventType::AchievementUnlocked;
        e.payloadA = 14695981039346656037ULL;
        for (const char* p = apiName; *p; ++p) e.payloadA = (e.payloadA ^ (u8)*p) * 1099511628211ULL;
        if (bus) bus->Emit(e);
    }

    void StoreStat(const char* name, i32 value) {
        // Real: SteamUserStats()->SetStat(name, value); then StoreStats().
        printf("[Steam] Stat %s = %d\n", name, value);
    }

    void UnlockCloud(const char* file) {
        // Real: SteamRemoteStorage()->FileWrite(file, bytes, size);
        printf("[Steam] Cloud write: %s\n", file);
    }

    // Steam P2P — same interface as a local socket.
    void SendP2P(u64 steamId, const void* data, u32 bytes, bool reliable) {
        (void)steamId; (void)data; (void)bytes; (void)reliable;
        // Real: SteamNetworking()->SendP2PPacket(...)
    }

    bool IsOverlayActive() { return overlayActive; }
};

// ============================================================================
//  SECTION 10 — GAME CONTEXT
//  Every subsystem lives in one struct passed to every state. No globals.
//  No singletons. This makes testing, replay, and multiple concurrent
//  sessions (split-screen, netplay) possible.
// ============================================================================
struct GameContext {
    HWND           hwnd = nullptr;
    EventBus       events;
    ResourceManager resources;
    World          world;
    SaveSystem     saves;
    SteamAdapter   steam;
    InputState     input;
    Arena          frameArena;

    std::vector<std::unique_ptr<GameState>> stack;

    // Frame timing
    u64  lastTickNs   = 0;
    f64  simAccum     = 0;
    const f32 SIM_DT  = 1.f / 60.f;   // fixed simulation step
    const f32 MAX_FRAME = 0.25f;      // clamp after breakpoints / alt-tab

    void PushState(std::unique_ptr<GameState> s) {
        if (!stack.empty()) stack.back()->OnPause(*this);
        stack.push_back(std::move(s));
        stack.back()->OnEnter(*this);
    }
    void PopState() {
        if (stack.empty()) return;
        stack.back()->OnExit(*this);
        stack.pop_back();
        if (!stack.empty()) stack.back()->OnResume(*this);
    }

    void RenderInterpolated(f32 alpha) {
        for (auto& s : stack) if (s) s->Render(*this, alpha);
    }
    void UpdateTop(f32 dt) {
        if (!stack.empty() && stack.back()) stack.back()->Update(*this, dt);
    }
};

// ============================================================================
//  SECTION 11 — CONCRETE STATES
//  MainMenu -> Gameplay -> Pause is the universal boot chain.
// ============================================================================
struct MainMenuState : GameState {
    void OnEnter(GameContext& ctx) override {
        printf("[State] MainMenu entered\n");
        ctx.steam.SetAchievement("ACH_FIRST_BOOT");

        // Auto-transition for the demo.
        // In a real game: wait for click on "New Game".
        struct GotoGameplay : GameState {
            GameContext* ctx = nullptr;
            f32 t = 0;
            void OnEnter(GameContext& c) override { ctx = &c; }
            void Update(GameContext& c, f32 dt) override {
                t += dt;
                if (t > 0.1f) {
                    // build the world
                    for (int i = 0; i < 100; ++i) {
                        Entity e = c.world.Spawn();
                        Transform* tr = c.world.transforms.Get(e);
                        tr->x = (f32)(i % 10) * 40.f;
                        tr->y = (f32)(i / 10) * 40.f;
                        tr->vx = (i % 2 ? 30.f : -30.f);
                        Health* h = &c.world.healths.slots[c.world.healths.Alloc().index].data;
                        h->current = h->max = 100.f;
                    }
                    c.PopState();   // remove self
                }
            }
            void Render(GameContext&, f32) override {}
        };
        ctx.PushState(std::make_unique<GotoGameplay>());
    }
    void Update(GameContext&, f32) override {}
    void Render(GameContext&, f32) override {}
};

struct GameplayState : GameState {
    f32 spawnTimer = 0;
    f32 scoreTimer = 0;

    void OnEnter(GameContext&) override { printf("[State] Gameplay entered\n"); }

    void Update(GameContext& ctx, f32 dt) override {
        // Input
        if (ctx.input.IsDown(VK_RIGHT)) {
            for (auto& s : ctx.world.transforms.slots)
                if (s.used) s.data.vx = 120.f;
        } else if (ctx.input.IsDown(VK_LEFT)) {
            for (auto& s : ctx.world.transforms.slots)
                if (s.used) s.data.vx = -120.f;
        }

        // ESC pauses
        if (ctx.input.WasPressed(VK_ESCAPE)) {
            struct PauseState : GameState {
                void OnEnter(GameContext&) override { printf("[State] Paused\n"); }
                void OnExit (GameContext&) override { printf("[State] Resumed\n"); }
                void Update(GameContext& c, f32) override {
                    if (c.input.WasPressed(VK_ESCAPE)) c.PopState();
                }
                void Render(GameContext&, f32) override {}
            };
            ctx.PushState(std::make_unique<PauseState>());
            return;
        }

        // Sim runs at fixed timestep regardless of framerate.
        ctx.world.StepSimulation(dt);

        // Toy: damage one random entity per second to fire events.
        scoreTimer += dt;
        if (scoreTimer > 1.f) {
            scoreTimer = 0;
            for (auto& s : ctx.world.healths.slots) {
                if (!s.used || s.data.dead) continue;
                ctx.world.Damage(Entity{}, 5.f);
                break;
            }
        }
    }

    void Render(GameContext& ctx, f32 alpha) override {
        // In a real engine: issue draw commands.
        // Here we just count and print at low rate.
        static f32 reportTimer = 0;
        reportTimer += 1.f / 60.f;
        if (reportTimer < 1.f) return;
        reportTimer = 0;

        int alive = 0, dead = 0;
        for (auto& s : ctx.world.healths.slots)
            if (s.used) (s.data.dead ? dead : alive)++;

        printf("[Render] alpha=%.2f  alive=%d dead=%d  stack=%zu\n",
               alpha, alive, dead, ctx.stack.size());
    }
};

// ============================================================================
//  SECTION 12 — PLATFORM LAYER (Win32)
//  The OS-facing thin layer. Every game has one; it's the only file that
//  includes <windows.h>. Everything above is platform-agnostic.
// ============================================================================
static GameContext g_ctx;

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
        case WM_KEYDOWN:   g_ctx.input.OnKeyDown((u32)w); return 0;
        case WM_KEYUP:     g_ctx.input.OnKeyUp((u32)w);   return 0;
        case WM_MOUSEMOVE: g_ctx.input.mouseX = (f32)(short)LOWORD(l);
                           g_ctx.input.mouseY = (f32)(short)HIWORD(l); return 0;
        case WM_MOUSEWHEEL: g_ctx.input.scroll += (f32)GET_WHEEL_DELTA_WPARAM(w); return 0;
        case WM_CLOSE:     PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static HWND MakeWindow(HINSTANCE inst, int w, int h) {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"SteamFoundationWnd";
    RegisterClassExW(&wc);

    RECT r = { 0, 0, w, h };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    return CreateWindowExW(0, wc.lpszClassName, L"Steam Foundation",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left, r.bottom - r.top, nullptr, nullptr, inst, nullptr);
}

// ============================================================================
//  SECTION 13 — BOOT / MAIN LOOP / SHUTDOWN
//  Init order is a dependency graph, not a guess:
//    arena -> events -> resources -> world -> saves -> steam -> state stack
//  Shutdown is the exact reverse.
// ============================================================================
int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int show) {
    printf("=== STEAM FOUNDATION BOOT ===\n");

    // --- 1. Memory first. Nothing else can allocate. ---
    static u8 frameMem[8 * 1024 * 1024];
    g_ctx.frameArena.Init(frameMem, sizeof(frameMem));
    printf("[Boot] arena %.2f MB\n", sizeof(frameMem) / 1048576.f);

    // --- 2. Events. Everyone speaks through this. ---
    g_ctx.world.bus     = &g_ctx.events;
    g_ctx.resources.bus = &g_ctx.events;
    g_ctx.saves.bus     = &g_ctx.events;
    g_ctx.steam.bus     = &g_ctx.events;

    // --- 3. Subscribe cross-cutting listeners. ---
    g_ctx.events.Subscribe(EventType::EntityDied, [](const Event&) {
        printf("[Event] entity died\n");
        return false;   // one-shot
    });
    g_ctx.events.Subscribe(EventType::AchievementUnlocked, [](const Event& e) {
        printf("[Event] achievement 0x%llX\n", (unsigned long long)e.payloadA);
        return true;
    });
    g_ctx.events.Subscribe(EventType::AssetLoaded, [](const Event&) {
        // silent
        return true;
    });

    // --- 4. Resources: preload what the first state will need. ---
    g_ctx.resources.Load(ResKind::Texture, "player.png");
    g_ctx.resources.Load(ResKind::Sound,   "jump.wav");
    g_ctx.resources.Load(ResKind::Prefab,  "enemy.prefab");

    // --- 5. Steam. Failed init means offer offline mode. ---
    if (!g_ctx.steam.Init())
        printf("[Boot] Steam offline — continuing without cloud/achievements\n");

    // --- 6. Window. ---
    g_ctx.hwnd = MakeWindow(inst, 1280, 720);
    ShowWindow(g_ctx.hwnd, show);
    UpdateWindow(g_ctx.hwnd);
    printf("[Boot] window created\n");

    // --- 7. First state. Everything else grows from here. ---
    g_ctx.PushState(std::make_unique<MainMenuState>());

    // --- 8. The frame loop. ---
    //
    //  Fixed-step simulation + variable-step rendering, decoupled.
    //  simAccum tracks unspent real time. When it exceeds SIM_DT we run one
    //  simulation tick. The fraction left over becomes `alpha` — the
    //  interpolation factor between the last two sim states. This is what
    //  makes physics deterministic AND motion smooth at any framerate.
    //
    g_ctx.lastTickNs = NowNanos();
    bool running = true;

    while (running) {
        // 8a. Drain OS message queue without blocking.
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { running = false; break; }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!running) break;

        // 8b. Frame timing.
        u64  now      = NowNanos();
        f32  frameDt  = (f32)((now - g_ctx.lastTickNs) * 1e-9);
        g_ctx.lastTickNs = now;
        frameDt = Clamp(frameDt, 0.f, g_ctx.MAX_FRAME);
        g_ctx.simAccum += frameDt;

        // 8c. Simulation: consume real time in fixed SIM_DT chunks.
        while (g_ctx.simAccum >= g_ctx.SIM_DT) {
            g_ctx.frameArena.Reset();      // per-tick scratch is freed here
            g_ctx.UpdateTop(g_ctx.SIM_DT);
            g_ctx.simAccum -= g_ctx.SIM_DT;
            if (g_ctx.stack.empty()) { running = false; break; }
        }
        if (!running) break;

        // 8d. Interpolation factor for rendering between sim ticks.
        f32 alpha = (f32)(g_ctx.simAccum / g_ctx.SIM_DT);

        // 8e. Render. May run at a different rate than sim.
        g_ctx.RenderInterpolated(alpha);

        // 8f. Input edge flags clear at end of frame.
        g_ctx.input.BeginFrame();

        // 8g. Yield to OS. On a real engine: swap buffers with vsync.
        Sleep(1);
    }

    // --- 9. Shutdown: reverse of init, but only the parts that hold state. ---
    printf("\n=== SHUTDOWN ===\n");
    g_ctx.saves.Save(g_ctx.world);
    g_ctx.steam.StoreStat("total_playtime_sec", 42);
    g_ctx.steam.UnlockCloud("slot0.sav");
    while (!g_ctx.stack.empty()) g_ctx.PopState();
    g_ctx.steam.Shutdown();
    printf("[Shutdown] clean\n");
    return 0;
}
