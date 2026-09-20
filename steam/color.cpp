// steam_color.cpp
// -----------------------------------------------------------------------------
// Pure Win32 C++ — color every part of every object in the scene, per pixel.
//
//  · Every drawable thing is a Part
//  · Every Part points at a Material (index into g_mats[])
//  · Every Material holds base / emissive colors, roughness, metallic
//  · Shapes are rasterized pixel by pixel with an analytic normal per pixel
//    (circle, capsule, rounded box, triangle, gradient)
//  · Shading = Lambert diffuse + Blinn-Phong specular + emissive
//  · Click any part to select it. Click a swatch to recolor it.
//
// Build (MSVC):  cl /EHsc /std:c++17 /DUNICODE /D_UNICODE steam_color.cpp user32.lib gdi32.lib
// Build (MinGW): g++ steam_color.cpp -o steam_color.exe -mwindows -O2 -std=c++17 -lgdi32 -luser32
// -----------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#define PS_CIRCLE 0x00000002
#define PS_TRIANGLE 0x00000004
#define PS_BOX 0x00000008
#define PS_CAPSULE 0x00000010
#define PS_GRADIENT 0x00000020
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#include <windows.h>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <algorithm>

// ============================================================ MATH
static const float PI = 3.14159265358979f;

struct Vec2 {
    float x = 0, y = 0;
    Vec2() = default;
    Vec2(float X, float Y) : x(X), y(Y) {}
    Vec2 operator+(Vec2 o) const { return { x + o.x, y + o.y }; }
    Vec2 operator-(Vec2 o) const { return { x - o.x, y - o.y }; }
    Vec2 operator*(float s) const { return { x * s, y * s }; }
};
static float Dot(Vec2 a, Vec2 b)  { return a.x * b.x + a.y * b.y; }
static float Len(Vec2 a)          { return sqrtf(a.x * a.x + a.y * a.y); }
static Vec2  Norm(Vec2 a)         { float l = Len(a); return l > 1e-6f ? Vec2{ a.x / l, a.y / l } : Vec2{ 0, 0 }; }
static float Clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static float Lerpf (float a, float b, float t) { return a + (b - a) * t; }

// ============================================================ COLOR
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;
    Color() = default;
    Color(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 255) : r(R), g(G), b(B), a(A) {}
    static Color RGBf(float R, float G, float B, float A = 1.f) {
        return Color(
            (uint8_t)Clampf(R * 255.f, 0, 255),
            (uint8_t)Clampf(G * 255.f, 0, 255),
            (uint8_t)Clampf(B * 255.f, 0, 255),
            (uint8_t)Clampf(A * 255.f, 0, 255));
    }
    uint32_t Pack() const { return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b; }
};

static Color LerpC(Color A, Color B, float t) {
    t = Clampf(t, 0, 1);
    return Color(
        (uint8_t)Lerpf(A.r, B.r, t),
        (uint8_t)Lerpf(A.g, B.g, t),
        (uint8_t)Lerpf(A.b, B.b, t),
        (uint8_t)Lerpf(A.a, B.a, t));
}

// ============================================================ FRAMEBUFFER
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
    inline void Blend(int x, int y, Color c) {
        if (c.a == 0) return;
        if ((unsigned)x >= (unsigned)w || (unsigned)y >= (unsigned)h) return;
        if (c.a == 255) { Set(x, y, c.Pack()); return; }
        uint32_t d = px[(size_t)y * w + x];
        float t = c.a / 255.f;
        int dr = (d >> 16) & 0xFF, dg = (d >> 8) & 0xFF, db = d & 0xFF;
        int r = (int)(dr + (c.r - dr) * t);
        int g = (int)(dg + (c.g - dg) * t);
        int bl = (int)(db + (c.b - db) * t);
        Set(x, y, ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)bl);
    }
};

// ============================================================ MATERIAL + LIGHT
struct Material {
    Color  base{ 150, 150, 150 };
    Color  emissive{ 0, 0, 0 };
    float  emissiveStrength = 0.f;
    float  roughness        = 0.6f;   // 0 = mirror, 1 = matte
    float  metallic         = 0.f;
    float  alpha            = 1.f;
};

struct Light {
    Vec2   dir{-0.55f, -0.75f};        // points TO the light, screen space
    float  dirZ = 0.55f;
    Color  color{ 255, 245, 225 };
    Color  ambient{ 46, 56, 78 };
    float  intensity = 1.0f;
};
static Light g_light;

