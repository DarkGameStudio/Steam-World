// steam_clone_math.cpp — Steam-like launcher with advanced math in pure Win32 C++
// Build (MinGW):  g++ steam_clone_math.cpp -o steam.exe -mwindows -O2 -std=c++17 -lgdi32 -luser32
// Build (MSVC):   cl /EHsc /std:c++17 /DUNICODE /D_UNICODE steam_clone_math.cpp user32.lib gdi32.lib

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <cstdint>

// ============================================================ MATH CORE
static const float PI = 3.14159265358979f;

struct Vec2 {
    float x = 0, y = 0;
    Vec2 operator+(Vec2 o) const { return { x + o.x, y + o.y }; }
    Vec2 operator-(Vec2 o) const { return { x - o.x, y - o.y }; }
    Vec2 operator*(float s) const { return { x * s, y * s }; }
    Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
};

static float Clamp(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static float Lerp(float a, float b, float t)   { return a + (b - a) * t; }
static float Smoothstep(float t)                { return t * t * (3.f - 2.f * t); }

// --- Easing curves (polynomial + Bézier) ---
static float EaseOutCubic(float t)   { return 1.f - powf(1.f - t, 3.f); }
static float EaseInOutCubic(float t) {
    return t < 0.5f ? 4.f * t * t * t : 1.f - powf(-2.f * t + 2.f, 3.f) * 0.5f;
}
static float EaseOutBack(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1.f;
    return 1.f + c3 * powf(t - 1.f, 3.f) + c1 * powf(t - 1.f, 2.f);
}
// Solve cubic Bézier y(x) via Newton-Raphson iteration on x(t) = target.
static float BezierEase(float x1, float y1, float x2, float y2, float target) {
    float u = target;
    for (int i = 0; i < 6; ++i) {
        float x  = 3.f*(1-u)*(1-u)*u*x1 + 3.f*(1-u)*u*u*x2 + u*u*u;
        float dx = 3.f*(1-u)*(1-u)*x1 + 6.f*(1-u)*u*(x2-x1) + 3.f*u*u*(1.f-x2);
        if (fabsf(dx) < 1e-4f) break;
        u = Clamp(u - (x - target) / dx, 0.f, 1.f);
    }
    return 3.f*(1-u)*(1-u)*u*y1 + 3.f*(1-u)*u*u*y2 + u*u*u;
}

// --- Critically damped spring: x'' = k(target - x) - c*x' ---
struct Spring {
    float x = 0, v = 0, target = 0;
    float k = 120.f, c = 22.f;   // k stiffness, c damping (c ≈ 2√k for critical)
    void Set(float t) { target = t; }
    void Snap(float t) { x = target = t; v = 0; }
    void Step(float dt) {
        float a = k * (target - x) - c * v;
        v += a * dt;
        x += v * dt;
    }
};

// --- Value noise + fractal Brownian motion (fBm) ---
static uint32_t Hash(int x, int y, int seed) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float Rand01(int x, int y, int seed) {
    return Hash(x, y, seed) / (float)0xFFFFFFFFu;
}
static float ValueNoise(float x, float y, int seed) {
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float xf = x - xi, yf = y - yi;
    float u = Smoothstep(xf), v = Smoothstep(yf);
    float a = Rand01(xi,     yi,     seed);
    float b = Rand01(xi + 1, yi,     seed);
    float c = Rand01(xi,     yi + 1, seed);
    float d = Rand01(xi + 1, yi + 1, seed);
    return Lerp(Lerp(a, b, u), Lerp(c, d, u), v);
}
static float FBM(float x, float y, int seed, int octaves = 4) {
    float sum = 0.f, amp = 0.5f, freq = 1.f;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * ValueNoise(x * freq, y * freq, seed + i * 31);
        freq *= 2.f;
        amp  *= 0.5f;
    }
    return sum;
}

// --- Verlet particle with drag ---
struct Particle {
    Vec2 pos, prev;
    float life = 0, maxLife = 1;
    void Integrate(float dt, Vec2 accel, float drag) {
        Vec2 v = (pos - prev) * drag;
        prev = pos;
        pos = pos + v + accel * (dt * dt);
    }
};

