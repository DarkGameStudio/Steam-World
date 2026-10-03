// ============================================================================
// three.cpp - A 3D Graphics Library for C++ (Similar to three.js)
// ============================================================================

#ifndef THREE_CPP_H
#define THREE_CPP_H

#include <iostream>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <algorithm>
#include <functional>
#include <optional>
#include <tuple>
#include <string>
#include <sstream>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <random>
#include <iomanip>
#define M_PI 3.14
// ============================================================================
// PART 1: CORE MATHEMATICS
// ============================================================================

namespace three {

// Forward declarations
class Object3D;
class Scene;
class Camera;
class PerspectiveCamera;
class OrthographicCamera;
class Mesh;
class Geometry;
class BufferGeometry;
class Material;
class MeshBasicMaterial;
class MeshPhongMaterial;
class MeshStandardMaterial;
class Light;
class DirectionalLight;
class PointLight;
class AmbientLight;
class Renderer;
class WebGLRenderer;
class Group;
class Texture;
class Raycaster;
class Vector2;
class Vector3;
class Vector4;
class Matrix3;
class Matrix4;
class Quaternion;
class Euler;
class Color;
class Ray;
class Plane;
class Sphere;
class Box3;
class Frustum;

// ============================================================================
// VECTOR2 - 2D Vector
// ============================================================================

class Vector2 {
public:
    double x, y;
    
    Vector2(double x = 0, double y = 0) : x(x), y(y) {}
    
    Vector2& set(double x, double y) {
        this->x = x; this->y = y;
        return *this;
    }
    
    Vector2 clone() const { return Vector2(x, y); }
    
    Vector2 add(const Vector2& v) const { return Vector2(x + v.x, y + v.y); }
    Vector2 sub(const Vector2& v) const { return Vector2(x - v.x, y - v.y); }
    Vector2 multiply(const Vector2& v) const { return Vector2(x * v.x, y * v.y); }
    Vector2 divide(const Vector2& v) const { return Vector2(x / v.x, y / v.y); }
    
    Vector2 addScalar(double s) const { return Vector2(x + s, y + s); }
    Vector2 multiplyScalar(double s) const { return Vector2(x * s, y * s); }
    
    double dot(const Vector2& v) const { return x * v.x + y * v.y; }
    double cross(const Vector2& v) const { return x * v.y - y * v.x; }
    
    double length() const { return std::sqrt(x * x + y * y); }
    double lengthSq() const { return x * x + y * y; }
    
    Vector2 normalize() const {
        double len = length();
        if (len > 0) return Vector2(x / len, y / len);
        return Vector2(0, 0);
    }
    
    double distanceTo(const Vector2& v) const {
        return std::sqrt(std::pow(x - v.x, 2) + std::pow(y - v.y, 2));
    }
    
    double angle() const { return std::atan2(y, x); }
    
    Vector2 rotateAround(const Vector2& center, double angle) const {
        double c = std::cos(angle), s = std::sin(angle);
        double dx = x - center.x, dy = y - center.y;
        return Vector2(dx * c - dy * s + center.x, dx * s + dy * c + center.y);
    }
    
    Vector2 lerp(const Vector2& v, double alpha) const {
        return Vector2(x + (v.x - x) * alpha, y + (v.y - y) * alpha);
    }
    
    bool equals(const Vector2& v) const {
        return std::abs(x - v.x) < 1e-10 && std::abs(y - v.y) < 1e-10;
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "Vector2(" << x << ", " << y << ")";
        return oss.str();
    }
    
    // Operators
    Vector2 operator+(const Vector2& v) const { return add(v); }
    Vector2 operator-(const Vector2& v) const { return sub(v); }
    Vector2 operator*(const Vector2& v) const { return multiply(v); }
    Vector2 operator/(const Vector2& v) const { return divide(v); }
    Vector2 operator*(double s) const { return multiplyScalar(s); }
    Vector2 operator/(double s) const { return Vector2(x / s, y / s); }
    Vector2& operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }
    Vector2& operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }
    Vector2& operator*=(double s) { x *= s; y *= s; return *this; }
    
    double& operator[](int index) {
        return index == 0 ? x : y;
    }
    
    const double& operator[](int index) const {
        return index == 0 ? x : y;
    }
    
    static Vector2 zero() { return Vector2(0, 0); }
    static Vector2 one() { return Vector2(1, 1); }
};

// ============================================================================
// VECTOR3 - 3D Vector (Core)
// ============================================================================

class Vector3 {
public:
    double x, y, z;
    
    Vector3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
    
    Vector3& set(double x, double y, double z) {
        this->x = x; this->y = y; this->z = z;
        return *this;
    }
    
    Vector3 clone() const { return Vector3(x, y, z); }
    
    Vector3 add(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    Vector3 sub(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    Vector3 multiply(const Vector3& v) const { return Vector3(x * v.x, y * v.y, z * v.z); }
    Vector3 divide(const Vector3& v) const { return Vector3(x / v.x, y / v.y, z / v.z); }
    
    Vector3 addScalar(double s) const { return Vector3(x + s, y + s, z + s); }
    Vector3 multiplyScalar(double s) const { return Vector3(x * s, y * s, z * s); }
    
    double dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
    
    Vector3 cross(const Vector3& v) const {
        return Vector3(y * v.z - z * v.y,
                      z * v.x - x * v.z,
                      x * v.y - y * v.x);
    }
    
    double length() const { return std::sqrt(x * x + y * y + z * z); }
    double lengthSq() const { return x * x + y * y + z * z; }
    double manhattanLength() const { return std::abs(x) + std::abs(y) + std::abs(z); }
    
    Vector3 normalize() const {
        double len = length();
        if (len > 0) return Vector3(x / len, y / len, z / len);
        return Vector3(0, 0, 0);
    }
    
    Vector3 negate() const { return Vector3(-x, -y, -z); }
    
    double distanceTo(const Vector3& v) const {
        return std::sqrt(std::pow(x - v.x, 2) + std::pow(y - v.y, 2) + std::pow(z - v.z, 2));
    }
    
    double distanceToSquared(const Vector3& v) const {
        return std::pow(x - v.x, 2) + std::pow(y - v.y, 2) + std::pow(z - v.z, 2);
    }
    
    Vector3 lerp(const Vector3& v, double alpha) const {
        return Vector3(x + (v.x - x) * alpha,
                      y + (v.y - y) * alpha,
                      z + (v.z - z) * alpha);
    }
    
    Vector3 projectOnVector(const Vector3& v) const {
        double denominator = v.lengthSq();
        if (denominator == 0) return Vector3(0, 0, 0);
        double scalar = dot(v) / denominator;
        return v.multiplyScalar(scalar);
    }
    
    Vector3 reflect(const Vector3& normal) const {
        return sub(normal.multiplyScalar(2 * dot(normal)));
    }
    
    double angleTo(const Vector3& v) const {
        double denominator = std::sqrt(lengthSq() * v.lengthSq());
        if (denominator == 0) return 0;
        double theta = dot(v) / denominator;
        return std::acos(std::clamp(theta, -1.0, 1.0));
    }
    
    bool equals(const Vector3& v) const {
        return std::abs(x - v.x) < 1e-10 && 
               std::abs(y - v.y) < 1e-10 && 
               std::abs(z - v.z) < 1e-10;
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "Vector3(" << x << ", " << y << ", " << z << ")";
        return oss.str();
    }
    
    // Operators
    Vector3 operator+(const Vector3& v) const { return add(v); }
    Vector3 operator-(const Vector3& v) const { return sub(v); }
    Vector3 operator*(const Vector3& v) const { return multiply(v); }
    Vector3 operator/(const Vector3& v) const { return divide(v); }
    Vector3 operator*(double s) const { return multiplyScalar(s); }
    Vector3 operator/(double s) const { return Vector3(x / s, y / s, z / s); }
    Vector3 operator-() const { return negate(); }
    Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    Vector3& operator*=(double s) { x *= s; y *= s; z *= s; return *this; }
    Vector3& operator/=(double s) { x /= s; y /= s; z /= s; return *this; }
    
    double& operator[](int index) {
        switch(index) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default: throw std::out_of_range("Vector3 index out of range");
        }
    }
    
    const double& operator[](int index) const {
        switch(index) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default: throw std::out_of_range("Vector3 index out of range");
        }
    }
    