// Per-pixel shading. n2 = (nx, ny) tangent part of the surface normal,
// nz = z component toward viewer. Together they form a unit vector.
static Color ShadePixel(const Material& m, Vec2 n2, float nz) {
    float lx = g_light.dir.x, ly = g_light.dir.y, lz = g_light.dirZ;
    float ll = sqrtf(lx * lx + ly * ly + lz * lz);
    lx /= ll; ly /= ll; lz /= ll;

    float ndl = Clampf(n2.x * lx + n2.y * ly + nz * lz, 0.f, 1.f);

    // Blinn-Phong
    float hx = lx, hy = ly, hz = lz + 1.f;
    float hl = sqrtf(hx * hx + hy * hy + hz * hz);
    hx /= hl; hy /= hl; hz /= hl;
    float ndh  = Clampf(n2.x * hx + n2.y * hy + nz * hz, 0.f, 1.f);
    float shin = Lerpf(128.f, 6.f, m.roughness);
    float spec = powf(ndh, shin) * (1.f - m.roughness) * (1.f - m.roughness) * 1.25f;

    float ar = g_light.ambient.r / 255.f, ag = g_light.ambient.g / 255.f, ab = g_light.ambient.b / 255.f;
    float lr = g_light.color.r  / 255.f, lg = g_light.color.g  / 255.f, lb = g_light.color.b  / 255.f;
    float br = m.base.r / 255.f, bg = m.base.g / 255.f, bb = m.base.b / 255.f;
    float er = m.emissive.r / 255.f, eg = m.emissive.g / 255.f, eb = m.emissive.b / 255.f;

    float diff = ndl * g_light.intensity;
    float R = br * (ar + lr * diff) + spec * lr + er * m.emissiveStrength;
    float G = bg * (ag + lg * diff) + spec * lg + eg * m.emissiveStrength;
    float B = bb * (ab + lb * diff) + spec * lb + eb * m.emissiveStrength;

    return Color::RGBf(R, G, B, m.alpha);
}

// ============================================================ SHAPE RASTERISERS
// Each returns a per-pixel surface normal so ShadePixel can color every pixel.

static void DrawCircle(Framebuffer& fb, Vec2 c, float r, const Material& m, float softness = 0.5f) {
    int x0 = (int)floorf(c.x - r - softness - 1), x1 = (int)ceilf(c.x + r + softness + 1);
    int y0 = (int)floorf(c.y - r - softness - 1), y1 = (int)ceilf(c.y + r + softness + 1);
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            Vec2 p{ x + 0.5f, y + 0.5f };
            float dist = Len(p - c);
            if (dist > r + softness) continue;
            float alpha = Clampf((r + softness - dist) / std::max(softness, 0.01f), 0.f, 1.f);
            Vec2 n2 = (p - c) * (1.f / r);
            float r2 = n2.x * n2.x + n2.y * n2.y;
            float nz = 1.f;
            if (r2 > 1e-6f) {
                if (r2 > 1.f) { n2 = Norm(n2); nz = 0.f; }
                else          { nz = sqrtf(1.f - r2); }
            }
            Color cc = ShadePixel(m, n2, nz);
            cc.a = (uint8_t)Clampf(alpha * m.alpha * 255.f, 0, 255);
            fb.Blend(x, y, cc);
        }
    }
}

static void DrawCapsule(Framebuffer& fb, Vec2 a, Vec2 b, float r, const Material& m) {
    Vec2 ab = b - a;
    float abLen2 = Dot(ab, ab);
    int x0 = (int)floorf(std::min(a.x, b.x) - r - 2), x1 = (int)ceilf(std::max(a.x, b.x) + r + 2);
    int y0 = (int)floorf(std::min(a.y, b.y) - r - 2), y1 = (int)ceilf(std::max(a.y, b.y) + r + 2);
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            Vec2 p{ x + 0.5f, y + 0.5f };
            float t = abLen2 > 1e-6f ? Clampf(Dot(p - a, ab) / abLen2, 0.f, 1.f) : 0.f;
            Vec2 q = a + ab * t;
            float dist = Len(p - q);
            if (dist > r + 1.f) continue;
            float alpha = Clampf(r - dist + 0.5f, 0.f, 1.f);
            Vec2 n2 = (p - q) * (1.f / r);
            float r2 = n2.x * n2.x + n2.y * n2.y;
            float nz = 1.f;
            if (r2 > 1e-6f) {
                if (r2 > 1.f) { n2 = Norm(n2); nz = 0.f; }
                else          { nz = sqrtf(1.f - r2); }
            }
            Color cc = ShadePixel(m, n2, nz);
            cc.a = (uint8_t)Clampf(alpha * m.alpha * 255.f, 0, 255);
            fb.Blend(x, y, cc);
        }
    }
}

// Rounded box — SDF-based inside test, analytic normals on face / edge / corner.
static void DrawBox(Framebuffer& fb, Vec2 c, Vec2 he, float cr, const Material& m) {
    if (cr < 0.5f) cr = 0.5f;
    int x0 = (int)floorf(c.x - he.x - 2), x1 = (int)ceilf(c.x + he.x + 2);
    int y0 = (int)floorf(c.y - he.y - 2), y1 = (int)ceilf(c.y + he.y + 2);
    Vec2 inner{ he.x - cr, he.y - cr };
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            Vec2 p{ x + 0.5f, y + 0.5f };
            Vec2 d = p - c;
            float qx = fabsf(d.x) - inner.x;
            float qy = fabsf(d.y) - inner.y;
            float sdf = Len(Vec2{ std::max(qx, 0.f), std::max(qy, 0.f) })
                      + std::min(std::max(qx, qy), 0.f) - cr;
            if (sdf > 1.f) continue;
            float alpha = Clampf(-sdf + 0.5f, 0.f, 1.f);

            Vec2 n2{ 0, 0 };
            float nz = 1.f;
            if (qx > 0 && qy > 0) {
                // Corner quarter-circle
                Vec2 cc{ copysignf(inner.x, d.x), copysignf(inner.y, d.y) };
                Vec2 rel{ d.x - cc.x, d.y - cc.y };
                float dl = Len(rel);
                if (dl > 1e-4f) {
                    float k = Clampf(dl / cr, 0.f, 1.f);
                    n2 = rel * (k / dl);
                    nz = sqrtf(Clampf(1.f - k * k, 0.f, 1.f));
                }
            } else if (qx > 0) {
                // Vertical side band — curves only in x
                float k = Clampf(copysignf(qx, d.x) / cr, -1.f, 1.f);
                n2 = { k, 0 };
                nz = sqrtf(Clampf(1.f - k * k, 0.f, 1.f));
            } else if (qy > 0) {
                // Horizontal side band — curves only in y
                float k = Clampf(copysignf(qy, d.y) / cr, -1.f, 1.f);
                n2 = { 0, k };
                nz = sqrtf(Clampf(1.f - k * k, 0.f, 1.f));
            }
            // else: front face — normal is straight at the viewer

            Color cc = ShadePixel(m, n2, nz);
            cc.a = (uint8_t)Clampf(alpha * m.alpha * 255.f, 0, 255);
            fb.Blend(x, y, cc);
        }
    }
}

