// steam_printer.cpp — a pixel-by-pixel Steam money printer
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

// ============================================================ helpers
inline uint32_t RGB32(int r, int g, int b) {
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

// ============================================================ framebuffer
struct Framebuffer {
    int w = 0, h = 0;
    std::vector<uint32_t> px;

    void Init(int W, int H) { w = W; h = H; px.assign((size_t)w * h, 0); }

    inline void Set(int x, int y, uint32_t c) {
        if ((unsigned)x < (unsigned)w && (unsigned)y < (unsigned)h)
            px[(size_t)y * w + x] = c;
    }
    inline uint32_t Get(int x, int y) const {
        if ((unsigned)x < (unsigned)w && (unsigned)y < (unsigned)h)
            return px[(size_t)y * w + x];
        return 0;
    }
    inline void Blend(int x, int y, uint32_t c, float a) {
        if ((unsigned)x >= (unsigned)w || (unsigned)y >= (unsigned)h) return;
        if (a <= 0.f) return;
        if (a >= 1.f) { px[(size_t)y*w+x] = c; return; }
        uint32_t d = px[(size_t)y*w+x];
        int dr = (d >> 16) & 0xFF, dg = (d >> 8) & 0xFF, db = d & 0xFF;
        int sr = (c >> 16) & 0xFF, sg = (c >> 8) & 0xFF, sb = c & 0xFF;
        int r = (int)(dr + (sr - dr) * a);
        int g = (int)(dg + (sg - dg) * a);
        int b = (int)(db + (sb - db) * a);
        px[(size_t)y*w+x] = RGB32(r, g, b);
    }
    void FillRect(int x, int y, int ww, int hh, uint32_t c) {
        int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
        int x1 = x + ww; if (x1 > w) x1 = w;
        int y1 = y + hh; if (y1 > h) y1 = h;
        for (int yy = y0; yy < y1; ++yy) {
            uint32_t* row = &px[(size_t)yy * w];
            for (int xx = x0; xx < x1; ++xx) row[xx] = c;
        }
    }
    void HLine(int x0, int x1, int y, uint32_t c) {
        if (y < 0 || y >= h) return;
        if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
        if (x0 < 0) x0 = 0;
        if (x1 >= w) x1 = w - 1;
        uint32_t* row = &px[(size_t)y * w];
        for (int x = x0; x <= x1; ++x) row[x] = c;
    }
    void VLine(int x, int y0, int y1, uint32_t c) {
        if (x < 0 || x >= w) return;
        if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
        if (y0 < 0) y0 = 0;
        if (y1 >= h) y1 = h - 1;
        for (int y = y0; y <= y1; ++y) px[(size_t)y*w+x] = c;
    }
    void RectOutline(int x, int y, int ww, int hh, uint32_t c) {
        HLine(x, x + ww - 1, y, c);
        HLine(x, x + ww - 1, y + hh - 1, c);
        VLine(x, y, y + hh - 1, c);
        VLine(x + ww - 1, y, y + hh - 1, c);
    }
};

// ============================================================ 3x5 font
static const uint8_t FONT3x5[][5] = {
    { 0b111,0b101,0b101,0b101,0b111 }, // 0
    { 0b010,0b110,0b010,0b010,0b111 }, // 1
    { 0b111,0b001,0b111,0b100,0b111 }, // 2
    { 0b111,0b001,0b111,0b001,0b111 }, // 3
    { 0b101,0b101,0b111,0b001,0b001 }, // 4
    { 0b111,0b100,0b111,0b001,0b111 }, // 5
    { 0b111,0b100,0b111,0b101,0b111 }, // 6
    { 0b111,0b001,0b010,0b010,0b010 }, // 7
    { 0b111,0b101,0b111,0b101,0b111 }, // 8
    { 0b111,0b101,0b111,0b001,0b111 }, // 9
    { 0b010,0b101,0b111,0b101,0b101 }, // A
    { 0b110,0b101,0b110,0b101,0b110 }, // B
    { 0b011,0b100,0b100,0b100,0b011 }, // C
    { 0b110,0b101,0b101,0b101,0b110 }, // D
    { 0b111,0b100,0b110,0b100,0b111 }, // E
    { 0b111,0b100,0b110,0b100,0b100 }, // F
    { 0b011,0b100,0b101,0b101,0b011 }, // G
    { 0b101,0b101,0b111,0b101,0b101 }, // H
    { 0b111,0b010,0b010,0b010,0b111 }, // I
    { 0b001,0b001,0b001,0b101,0b111 }, // J
    { 0b101,0b101,0b110,0b101,0b101 }, // K
    { 0b100,0b100,0b100,0b100,0b111 }, // L
    { 0b101,0b111,0b111,0b101,0b101 }, // M
    { 0b110,0b101,0b101,0b101,0b101 }, // N
    { 0b111,0b101,0b101,0b101,0b111 }, // O
    { 0b111,0b101,0b111,0b100,0b100 }, // P
    { 0b111,0b101,0b101,0b111,0b001 }, // Q
    { 0b111,0b101,0b110,0b101,0b101 }, // R
    { 0b011,0b100,0b111,0b001,0b110 }, // S
    { 0b111,0b010,0b010,0b010,0b010 }, // T
    { 0b101,0b101,0b101,0b101,0b111 }, // U
    { 0b101,0b101,0b101,0b101,0b010 }, // V
    { 0b101,0b101,0b111,0b111,0b101 }, // W
    { 0b101,0b101,0b010,0b101,0b101 }, // X
    { 0b101,0b101,0b010,0b010,0b010 }, // Y
    { 0b111,0b001,0b010,0b100,0b111 }, // Z
    { 0,0,0,0,0 },      // space
    { 0,0,0,0,0b010 },  // .   (dot on bottom row)
    { 0b011,0b101,0b111,0b101,0b101 }, // $  (roughly)
    { 0b000,0b010,0b000,0b010,0b000 }, // : 
    { 0b000,0b000,0b111,0b000,0b000 }, // -
    { 0b010,0b010,0b010,0b000,0b010 }, // !
    { 0b000,0b001,0b010,0b100,0b000 }, // /
    { 0b010,0b101,0b111,0b101,0b010 }, // (heart-ish, for fun)
};

static int GlyphIndex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return 10 + (c - 'A');
    if (c >= 'a' && c <= 'z') return 10 + (c - 'a');
    switch (c) {
        case ' ': return 36;
        case '.': return 37;
        case '$': return 38;
        case ':': return 39;
        case '-': return 40;
        case '!': return 41;
        case '/': return 42;
        default:  return 36;
    }
}