    // Static utility methods
    static Vector3 zero() { return Vector3(0, 0, 0); }
    static Vector3 one() { return Vector3(1, 1, 1); }
    static Vector3 up() { return Vector3(0, 1, 0); }
    static Vector3 down() { return Vector3(0, -1, 0); }
    static Vector3 forward() { return Vector3(0, 0, -1); }
    static Vector3 backward() { return Vector3(0, 0, 1); }
    static Vector3 right() { return Vector3(1, 0, 0); }
    static Vector3 left() { return Vector3(-1, 0, 0); }
};

// ============================================================================
// VECTOR4 - 4D Vector
// ============================================================================

class Vector4 {
public:
    double x, y, z, w;
    
    Vector4(double x = 0, double y = 0, double z = 0, double w = 1) 
        : x(x), y(y), z(z), w(w) {}
    
    Vector4 clone() const { return Vector4(x, y, z, w); }
    
    Vector4 add(const Vector4& v) const { return Vector4(x + v.x, y + v.y, z + v.z, w + v.w); }
    Vector4 sub(const Vector4& v) const { return Vector4(x - v.x, y - v.y, z - v.z, w - v.w); }
    Vector4 multiplyScalar(double s) const { return Vector4(x * s, y * s, z * s, w * s); }
    
    double dot(const Vector4& v) const { return x * v.x + y * v.y + z * v.z + w * v.w; }
    double length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
    Vector4 normalize() const {
        double len = length();
        if (len > 0) return Vector4(x / len, y / len, z / len, w / len);
        return Vector4(0, 0, 0, 0);
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "Vector4(" << x << ", " << y << ", " << z << ", " << w << ")";
        return oss.str();
    }
    
    Vector4 operator+(const Vector4& v) const { return add(v); }
    Vector4 operator-(const Vector4& v) const { return sub(v); }
    Vector4 operator*(double s) const { return multiplyScalar(s); }
    
    double& operator[](int index) {
        switch(index) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            case 3: return w;
            default: throw std::out_of_range("Vector4 index out of range");
        }
    }
};

// ============================================================================
// MATRIX3 - 3x3 Matrix
// ============================================================================

class Matrix3 {
public:
    std::array<double, 9> elements;
    
    Matrix3() {
        elements.fill(0);
        elements[0] = elements[4] = elements[8] = 1; // Identity
    }
    
    Matrix3(const std::array<double, 9>& values) : elements(values) {}
    
    static Matrix3 identity() { return Matrix3(); }
    
    Matrix3 clone() const { return Matrix3(elements); }
    
    Matrix3 multiply(const Matrix3& m) const {
        Matrix3 result;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                result.elements[i * 3 + j] = 
                    elements[i * 3] * m.elements[j] +
                    elements[i * 3 + 1] * m.elements[j + 3] +
                    elements[i * 3 + 2] * m.elements[j + 6];
            }
        }
        return result;
    }
    
    Vector3 multiplyVector3(const Vector3& v) const {
        return Vector3(
            elements[0] * v.x + elements[1] * v.y + elements[2] * v.z,
            elements[3] * v.x + elements[4] * v.y + elements[5] * v.z,
            elements[6] * v.x + elements[7] * v.y + elements[8] * v.z
        );
    }
    
    double determinant() const {
        return elements[0] * (elements[4] * elements[8] - elements[5] * elements[7]) -
               elements[1] * (elements[3] * elements[8] - elements[5] * elements[6]) +
               elements[2] * (elements[3] * elements[7] - elements[4] * elements[6]);
    }
    
    Matrix3 transpose() const {
        return Matrix3({
            elements[0], elements[3], elements[6],
            elements[1], elements[4], elements[7],
            elements[2], elements[5], elements[8]
        });
    }
    
    double& operator()(int row, int col) {
        return elements[row * 3 + col];
    }
    
    const double& operator()(int row, int col) const {
        return elements[row * 3 + col];
    }
};

// ============================================================================
// MATRIX4 - 4x4 Matrix (Transformation Matrix)
// ============================================================================

class Matrix4 {
public:
    std::array<double, 16> elements;
    
    Matrix4() {
        elements.fill(0);
        elements[0] = elements[5] = elements[10] = elements[15] = 1; // Identity
    }
    
    Matrix4(const std::array<double, 16>& values) : elements(values) {}
    
    static Matrix4 identity() { return Matrix4(); }
    
    Matrix4 clone() const { return Matrix4(elements); }
    