static void DrawTriangle(Framebuffer& fb, Vec2 A, Vec2 B, Vec2 C, const Material& m) {
    int minx = (int)floorf(std::min({ A.x, B.x, C.x }));
    int maxx = (int)ceilf (std::max({ A.x, B.x, C.x }));
    int miny = (int)floorf(std::min({ A.y, B.y, C.y }));
    int maxy = (int)ceilf (std::max({ A.y, B.y, C.y }));
    float den = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y);
    if (fabsf(den) < 1e-6f) return;

    // Fixed "top-lit" surface normal, gently tilted by the centroid's x-slope
    float lean = ((A.x + B.x + C.x) / 3.f - 160.f) * 0.004f;
    Vec2 n2{ Clampf(lean, -0.45f, 0.45f), -0.30f };
    float n2l = Len(n2);
    if (n2l > 0.65f) n2 = n2 * (0.65f / n2l);
    float nz = sqrtf(Clampf(1.f - Dot(n2, n2), 0.f, 1.f));
    uint32_t packed = ShadePixel(m, n2, nz).Pack();

    for (int y = miny; y <= maxy; ++y) {
        if ((unsigned)y >= (unsigned)fb.h) continue;
        for (int x = minx; x <= maxx; ++x) {
            if ((unsigned)x >= (unsigned)fb.w) continue;
            Vec2 p{ x + 0.5f, y + 0.5f };
            float w0 = ((B.y - C.y) * (p.x - C.x) + (C.x - B.x) * (p.y - C.y)) / den;
            float w1 = ((C.y - A.y) * (p.x - C.x) + (A.x - C.x) * (p.y - C.y)) / den;
            float w2 = 1.f - w0 - w1;
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) fb.Set(x, y, packed);
        }
    }
}

static void DrawGradient(Framebuffer& fb, Vec2 tl, Vec2 br,
                         const Material& mt, const Material& mb) {
    int x0 = (int)floorf(tl.x), x1 = (int)ceilf(br.x);
    int y0 = (int)floorf(tl.y), y1 = (int)ceilf(br.y);
    for (int y = y0; y < y1; ++y) {
        float t = (y - tl.y) / std::max(br.y - tl.y, 1.f);
        Color c = LerpC(mt.base, mb.base, t * t);
        for (int x = x0; x < x1; ++x) fb.Set(x, y, c.Pack());
    }
}

// ============================================================ PART + SCENE
enum PartShape { PS_CIRCLE, PS_CAPSULE, PS_BOX, PS_TRIANGLE, PS_GRADIENT };

struct Part {
    const char* name;
    PartShape   shape;
    Vec2 a;             // circle: center | capsule: end A | box: center | tri: v0 | grad: TL
    Vec2 b;             // capsule: end B | box: half-extents | tri: v1 | grad: BR
    Vec2 c;             // tri: v2
    float radius;       // circle / capsule radius, or box corner radius
    int   matId;        // index into g_mats
    int   matId2;       // gradient bottom material (-1 if unused)
    bool  interactive;  // clickable?
};

// --- material slots -----------------------------------------------------------
enum {
    M_SKY_TOP, M_SKY_BOT, M_SUN, M_CLOUD,
    M_MTN_FAR, M_MTN_NEAR, M_GROUND, M_GRASS,
    M_HEAD, M_EYE, M_ANTENNA, M_BEACON,
    M_TORSO, M_CORE, M_ARM, M_HAND,
    M_LEG, M_FOOT, M_PARTICLE,
    M_COUNT
};
static Material g_mats[M_COUNT];

static std::vector<Part> g_parts;