static int DrawText3x5(Framebuffer& f, const char* s, int x, int y, uint32_t color) {
    int cx = x;
    for (const char* p = s; *p; ++p) {
        int gi = GlyphIndex(*p);
        const uint8_t* g = FONT3x5[gi];
        for (int row = 0; row < 5; ++row) {
            uint8_t bits = g[row];
            for (int col = 0; col < 3; ++col) {
                if (bits & (1 << (2 - col))) f.Set(cx + col, y + row, color);
            }
        }
        cx += 4;
    }
    return cx - x;
}

// ============================================================ palette
static const uint32_t C_BG_TOP    = RGB32( 18,  26,  38);
static const uint32_t C_BG_BOT    = RGB32( 32,  48,  68);
static const uint32_t C_BAR       = RGB32( 12,  18,  28);
static const uint32_t C_LINE      = RGB32( 60,  90, 120);
static const uint32_t C_STEAM_BLU = RGB32(102, 192, 244);
static const uint32_t C_STEAM_GRN = RGB32(117, 176,  34);
static const uint32_t C_STEAM_LGT = RGB32(199, 244, 100);
static const uint32_t C_METAL     = RGB32( 55,  70,  90);
static const uint32_t C_METAL_HI  = RGB32( 90, 110, 140);
static const uint32_t C_METAL_LO  = RGB32( 30,  40,  55);
static const uint32_t C_BILL_G    = RGB32( 70, 130,  40);
static const uint32_t C_BILL_L    = RGB32(160, 220,  70);
static const uint32_t C_BILL_D    = RGB32( 30,  60,  20);

// ============================================================ bill
struct Bill {
    float x, y, vx, vy, angle, spin;
    float life;
    bool  landed = false;
};

// ============================================================ app
struct App {
    Framebuffer fb;
    std::vector<Bill> bills;
    float time = 0.f;
    float spawnTimer = 0.f;
    double money = 0.0;
    bool  turbo = false;

    void Init(int W, int H) { fb.Init(W, H); }

    void SpawnBill() {
        Bill b;
        b.x = 160.f + (float)((rand() % 13) - 6);
        b.y = 146.f;
        b.vx = (float)((rand() % 41) - 20) * 0.6f;
        b.vy = -20.f - (float)(rand() % 25);
        b.angle = 0.f;
        b.spin = (float)((rand() % 101) - 50) * 0.08f;
        b.life = 0.f;
        b.landed = false;
        bills.push_back(b);
    }