    Matrix4 multiply(const Matrix4& m) const {
        Matrix4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.elements[i * 4 + j] = 
                    elements[i * 4] * m.elements[j] +
                    elements[i * 4 + 1] * m.elements[j + 4] +
                    elements[i * 4 + 2] * m.elements[j + 8] +
                    elements[i * 4 + 3] * m.elements[j + 12];
            }
        }
        return result;
    }
    
    Vector3 multiplyVector3(const Vector3& v) const {
        double x = elements[0] * v.x + elements[1] * v.y + elements[2] * v.z + elements[3];
        double y = elements[4] * v.x + elements[5] * v.y + elements[6] * v.z + elements[7];
        double z = elements[8] * v.x + elements[9] * v.y + elements[10] * v.z + elements[11];
        double w = elements[12] * v.x + elements[13] * v.y + elements[14] * v.z + elements[15];
        
        if (std::abs(w) > 1e-10) {
            return Vector3(x / w, y / w, z / w);
        }
        return Vector3(x, y, z);
    }
    
    Matrix4 transpose() const {
        return Matrix4({
            elements[0], elements[4], elements[8], elements[12],
            elements[1], elements[5], elements[9], elements[13],
            elements[2], elements[6], elements[10], elements[14],
            elements[3], elements[7], elements[11], elements[15]
        });
    }
    
    Matrix4 inverse() const {
        // Implementation of matrix inverse using cofactors
        double inv[16], det;
        double m[16];
        
        for (int i = 0; i < 16; ++i) m[i] = elements[i];
        
        inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] +
                 m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
        inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] -
                 m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
        inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] +
                 m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
        inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] -
                  m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
        inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] -
                 m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
        inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] +
                 m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
        inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] -
                 m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
        inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] +
                  m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
        inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] +
                 m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
        inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] -
                 m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
        inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] +
                  m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
        inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] -
                  m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
        inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] -
                 m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
        inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] +
                 m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
        inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] -
                  m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
        inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] +
                  m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];
        
        det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
        
        if (std::abs(det) < 1e-10) {
            return Matrix4::identity(); // Singular matrix
        }
        
        det = 1.0 / det;
        
        std::array<double, 16> invArray;
        for (int i = 0; i < 16; ++i) {
            invArray[i] = inv[i] * det;
        }
        
        return Matrix4(invArray);
    }
    
    // Static factory methods
    static Matrix4 makeTranslation(double x, double y, double z) {
        Matrix4 m;
        m.elements[3] = x;
        m.elements[7] = y;
        m.elements[11] = z;
        return m;
    }
    
    static Matrix4 makeRotationX(double angle) {
        double c = std::cos(angle), s = std::sin(angle);
        Matrix4 m;
        m.elements[5] = c;
        m.elements[6] = -s;
        m.elements[9] = s;
        m.elements[10] = c;
        return m;
    }
    
    static Matrix4 makeRotationY(double angle) {
        double c = std::cos(angle), s = std::sin(angle);
        Matrix4 m;
        m.elements[0] = c;
        m.elements[2] = s;
        m.elements[8] = -s;
        m.elements[10] = c;
        return m;
    }
    
    static Matrix4 makeRotationZ(double angle) {
        double c = std::cos(angle), s = std::sin(angle);
        Matrix4 m;
        m.elements[0] = c;
        m.elements[1] = -s;
        m.elements[4] = s;
        m.elements[5] = c;
        return m;
    }
    
    static Matrix4 makeScale(double x, double y, double z) {
        Matrix4 m;
        m.elements[0] = x;
        m.elements[5] = y;
        m.elements[10] = z;
        return m;
    }
    
    static Matrix4 makePerspective(double fov, double aspect, double near, double far) {
        double f = 1.0 / std::tan(fov / 2.0);
        double nf = 1.0 / (near - far);
        
        Matrix4 m;
        m.elements.fill(0);
        m.elements[0] = f / aspect;
        m.elements[5] = f;
        m.elements[10] = (far + near) * nf;
        m.elements[11] = -1;
        m.elements[14] = 2 * far * near * nf;
        return m;
    }
    
    static Matrix4 makeOrthographic(double left, double right, double top, 
                                    double bottom, double near, double far) {
        Matrix4 m;
        m.elements[0] = 2 / (right - left);
        m.elements[5] = 2 / (top - bottom);
        m.elements[10] = -2 / (far - near);
        m.elements[3] = -(right + left) / (right - left);
        m.elements[7] = -(top + bottom) / (top - bottom);
        m.elements[11] = -(far + near) / (far - near);
        return m;
    }
    
    double& operator()(int row, int col) {
        return elements[row * 4 + col];
    }
    
    const double& operator()(int row, int col) const {
        return elements[row * 4 + col];
    }
};

// ============================================================================
// QUATERNION - For rotation without gimbal lock
// ============================================================================

class Quaternion {
public:
    double x, y, z, w;
    
    Quaternion(double x = 0, double y = 0, double z = 0, double w = 1) 
        : x(x), y(y), z(z), w(w) {}
    
    Quaternion clone() const { return Quaternion(x, y, z, w); }
    
    Quaternion multiply(const Quaternion& q) const {
        return Quaternion(
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y + y * q.w + z * q.x - x * q.z,
            w * q.z + z * q.w + x * q.y - y * q.x,
            w * q.w - x * q.x - y * q.y - z * q.z
        );
    }
    
    Quaternion conjugate() const { return Quaternion(-x, -y, -z, w); }
    
    double length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
    
    Quaternion normalize() const {
        double len = length();
        if (len > 0) return Quaternion(x / len, y / len, z / len, w / len);
        return Quaternion(0, 0, 0, 1);
    }
    
    Quaternion inverse() const {
        return conjugate().normalize();
    }
    
    Vector3 rotateVector(const Vector3& v) const {
        Quaternion qv(0, v.x, v.y, v.z);
        Quaternion result = multiply(qv).multiply(inverse());
        return Vector3(result.x, result.y, result.z);
    }
    
    static Quaternion fromAxisAngle(const Vector3& axis, double angle) {
        double halfAngle = angle / 2.0;
        double s = std::sin(halfAngle);
        Vector3 normalizedAxis = axis.normalize();
        return Quaternion(
            normalizedAxis.x * s,
            normalizedAxis.y * s,
            normalizedAxis.z * s,
            std::cos(halfAngle)
        );
    }
    
    static Quaternion fromEuler(double x, double y, double z) {
        double c1 = std::cos(x / 2), c2 = std::cos(y / 2), c3 = std::cos(z / 2);
        double s1 = std::sin(x / 2), s2 = std::sin(y / 2), s3 = std::sin(z / 2);
        
        return Quaternion(
            s1 * c2 * c3 + c1 * s2 * s3,
            c1 * s2 * c3 - s1 * c2 * s3,
            c1 * c2 * s3 + s1 * s2 * c3,
            c1 * c2 * c3 - s1 * s2 * s3
        );
    }
    