// --- scene construction -------------------------------------------------------
static void BuildScene() {
    g_parts.clear();

    // ---- Background layers (bottom of list = drawn first) ----
    g_parts.push_back({ "Sky",        PS_GRADIENT, { 0, 0 },   { 320, 128 }, { 0, 0 }, 0, M_SKY_TOP, M_SKY_BOT, true });
    g_parts.push_back({ "Sun",        PS_CIRCLE,   { 62, 42 }, { 0, 0 },     { 0, 0 }, 15.f, M_SUN, -1, true });
    g_parts.push_back({ "Cloud A",    PS_CIRCLE,   { 232, 38 },{ 0, 0 },     { 0, 0 }, 11.f, M_CLOUD, -1, true });
    g_parts.push_back({ "Cloud B",    PS_CIRCLE,   { 246, 34 },{ 0, 0 },     { 0, 0 },  8.f, M_CLOUD, -1, true });
    g_parts.push_back({ "Cloud C",    PS_CIRCLE,   { 218, 33 },{ 0, 0 },     { 0, 0 },  7.f, M_CLOUD, -1, true });
    g_parts.push_back({ "Cloud D",    PS_CIRCLE,   { 118, 58 },{ 0, 0 },     { 0, 0 },  9.f, M_CLOUD, -1, true });
    g_parts.push_back({ "Cloud E",    PS_CIRCLE,   { 130, 56 },{ 0, 0 },     { 0, 0 },  6.f, M_CLOUD, -1, true });

    // ---- Mountains ----
    g_parts.push_back({ "Peak Left",  PS_TRIANGLE, { -20, 128 }, {  55,  48 }, { 120, 128 }, 0, M_MTN_FAR,  -1, true });
    g_parts.push_back({ "Peak Mid",   PS_TRIANGLE, {  70, 128 }, { 150,  34 }, { 230, 128 }, 0, M_MTN_FAR,  -1, true });
    g_parts.push_back({ "Peak Right", PS_TRIANGLE, { 190, 128 }, { 260,  58 }, { 340, 128 }, 0, M_MTN_FAR,  -1, true });
    g_parts.push_back({ "Ridge L",    PS_TRIANGLE, { -20, 128 }, {  40,  84 }, { 110, 128 }, 0, M_MTN_NEAR, -1, true });
    g_parts.push_back({ "Ridge R",    PS_TRIANGLE, { 240, 128 }, { 290,  92 }, { 340, 128 }, 0, M_MTN_NEAR, -1, true });

    // ---- Ground + grass strip ----
    g_parts.push_back({ "Ground",     PS_BOX, { 160, 154 }, { 160, 30 }, { 0, 0 }, 0.f, M_GROUND, -1, true });
    g_parts.push_back({ "Grass",      PS_BOX, { 160, 126 }, { 160,  3 }, { 0, 0 }, 0.f, M_GRASS,  -1, true });

    // ---- Robot (the star of the show) ----
    g_parts.push_back({ "Antenna",    PS_CAPSULE, { 160, 59 }, { 160, 67 }, { 0, 0 }, 0.9f, M_ANTENNA, -1, true });
    g_parts.push_back({ "Beacon",     PS_CIRCLE,  { 160, 57 }, { 0, 0 },    { 0, 0 }, 2.4f, M_BEACON,  -1, true });
    g_parts.push_back({ "Head",       PS_BOX,     { 160, 75 }, { 10, 7.5f },{ 0, 0 }, 3.5f, M_HEAD,    -1, true });
    g_parts.push_back({ "Eye",        PS_CIRCLE,  { 160, 75 }, { 0, 0 },    { 0, 0 }, 3.0f, M_EYE,     -1, true });
    g_parts.push_back({ "Torso",      PS_BOX,     { 160, 95 }, { 11, 13 },  { 0, 0 }, 4.0f, M_TORSO,   -1, true });
    g_parts.push_back({ "Core",       PS_CIRCLE,  { 160, 94 }, { 0, 0 },    { 0, 0 }, 3.2f, M_CORE,    -1, true });
    g_parts.push_back({ "Arm L",      PS_CAPSULE, { 150, 87 }, { 143, 100 },{ 0, 0 }, 2.8f, M_ARM,     -1, true });
    g_parts.push_back({ "Hand L",     PS_CIRCLE,  { 143, 101 },{ 0, 0 },    { 0, 0 }, 3.0f, M_HAND,    -1, true });
    g_parts.push_back({ "Arm R",      PS_CAPSULE, { 170, 87 }, { 177, 100 },{ 0, 0 }, 2.8f, M_ARM,     -1, true });
    g_parts.push_back({ "Hand R",     PS_CIRCLE,  { 177, 101 },{ 0, 0 },    { 0, 0 }, 3.0f, M_HAND,    -1, true });
    g_parts.push_back({ "Leg L",      PS_CAPSULE, { 154, 109 },{ 152, 121 },{ 0, 0 }, 3.0f, M_LEG,     -1, true });
    g_parts.push_back({ "Foot L",     PS_BOX,     { 151, 123 },{ 5, 2.5f }, { 0, 0 }, 2.0f, M_FOOT,    -1, true });
    g_parts.push_back({ "Leg R",      PS_CAPSULE, { 166, 109 },{ 168, 121 },{ 0, 0 }, 3.0f, M_LEG,     -1, true });
    g_parts.push_back({ "Foot R",     PS_BOX,     { 169, 123 },{ 5, 2.5f }, { 0, 0 }, 2.0f, M_FOOT,    -1, true });
}