    void Update(float dt) {
        time += dt;

        // Money accumulator — rate wobbles like a real clicker
        float rate = 420.f + 260.f * sinf(time * 0.9f) + 180.f * sinf(time * 2.3f + 1.2f);
        if (turbo) rate *= 6.f;
        money += rate * dt;

        // Spawn
        spawnTimer -= dt;
        float interval = turbo ? 0.035f : 0.11f;
        while (spawnTimer <= 0.f) {
            SpawnBill();
            spawnTimer += interval;
        }

        // Physics
        for (auto& b : bills) {
            b.life += dt;
            if (!b.landed) {
                b.vy += 110.f * dt;
                b.x += b.vx * dt;
                b.y += b.vy * dt;
                b.angle += b.spin * dt;
                if (b.y >= 166.f) {
                    b.y = 166.f;
                    b.vy *= -0.32f;
                    b.vx *= 0.55f;
                    b.spin *= 0.55f;
                    if (fabsf(b.vy) < 8.f) {
                        b.landed = true;
                        b.vy = b.vx = b.spin = 0.f;
                        b.angle = 0.f;
                    }
                }
                if (b.x < 8.f)   { b.x = 8.f;   b.vx = -b.vx * 0.6f; }
                if (b.x > 312.f) { b.x = 312.f; b.vx = -b.vx * 0.6f; }
            }
        }

        // Reap old / cap count
        while (!bills.empty() && bills.front().life > 7.f) bills.erase(bills.begin());
        while (bills.size() > 90) bills.erase(bills.begin());
    }

    void DrawBackground() {
        // vertical gradient
        for (int y = 0; y < 180; ++y) {
            float t = y / 179.f;
            int r = (int)(18 + (32 - 18) * t);
            int g = (int)(26 + (48 - 26) * t);
            int b = (int)(38 + (68 - 38) * t);
            uint32_t c = RGB32(r, g, b);
            for (int x = 0; x < 320; ++x) fb.Set(x, y, c);
        }
        // subtle dot grid
        for (int y = 4; y < 180; y += 8) {
            for (int x = 4; x < 320; x += 8) {
                uint32_t c = fb.Get(x, y);
                int r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
                fb.Set(x, y, RGB32(r + 4, g + 5, b + 7));
            }
        }
    }