// ============================================================ PALETTE
static const COLORREF C_BG    = RGB( 27,  40,  56);
static const COLORREF C_NAV   = RGB( 23,  33,  46);
static const COLORREF C_CARD  = RGB( 36,  52,  71);
static const COLORREF C_SEL   = RGB( 45,  66,  90);
static const COLORREF C_TEXT  = RGB(199, 213, 224);
static const COLORREF C_DIM   = RGB(143, 160, 175);
static const COLORREF C_BLUE  = RGB(102, 192, 244);
static const COLORREF C_GREEN = RGB(117, 176,  34);

static COLORREF Blend(COLORREF a, COLORREF b, float t) {
    t = Clamp(t, 0.f, 1.f);
    return RGB((int)(GetRValue(a)*(1-t) + GetRValue(b)*t),
               (int)(GetGValue(a)*(1-t) + GetGValue(b)*t),
               (int)(GetBValue(a)*(1-t) + GetBValue(b)*t));
}

// ============================================================ MODEL
struct Game {
    std::wstring title, genre, desc;
    int      price;
    double   sizeGB;
    COLORREF accent;
    int      noiseSeed;
    bool     installed  = false;
    bool     installing = false;
    float    progress   = 0.f;
    float    dlTime     = 0.f;   // drives fBm-modulated download speed
};

static std::vector<Game> g_games = {
 { L"Neon Drift",        L"Racing",     L"Tear through a rain-slicked megacity at 300 km/h. Build your ride, outrun the law, and chase the perfect line across 40 neon circuits.", 1999,  8.4, RGB(255,  82, 120), 11 },
 { L"Hollow Depths",     L"Roguelike",  L"Descend into a procedurally generated abyss. Every run reshapes the dungeon, every death teaches you something new.",                    2499,  3.2, RGB( 90, 200, 180), 23 },
 { L"Starforge Tactics", L"Strategy",   L"Command a fleet across a living galaxy. Diplomacy, logistics and orbital bombardment — all in real time.",                              3999, 22.1, RGB(120, 140, 255), 37 },
 { L"Pixel Harvest",     L"Simulation", L"A cozy farming sim with a difference: your crops keep growing in real time, even while you are away.",                                   1499,  1.1, RGB(240, 180,  70), 51 },
 { L"Void Runner",       L"Action",     L"Free-to-play parkour shooter. Chain wall-runs, grapple swings and headshots in zero-gravity arenas.",                                    0, 14.7, RGB(180, 110, 240), 67 },
 { L"Chrono Divide",     L"RPG",        L"A 60-hour open-world RPG where every choice forks the timeline. Six endings. One you.",                                                  5999, 48.9, RGB( 80, 200, 120), 83 },
};

// Per-card animation springs (position, hover glow, press squash)
struct CardAnim {
    Spring hover;      // 0..1
    Spring press;      // 0..1
    Spring lift;       // pixels lifted on hover
};
static CardAnim g_cardAnim[16];
static Spring   g_tabIndicator;   // slides under active tab
static Spring   g_viewSlide;      // incoming-view offset (px)
static std::vector<Particle> g_dust;

// ============================================================ UI STATE
enum { H_NONE = 0, H_TAB_STORE = 1, H_TAB_LIB = 2, H_TAB_COMM = 3,
       H_CARD = 100, H_CARD_BTN = 200, H_LIB = 300, H_DETAIL_BTN = 400 };

struct Hit { RECT r; int id; };
static HFONT g_fLogo, g_fHuge, g_fH1, g_fH2, g_fBody, g_fSmall, g_fBtn;
static int   g_cw = 1100, g_ch = 740;
static int   g_view = 0;
static int   g_prevView = 0;
static int   g_sel  = 0;
static int   g_hover = H_NONE;
static bool  g_tracking = false;
static float g_time = 0.f;
static float g_dt   = 0.f;

static std::vector<Hit>  g_hits;
static std::vector<RECT> g_cards, g_libRows;
static RECT g_tabStore, g_tabLib, g_tabComm, g_detailRect, g_detailBtn;

// ============================================================ GDI HELPERS
static void Fill(HDC dc, RECT r, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c);
    ::FillRect(dc, &r, b);
    DeleteObject(b);
}
static void FillRound(HDC dc, RECT r, int rad, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c);
    HPEN   p = CreatePen(PS_SOLID, 1, c);
    HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, p);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, rad, rad);
    SelectObject(dc, ob); SelectObject(dc, op);
    DeleteObject(b); DeleteObject(p);
}
static void Text(HDC dc, const std::wstring& s, RECT r, COLORREF c, HFONT f, UINT fmt) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, c);
    HGDIOBJ o = SelectObject(dc, f);
    DrawTextW(dc, s.c_str(), (int)s.size(), &r, fmt);
    SelectObject(dc, o);
}
static bool PtIn(const RECT& r, int x, int y) {
    return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
}
static int HitTest(int x, int y) {
    for (int i = (int)g_hits.size() - 1; i >= 0; --i)
        if (PtIn(g_hits[i].r, x, y)) return g_hits[i].id;
    return H_NONE;
}