// --- default colors -----------------------------------------------------------
static void InitMaterials() {
    auto mk = [](Color base, float rough = 0.6f, float metal = 0.f) {
        Material m; m.base = base; m.roughness = rough; m.metallic = metal; return m;
    };
    g_mats[M_SKY_TOP]  = mk({  52,  80, 138 });
    g_mats[M_SKY_BOT]  = mk({ 235, 150, 100 });
    g_mats[M_SUN]      = mk({ 255, 228, 150 });
    g_mats[M_SUN].emissive = { 255, 228, 150 };
    g_mats[M_SUN].emissiveStrength = 1.6f;
    g_mats[M_CLOUD]    = mk({ 232, 216, 240 }, 0.95f);
    g_mats[M_MTN_FAR]  = mk({  84,  98, 138 }, 0.9f);
    g_mats[M_MTN_NEAR] = mk({  52,  66, 100 }, 0.9f);
    g_mats[M_GROUND]   = mk({  46,  62,  50 }, 0.95f);
    g_mats[M_GRASS]    = mk({  76, 128,  66 }, 0.9f);

    g_mats[M_HEAD]     = mk({ 192, 204, 220 }, 0.28f, 0.7f);
    g_mats[M_EYE]      = mk({  90, 224, 255 }, 0.08f);
    g_mats[M_EYE].emissive = { 90, 224, 255 };
    g_mats[M_EYE].emissiveStrength = 1.5f;
    g_mats[M_ANTENNA]  = mk({ 148, 158, 174 }, 0.35f, 0.6f);
    g_mats[M_BEACON]   = mk({ 255,  92,  92 }, 0.15f);
    g_mats[M_BEACON].emissive = { 255, 92, 92 };
    g_mats[M_BEACON].emissiveStrength = 1.4f;
    g_mats[M_TORSO]    = mk({ 152, 168, 188 }, 0.32f, 0.6f);
    g_mats[M_CORE]     = mk({  90, 200, 220 }, 0.18f);
    g_mats[M_CORE].emissive = { 90, 200, 220 };
    g_mats[M_CORE].emissiveStrength = 0.9f;
    g_mats[M_ARM]      = mk({ 132, 146, 164 }, 0.38f, 0.5f);
    g_mats[M_HAND]     = mk({ 200, 210, 226 }, 0.28f, 0.7f);
    g_mats[M_LEG]      = mk({  92, 106, 128 }, 0.45f, 0.4f);
    g_mats[M_FOOT]     = mk({  58,  70,  88 }, 0.5f, 0.5f);
    g_mats[M_PARTICLE] = mk({ 255, 240, 150 }, 0.2f);
    g_mats[M_PARTICLE].emissive = { 255, 240, 150 };
    g_mats[M_PARTICLE].emissiveStrength = 1.8f;
}

// ============================================================ PARTICLES
struct Particle { Vec2 pos, vel; float life = 0, maxLife = 1, size = 2; };
static std::vector<Particle> g_particles;

static void UpdateParticles(float dt) {
    static float spawn = 0.f;
    spawn -= dt;
    while (spawn <= 0.f) {
        spawn += 0.14f;
        Particle p;
        p.pos     = { (float)(rand() % 320), 130.f + (float)(rand() % 50) };
        p.vel     = { (float)(rand() % 21 - 10) * 0.35f, -10.f - (float)(rand() % 20) };
        p.maxLife = 2.5f + (rand() % 40) * 0.1f;
        p.size    = 1.0f + (rand() % 12) * 0.1f;
        g_particles.push_back(p);
    }
    for (auto& p : g_particles) {
        p.life += dt;
        p.pos.x += p.vel.x * dt + sinf((p.life + p.pos.y) * 3.5f) * 10.f * dt;
        p.pos.y += p.vel.y * dt;
        p.vel.y += 4.f * dt;
    }
    for (int i = (int)g_particles.size() - 1; i >= 0; --i)
        if (g_particles[i].life > g_particles[i].maxLife || g_particles[i].pos.y < -5)
            g_particles.erase(g_particles.begin() + i);
}

static void DrawParticles(Framebuffer& fb) {
    for (auto& p : g_particles) {
        float t = p.life / p.maxLife;
        float a = t < 0.5f ? t * 2.f : (1.f - t) * 2.f;
        Material m = g_mats[M_PARTICLE];
        m.alpha = a * 0.9f;
        DrawCircle(fb, p.pos, p.size, m, p.size * 1.6f);
    }
}

// ============================================================ SCENE RENDER
static void DrawPart(Framebuffer& fb, const Part& p) {
    const Material& m = g_mats[p.matId];
    switch (p.shape) {
    case PS_CIRCLE:
        // Sun gets a wide soft halo
        if (p.matId == M_SUN) DrawCircle(fb, p.a, p.radius, m, 14.f);
        else                  DrawCircle(fb, p.a, p.radius, m, 0.7f);
        break;
    case PS_CAPSULE:  DrawCapsule(fb, p.a, p.b, p.radius, m); break;
    case PS_BOX:      DrawBox(fb, p.a, p.b, p.radius, m); break;
    case PS_TRIANGLE: DrawTriangle(fb, p.a, p.b, p.c, m); break;
    case PS_GRADIENT:
        DrawGradient(fb, p.a, p.b, g_mats[p.matId], g_mats[p.matId2]);
        break;
    }
}