    Quaternion slerp(const Quaternion& q, double t) const {
        double dot = x * q.x + y * q.y + z * q.z + w * q.w;
        
        Quaternion q2 = q;
        if (dot < 0) {
            q2 = Quaternion(-q.x, -q.y, -q.z, -q.w);
            dot = -dot;
        }
        
        if (dot > 0.9995) {
            Quaternion result(
                x + t * (q2.x - x),
                y + t * (q2.y - y),
                z + t * (q2.z - z),
                w + t * (q2.w - w)
            );
            return result.normalize();
        }
        
        double theta0 = std::acos(dot);
        double theta = theta0 * t;
        double sinTheta = std::sin(theta);
        double sinTheta0 = std::sin(theta0);
        
        double s0 = std::cos(theta) - dot * sinTheta / sinTheta0;
        double s1 = sinTheta / sinTheta0;
        
        return Quaternion(
            s0 * x + s1 * q2.x,
            s0 * y + s1 * q2.y,
            s0 * z + s1 * q2.z,
            s0 * w + s1 * q2.w
        );
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "Quaternion(" << x << ", " << y << ", " << z << ", " << w << ")";
        return oss.str();
    }
    
    Quaternion operator*(const Quaternion& q) const { return multiply(q); }
};

// ============================================================================
// EULER - Euler angles
// ============================================================================

class Euler {
public:
    enum class RotationOrder { XYZ, XZY, YXZ, YZX, ZXY, ZYX };
    
    double x, y, z; // Angles in radians
    RotationOrder order;
    
    Euler(double x = 0, double y = 0, double z = 0, 
          RotationOrder order = RotationOrder::XYZ)
        : x(x), y(y), z(z), order(order) {}
    
    void set(double x, double y, double z) {
        this->x = x; this->y = y; this->z = z;
    }
    
    Quaternion toQuaternion() const {
        return Quaternion::fromEuler(x, y, z);
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "Euler(" << x << ", " << y << ", " << z << ")";
        return oss.str();
    }
};

// ============================================================================
// COLOR - RGBA Color
// ============================================================================

class Color {
public:
    double r, g, b, a;
    
    Color(double r = 1, double g = 1, double b = 1, double a = 1) 
        : r(r), g(g), b(b), a(a) {}
    
    Color clone() const { return Color(r, g, b, a); }
    
    void set(double r, double g, double b, double a = 1) {
        this->r = r; this->g = g; this->b = b; this->a = a;
    }
    
    void setHex(int hex) {
        r = ((hex >> 16) & 0xff) / 255.0;
        g = ((hex >> 8) & 0xff) / 255.0;
        b = (hex & 0xff) / 255.0;
    }
    
    int getHex() const {
        return (static_cast<int>(r * 255) << 16) |
               (static_cast<int>(g * 255) << 8) |
               static_cast<int>(b * 255);
    }
    
    Color lerp(const Color& c, double alpha) const {
        return Color(
            r + (c.r - r) * alpha,
            g + (c.g - g) * alpha,
            b + (c.b - b) * alpha,
            a + (c.a - a) * alpha
        );
    }
    
    // Common colors
    static Color red() { return Color(1, 0, 0); }
    static Color green() { return Color(0, 1, 0); }
    static Color blue() { return Color(0, 0, 1); }
    static Color white() { return Color(1, 1, 1); }
    static Color black() { return Color(0, 0, 0); }
    static Color yellow() { return Color(1, 1, 0); }
    static Color cyan() { return Color(0, 1, 1); }
    static Color magenta() { return Color(1, 0, 1); }
    static Color gray() { return Color(0.5, 0.5, 0.5); }
};

// ============================================================================
// PART 2: CORE OBJECTS
// ============================================================================

// Object3D - Base class for all 3D objects
class Object3D {
public:
    std::string name;
    std::string type;
    
    Vector3 position;
    Quaternion quaternion;
    Euler rotation;
    Vector3 scale;
    
    Matrix4 matrix;
    Matrix4 matrixWorld;
    
    bool visible;
    bool castShadow;
    bool receiveShadow;
    
    Object3D* parent;
    std::vector<std::shared_ptr<Object3D>> children;
    
    Object3D() 
        : type("Object3D"),
          position(0, 0, 0),
          quaternion(0, 0, 0, 1),
          rotation(0, 0, 0),
          scale(1, 1, 1),
          matrix(Matrix4::identity()),
          matrixWorld(Matrix4::identity()),
          visible(true),
          castShadow(false),
          receiveShadow(false),
          parent(nullptr) {}
    
    virtual ~Object3D() = default;
    
    void add(std::shared_ptr<Object3D> object) {
        if (object->parent) {
            object->parent->remove(object);
        }
        object->parent = this;
        children.push_back(object);
    }
    
    void remove(std::shared_ptr<Object3D> object) {
        auto it = std::find(children.begin(), children.end(), object);
        if (it != children.end()) {
            (*it)->parent = nullptr;
            children.erase(it);
        }
    }
    
    void updateMatrix() {
        Matrix4 translationMatrix = Matrix4::makeTranslation(position.x, position.y, position.z);
        Matrix4 rotationMatrix = quaternionToMatrix(quaternion);
        Matrix4 scaleMatrix = Matrix4::makeScale(scale.x, scale.y, scale.z);
        
        matrix = translationMatrix * rotationMatrix * scaleMatrix;
    }
    
    void updateMatrixWorld(bool force = false) {
        updateMatrix();
        
        if (parent) {
            matrixWorld = parent->matrixWorld * matrix;
        } else {
            matrixWorld = matrix;
        }
        
        for (auto& child : children) {
            child->updateMatrixWorld(force);
        }
    }
    
    Vector3 getWorldPosition() const {
        return matrixWorld.multiplyVector3(Vector3(0, 0, 0));
    }
    
    Quaternion getWorldQuaternion() const {
        // Simplified: extract rotation from matrix
        return quaternion;
    }
    
    void lookAt(const Vector3& target) {
        Vector3 direction = target.sub(position).normalize();
        Vector3 up = Vector3(0, 1, 0);
        Vector3 right = up.cross(direction).normalize();
        Vector3 newUp = direction.cross(right);
        
        // Create rotation matrix from basis vectors
        Matrix4 rotMatrix;
        rotMatrix.elements[0] = right.x;
        rotMatrix.elements[1] = right.y;
        rotMatrix.elements[2] = right.z;
        rotMatrix.elements[4] = newUp.x;
        rotMatrix.elements[5] = newUp.y;
        rotMatrix.elements[6] = newUp.z;
        rotMatrix.elements[8] = -direction.x;
        rotMatrix.elements[9] = -direction.y;
        rotMatrix.elements[10] = -direction.z;
        
        quaternion = matrixToQuaternion(rotMatrix);
    }
    
    void translateX(double distance) {
        position.x += distance;
    }
    
    void translateY(double distance) {
        position.y += distance;
    }
    
    void translateZ(double distance) {
        position.z += distance;
    }
    
    void rotateX(double angle) {
        rotation.x += angle;
        quaternion = rotation.toQuaternion();
    }
    
    void rotateY(double angle) {
        rotation.y += angle;
        quaternion = rotation.toQuaternion();
    }
    