// ============================================================ NOISE BANNER
// Renders the card banner as an animated fBm field: colour( i,j,t ) = Bl(dark, accent, fBm)
static void DrawNoiseBanner(HDC dc, RECT r, COLORREF accent, int seed, float t) {
    const int cols = 20, rows = 8;
    int cw = (r.right - r.left) / cols;
    int ch = (r.bottom - r.top) / rows;
    if (cw < 1 || ch < 1) return;

    COLORREF dark = Blend(accent, RGB(0, 0, 0), 0.72f);
    for (int j = 0; j < rows; ++j) {
        for (int i = 0; i < cols; ++i) {
            // Two scrolling fBm fields for organic motion
            float n = FBM(i * 0.30f + t * 0.55f, j * 0.30f + t * 0.20f, seed, 4);
            n = Clamp((n - 0.25f) * 1.6f, 0.f, 1.f);
            COLORREF c = Blend(dark, accent, n);
            RECT cr = { r.left + i*cw, r.top + j*ch,
                        r.left + (i+1)*cw + 1, r.top + (j+1)*ch + 1 };
            Fill(dc, cr, c);
        }
    }
}

// ============================================================ PARTICLES
static void SpawnDust(int w, int h) {
    Particle p;
    float x = (float)(rand() % w);
    p.pos = { x, (float)h + 4.f };
    p.prev = { x + (rand() % 7 - 3) * 0.3f, p.pos.y + 1.f };
    p.maxLife = 6.f + (rand() % 60) * 0.1f;
    p.life = 0.f;
    g_dust.push_back(p);
}
static void StepDust(float dt, int w, int h) {
    // Upward buoyancy + horizontal Perlin drift
    for (auto& p : g_dust) {
        float drift = (FBM(p.pos.x * 0.004f, p.pos.y * 0.004f + g_time * 0.2f, 7, 2) - 0.5f) * 12.f;
        Vec2 accel = { drift, -8.f };
        p.Integrate(dt, accel, 0.995f);
        p.life += dt;
    }
    // Reap
    for (int i = (int)g_dust.size() - 1; i >= 0; --i)
        if (g_dust[i].life > g_dust[i].maxLife || g_dust[i].pos.y < -10) g_dust.swap(g_dust.begin() + i), g_dust.pop_back();
    // Maintain count
    while ((int)g_dust.size() < 60) SpawnDust(w, h);
}
static void DrawDust(HDC dc, COLORREF bg) {
    for (auto& p : g_dust) {
        float a = p.life / p.maxLife;
        a = a < 0.5f ? a * 2.f : (1.f - a) * 2.f;   // fade in/out
        int s = 1 + (int)(a * 2.f);
        COLORREF c = Blend(bg, RGB(150, 190, 220), a * 0.35f);
        Fill(dc, { (int)p.pos.x, (int)p.pos.y, (int)p.pos.x + s, (int)p.pos.y + s }, c);
    }
}

