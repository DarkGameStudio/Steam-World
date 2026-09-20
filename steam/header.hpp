// ============================================================
//  essentials.hpp
//  Single-header toolkit for game & systems C++.
//  C++17, header-only, no dependencies.
//
//  Drop this file in your project and #include "essentials.hpp".
//  Everything lives in namespace `ess`. Nothing is global.
//
//  Sections:
//    1.  Platform & compiler macros
//    2.  Basic integer / float types
//    3.  Scalar math helpers
//    4.  Vec2, Vec3, Vec4
//    5.  Mat4 (column-major, OpenGL-style)
//    6.  Quat
//    7.  Random (PCG32)
//    8.  Hashing (FNV-1a)
//    9.  Arena allocator
//   10.  Array<T>  (growable, move-only)
//   11.  StringView
//   12.  Logging
//   13.  Timing
//   14.  Assertions & debug helpers
// ============================================================
#pragma once

// ------------------------------------------------------------
// 1. Platform
// ------------------------------------------------------------
#if defined(_WIN32) || defined(_WIN64)
    #define ESS_WINDOWS 1
#else
    #define ESS_WINDOWS 0
#endif
#if defined(__linux__)
    #define ESS_LINUX 1
#else
    #define ESS_LINUX 0
#endif
#if defined(__APPLE__)
    #define ESS_MACOS 1
#else
    #define ESS_MACOS 0
#endif

// ------------------------------------------------------------
// 1b. Compiler
// ------------------------------------------------------------
#if defined(_MSC_VER)
    #define ESS_MSVC 1
    #define ESS_FORCEINLINE __forceinline
    #define ESS_NOINLINE    __declspec(noinline)
    #define ESS_DEBUGBREAK() __debugbreak()
    #define ESS_UNREACHABLE() __assume(0)
    #define ESS_RESTRICT __restrict
#elif defined(__clang__)
    #define ESS_CLANG 1
    #define ESS_FORCEINLINE inline __attribute__((always_inline))
    #define ESS_NOINLINE    __attribute__((noinline))
    #define ESS_DEBUGBREAK() __builtin_trap()
    #define ESS_UNREACHABLE() __builtin_unreachable()
    #define ESS_RESTRICT __restrict__
#elif defined(__GNUC__)
    #define ESS_GCC 1
    #define ESS_FORCEINLINE inline __attribute__((always_inline))
    #define ESS_NOINLINE    __attribute__((noinline))
    #define ESS_DEBUGBREAK() __builtin_trap()
    #define ESS_UNREACHABLE() __builtin_unreachable()
    #define ESS_RESTRICT __restrict__
#else
    #define ESS_FORCEINLINE inline
    #define ESS_NOINLINE
    #define ESS_DEBUGBREAK() ((void)0)
    #define ESS_UNREACHABLE() ((void)0)
    #define ESS_RESTRICT
#endif

// ------------------------------------------------------------
// Includes
// ------------------------------------------------------------
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <cassert>
#include <new>
#include <utility>
#include <type_traits>
#include <chrono>
#include <atomic>