    void DrawPrinter() {
        const int px = 90, py = 40, pw = 140, ph = 108;

        // Shadow under printer
        fb.FillRect(px + 4, py + ph - 3, pw, 6, RGB32(6, 10, 16));

        // Body — layered for fake bevel
        fb.FillRect(px, py, pw, ph, C_METAL);
        fb.FillRect(px, py, pw, 2, C_METAL_HI);            // top highlight
        fb.FillRect(px, py + ph - 3, pw, 3, C_METAL_LO);   // bottom shadow
        fb.FillRect(px + pw - 3, py, 3, ph, C_METAL_LO);   // right shadow
        fb.RectOutline(px, py, pw, ph, RGB32(15, 20, 30));

        // Screws on the corners
        fb.Set(px + 3, py + 3, C_METAL_LO);
        fb.Set(px + pw - 4, py + 3, C_METAL_LO);
        fb.Set(px + 3, py + ph - 4, C_METAL_LO);
        fb.Set(px + pw - 4, py + ph - 4, C_METAL_LO);

        // LED strip
        static const uint32_t ledCols[4] = {
            RGB32(117, 176, 34),
            RGB32(230, 180, 40),
            RGB32(102, 192, 244),
            RGB32(220, 70, 70),
        };
        int beat = (int)(time * 6.f);
        for (int i = 0; i < 4; ++i) {
            int lx = px + 8 + i * 9;
            int ly = py + 6;
            bool on = ((beat + i) % 4 == 0);
            if (on) {
                fb.FillRect(lx, ly, 3, 3, ledCols[i]);
                fb.Blend(lx - 1, ly, ledCols[i], 0.35f);
                fb.Blend(lx + 3, ly, ledCols[i], 0.35f);
                fb.Blend(lx, ly - 1, ledCols[i], 0.35f);
                fb.Blend(lx, ly + 3, ledCols[i], 0.35f);
            } else {
                fb.FillRect(lx, ly, 3, 3, RGB32(28, 38, 52));
            }
        }

        // Display screen
        int sx = px + 10, sy = py + 14, sw = pw - 20, sh = 26;
        fb.FillRect(sx, sy, sw, sh, RGB32(8, 24, 14));
        fb.RectOutline(sx, sy, sw, sh, RGB32(15, 20, 30));
        fb.RectOutline(sx + 1, sy + 1, sw - 2, sh - 2, RGB32(45, 85, 55));

        // Screen scanlines
        for (int y = sy + 2; y < sy + sh - 2; y += 2) {
            fb.HLine(sx + 2, sx + sw - 3, y, RGB32(10, 30, 18));
        }

        DrawText3x5(fb, "PRINTING", sx + 4, sy + 4, RGB32(120, 220, 100));
        char buf[32];
        snprintf(buf, sizeof(buf), "$%.2f", money);
        DrawText3x5(fb, buf, sx + 4, sy + 13, RGB32(180, 240, 130));
        if (fmodf(time * 3.f, 1.f) < 0.5f) {
            int w = (int)strlen(buf) * 4;
            DrawText3x5(fb, "_", sx + 4 + w, sy + 13, RGB32(180, 240, 130));
        }

        // Centre: animated swirl (Steam-ish)
        int cx = px + pw / 2, cy = py + 76;
        for (int ring = 16; ring >= 1; --ring) {
            float t = ring / 16.f;
            // sweep angle advances over time
            float base = time * 1.8f + t * 4.f;
            int steps = 40 + ring * 8;
            for (int i = 0; i < steps; ++i) {
                float f = (float)i / steps;
                float a = base + f * 3.14159f * 2.f;
                int x = cx + (int)(cosf(a) * ring);
                int y = cy + (int)(sinf(a) * ring * 0.85f);
                uint32_t c = RGB32(
                    40 + (int)(120 * (1.f - t)),
                    90 + (int)(140 * (1.f - t)),
                    130 + (int)(120 * (1.f - t))
                );
                fb.Blend(x, y, c, 0.7f);
            }
        }
        // Bright core
        float pulse = 0.5f + 0.5f * sinf(time * 5.f);
        fb.FillRect(cx - 2, cy - 2, 4, 4, RGB32(
            (int)(180 + 60 * pulse),
            (int)(230 + 25 * pulse),
            255
        ));

        // Output slot
        int slx = px + 12, sly = py + ph - 12, slw = pw - 24, slh = 5;
        fb.FillRect(slx, sly, slw, slh, RGB32(8, 10, 16));
        fb.RectOutline(slx, sly, slw, slh, RGB32(20, 28, 40));
        // Slot glow
        for (int x = slx + 1; x < slx + slw - 1; ++x) {
            float a = 0.25f + 0.45f * pulse;
            fb.Blend(x, sly + 1, C_STEAM_BLU, a);
        }

        // Feet
        fb.FillRect(px + 6, py + ph, 12, 3, RGB32(18, 24, 34));
        fb.FillRect(px + pw - 18, py + ph, 12, 3, RGB32(18, 24, 34));
    }

    void DrawBill(const Bill& b) {
        const float hw = 7.5f, hh = 3.5f;
        float ca = cosf(b.angle), sa = sinf(b.angle);
        float ext = 9.f;
        int x0 = (int)(b.x - ext), x1 = (int)(b.x + ext);
        int y0 = (int)(b.y - ext), y1 = (int)(b.y + ext);
        float fade = 1.f;
        if (b.life > 5.5f) fade = 1.f - (b.life - 5.5f) / 1.5f;
        if (fade <= 0.f) return;

        for (int py = y0; py <= y1; ++py) {
            for (int px = x0; px <= x1; ++px) {
                float dx = px + 0.5f - b.x;
                float dy = py + 0.5f - b.y;
                float lx =  ca * dx + sa * dy;
                float ly = -sa * dx + ca * dy;
                if (lx < -hw || lx > hw || ly < -hh || ly > hh) continue;

                uint32_t col;
                bool border = (ly < -hh + 1.f) || (ly > hh - 1.f) ||
                              (lx < -hw + 1.f) || (lx > hw - 1.f);
                if (border) col = C_BILL_D;
                else if (fabsf(lx) < 4.f && fabsf(ly) < 1.2f) col = C_BILL_L;
                else col = C_BILL_G;

                if (fade < 1.f) {
                    // Blend with background for fade-out
                    uint32_t bg = fb.Get(px, py);
                    int dr = (bg >> 16) & 0xFF, dg = (bg >> 8) & 0xFF, db = bg & 0xFF;
                    int sr = (col >> 16) & 0xFF, sg = (col >> 8) & 0xFF, sb = col & 0xFF;
                    col = RGB32(
                        (int)(dr + (sr - dr) * fade),
                        (int)(dg + (sg - dg) * fade),
                        (int)(db + (sb - db) * fade)
                    );
                }
                fb.Set(px, py, col);
            }
        }
    }