    void rotateZ(double angle) {
        rotation.z += angle;
        quaternion = rotation.toQuaternion();
    }
    
    virtual void dispose() {}
    
private:
    static Matrix4 quaternionToMatrix(const Quaternion& q) {
        Matrix4 m;
        double x = q.x, y = q.y, z = q.z, w = q.w;
        
        m.elements[0] = 1 - 2*(y*y + z*z);
        m.elements[1] = 2*(x*y - w*z);
        m.elements[2] = 2*(x*z + w*y);
        m.elements[4] = 2*(x*y + w*z);
        m.elements[5] = 1 - 2*(x*x + z*z);
        m.elements[6] = 2*(y*z - w*x);
        m.elements[8] = 2*(x*z - w*y);
        m.elements[9] = 2*(y*z + w*x);
        m.elements[10] = 1 - 2*(x*x + y*y);
        
        return m;
    }
    
    static Quaternion matrixToQuaternion(const Matrix4& m) {
        double trace = m.elements[0] + m.elements[5] + m.elements[10];
        Quaternion q;
        
        if (trace > 0) {
            double s = std::sqrt(trace + 1.0) * 2;
            q.w = 0.25 * s;
            q.x = (m.elements[6] - m.elements[9]) / s;
            q.y = (m.elements[8] - m.elements[2]) / s;
            q.z = (m.elements[1] - m.elements[4]) / s;
        } else if (m.elements[0] > m.elements[5] && m.elements[0] > m.elements[10]) {
            double s = std::sqrt(1.0 + m.elements[0] - m.elements[5] - m.elements[10]) * 2;
            q.w = (m.elements[6] - m.elements[9]) / s;
            q.x = 0.25 * s;
            q.y = (m.elements[1] + m.elements[4]) / s;
            q.z = (m.elements[2] + m.elements[8]) / s;
        } else if (m.elements[5] > m.elements[10]) {
            double s = std::sqrt(1.0 + m.elements[5] - m.elements[0] - m.elements[10]) * 2;
            q.w = (m.elements[8] - m.elements[2]) / s;
            q.x = (m.elements[1] + m.elements[4]) / s;
            q.y = 0.25 * s;
            q.z = (m.elements[6] + m.elements[9]) / s;
        } else {
            double s = std::sqrt(1.0 + m.elements[10] - m.elements[0] - m.elements[5]) * 2;
            q.w = (m.elements[1] - m.elements[4]) / s;
            q.x = (m.elements[2] + m.elements[8]) / s;
            q.y = (m.elements[6] + m.elements[9]) / s;
            q.z = 0.25 * s;
        }
        
        return q.normalize();
    }
};

// Group - Container for multiple objects
class Group : public Object3D {
public:
    Group() {
        type = "Group";
    }
};

// Scene - Root object for all 3D content
class Scene : public Object3D {
public:
    Color background;
    Color fog;
    
    Scene() {
        type = "Scene";
        background = Color(0, 0, 0);
        fog = Color(1, 1, 1);
    }
};

// ============================================================================
// PART 3: CAMERAS
// ============================================================================

class Camera : public Object3D {
public:
    Matrix4 projectionMatrix;
    Matrix4 viewMatrix;
    
    Camera() {
        type = "Camera";
        projectionMatrix = Matrix4::identity();
        viewMatrix = Matrix4::identity();
    }
    
    virtual void updateProjectionMatrix() = 0;
    
    Vector3 getWorldDirection() const {
        Quaternion q = getWorldQuaternion();
        return q.rotateVector(Vector3(0, 0, -1));
    }
};

class PerspectiveCamera : public Camera {
public:
    double fov;
    double aspect;
    double near;
    double far;
    
    PerspectiveCamera(double fov = 75, double aspect = 16.0/9.0, 
                     double near = 0.1, double far = 1000)
        : fov(fov), aspect(aspect), near(near), far(far) {
        type = "PerspectiveCamera";
        updateProjectionMatrix();
    }
    
    void updateProjectionMatrix() override {
        projectionMatrix = Matrix4::makePerspective(
            fov * M_PI / 180.0, aspect, near, far);
    }
    
    void setFocalLength(double focalLength) {
        fov = 2 * std::atan(35.0 / (2 * focalLength)) * 180.0 / M_PI;
        updateProjectionMatrix();
    }
};

class OrthographicCamera : public Camera {
public:
    double left, right, top, bottom, near, far;
    
    OrthographicCamera(double left = -1, double right = 1,
                      double top = 1, double bottom = -1,
                      double near = 0.1, double far = 1000)
        : left(left), right(right), top(top), bottom(bottom),
          near(near), far(far) {
        type = "OrthographicCamera";
        updateProjectionMatrix();
    }
    
    void updateProjectionMatrix() override {
        projectionMatrix = Matrix4::makeOrthographic(
            left, right, top, bottom, near, far);
    }
};

// ============================================================================
// PART 4: GEOMETRY
// ============================================================================

class Geometry {
public:
    std::string type;
    std::vector<Vector3> vertices;
    std::vector<Vector3> normals;
    std::vector<Vector2> uvs;
    std::vector<int> indices;
    
    Geometry() : type("Geometry") {}
    virtual ~Geometry() = default;
    
    void computeVertexNormals() {
        normals.clear();
        normals.resize(vertices.size(), Vector3(0, 0, 0));
        
        for (size_t i = 0; i < indices.size(); i += 3) {
            Vector3 v0 = vertices[indices[i]];
            Vector3 v1 = vertices[indices[i + 1]];
            Vector3 v2 = vertices[indices[i + 2]];
            
            Vector3 normal = (v1 - v0).cross(v2 - v0).normalize();
            
            normals[indices[i]] += normal;
            normals[indices[i + 1]] += normal;
            normals[indices[i + 2]] += normal;
        }
        
        for (auto& normal : normals) {
            normal = normal.normalize();
        }
    }
    
    void dispose() {
        vertices.clear();
        normals.clear();
        uvs.clear();
        indices.clear();
    }
};

// BoxGeometry
class BoxGeometry : public Geometry {
public:
    BoxGeometry(double width = 1, double height = 1, double depth = 1) {
        type = "BoxGeometry";
        
        double w = width / 2, h = height / 2, d = depth / 2;
        
        // 8 vertices
        vertices = {
            Vector3(-w, -h, -d), Vector3(w, -h, -d), Vector3(w, h, -d), Vector3(-w, h, -d), // Back face
            Vector3(-w, -h, d), Vector3(w, -h, d), Vector3(w, h, d), Vector3(-w, h, d)      // Front face
        };
        
        // 12 triangles (6 faces)
        indices = {
            0, 1, 2, 0, 2, 3,  // Back face
            4, 6, 5, 4, 7, 6,  // Front face
            0, 4, 5, 0, 5, 1,  // Bottom face
            3, 2, 6, 3, 6, 7,  // Top face
            0, 3, 7, 0, 7, 4,  // Left face
            1, 5, 6, 1, 6, 2   // Right face
        };
        
        computeVertexNormals();
    }
};