// ============================================================ 3x5 PIXEL FONT
static const uint8_t FONT3x5[][5] = {
    {0b111,0b101,0b101,0b101,0b111}, // 0
    {0b010,0b110,0b010,0b010,0b111}, // 1
    {0b111,0b001,0b111,0b100,0b111}, // 2
    {0b111,0b001,0b111,0b001,0b111}, // 3
    {0b101,0b101,0b111,0b001,0b001}, // 4
    {0b111,0b100,0b111,0b001,0b111}, // 5
    {0b111,0b100,0b111,0b101,0b111}, // 6
    {0b111,0b001,0b010,0b010,0b010}, // 7
    {0b111,0b101,0b111,0b101,0b111}, // 8
    {0b111,0b101,0b111,0b001,0b111}, // 9
    {0b010,0b101,0b111,0b101,0b101}, // A
    {0b110,0b101,0b110,0b101,0b110}, // B
    {0b011,0b100,0b100,0b100,0b011}, // C
    {0b110,0b101,0b101,0b101,0b110}, // D
    {0b111,0b100,0b110,0b100,0b111}, // E
    {0b111,0b100,0b110,0b100,0b100}, // F
    {0b011,0b100,0b101,0b101,0b011}, // G
    {0b101,0b101,0b111,0b101,0b101}, // H
    {0b111,0b010,0b010,0b010,0b111}, // I
    {0b001,0b001,0b001,0b101,0b111}, // J
    {0b101,0b101,0b110,0b101,0b101}, // K
    {0b100,0b100,0b100,0b100,0b111}, // L
    {0b101,0b111,0b111,0b101,0b101}, // M
    {0b110,0b101,0b101,0b101,0b101}, // N
    {0b111,0b101,0b101,0b101,0b111}, // O
    {0b111,0b101,0b111,0b100,0b100}, // P
    {0b111,0b101,0b101,0b111,0b001}, // Q
    {0b111,0b101,0b110,0b101,0b101}, // R
    {0b011,0b100,0b111,0b001,0b110}, // S
    {0b111,0b010,0b010,0b010,0b010}, // T
    {0b101,0b101,0b101,0b101,0b111}, // U
    {0b101,0b101,0b101,0b101,0b010}, // V
    {0b101,0b101,0b111,0b111,0b101}, // W
    {0b101,0b101,0b010,0b101,0b101}, // X
    {0b101,0b101,0b010,0b010,0b010}, // Y
    {0b111,0b001,0b010,0b100,0b111}, // Z
    {0,0,0,0,0},                     // space
    {0,0,0,0,0b010},                 // .
    {0,0,0,0,0b111},                 // _
};

static int GlyphIndex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return 10 + (c - 'A');
    if (c >= 'a' && c <= 'z') return 10 + (c - 'a');
    switch (c) {
        case ' ': return 36;
        case '.': return 37;
        case '_': return 38;
        default:  return 36;
    }
}
static void DrawString(Framebuffer& fb, const char* s, int x, int y, uint32_t col) {
    int cx = x;
    for (const char* p = s; *p; ++p) {
        const uint8_t* g = FONT3x5[GlyphIndex(*p)];
        for (int row = 0; row < 5; ++row) {
            uint8_t bits = g[row];
            if (bits & 0b100) fb.Set(cx,     y + row, col);
            if (bits & 0b010) fb.Set(cx + 1, y + row, col);
            if (bits & 0b001) fb.Set(cx + 2, y + row, col);
        }
        cx += 4;
    }
}

// ============================================================ INTERACTION
static const Color PALETTE[12] = {
    { 240, 246, 255 }, { 222,  72,  84 }, { 245, 162,  52 }, { 250, 226,  92 },
    { 112, 212, 102 }, {  72, 202, 212 }, {  82, 142, 246 }, { 172, 102, 236 },
    { 246, 122, 192 }, { 152, 162, 178 }, {  72,  82,  98 }, {  30,  36,  50 },
};

static int  g_selectedPart = -1;
static int  g_hoverPart    = -1;
static int  g_currentSwatch = 0;
static RECT g_swatchRect[12];

// Bounds helper (for drawing the selection outline)
struct Bounds { Vec2 mn, mx; };
static Bounds GetBounds(const Part& p) {
    Bounds b{ { 1e9f, 1e9f }, { -1e9f, -1e9f } };
    auto ex = [&](Vec2 q, float r) {
        b.mn.x = std::min(b.mn.x, q.x - r); b.mn.y = std::min(b.mn.y, q.y - r);
        b.mx.x = std::max(b.mx.x, q.x + r); b.mx.y = std::max(b.mx.y, q.y + r);
    };
    switch (p.shape) {
    case PS_CIRCLE:   ex(p.a, p.radius); break;
    case PS_CAPSULE:  ex(p.a, p.radius); ex(p.b, p.radius); break;
    case PS_BOX:      ex(p.a, 0); b.mn.x -= p.b.x; b.mn.y -= p.b.y;
                      b.mx.x += p.b.x; b.mx.y += p.b.y; break;
    case PS_TRIANGLE: ex(p.a, 0); ex(p.b, 0); ex(p.c, 0); break;
    case PS_GRADIENT: ex(p.a, 0); ex(p.b, 0); break;
    }
    return b;
}

// Reverse-order pick so topmost drawn element wins.
static int PickPart(Vec2 p) {
    for (int i = (int)g_parts.size() - 1; i >= 0; --i) {
        const Part& part = g_parts[i];
        if (!part.interactive) continue;
        switch (part.shape) {
        case PS_CIRCLE:
            if (Len(p - part.a) <= part.radius + 1.f) return i;
            break;
        case PS_CAPSULE: {
            Vec2 ab = part.b - part.a;
            float L2 = Dot(ab, ab);
            float t = L2 > 1e-6f ? Clampf(Dot(p - part.a, ab) / L2, 0.f, 1.f) : 0.f;
            if (Len(p - (part.a + ab * t)) <= part.radius + 1.f) return i;
            break;
        }
        case PS_BOX: {
            Vec2 d = p - part.a;
            if (fabsf(d.x) <= part.b.x + 1.f && fabsf(d.y) <= part.b.y + 1.f) return i;
            break;
        }
        case PS_TRIANGLE: {
            Vec2 A = part.a, B = part.b, C = part.c;
            float den = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y);
            if (fabsf(den) < 1e-6f) break;
            float w0 = ((B.y - C.y) * (p.x - C.x) + (C.x - B.x) * (p.y - C.y)) / den;
            float w1 = ((C.y - A.y) * (p.x - C.x) + (A.x - C.x) * (p.y - C.y)) / den;
            float w2 = 1.f - w0 - w1;
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) return i;
            break;
        }
        case PS_GRADIENT:
            if (p.x >= part.a.x && p.x <= part.b.x && p.y >= part.a.y && p.y <= part.b.y)
                return i;
            break;
        }
    }
    return -1;
}