    void DrawHud() {
        // Top bar
        fb.FillRect(0, 0, 320, 14, C_BAR);
        fb.HLine(0, 319, 14, C_LINE);
        DrawText3x5(fb, "STEAM MONEY PRINTER", 6, 4, C_STEAM_BLU);

        // Money counter
        char buf[64];
        snprintf(buf, sizeof(buf), "$%.2f", money);
        int tw = (int)strlen(buf) * 4 - 1;
        // Backing plate
        fb.FillRect(320 - 8 - tw - 3, 2, tw + 6, 10, RGB32(20, 30, 45));
        DrawText3x5(fb, buf, 320 - 6 - tw, 4, C_STEAM_GRN);

        // Bottom bar
        fb.FillRect(0, 172, 320, 8, C_BAR);
        fb.HLine(0, 319, 172, C_LINE);
        const char* msg = "ENTER=TURBO   SPACE=DUMP";
        DrawText3x5(fb, msg, 6, 174, RGB32(143, 160, 175));
        // Blinker
        if (fmodf(time, 1.0f) < 0.6f) {
            fb.FillRect(6 + (int)strlen(msg) * 4 + 2, 174, 2, 5, C_STEAM_BLU);
        }
    }

    void Render() {
        DrawBackground();
        DrawPrinter();

        // Bills drawn on top of the printer so they visually emerge from the slot
        for (const auto& b : bills) DrawBill(b);

        DrawHud();

        // Scanlines over everything
        for (int y = 0; y < 180; y += 2) {
            for (int x = 0; x < 320; ++x) {
                uint32_t c = fb.Get(x, y);
                int r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
                fb.Set(x, y, RGB32(r * 88 / 100, g * 88 / 100, b * 88 / 100));
            }
        }

        // Vignette (simple darkening of 4 corners)
        for (int y = 0; y < 180; ++y) {
            for (int x = 0; x < 320; ++x) {
                float dx = (x - 160.f) / 160.f;
                float dy = (y - 90.f) / 90.f;
                float d = dx * dx + dy * dy;
                if (d > 0.8f) {
                    float a = (d - 0.8f) * 0.6f;
                    if (a > 0.5f) a = 0.5f;
                    fb.Blend(x, y, 0x000000, a);
                }
            }
        }
    }
};

static App g_app;
static const int CANVAS_W = 320;
static const int CANVAS_H = 180;

// ============================================================ present
static void Present(HWND hwnd) {
    HDC dc = GetDC(hwnd);
    RECT cr;
    GetClientRect(hwnd, &cr);
    int cw = cr.right, ch = cr.bottom;

    int sx = cw / CANVAS_W;
    int sy = ch / CANVAS_H;
    int scale = (sx < sy) ? sx : sy;
    if (scale < 1) scale = 1;
    int dw = CANVAS_W * scale, dh = CANVAS_H * scale;
    int ox = (cw - dw) / 2, oy = (ch - dh) / 2;

    // letterbox
    HBRUSH bg = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(dc, &cr, bg);
    DeleteObject(bg);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = CANVAS_W;
    bmi.bmiHeader.biHeight = -CANVAS_H;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetStretchBltMode(dc, COLORONCOLOR);
    StretchDIBits(dc, ox, oy, dw, dh, 0, 0, CANVAS_W, CANVAS_H,
                  g_app.fb.px.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(hwnd, dc);
}

// ============================================================ window
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        g_app.Init(CANVAS_W, CANVAS_H);
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;

    case WM_SIZE:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mm = (MINMAXINFO*)lp;
        mm->ptMinTrackSize.x = 640;
        mm->ptMinTrackSize.y = 360;
        return 0;
    }

    case WM_ERASEBKGND: return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        Present(hwnd);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_KEYDOWN:
        if (wp == VK_RETURN)  { g_app.turbo = !g_app.turbo; InvalidateRect(hwnd, nullptr, FALSE); return 0; }
        if (wp == VK_SPACE)   { for (int i = 0; i < 40; ++i) g_app.SpawnBill(); InvalidateRect(hwnd, nullptr, FALSE); return 0; }
        if (wp == VK_ESCAPE)  { PostQuitMessage(0); return 0; }
        return 0;

    case WM_TIMER: {
        static LARGE_INTEGER freq = {}, last = {};
        static bool init = false;
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        if (!init) { QueryPerformanceFrequency(&freq); last = now; init = true; }
        float dt = (float)((double)(now.QuadPart - last.QuadPart) / freq.QuadPart);
        last = now;
        if (dt < 0.001f) dt = 0.001f;
        if (dt > 0.05f) dt = 0.05f;

        g_app.Update(dt);
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

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"SteamMoneyPrinterWnd";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, L"SteamMoneyPrinterWnd", L"Steam Money Printer",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 800,
        nullptr, nullptr, hInst, nullptr);
    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}