// SphereGeometry
class SphereGeometry : public Geometry {
public:
    SphereGeometry(double radius = 1, int widthSegments = 32, int heightSegments = 16) {
        type = "SphereGeometry";
        
        for (int y = 0; y <= heightSegments; ++y) {
            double v = static_cast<double>(y) / heightSegments;
            double phi = v * M_PI;
            
            for (int x = 0; x <= widthSegments; ++x) {
                double u = static_cast<double>(x) / widthSegments;
                double theta = u * 2 * M_PI;
                
                double px = -radius * std::cos(theta) * std::sin(phi);
                double py = radius * std::cos(phi);
                double pz = radius * std::sin(theta) * std::sin(phi);
                
                vertices.push_back(Vector3(px, py, pz));
                normals.push_back(Vector3(px / radius, py / radius, pz / radius));
                uvs.push_back(Vector2(u, v));
            }
        }
        
        for (int y = 0; y < heightSegments; ++y) {
            for (int x = 0; x < widthSegments; ++x) {
                int a = y * (widthSegments + 1) + x;
                int b = a + widthSegments + 1;
                
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(a + 1);
                
                indices.push_back(b);
                indices.push_back(b + 1);
                indices.push_back(a + 1);
            }
        }
    }
};

// PlaneGeometry
class PlaneGeometry : public Geometry {
public:
    PlaneGeometry(double width = 1, double height = 1, 
                  int widthSegments = 1, int heightSegments = 1) {
        type = "PlaneGeometry";
        
        double w = width / 2, h = height / 2;
        
        for (int y = 0; y <= heightSegments; ++y) {
            for (int x = 0; x <= widthSegments; ++x) {
                double u = static_cast<double>(x) / widthSegments;
                double v = static_cast<double>(y) / heightSegments;
                
                vertices.push_back(Vector3(
                    -w + u * width,
                    0,
                    -h + v * height
                ));
                normals.push_back(Vector3(0, 1, 0));
                uvs.push_back(Vector2(u, v));
            }
        }
        
        for (int y = 0; y < heightSegments; ++y) {
            for (int x = 0; x < widthSegments; ++x) {
                int a = y * (widthSegments + 1) + x;
                int b = a + widthSegments + 1;
                
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(a + 1);
                
                indices.push_back(b);
                indices.push_back(b + 1);
                indices.push_back(a + 1);
            }
        }
    }
};

// CylinderGeometry
class CylinderGeometry : public Geometry {
public:
    CylinderGeometry(double radiusTop = 1, double radiusBottom = 1, 
                     double height = 1, int radialSegments = 32) {
        type = "CylinderGeometry";
        
        double halfHeight = height / 2;
        
        for (int x = 0; x <= radialSegments; ++x) {
            double u = static_cast<double>(x) / radialSegments;
            double theta = u * 2 * M_PI;
            double cosTheta = std::cos(theta);
            double sinTheta = std::sin(theta);
            
            // Bottom vertex
            vertices.push_back(Vector3(
                radiusBottom * sinTheta,
                -halfHeight,
                radiusBottom * cosTheta
            ));
            normals.push_back(Vector3(sinTheta, 0, cosTheta));
            uvs.push_back(Vector2(u, 0));
            
            // Top vertex
            vertices.push_back(Vector3(
                radiusTop * sinTheta,
                halfHeight,
                radiusTop * cosTheta
            ));
            normals.push_back(Vector3(sinTheta, 0, cosTheta));
            uvs.push_back(Vector2(u, 1));
        }
        
        for (int x = 0; x < radialSegments; ++x) {
            int a = x * 2;
            int b = a + 1;
            int c = (x + 1) * 2;
            int d = c + 1;
            
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(c);
            
            indices.push_back(b);
            indices.push_back(d);
            indices.push_back(c);
        }
    }
};

// ============================================================================
// PART 5: MATERIALS
// ============================================================================

class Material {
public:
    std::string type;
    bool transparent;
    double opacity;
    Color color;
    bool wireframe;
    bool visible;
    
    Material() 
        : type("Material"),
          transparent(false),
          opacity(1),
          color(1, 1, 1),
          wireframe(false),
          visible(true) {}
    
    virtual ~Material() = default;
    
    virtual void dispose() {}
};

class MeshBasicMaterial : public Material {
public:
    MeshBasicMaterial() {
        type = "MeshBasicMaterial";
    }
};

class MeshPhongMaterial : public Material {
public:
    double shininess;
    Color specular;
    Color emissive;
    
    MeshPhongMaterial() 
        : shininess(30),
          specular(0.1, 0.1, 0.1),
          emissive(0, 0, 0) {
        type = "MeshPhongMaterial";
    }
};

class MeshStandardMaterial : public Material {
public:
    double roughness;
    double metalness;
    Color emissive;
    
    MeshStandardMaterial() 
        : roughness(0.5),
          metalness(0.5),
          emissive(0, 0, 0) {
        type = "MeshStandardMaterial";
    }
};

// ============================================================================
// PART 6: MESH
// ============================================================================

class Mesh : public Object3D {
public:
    std::shared_ptr<Geometry> geometry;
    std::shared_ptr<Material> material;
    
    Mesh(std::shared_ptr<Geometry> geometry = nullptr,
         std::shared_ptr<Material> material = nullptr)
        : geometry(geometry), material(material) {
        type = "Mesh";
    }
    
    void dispose() override {
        if (geometry) geometry->dispose();
        if (material) material->dispose();
    }
};

// ============================================================================
// PART 7: LIGHTS
// ============================================================================

class Light : public Object3D {
public:
    Color color;
    double intensity;
    
    Light(Color color = Color(1, 1, 1), double intensity = 1)
        : color(color), intensity(intensity) {
        type = "Light";
    }
};

class AmbientLight : public Light {
public:
    AmbientLight(Color color = Color(1, 1, 1), double intensity = 1)
        : Light(color, intensity) {
        type = "AmbientLight";
    }
};

class DirectionalLight : public Light {
public:
    Vector3 target;
    
    DirectionalLight(Color color = Color(1, 1, 1), double intensity = 1)
        : Light(color, intensity), target(0, 0, 0) {
        type = "DirectionalLight";
    }
};

class PointLight : public Light {
public:
    double distance;
    double decay;
    
    PointLight(Color color = Color(1, 1, 1), double intensity = 1,
               double distance = 0, double decay = 1)
        : Light(color, intensity), distance(distance), decay(decay) {
        type = "PointLight";
    }
};