// ============================================================ LAYOUT
static void ComputeLayout() {
    g_hits.clear(); g_cards.clear(); g_libRows.clear();

    g_tabStore = { 170, 10, 280, 46 };
    g_tabLib   = { 280, 10, 400, 46 };
    g_tabComm  = { 400, 10, 530, 46 };
    g_hits.push_back({ g_tabStore, H_TAB_STORE });
    g_hits.push_back({ g_tabLib,   H_TAB_LIB   });
    g_hits.push_back({ g_tabComm,  H_TAB_COMM  });

    if (g_view == 0) {
        const int pad = 24, gap = 18, top = 120, cardH = 260;
        int cardW = (g_cw - pad * 2 - gap * 2) / 3;
        for (int i = 0; i < (int)g_games.size(); ++i) {
            int col = i % 3, row = i / 3;
            RECT r = { pad + col * (cardW + gap), top + row * (cardH + gap), 0, 0 };
            r.right  = r.left + cardW;
            r.bottom = r.top + cardH;
            g_cards.push_back(r);
            g_hits.push_back({ r, H_CARD + i });
            RECT b = { r.right - 126, r.bottom - 52, r.right - 16, r.bottom - 16 };
            g_hits.push_back({ b, H_CARD_BTN + i });
        }
    } else if (g_view == 1) {
        int y = 88;
        for (int i = 0; i < (int)g_games.size(); ++i) {
            RECT r = { 32, y, 316, y + 64 };
            g_libRows.push_back(r);
            g_hits.push_back({ r, H_LIB + i });
            y += 70;
        }
        g_detailRect = { 348, 70, g_cw - 24, g_ch - 24 };
        g_detailBtn  = { g_detailRect.left + 24, g_detailRect.bottom - 76,
                         g_detailRect.left + 24 + 210, g_detailRect.bottom - 24 };
        g_hits.push_back({ g_detailBtn, H_DETAIL_BTN });
    }
}

// ============================================================ DRAWING
static std::wstring BtnLabel(const Game& g) {
    if (g.installed)  return L"PLAY";
    if (g.installing) return L"CANCEL";
    if (g.price > 0) { wchar_t b[64]; swprintf(b, 64, L"BUY  $%d.%02d", g.price/100, g.price%100); return b; }
    return L"INSTALL";
}
static COLORREF BtnColor(const Game& g) {
    if (g.installing) return RGB(70, 92, 112);
    if (g.installed || g.price == 0) return C_GREEN;
    return RGB(60, 130, 200);
}