namespace ess {

// ============================================================
// 2. Basic types
// ============================================================
using u8    = std::uint8_t;
using u16   = std::uint16_t;
using u32   = std::uint32_t;
using u64   = std::uint64_t;
using i8    = std::int8_t;
using i16   = std::int16_t;
using i32   = std::int32_t;
using i64   = std::int64_t;
using f32   = float;
using f64   = double;
using usize = std::size_t;
using isize = std::ptrdiff_t;

// ============================================================
// 3. Scalar math helpers
// ============================================================
constexpr f32 PI      = 3.14159265358979323846f;
constexpr f32 TAU     = 6.28318530717958647692f;
constexpr f32 HALF_PI = 1.57079632679489661923f;
constexpr f32 DEG2RAD = PI / 180.f;
constexpr f32 RAD2DEG = 180.f / PI;
constexpr f32 EPSILON = 1e-6f;

template <typename T> constexpr T Min(T a, T b) { return a < b ? a : b; }
template <typename T> constexpr T Max(T a, T b) { return a > b ? a : b; }
template <typename T> constexpr T Clamp(T v, T lo, T hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
template <typename T> constexpr T Abs(T v) { return v < T(0) ? -v : v; }
template <typename T> constexpr T Sign(T v) {
    return T(v > T(0)) - T(v < T(0));
}
template <typename T> constexpr void Swap(T& a, T& b) {
    T t = a; a = b; b = t;
}

inline f32 Lerp(f32 a, f32 b, f32 t) { return a + (b - a) * t; }
inline f64 Lerp(f64 a, f64 b, f32 t) { return a + (b - a) * t; }

constexpr f32 Saturate(f32 v)   { return Clamp(v, 0.f, 1.f); }
constexpr f32 Smoothstep(f32 t) { return t * t * (3.f - 2.f * t); }
constexpr f32 InvLerp(f32 a, f32 b, f32 v) { return (v - a) / (b - a); }

inline f32 Sqrt(f32 v)          { return sqrtf(v); }
inline f32 Sin(f32 v)           { return sinf(v); }
inline f32 Cos(f32 v)           { return cosf(v); }
inline f32 Tan(f32 v)           { return tanf(v); }
inline f32 Pow(f32 a, f32 b)    { return powf(a, b); }
inline f32 Floor(f32 v)         { return floorf(v); }
inline f32 Ceil(f32 v)          { return ceilf(v); }
inline f32 Fmod(f32 a, f32 b)   { return fmodf(a, b); }

inline bool NearlyEqual(f32 a, f32 b, f32 eps = EPSILON) {
    return Abs(a - b) <= eps;
}

// ============================================================
// 4. Vectors
// ============================================================
struct Vec2 {
    f32 x = 0, y = 0;
    constexpr Vec2() = default;
    constexpr Vec2(f32 X, f32 Y) : x(X), y(Y) {}

    Vec2  operator+(Vec2 o)  const { return {x + o.x, y + o.y}; }
    Vec2  operator-(Vec2 o)  const { return {x - o.x, y - o.y}; }
    Vec2  operator*(f32 s)   const { return {x * s,   y * s};   }
    Vec2  operator/(f32 s)   const { return {x / s,   y / s};   }
    Vec2  operator-()        const { return {-x, -y}; }
    Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(f32 s)  { x *= s;   y *= s;   return *this; }
    bool  operator==(Vec2 o) const { return x == o.x && y == o.y; }
};
inline Vec2 operator*(f32 s, Vec2 v) { return v * s; }
inline f32  Dot(Vec2 a, Vec2 b)      { return a.x*b.x + a.y*b.y; }
inline f32  LengthSq(Vec2 v)         { return Dot(v, v); }
inline f32  Length(Vec2 v)           { return sqrtf(LengthSq(v)); }
inline f32  Distance(Vec2 a, Vec2 b) { return Length(b - a); }
inline f32  Cross(Vec2 a, Vec2 b)    { return a.x*b.y - a.y*b.x; }
inline Vec2 Normalize(Vec2 v) {
    f32 l = Length(v);
    return l > EPSILON ? Vec2{v.x / l, v.y / l} : Vec2{0, 0};
}
inline Vec2 Lerp(Vec2 a, Vec2 b, f32 t) { return a + (b - a) * t; }

struct Vec3 {
    f32 x = 0, y = 0, z = 0;
    constexpr Vec3() = default;
    constexpr Vec3(f32 X, f32 Y, f32 Z) : x(X), y(Y), z(Z) {}

    Vec3  operator+(Vec3 o)  const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3  operator-(Vec3 o)  const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3  operator*(f32 s)   const { return {x * s,   y * s,   z * s};   }
    Vec3  operator/(f32 s)   const { return {x / s,   y / s,   z / s};   }
    Vec3  operator-()        const { return {-x, -y, -z}; }
    Vec3& operator+=(Vec3 o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(Vec3 o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(f32 s)  { x *= s;   y *= s;   z *= s;   return *this; }
    bool  operator==(Vec3 o) const { return x == o.x && y == o.y && z == o.z; }
};
inline Vec3 operator*(f32 s, Vec3 v) { return v * s; }
inline f32  Dot(Vec3 a, Vec3 b)      { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline f32  LengthSq(Vec3 v)         { return Dot(v, v); }
inline f32  Length(Vec3 v)           { return sqrtf(LengthSq(v)); }
inline f32  Distance(Vec3 a, Vec3 b) { return Length(b - a); }
inline Vec3 Cross(Vec3 a, Vec3 b) {
    return { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
}
inline Vec3 Normalize(Vec3 v) {
    f32 l = Length(v);
    return l > EPSILON ? Vec3{v.x / l, v.y / l, v.z / l} : Vec3{0, 0, 0};
}
inline Vec3 Lerp(Vec3 a, Vec3 b, f32 t) { return a + (b - a) * t; }

struct Vec4 {
    f32 x = 0, y = 0, z = 0, w = 0;
    constexpr Vec4() = default;
    constexpr Vec4(f32 X, f32 Y, f32 Z, f32 W) : x(X), y(Y), z(Z), w(W) {}
    constexpr Vec4(Vec3 v, f32 W) : x(v.x), y(v.y), z(v.z), w(W) {}

    Vec4  operator+(Vec4 o)  const { return {x+o.x, y+o.y, z+o.z, w+o.w}; }
    Vec4  operator-(Vec4 o)  const { return {x-o.x, y-o.y, z-o.z, w-o.w}; }
    Vec4  operator*(f32 s)   const { return {x*s,   y*s,   z*s,   w*s};   }
    Vec4& operator+=(Vec4 o) { x+=o.x; y+=o.y; z+=o.z; w+=o.w; return *this; }
    Vec4& operator*=(f32 s)  { x*=s; y*=s; z*=s; w*=s; return *this; }
    Vec3  XYZ() const { return {x, y, z}; }
};
inline Vec4 operator*(f32 s, Vec4 v) { return v * s; }
inline f32  Dot(Vec4 a, Vec4 b)      { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }

// ============================================================
// 5. Mat4  (column-major — matches OpenGL, Vulkan, GLSL)
//    m[col][row]
// ============================================================
struct Mat4 {
    f32 m[4][4] = {};

    static Mat4 Identity() {
        Mat4 r;
        r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1.f;
        return r;
    }
    static Mat4 Zero() { return {}; }

    static Mat4 Translation(Vec3 t) {
        Mat4 r = Identity();
        r.m[3][0] = t.x; r.m[3][1] = t.y; r.m[3][2] = t.z;
        return r;
    }
    static Mat4 Scale(Vec3 s) {
        Mat4 r = Identity();
        r.m[0][0] = s.x; r.m[1][1] = s.y; r.m[2][2] = s.z;
        return r;
    }
    static Mat4 RotationX(f32 a) {
        Mat4 r = Identity();
        f32 c = cosf(a), s = sinf(a);
        r.m[1][1] =  c; r.m[1][2] = s;
        r.m[2][1] = -s; r.m[2][2] = c;
        return r;
    }
    static Mat4 RotationY(f32 a) {
        Mat4 r = Identity();
        f32 c = cosf(a), s = sinf(a);
        r.m[0][0] =  c; r.m[0][2] = -s;
        r.m[2][0] =  s; r.m[2][2] =  c;
        return r;
    }
    static Mat4 RotationZ(f32 a) {
        Mat4 r = Identity();
        f32 c = cosf(a), s = sinf(a);
        r.m[0][0] =  c; r.m[0][1] = s;
        r.m[1][0] = -s; r.m[1][1] = c;
        return r;
    }
    static Mat4 Perspective(f32 fovYRad, f32 aspect, f32 zn, f32 zf) {
        Mat4 r{};
        f32 f = 1.f / tanf(fovYRad * 0.5f);
        r.m[0][0] = f / aspect;
        r.m[1][1] = f;
        r.m[2][2] = (zf + zn) / (zn - zf);
        r.m[2][3] = -1.f;
        r.m[3][2] = (2.f * zf * zn) / (zn - zf);
        return r;
    }
    static Mat4 Ortho(f32 l, f32 r_, f32 b, f32 t, f32 zn, f32 zf) {
        Mat4 r = Identity();
        r.m[0][0] =  2.f / (r_ - l);
        r.m[1][1] =  2.f / (t  - b);
        r.m[2][2] = -2.f / (zf - zn);
        r.m[3][0] = -(r_ + l) / (r_ - l);
        r.m[3][1] = -(t  + b) / (t  - b);
        r.m[3][2] = -(zf + zn) / (zf - zn);
        return r;
    }
    static Mat4 LookAt(Vec3 eye, Vec3 center, Vec3 up) {
        Vec3 f = Normalize(center - eye);
        Vec3 s = Normalize(Cross(f, up));
        Vec3 u = Cross(s, f);
        Mat4 r = Identity();
        r.m[0][0] =  s.x; r.m[0][1] =  u.x; r.m[0][2] = -f.x;
        r.m[1][0] =  s.y; r.m[1][1] =  u.y; r.m[1][2] = -f.y;
        r.m[2][0] =  s.z; r.m[2][1] =  u.z; r.m[2][2] = -f.z;
        r.m[3][0] = -Dot(s, eye);
        r.m[3][1] = -Dot(u, eye);
        r.m[3][2] =  Dot(f, eye);
        return r;
    }

    Mat4 Transposed() const {
        Mat4 r{};
        for (int c = 0; c < 4; ++c)
            for (int rw = 0; rw < 4; ++rw)
                r.m[c][rw] = m[rw][c];
        return r;
    }
};

inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int rw = 0; rw < 4; ++rw) {
            f32 sum = 0;
            for (int k = 0; k < 4; ++k) sum += a.m[k][rw] * b.m[c][k];
            r.m[c][rw] = sum;
        }
    return r;
}
inline Vec4 operator*(const Mat4& m, Vec4 v) {
    return {
        m.m[0][0]*v.x + m.m[1][0]*v.y + m.m[2][0]*v.z + m.m[3][0]*v.w,
        m.m[0][1]*v.x + m.m[1][1]*v.y + m.m[2][1]*v.z + m.m[3][1]*v.w,
        m.m[0][2]*v.x + m.m[1][2]*v.y + m.m[2][2]*v.z + m.m[3][2]*v.w,
        m.m[0][3]*v.x + m.m[1][3]*v.y + m.m[2][3]*v.z + m.m[3][3]*v.w,
    };
}
inline Vec3 TransformPoint(const Mat4& m, Vec3 p) {
    Vec4 r = m * Vec4(p, 1.f);
    if (Abs(r.w) > EPSILON) return {r.x / r.w, r.y / r.w, r.z / r.w};
    return {r.x, r.y, r.z};
}
inline Vec3 TransformDir(const Mat4& m, Vec3 d) {
    Vec4 r = m * Vec4(d, 0.f);
    return {r.x, r.y, r.z};
}

// ============================================================
// 6. Quat — unit quaternion, w is the scalar part
// ============================================================
struct Quat {
    f32 x = 0, y = 0, z = 0, w = 1;

    static Quat Identity() { return {0, 0, 0, 1}; }

    static Quat FromAxisAngle(Vec3 axis, f32 angle) {
        Vec3 a = Normalize(axis);
        f32 h = angle * 0.5f;
        f32 s = sinf(h);
        return { a.x * s, a.y * s, a.z * s, cosf(h) };
    }

    Quat operator*(Quat o) const {
        return {
            w*o.x + x*o.w + y*o.z - z*o.y,
            w*o.y - x*o.z + y*o.w + z*o.x,
            w*o.z + x*o.y - y*o.x + z*o.w,
            w*o.w - x*o.x - y*o.y - z*o.z,
        };
    }

    Quat Normalized() const {
        f32 l = sqrtf(x*x + y*y + z*z + w*w);
        if (l < EPSILON) return Identity();
        return { x/l, y/l, z/l, w/l };
    }

    Vec3 Rotate(Vec3 v) const {
        Vec3 u{ x, y, z };
        Vec3 t = Cross(u, v) * 2.f;
        return v + t * w + Cross(u, t);
    }

    Mat4 ToMat4() const {
        Mat4 r = Mat4::Identity();
        f32 xx = x*x, yy = y*y, zz = z*z;
        f32 xy = x*y, xz = x*z, yz = y*z;
        f32 wx = w*x, wy = w*y, wz = w*z;
        r.m[0][0] = 1 - 2*(yy + zz); r.m[0][1] = 2*(xy + wz);     r.m[0][2] = 2*(xz - wy);
        r.m[1][0] = 2*(xy - wz);     r.m[1][1] = 1 - 2*(xx + zz); r.m[1][2] = 2*(yz + wx);
        r.m[2][0] = 2*(xz + wy);     r.m[2][1] = 2*(yz - wx);     r.m[2][2] = 1 - 2*(xx + yy);
        return r;
    }
};

// ============================================================
// 7. Random — PCG32
//    Small state, good statistical quality, deterministic.
// ============================================================
struct Random {
    u64 state = 0;
    u64 inc   = 1;

    explicit Random(u64 seed = 0x853c49e6748fea9bULL) {
        state = 0;
        inc   = (seed << 1u) | 1u;
        NextU32();
        state += seed;
        NextU32();
    }

    u32 NextU32() {
        u64 old = state;
        state = old * 6364136223846793005ULL + inc;
        u32 xorshifted = u32(((old >> 18u) ^ old) >> 27u);
        u32 rot = u32(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }
    u64 NextU64() {
        return ((u64)NextU32() << 32) | (u64)NextU32();
    }
    f32 NextFloat() {
        return (NextU32() >> 8) * (1.f / 16777216.f); // [0, 1)
    }
    f32 Range(f32 lo, f32 hi) { return lo + (hi - lo) * NextFloat(); }
    i32 RangeI(i32 lo, i32 hi) {
        return lo + i32(NextU32() % u32(hi - lo + 1));
    }
    bool CoinFlip() { return (NextU32() & 1u) != 0; }
};

// ============================================================
// 8. Hashing — FNV-1a (constexpr, good for string keys)
// ============================================================
constexpr u64 HashFNV1a(const char* s, u64 h = 14695981039346656037ULL) {
    return *s
        ? HashFNV1a(s + 1, (h ^ u64(u8(*s))) * 1099511628211ULL)
        : h;
}
constexpr u64 HashFNV1a(const void* data, usize n,
                        u64 h = 14695981039346656037ULL) {
    const u8* p = (const u8*)data;
    for (usize i = 0; i < n; ++i)
        h = (h ^ u64(p[i])) * 1099511628211ULL;
    return h;
}

// Cheap integer hash — useful for scattering pool lookups.
constexpr u32 HashU32(u32 x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

// ============================================================
// 9. Arena allocator
//    Bump pointer, never frees individual items. Ideal for
//    per-frame scratch, per-level, per-load-tick work.
// ============================================================
struct Arena {
    u8*   base = nullptr;
    usize cap  = 0;
    usize used = 0;

    void Init(void* mem, usize bytes) {
        base = (u8*)mem;
        cap  = bytes;
        used = 0;
    }
    void Reset() { used = 0; }

    usize Remaining() const { return cap - used; }

    void* Push(usize bytes, usize align = 8) {
        if (bytes == 0) return nullptr;
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

// ============================================================
// 10. Array<T> — growable, move-only, malloc-backed
//     Explicit and non-magical. No iterators to hide bugs.
// ============================================================
template <typename T>
struct Array {
    T*    data  = nullptr;
    usize count = 0;
    usize cap   = 0;

    Array() = default;
    Array(const Array&) = delete;
    Array& operator=(const Array&) = delete;

    Array(Array&& o) noexcept
        : data(o.data), count(o.count), cap(o.cap) {
        o.data = nullptr; o.count = 0; o.cap = 0;
    }
    Array& operator=(Array&& o) noexcept {
        if (this != &o) {
            Free();
            data = o.data; count = o.count; cap = o.cap;
            o.data = nullptr; o.count = 0; o.cap = 0;
        }
        return *this;
    }
    ~Array() { Free(); }

    void Free() {
        for (usize i = 0; i < count; ++i) data[i].~T();
        std::free(data);
        data = nullptr; count = cap = 0;
    }

    void Reserve(usize n) {
        if (n <= cap) return;
        usize nc = cap ? cap * 2 : 8;
        if (nc < n) nc = n;
        T* nd = (T*)std::realloc(data, nc * sizeof(T));
        if (!nd) { std::fprintf(stderr, "ess::Array OOM\n"); std::abort(); }
        data = nd; cap = nc;
    }

    template <typename... Args>
    T& Emplace(Args&&... args) {
        if (count >= cap) Reserve(count + 1);
        new (&data[count]) T(std::forward<Args>(args)...);
        return data[count++];
    }
    void Push(const T& v) { Emplace(v); }
    void Push(T&& v)      { Emplace(std::move(v)); }

    void Pop() {
        if (count) data[--count].~T();
    }
    void Clear() {
        for (usize i = 0; i < count; ++i) data[i].~T();
        count = 0;
    }
    void Resize(usize n) {
        if (n < count) { for (usize i = n; i < count; ++i) data[i].~T(); }
        else if (n > count) {
            Reserve(n);
            for (usize i = count; i < n; ++i) new (&data[i]) T();
        }
        count = n;
    }

    T&       operator[](usize i)       { return data[i]; }
    const T& operator[](usize i) const { return data[i]; }
    T*       begin()       { return data; }
    T*       end()         { return data + count; }
    const T* begin() const { return data; }
    const T* end()   const { return data + count; }
    T&       Front()       { return data[0]; }
    T&       Back()        { return data[count - 1]; }
    bool     Empty() const { return count == 0; }
    usize    Size()  const { return count; }
};

// ============================================================
// 11. StringView — non-owning slice of a char buffer
// ============================================================
struct StringView {
    const char* data = nullptr;
    usize       size = 0;

    constexpr StringView() = default;
    constexpr StringView(const char* s)
        : data(s), size(s ? StrLen(s) : 0) {}
    constexpr StringView(const char* s, usize n)
        : data(s), size(n) {}

    static constexpr usize StrLen(const char* s) {
        usize n = 0;
        while (s && s[n]) ++n;
        return n;
    }

    bool Empty() const { return size == 0; }
    char operator[](usize i) const { return data[i]; }

    StringView Sub(usize start, usize len) const {
        if (start >= size) return {};
        usize n = (start + len > size) ? (size - start) : len;
        return { data + start, n };
    }

    bool StartsWith(StringView p) const {
        if (p.size > size) return false;
        for (usize i = 0; i < p.size; ++i)
            if (data[i] != p.data[i]) return false;
        return true;
    }
    bool EndsWith(StringView p) const {
        if (p.size > size) return false;
        for (usize i = 0; i < p.size; ++i)
            if (data[size - p.size + i] != p.data[i]) return false;
        return true;
    }
    bool Contains(char c) const {
        for (usize i = 0; i < size; ++i)
            if (data[i] == c) return true;
        return false;
    }
    bool operator==(StringView o) const {
        if (size != o.size) return false;
        for (usize i = 0; i < size; ++i)
            if (data[i] != o.data[i]) return false;
        return true;
    }
    bool operator!=(StringView o) const { return !(*this == o); }

    // Hash for use as a map key
    u64 Hash() const { return HashFNV1a(data, size); }
};

// Case-insensitive equality for config keys, file extensions, etc.
inline bool EqualsIgnoreCase(StringView a, StringView b) {
    if (a.size != b.size) return false;
    for (usize i = 0; i < a.size; ++i) {
        char ca = a.data[i], cb = b.data[i];
        if (ca >= 'A' && ca <= 'Z') ca = char(ca + 32);
        if (cb >= 'A' && cb <= 'Z') cb = char(cb + 32);
        if (ca != cb) return false;
    }
    return true;
}

// ============================================================
// 12. Logging
// ============================================================
enum class LogLevel : int {
    Trace = 0, Debug = 1, Info = 2, Warn = 3, Error = 4, Off = 5
};

inline LogLevel& LogThreshold() {
    static LogLevel lvl = LogLevel::Info;
    return lvl;
}

inline void LogSetLevel(LogLevel lvl) { LogThreshold() = lvl; }

inline void Log(LogLevel lvl, const char* file, int line,
                const char* fmt, ...) {
    if (int(lvl) < int(LogThreshold())) return;

    const char* tag = "INFO";
    const char* col = "\x1b[32m";
    switch (lvl) {
        case LogLevel::Trace: tag = "TRACE"; col = "\x1b[90m"; break;
        case LogLevel::Debug: tag = "DEBUG"; col = "\x1b[36m"; break;
        case LogLevel::Info:  tag = "INFO "; col = "\x1b[32m"; break;
        case LogLevel::Warn:  tag = "WARN "; col = "\x1b[33m"; break;
        case LogLevel::Error: tag = "ERROR"; col = "\x1b[31m"; break;
        default: break;
    }

    // Print only the basename of the file.
    const char* base = file;
    for (const char* p = file; *p; ++p)
        if (*p == '/' || *p == '\\') base = p + 1;

    std::fprintf(stderr, "%s[%s]\x1b[0m %s:%d  ", col, tag, base, line);
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
    std::fputc('\n', stderr);
}

#define ESS_TRACE(...) ::ess::Log(::ess::LogLevel::Trace, __FILE__, __LINE__, __VA_ARGS__)
#define ESS_DEBUG(...) ::ess::Log(::ess::LogLevel::Debug, __FILE__, __LINE__, __VA_ARGS__)
#define ESS_INFO(...)  ::ess::Log(::ess::LogLevel::Info,  __FILE__, __LINE__, __VA_ARGS__)
#define ESS_WARN(...)  ::ess::Log(::ess::LogLevel::Warn,  __FILE__, __LINE__, __VA_ARGS__)
#define ESS_ERROR(...) ::ess::Log(::ess::LogLevel::Error, __FILE__, __LINE__, __VA_ARGS__)

// ============================================================
// 13. Timing
// ============================================================
inline u64 NowNanos() {
    using namespace std::chrono;
    return u64(duration_cast<nanoseconds>(
        steady_clock::now().time_since_epoch()).count());
}
inline f64 NowSeconds() { return f64(NowNanos()) * 1e-9; }

// Simple frame clock: call Tick() once per frame and read Delta().
struct Clock {
    u64 last = 0;
    f32 delta = 0.f;

    void Start() { last = NowNanos(); }
    void Tick() {
        u64 now = NowNanos();
        delta = f32(f64(now - last) * 1e-9);
        last = now;
    }
    f32 Delta() const { return delta; }
};

// ============================================================
// 14. Assertions
// ============================================================
[[noreturn]] inline void AssertFail(const char* expr,
                                    const char* file,
                                    int line) {
    std::fprintf(stderr,
        "\n*** ASSERT FAILED ***\n"
        "  expression : %s\n"
        "  location   : %s:%d\n\n",
        expr, file, line);
    ESS_DEBUGBREAK();
    std::abort();
}

} // namespace ess

// ------------------------------------------------------------
// Assert macros — outside namespace so they're easy to call.
// ------------------------------------------------------------
#ifdef NDEBUG
    #define ESS_ASSERT(expr)          ((void)0)
    #define ESS_ASSERT_MSG(expr, msg) ((void)0)
#else
    #define ESS_ASSERT(expr) \
        do { if (!(expr)) ::ess::AssertFail(#expr, __FILE__, __LINE__); } while (0)
    #define ESS_ASSERT_MSG(expr, msg) \
        do { if (!(expr)) { std::fprintf(stderr, "assert: %s\n", msg); \
             ::ess::AssertFail(#expr, __FILE__, __LINE__); } } while (0)
#endif

// ============================================================
// End of essentials.hpp
// ============================================================

/* ---------------------------------------------------------------
   EXAMPLE USAGE
   ---------------------------------------------------------------
   #include "essentials.hpp"
   using namespace ess;

   int main() {
       // Logging
       LogSetLevel(LogLevel::Debug);
       ESS_INFO("boot: %s v%d", "engine", 1);

       // Math
       Vec3 a{1, 0, 0}, b{0, 1, 0};
       Vec3 n = Normalize(Cross(a, b));       // (0,0,1)

       Mat4 view = Mat4::LookAt({0,0,5}, {0,0,0}, {0,1,0});
       Mat4 proj = Mat4::Perspective(60 * DEG2RAD, 16.f/9.f, 0.1f, 100.f);
       Vec3 world{1, 2, 3};
       Vec3 screen = TransformPoint(proj * view, world);

       // Quaternion
       Quat q = Quat::FromAxisAngle({0,1,0}, 45 * DEG2RAD);
       Vec3 rotated = q.Rotate({1, 0, 0});

       // Random
       Random rng(1234);
       f32 x = rng.Range(-1.f, 1.f);
       i32 d6 = rng.RangeI(1, 6);

       // Arena (backing buffer is yours to own)
       static u8 scratch[1 << 20];
       Arena arena;
       arena.Init(scratch, sizeof(scratch));
       int* nums = arena.Make<int>(42);

       // Array<T>
       Array<Vec3> verts;
       verts.Push({0, 0, 0});
       verts.Push({1, 0, 0});
       for (Vec3& v : verts) v.y += 1.f;

       // StringView + hashing
       StringView line = "player.health=100";
       if (line.StartsWith("player.")) {
           u64 key = StringView("player.health").Hash();
           ESS_DEBUG("key=%llu value=%.*s", (unsigned long long)key,
                     int(line.size - 14), line.data + 14);
       }

       // Clock
       Clock clock; clock.Start();
       // ... frame work ...
       clock.Tick();
       ESS_TRACE("frame %.2f ms", clock.Delta() * 1000.f);

       // Assertion
       ESS_ASSERT(verts.Size() == 2);
       return 0;
   }
   --------------------------------------------------------------- */