// ============================================================================
// PART 8: RAYCASTER
// ============================================================================

class Raycaster {
public:
    Vector3 origin;
    Vector3 direction;
    double near;
    double far;
    
    Raycaster(Vector3 origin = Vector3(0, 0, 0),
              Vector3 direction = Vector3(0, 0, -1),
              double near = 0, double far = std::numeric_limits<double>::infinity())
        : origin(origin), direction(direction.normalize()), near(near), far(far) {}
    
    void set(Vector3 origin, Vector3 direction) {
        this->origin = origin;
        this->direction = direction.normalize();
    }
    
    struct Intersection {
        double distance;
        Vector3 point;
        Vector3 normal;
        std::shared_ptr<Object3D> object;
        int faceIndex;
    };
    
    std::vector<Intersection> intersectObject(std::shared_ptr<Object3D> object) {
        std::vector<Intersection> intersections;
        
        if (auto mesh = std::dynamic_pointer_cast<Mesh>(object)) {
            intersectMesh(mesh, intersections);
        }
        
        for (auto& child : object->children) {
            auto childIntersections = intersectObject(child);
            intersections.insert(intersections.end(), 
                               childIntersections.begin(), 
                               childIntersections.end());
        }
        
        return intersections;
    }
    
private:
    void intersectMesh(std::shared_ptr<Mesh> mesh, 
                      std::vector<Intersection>& intersections) {
        if (!mesh->geometry) return;
        
        auto& vertices = mesh->geometry->vertices;
        auto& indices = mesh->geometry->indices;
        
        for (size_t i = 0; i < indices.size(); i += 3) {
            Vector3 v0 = vertices[indices[i]];
            Vector3 v1 = vertices[indices[i + 1]];
            Vector3 v2 = vertices[indices[i + 2]];
            
            // Apply mesh transform
            v0 = mesh->matrixWorld.multiplyVector3(v0);
            v1 = mesh->matrixWorld.multiplyVector3(v1);
            v2 = mesh->matrixWorld.multiplyVector3(v2);
            
            // Ray-triangle intersection (Möller–Trumbore algorithm)
            Vector3 edge1 = v1 - v0;
            Vector3 edge2 = v2 - v0;
            Vector3 h = direction.cross(edge2);
            double a = edge1.dot(h);
            
            if (std::abs(a) < 1e-10) continue;
            
            double f = 1.0 / a;
            Vector3 s = origin - v0;
            double u = f * s.dot(h);
            
            if (u < 0 || u > 1) continue;
            
            Vector3 q = s.cross(edge1);
            double v = f * direction.dot(q);
            
            if (v < 0 || u + v > 1) continue;
            
            double t = f * edge2.dot(q);
            
            if (t > near && t < far) {
                Intersection intersection;
                intersection.distance = t;
                intersection.point = origin + direction * t;
                intersection.normal = edge1.cross(edge2).normalize();
                intersection.object = mesh;
                intersection.faceIndex = i / 3;
                intersections.push_back(intersection);
            }
        }
    }
};

// ============================================================================
// PART 9: RENDERER
// ============================================================================

class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void render(Scene& scene, Camera& camera) = 0;
    virtual void setSize(int width, int height) = 0;
    virtual void clear() = 0;
};

// Software renderer (for demonstration)
class SoftwareRenderer : public Renderer {
private:
    int width, height;
    Color clearColor;
    std::vector<std::vector<Color>> framebuffer;
    std::vector<std::vector<double>> depthBuffer;
    
public:
    SoftwareRenderer(int width = 800, int height = 600)
        : width(width), height(height), clearColor(0, 0, 0) {
        resize(width, height);
    }
    
    void setSize(int w, int h) override {
        width = w;
        height = h;
        resize(w, h);
    }
    
    void setClearColor(const Color& color) {
        clearColor = color;
    }
    
    void clear() override {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                framebuffer[y][x] = clearColor;
                depthBuffer[y][x] = std::numeric_limits<double>::max();
            }
        }
    }
    
    void render(Scene& scene, Camera& camera) override {
        clear();
        scene.updateMatrixWorld();
        camera.updateMatrixWorld();
        
        renderObject(scene, scene, camera);
    }
    
    void saveToPPM(const std::string& filename) {
        std::ofstream file(filename);
        file << "P3\n" << width << " " << height << "\n255\n";
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Color& c = framebuffer[y][x];
                file << static_cast<int>(c.r * 255) << " "
                     << static_cast<int>(c.g * 255) << " "
                     << static_cast<int>(c.b * 255) << " ";
            }
            file << "\n";
        }
        file.close();
    }
    