static void DrawButton(HDC dc, RECT r, const std::wstring& label, COLORREF base, float hover, float press) {
    // Spring-driven squash: pressed scales horizontally +3%, vertically -3%
    int dx = (int)((r.right - r.left) * press * 0.03f);
    int dy = (int)((r.bottom - r.top) * press * 0.03f);
    RECT rr = { r.left - dx, r.top + dy, r.right + dx, r.bottom - dy };
    COLORREF c = Blend(base, RGB(255,255,255), hover * 0.18f + press * 0.08f);
    FillRound(dc, rr, 6, c);
    Text(dc, label, rr, RGB(255,255,255), g_fBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static void DrawNav(HDC dc) {
    Fill(dc, { 0, 0, g_cw, 56 }, C_NAV);
    Text(dc, L"STEAM", { 24, 0, 160, 56 }, C_TEXT, g_fLogo, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    struct Tab { RECT r; const wchar_t* label; int id; int view; };
    Tab tabs[] = {
        { g_tabStore, L"STORE",     H_TAB_STORE, 0 },
        { g_tabLib,   L"LIBRARY",   H_TAB_LIB,   1 },
        { g_tabComm,  L"COMMUNITY", H_TAB_COMM,  2 },
    };

    // Animated indicator: spring x interpolates between tab centres
    RECT active = tabs[g_view].r;
    float indX = g_tabIndicator.x;
    RECT ind = { (int)indX, 48, (int)indX + (active.right - active.left), 51 };
    FillRound(dc, ind, 3, C_BLUE);

    for (auto& t : tabs) {
        bool on = (g_view == t.view);
        if (on)                FillRound(dc, t.r, 6, C_SEL);
        else if (g_hover == t.id) FillRound(dc, t.r, 6, RGB(30, 44, 60));
        Text(dc, t.label, t.r, on ? C_TEXT : C_DIM, g_fBody,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

static void DrawStore(HDC dc) {
    Text(dc, L"FEATURED & RECOMMENDED", { 24, 70, g_cw - 24, 104 },
         C_TEXT, g_fH2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    for (int i = 0; i < (int)g_games.size(); ++i) {
        const Game& g = g_games[i];
        RECT base = g_cards[i];

        // Spring-driven lift on hover; pull card toward cursor vertically
        int lift = (int)g_cardAnim[i].lift.x;
        RECT r = { base.left, base.top - lift, base.right, base.bottom - lift };

        bool hot = (g_hover == H_CARD + i) || (g_hover == H_CARD_BTN + i);
        float hoverAmt = g_cardAnim[i].hover.x;

        FillRound(dc, r, 10, Blend(C_CARD, RGB(60, 90, 120), hoverAmt * 0.5f));

        // ---- Animated noise banner ----
        RECT ban = { r.left, r.top, r.right, r.top + 130 };
        HRGN rgn = CreateRoundRectRgn(r.left, r.top, r.right + 1, r.bottom + 1, 20, 20);
        SelectClipRgn(dc, rgn);
        float phase = g_time + i * 1.7f;
        DrawNoiseBanner(dc, ban, g.accent, g.noiseSeed, phase);

        // Title initial overlaid with blended colour for depth
        Text(dc, std::wstring(1, g.title[0]), ban,
             Blend(g.accent, RGB(0,0,0), 0.32f + hoverAmt * 0.15f),
             g_fHuge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectClipRgn(dc, NULL);
        DeleteObject(rgn);

        Text(dc, g.title, { r.left + 16, r.top + 142, r.right - 16, r.top + 170 },
             C_TEXT, g_fH2, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        wchar_t sub[96];
        swprintf(sub, 96, L"%s  ·  %.1f GB", g.genre.c_str(), g.sizeGB);
        Text(dc, sub, { r.left + 16, r.top + 168, r.right - 16, r.top + 190 },
             C_BLUE, g_fSmall, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        RECT btn = { r.right - 126, r.bottom - 52, r.right - 16, r.bottom - 16 };

        if (g.installing) {
            // Progress driven by fBm-modulated speed — see WM_TIMER
            FillRound(dc, btn, 6, RGB(20, 30, 42));
            RECT f = btn;
            f.right = btn.left + (int)((btn.right - btn.left) * g.progress);
            if (f.right > f.left + 4) FillRound(dc, f, 6, C_BLUE);
            wchar_t pct[16];
            swprintf(pct, 16, L"%d%%", (int)(g.progress * 100));
            Text(dc, pct, btn, RGB(255,255,255), g_fBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        } else {
            float press = g_cardAnim[i].press.x;
            DrawButton(dc, btn, BtnLabel(g), BtnColor(g), hoverAmt, press);
        }
    }
}

static void DrawLibrary(HDC dc) {
    // Global slide offset — the entire view eases into place on tab switch
    int slide = (int)g_viewSlide.x;

    FillRound(dc, { 24, 70 + slide, 324, g_ch - 24 + slide }, 10, C_NAV);

    for (int i = 0; i < (int)g_games.size(); ++i) {
        RECT r = g_libRows[i];
        r.top += slide; r.bottom += slide;
        const Game& g = g_games[i];
        bool sel = (i == g_sel);
        if (sel)                          FillRound(dc, r, 8, C_SEL);
        else if (g_hover == H_LIB + i)    FillRound(dc, r, 8, RGB(30, 44, 60));

        // Accent chip pulses with fBm — visual link to live data
        float pulse = 0.85f + 0.15f * FBM(i * 0.7f, g_time * 0.8f, g.noiseSeed, 3);
        FillRound(dc, { r.left + 12, r.top + 16, r.left + 44, r.top + 48 }, 6,
                  Blend(g.accent, RGB(0,0,0), 1.f - pulse));

        Text(dc, g.title, { r.left + 56, r.top + 6, r.right - 12, r.top + 36 },
             C_TEXT, g_fBody, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        std::wstring st = g.installed ? L"Installed"
                        : g.installing ? L"Downloading…" : L"Not installed";
        Text(dc, st, { r.left + 56, r.top + 34, r.right - 12, r.top + 58 },
             C_DIM, g_fSmall, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    const Game& g = g_games[g_sel];
    RECT d = g_detailRect;
    d.top += slide; d.bottom += slide;
    FillRound(dc, d, 10, C_NAV);

    RECT ban = { d.left + 16, d.top + 16, d.right - 16, d.top + 236 };
    HRGN rgn = CreateRoundRectRgn(ban.left, ban.top, ban.right + 1, ban.bottom + 1, 20, 20);
    SelectClipRgn(dc, rgn);
    DrawNoiseBanner(dc, ban, g.accent, g.noiseSeed, g_time + g_sel * 1.7f);
    Text(dc, std::wstring(1, g.title[0]), ban, Blend(g.accent, RGB(0,0,0), 0.32f),
         g_fHuge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectClipRgn(dc, NULL);
    DeleteObject(rgn);

    int y = ban.bottom + 16;
    Text(dc, g.title, { d.left + 24, y, d.right - 24, y + 40 },
         C_TEXT, g_fH1, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    y += 40;

    wchar_t meta[128];
    swprintf(meta, 128, L"%s   ·   %.1f GB", g.genre.c_str(), g.sizeGB);
    Text(dc, meta, { d.left + 24, y, d.right - 24, y + 24 },
         C_BLUE, g_fSmall, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    y += 32;

    Text(dc, g.desc, { d.left + 24, y, d.right - 24, g_detailBtn.top - 16 },
         C_DIM, g_fBody, DT_LEFT | DT_TOP | DT_WORDBREAK);

    if (g.installing) {
        RECT btn = g_detailBtn;
        FillRound(dc, btn, 6, RGB(20, 30, 42));
        RECT f = btn;
        f.right = btn.left + (int)((btn.right - btn.left) * g.progress);
        if (f.right > f.left + 4) FillRound(dc, f, 6, C_BLUE);
        wchar_t pct[48];
        swprintf(pct, 48, L"DOWNLOADING  %d%%", (int)(g.progress * 100));
        Text(dc, pct, btn, RGB(255,255,255), g_fBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else {
        float hover = (g_hover == H_DETAIL_BTN) ? 1.f : 0.f;
        DrawButton(dc, g_detailBtn, BtnLabel(g), BtnColor(g), hover, 0.f);
    }
}

static void DrawCommunity(HDC dc) {
    Text(dc, L"COMMUNITY", { 24, 70, g_cw - 24, 104 },
         C_TEXT, g_fH2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    Text(dc, L"Nothing here yet. Go install something.", { 24, 120, g_cw - 24, 160 },
         C_DIM, g_fBody, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

static void Paint(HDC dc) {
    ComputeLayout();
    Fill(dc, { 0, 0, g_cw, g_ch }, C_BG);

    DrawDust(dc, C_BG);   // ambient Verlet particle field

    DrawNav(dc);

    if      (g_view == 0) DrawStore(dc);
    else if (g_view == 1) DrawLibrary(dc);
    else                  DrawCommunity(dc);
}

// ============================================================ ACTIONS
static void PrimaryAction(HWND hwnd, int i) {
    Game& g = g_games[i];
    if (g.installed) {
        MessageBoxW(hwnd, (L"Launching " + g.title + L"...").c_str(), L"Steam", MB_OK | MB_ICONINFORMATION);
    } else if (g.installing) {
        g.installing = false;
        g.progress = 0.f;
        g.dlTime = 0.f;
    } else {
        g.installing = true;
        g.progress = 0.f;
        g.dlTime = (float)(i * 7);   // decorrelate per-game noise
    }
}

// ============================================================ WINDOW PROC
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        auto mk = [](int h, int w) {
            return CreateFontW(h, 0, 0, 0, w, 0, 0, 0, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        };
        g_fLogo  = mk(-20, FW_BOLD);
        g_fHuge  = mk(-96, FW_BOLD);
        g_fH1    = mk(-26, FW_BOLD);
        g_fH2    = mk(-17, FW_SEMIBOLD);
        g_fBody  = mk(-15, FW_NORMAL);
        g_fSmall = mk(-13, FW_NORMAL);
        g_fBtn   = mk(-15, FW_SEMIBOLD);

        for (auto& a : g_cardAnim) { a.hover.Snap(0.f); a.press.Snap(0.f); a.lift.Snap(0.f); }
        g_tabIndicator.Snap((float)g_tabStore.left);
        g_viewSlide.Snap(0.f);

        SetTimer(hwnd, 1, 16, nullptr);   // ~60 Hz
        return 0;
    }

    case WM_SIZE: g_cw = LOWORD(lp); g_ch = HIWORD(lp); return 0;

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mm = (MINMAXINFO*)lp;
        mm->ptMinTrackSize.x = 940;
        mm->ptMinTrackSize.y = 780;
        return 0;
    }

    case WM_ERASEBKGND: return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        HDC mem = CreateCompatibleDC(dc);
        HBITMAP bmp = CreateCompatibleBitmap(dc, g_cw, g_ch);
        HGDIOBJ old = SelectObject(mem, bmp);
        Paint(mem);
        BitBlt(dc, 0, 0, g_cw, g_ch, mem, 0, 0, SRCCOPY);
        SelectObject(mem, old);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (!g_tracking) {
            TRACKMOUSEEVENT t = { sizeof(t) }; t.dwFlags = TME_LEAVE; t.hwndTrack = hwnd;
            TrackMouseEvent(&t); g_tracking = true;
        }
        ComputeLayout();
        int id = HitTest((short)LOWORD(lp), (short)HIWORD(lp));
        if (id != g_hover) { g_hover = id; InvalidateRect(hwnd, nullptr, FALSE); }
        return 0;
    }

    case WM_MOUSELEAVE:
        g_tracking = false;
        if (g_hover != H_NONE) { g_hover = H_NONE; InvalidateRect(hwnd, nullptr, FALSE); }
        return 0;

    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) {
            POINT p; GetCursorPos(&p); ScreenToClient(hwnd, &p);
            ComputeLayout();
            SetCursor(LoadCursor(nullptr, HitTest(p.x, p.y) ? IDC_HAND : IDC_ARROW));
            return TRUE;
        }
        break;

    case WM_LBUTTONDOWN: {
        ComputeLayout();
        int id = HitTest((short)LOWORD(lp), (short)HIWORD(lp));
        if (id == H_TAB_STORE && g_view != 0)      { g_prevView = g_view; g_view = 0; g_viewSlide.Snap(24.f); }
        else if (id == H_TAB_LIB && g_view != 1)   { g_prevView = g_view; g_view = 1; g_viewSlide.Snap(24.f); }
        else if (id == H_TAB_COMM && g_view != 2)  { g_prevView = g_view; g_view = 2; g_viewSlide.Snap(24.f); }
        else if (id >= H_CARD && id < H_CARD + 100) {
            g_sel = id - H_CARD;
            g_prevView = g_view; g_view = 1; g_viewSlide.Snap(24.f);
        }
        else if (id >= H_CARD_BTN && id < H_CARD_BTN + 100) {
            int i = id - H_CARD_BTN;
            g_cardAnim[i].press.Snap(1.f);
            g_cardAnim[i].press.Set(0.f);
            PrimaryAction(hwnd, i);
        }
        else if (id >= H_LIB && id < H_LIB + 100)  g_sel = id - H_LIB;
        else if (id == H_DETAIL_BTN)               PrimaryAction(hwnd, g_sel);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_TIMER: {
        // Measure real dt via QPC for frame-rate-independent integration
        static LARGE_INTEGER freq = {}, last = {};
        static bool init = false;
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        if (!init) { QueryPerformanceFrequency(&freq); last = now; init = true; }
        float dt = (float)((double)(now.QuadPart - last.QuadPart) / freq.QuadPart);
        last = now;
        dt = Clamp(dt, 0.001f, 0.05f);
        g_dt = dt;
        g_time += dt;

        // ---- Spring integration (semi-implicit Euler, stable for k*dt^2 < ~2) ----
        for (int i = 0; i < (int)g_games.size(); ++i) {
            bool hot = (g_hover == H_CARD + i) || (g_hover == H_CARD_BTN + i);
            g_cardAnim[i].hover.Set(hot ? 1.f : 0.f);
            g_cardAnim[i].lift.Set(hot ? 6.f : 0.f);
            g_cardAnim[i].hover.Step(dt);
            g_cardAnim[i].press.Step(dt);
            g_cardAnim[i].lift.Step(dt);
        }

        // Tab indicator springs toward active tab's left edge
        RECT activeTab = (g_view == 0) ? g_tabStore : (g_view == 1) ? g_tabLib : g_tabComm;
        g_tabIndicator.Set((float)activeTab.left);
        g_tabIndicator.Step(dt);

        // View slide eases to 0 with cubic Bézier
        g_viewSlide.Step(dt);

        // ---- Realistic download: speed = base * fBm(t) ----
        // Produces plausible bursts and stalls, mirroring real network behavior
        for (auto& g : g_games) {
            if (!g.installing) continue;
            g.dlTime += dt;
            float f = FBM(g.dlTime * 0.8f, g.noiseSeed * 0.37f, g.noiseSeed, 5);
            float speed = 0.05f + 0.35f * f;   // 5% – 40% per second
            g.progress += speed * dt;
            if (g.progress >= 1.f) {
                g.progress = 1.f;
                g.installing = false;
                g.installed = true;
            }
        }

        // Ambient particle field
        StepDust(dt, g_cw, g_ch);

        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ============================================================ ENTRY
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"SteamCloneMathWnd";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, L"SteamCloneMathWnd", L"Steam",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1200, 820,
        nullptr, nullptr, hInst, nullptr);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}