// Apply a palette color to the selected part.
static void ApplyColor(int partIdx, Color c) {
    if (partIdx < 0) return;
    const Part& p = g_parts[partIdx];
    g_mats[p.matId].base = c;
    // Emissive parts keep their glow in sync with the base color.
    if (g_mats[p.matId].emissiveStrength > 0.5f)
        g_mats[p.matId].emissive = c;
    // Gradient parts derive a darker bottom from the same hue.
    if (p.matId2 >= 0) {
        g_mats[p.matId2].base = Color(
            (uint8_t)(c.r * 0.45f),
            (uint8_t)(c.g * 0.45f),
            (uint8_t)(c.b * 0.55f));
    }
}

// ============================================================ UI OVERLAY
static void DrawSelectionOutline(Framebuffer& fb, const Part& p, float time) {
    float pulse = 0.5f + 0.5f * sinf(time * 6.f);
    uint32_t on  = ((uint32_t)(180 + 70 * pulse) << 16)
                 | ((uint32_t)(240) << 8) | (uint32_t)255;
    Bounds b = GetBounds(p);
    int x0 = (int)b.mn.x - 2, x1 = (int)b.mx.x + 2;
    int y0 = (int)b.mn.y - 2, y1 = (int)b.mx.y + 2;
    // Dashed rectangle
    for (int x = x0; x <= x1; ++x) {
        if (((x + (int)(time * 20)) / 3) & 1) continue;
        fb.Set(x, y0, on); fb.Set(x, y1, on);
    }
    for (int y = y0; y <= y1; ++y) {
        if (((y + (int)(time * 20)) / 3) & 1) continue;
        fb.Set(x0, y, on); fb.Set(x1, y, on);
    }
}

static void DrawUI(Framebuffer& fb, float time) {
    // ---- top info bar ----
    for (int y = 0; y < 13; ++y)
        for (int x = 0; x < 320; ++x) fb.Blend(x, y, Color(10, 16, 26, 220));

    char buf[64];
    const char* label = (g_selectedPart >= 0) ? g_parts[g_selectedPart].name : "(click a part)";
    snprintf(buf, sizeof(buf), "SELECTED: %s", label);
    DrawString(fb, buf, 5, 4, 0x66C0F4);

    // Right side: hint
    DrawString(fb, "CLICK PART > SWATCH", 320 - 22 * 4 - 4, 4, 0x8FA0AF);

    // ---- bottom palette bar ----
    for (int y = 156; y < 180; ++y)
        for (int x = 0; x < 320; ++x) fb.Blend(x, y, Color(10, 16, 26, 220));

    const int sw = 22, sh = 18, gap = 2;
    const int total = 12 * sw + 11 * gap;
    const int x0 = (320 - total) / 2;
    const int y0 = 180 - sh - 3;

    for (int i = 0; i < 12; ++i) {
        int x = x0 + i * (sw + gap);
        RECT r{ x, y0, x + sw, y0 + sh };
        g_swatchRect[i] = r;

        // Swatch itself, shaded very gently so it still reads as flat color
        for (int py = r.top; py < r.bottom; ++py)
            for (int px = r.left; px < r.right; ++px) {
                float t = (float)(py - r.top) / sh;
                Color c = LerpC(PALETTE[i],
                                Color((uint8_t)(PALETTE[i].r * 0.7f),
                                      (uint8_t)(PALETTE[i].g * 0.7f),
                                      (uint8_t)(PALETTE[i].b * 0.7f)), t * 0.35f);
                fb.Set(px, py, c.Pack());
            }

        // Selection ring + pulse
        if (i == g_currentSwatch) {
            float pulse = 0.5f + 0.5f * sinf(time * 6.f);
            uint32_t ring = (uint32_t)(160 + 90 * pulse) << 16
                          | (uint32_t)255 << 8 | 255;
            for (int px = r.left - 1; px <= r.right; ++px) {
                fb.Set(px, r.top - 1, ring);
                fb.Set(px, r.bottom,  ring);
            }
            for (int py = r.top - 1; py <= r.bottom; ++py) {
                fb.Set(r.left - 1, py, ring);
                fb.Set(r.right,    py, ring);
            }
        }
    }
}

// ============================================================ APP
struct App {
    Framebuffer fb;
    float time = 0.f;

    void Init(int W, int H) { fb.Init(W, H); }