private:
    void resize(int w, int h) {
        framebuffer.resize(h, std::vector<Color>(w));
        depthBuffer.resize(h, std::vector<double>(w));
    }
    
    void renderObject(Object3D& object, Scene& scene, Camera& camera) {
        if (!object.visible) return;
        
        if (auto mesh = dynamic_cast<Mesh*>(&object)) {
            renderMesh(*mesh, camera);
        }
        
        for (auto& child : object.children) {
            renderObject(*child, scene, camera);
        }
    }
    
    void renderMesh(Mesh& mesh, Camera& camera) {
        if (!mesh.geometry || !mesh.material) return;
        if (!mesh.material->visible) return;
        
        auto& vertices = mesh.geometry->vertices;
        auto& indices = mesh.geometry->indices;
        
        Matrix4 mvp = camera.projectionMatrix * camera.matrixWorld.inverse() * mesh.matrixWorld;
        
        // Simple wireframe rendering
        if (mesh.material->wireframe) {
            for (size_t i = 0; i < indices.size(); i += 3) {
                Vector3 v0 = project(vertices[indices[i]], mvp);
                Vector3 v1 = project(vertices[indices[i + 1]], mvp);
                Vector3 v2 = project(vertices[indices[i + 2]], mvp);
                
                drawLine(v0, v1, mesh.material->color);
                drawLine(v1, v2, mesh.material->color);
                drawLine(v2, v0, mesh.material->color);
            }
        } else {
            // Simple face rendering
            for (size_t i = 0; i < indices.size(); i += 3) {
                Vector3 v0 = project(vertices[indices[i]], mvp);
                Vector3 v1 = project(vertices[indices[i + 1]], mvp);
                Vector3 v2 = project(vertices[indices[i + 2]], mvp);
                
                fillTriangle(v0, v1, v2, mesh.material->color);
            }
        }
    }
    
    Vector3 project(const Vector3& vertex, const Matrix4& mvp) {
        Vector4 clip(
            mvp.elements[0] * vertex.x + mvp.elements[1] * vertex.y + 
            mvp.elements[2] * vertex.z + mvp.elements[3],
            mvp.elements[4] * vertex.x + mvp.elements[5] * vertex.y + 
            mvp.elements[6] * vertex.z + mvp.elements[7],
            mvp.elements[8] * vertex.x + mvp.elements[9] * vertex.y + 
            mvp.elements[10] * vertex.z + mvp.elements[11],
            mvp.elements[12] * vertex.x + mvp.elements[13] * vertex.y + 
            mvp.elements[14] * vertex.z + mvp.elements[15]
        );
        
        if (std::abs(clip.w) > 1e-10) {
            clip.x /= clip.w;
            clip.y /= clip.w;
            clip.z /= clip.w;
        }
        
        return Vector3(
            (clip.x + 1) * width / 2,
            (1 - clip.y) * height / 2,
            clip.z
        );
    }
    
    void drawLine(const Vector3& start, const Vector3& end, const Color& color) {
        int x0 = static_cast<int>(start.x);
        int y0 = static_cast<int>(start.y);
        int x1 = static_cast<int>(end.x);
        int y1 = static_cast<int>(end.y);
        
        // Bresenham's line algorithm
        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx - dy;
        
        while (true) {
            if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
                double depth = start.z + (end.z - start.z) * 
                    (std::sqrt(std::pow(x0 - start.x, 2) + std::pow(y0 - start.y, 2)) /
                     std::sqrt(std::pow(end.x - start.x, 2) + std::pow(end.y - start.y, 2)));
                
                if (depth < depthBuffer[y0][x0]) {
                    framebuffer[y0][x0] = color;
                    depthBuffer[y0][x0] = depth;
                }
            }
            
            if (x0 == x1 && y0 == y1) break;
            
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y0 += sy;
            }
        }
    }
    
    void fillTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2,
                     const Color& color) {
        // Simple bounding box fill
        int minX = std::max(0, static_cast<int>(std::min({v0.x, v1.x, v2.x})));
        int maxX = std::min(width - 1, static_cast<int>(std::max({v0.x, v1.x, v2.x})));
        int minY = std::max(0, static_cast<int>(std::min({v0.y, v1.y, v2.y})));
        int maxY = std::min(height - 1, static_cast<int>(std::max({v0.y, v1.y, v2.y})));
        
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                Vector3 p(x + 0.5, y + 0.5, 0);
                
                if (pointInTriangle(p, v0, v1, v2)) {
                    // Simple depth calculation
                    double depth = (v0.z + v1.z + v2.z) / 3;
                    
                    if (depth < depthBuffer[y][x]) {
                        framebuffer[y][x] = color;
                        depthBuffer[y][x] = depth;
                    }
                }
            }
        }
    }
    
    bool pointInTriangle(const Vector3& p, const Vector3& a, 
                        const Vector3& b, const Vector3& c) {
        double d1 = sign(p, a, b);
        double d2 = sign(p, b, c);
        double d3 = sign(p, c, a);
        
        bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
        bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);
        
        return !(hasNeg && hasPos);
    }
    
    double sign(const Vector3& p1, const Vector3& p2, const Vector3& p3) {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    }
};

} // namespace three

#endif // THREE_CPP_H

// ============================================================================
// PART 10: EXAMPLE USAGE
// ============================================================================

#ifdef THREE_CPP_EXAMPLE

#include "three.cpp"

using namespace three;

int main() {
    std::cout << "=== three.cpp - 3D Graphics Library ===\n\n";
    
    // Create scene
    Scene scene;
    scene.background = Color(0.1, 0.1, 0.2);
    
    // Create camera
    PerspectiveCamera camera(75, 800.0/600.0, 0.1, 1000);
    camera.position = Vector3(5, 5, 5);
    camera.lookAt(Vector3(0, 0, 0));
    
    // Create a cube
    auto cubeGeometry = std::make_shared<BoxGeometry>(2, 2, 2);
    auto cubeMaterial = std::make_shared<MeshPhongMaterial>();
    cubeMaterial->color = Color::red();
    cubeMaterial->wireframe = true;
    
    auto cube = std::make_shared<Mesh>(cubeGeometry, cubeMaterial);
    cube->position = Vector3(0, 0, 0);
    scene.add(cube);
    
    // Create a sphere
    auto sphereGeometry = std::make_shared<SphereGeometry>(1.5, 32, 16);
    auto sphereMaterial = std::make_shared<MeshPhongMaterial>();
    sphereMaterial->color = Color::blue();
    sphereMaterial->wireframe = true;
    
    auto sphere = std::make_shared<Mesh>(sphereGeometry, sphereMaterial);
    sphere->position = Vector3(3, 0, 0);
    scene.add(sphere);
    
    // Create a plane
    auto planeGeometry = std::make_shared<PlaneGeometry>(10, 10);
    auto planeMaterial = std::make_shared<MeshBasicMaterial>();
    planeMaterial->color = Color::gray();
    planeMaterial->wireframe = true;
    
    auto plane = std::make_shared<Mesh>(planeGeometry, planeMaterial);
    plane->position = Vector3(0, -2, 0);
    plane->rotation.x = -M_PI / 2;
    scene.add(plane);
    
    // Create lights
    auto ambientLight = std::make_shared<AmbientLight>(Color(0.5, 0.5, 0.5));
    scene.add(ambientLight);
    
    auto directionalLight = std::make_shared<DirectionalLight>(Color(1, 1, 1), 1);
    directionalLight->position = Vector3(5, 10, 5);
    directionalLight->lookAt(Vector3(0, 0, 0));
    scene.add(directionalLight);
    
    // Create renderer
    SoftwareRenderer renderer(800, 600);
    renderer.setClearColor(scene.background);
    
    // Render scene
    renderer.render(scene, camera);
    renderer.saveToPPM("scene.ppm");
    
    std::cout << "Scene rendered to scene.ppm\n";
    std::cout << "Objects in scene:\n";
    std::cout << "  - Red wireframe cube (2x2x2)\n";
    std::cout << "  - Blue wireframe sphere (radius 1.5)\n";
    std::cout << "  - Gray wireframe plane (10x10)\n";
    std::cout << "  - Ambient light\n";
    std::cout << "  - Directional light\n";
    
    // Test raycaster
    Raycaster raycaster(
        Vector3(0, 0, 10),
        Vector3(0, 0, -1)
    );
    
    auto intersections = raycaster.intersectObject(
        std::dynamic_pointer_cast<Object3D>(cube));
    
    std::cout << "\nRaycaster found " << intersections.size() 
              << " intersections with cube\n";
    
    if (!intersections.empty()) {
        std::cout << "First intersection at distance: " 
                  << intersections[0].distance << "\n";
        std::cout << "Point: " << intersections[0].point.toString() << "\n";
    }
    
    return 0;
}

#endif