    void Render() {
        // 1. Everything in scene order — sky first, robot last.
        for (const auto& p : g_parts) DrawPart(fb, p);

        // 2. Ambient particle field floats above the scene.
        DrawParticles(fb);

        // 3. Selection outline draws on top of everything.
        if (g_selectedPart >= 0) DrawSelectionOutline(fb, g_parts[g_selectedPart], time);

        // 4. UI bars + palette on the very top.
        DrawUI(fb, time);

        // 5. Post-process: scanlines + vignette.
        for (int y = 0; y < 180; y += 2)
            for (int x = 0; x < 320; ++x) {
                uint32_t c = fb.Get(x, y);
                int r = ((c >> 16) & 0xFF) * 90 / 100;
                int g = ((c >>  8) & 0xFF) * 90 / 100;
                int b = ( c        & 0xFF) * 90 / 100;
                fb.Set(x, y, ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b);
            }
        for (int y = 0; y < 180; ++y)
            for (int x = 0; x < 320; ++x) {
                float dx = (x - 160.f) / 160.f;
                float dy = (y -  90.f) /  90.f;
                float d  = dx * dx + dy * dy;
                if (d > 0.85f) {
                    float a = Clampf((d - 0.85f) * 0.8f, 0.f, 0.45f);
                    fb.Blend(x, y, Color(0, 0, 0, (uint8_t)(a * 255)));
                }
            }
    }
};

static App g_app;
static const int CANVAS_W = 320;
static const int CANVAS_H = 180;

// ============================================================ PRESENT
static void Present(HWND hwnd) {
    HDC dc = GetDC(hwnd);
    RECT cr; GetClientRect(hwnd, &cr);
    int cw = cr.right, ch = cr.bottom;

    int scale = std::min(cw / CANVAS_W, ch / CANVAS_H);
    if (scale < 1) scale = 1;
    int dw = CANVAS_W * scale, dh = CANVAS_H * scale;
    int ox = (cw - dw) / 2, oy = (ch - dh) / 2;

    HBRUSH bg = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(dc, &cr, bg);
    DeleteObject(bg);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       =  CANVAS_W;
    bmi.bmiHeader.biHeight      = -CANVAS_H;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetStretchBltMode(dc, COLORONCOLOR);
    StretchDIBits(dc, ox, oy, dw, dh, 0, 0, CANVAS_W, CANVAS_H,
                  g_app.fb.px.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(hwnd, dc);
}

// Convert client pixel → framebuffer coordinate (returns false if outside)
static bool ScreenToCanvas(HWND hwnd, int mx, int my, Vec2& out) {
    RECT cr; GetClientRect(hwnd, &cr);
    int cw = cr.right, ch = cr.bottom;
    int scale = std::min(cw / CANVAS_W, ch / CANVAS_H);
    if (scale < 1) scale = 1;
    int dw = CANVAS_W * scale, dh = CANVAS_H * scale;
    int ox = (cw - dw) / 2, oy = (ch - dh) / 2;
    int fx = (mx - ox) / scale;
    int fy = (my - oy) / scale;
    if (fx < 0 || fx >= CANVAS_W || fy < 0 || fy >= CANVAS_H) return false;
    out = { (float)fx, (float)fy };
    return true;
}

// ============================================================ WINDOW
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        InitMaterials();
        BuildScene();
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

    case WM_MOUSEMOVE: {
        TRACKMOUSEEVENT tme{ sizeof(tme) };
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        TrackMouseEvent(&tme);

        Vec2 cp;
        if (ScreenToCanvas(hwnd, (short)LOWORD(lp), (short)HIWORD(lp), cp)) {
            int hp = PickPart(cp);
            if (hp != g_hoverPart) {
                g_hoverPart = hp;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        g_hoverPart = -1;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) {
            POINT p; GetCursorPos(&p); ScreenToClient(hwnd, &p);
            Vec2 cp;
            bool hot = ScreenToCanvas(hwnd, p.x, p.y, cp) && PickPart(cp) >= 0;
            SetCursor(LoadCursor(nullptr, hot ? IDC_HAND : IDC_ARROW));
            return TRUE;
        }
        break;

    case WM_LBUTTONDOWN: {
        Vec2 cp;
        if (ScreenToCanvas(hwnd, (short)LOWORD(lp), (short)HIWORD(lp), cp)) {
            // Palette takes priority over scene picking
            int mx = (int)cp.x, my = (int)cp.y;
            bool hitSwatch = false;
            for (int i = 0; i < 12; ++i) {
                const RECT& r = g_swatchRect[i];
                if (mx >= r.left && mx < r.right && my >= r.top && my < r.bottom) {
                    g_currentSwatch = i;
                    ApplyColor(g_selectedPart, PALETTE[i]);
                    hitSwatch = true;
                    break;
                }
            }
            if (!hitSwatch) {
                int idx = PickPart(cp);
                if (idx >= 0) g_selectedPart = idx;
            }
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) PostQuitMessage(0);
        // 1..9 + 0 apply palette slots to the selected part
        if (wp >= '1' && wp <= '9') {
            g_currentSwatch = wp - '1';
            ApplyColor(g_selectedPart, PALETTE[g_currentSwatch]);
            InvalidateRect(hwnd, nullptr, FALSE);
        } else if (wp == '0') {
            g_currentSwatch = 9;
            ApplyColor(g_selectedPart, PALETTE[9]);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_TIMER: {
        static LARGE_INTEGER freq = {}, last = {};
        static bool init = false;
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        if (!init) { QueryPerformanceFrequency(&freq); last = now; init = true; }
        float dt = (float)((double)(now.QuadPart - last.QuadPart) / freq.QuadPart);
        last = now;
        dt = Clampf(dt, 0.001f, 0.05f);

        g_app.time += dt;
        UpdateParticles(dt);
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
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"SteamColorWnd";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, L"SteamColorWnd", L"Per-Pixel Color Engine",
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
