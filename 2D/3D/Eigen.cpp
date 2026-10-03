#include <iostream>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <complex>
#include <functional>
#include <algorithm>
#include <random>
#include <optional>
#include <tuple>
#include <queue>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <limits>
#include <numeric>
#define M_PI 3.14
// ============================================================================
// PART 0: CUSTOM MATHEMATICAL STRUCTURES (Replacing Eigen)
// ============================================================================

// 3D Vector class
template<typename T = double>
class Vector3 {
public:
    T x, y, z;
    
    Vector3(T x = 0, T y = 0, T z = 0) : x(x), y(y), z(z) {}
    
    // Operator overloads
    Vector3 operator+(const Vector3& v) const {
        return Vector3(x + v.x, y + v.y, z + v.z);
    }
    
    Vector3 operator-(const Vector3& v) const {
        return Vector3(x - v.x, y - v.y, z - v.z);
    }
    
    Vector3 operator*(T scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }
    
    Vector3 operator/(T scalar) const {
        return Vector3(x / scalar, y / scalar, z / scalar);
    }
    
    Vector3& operator+=(const Vector3& v) {
        x += v.x; y += v.y; z += v.z;
        return *this;
    }
    
    Vector3& operator-=(const Vector3& v) {
        x -= v.x; y -= v.y; z -= v.z;
        return *this;
    }
    
    Vector3& operator*=(T scalar) {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }
    
    // Dot product
    T dot(const Vector3& v) const {
        return x * v.x + y * v.y + z * v.z;
    }
    
    // Cross product
    Vector3 cross(const Vector3& v) const {
        return Vector3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }
    
    // Magnitude
    T magnitude() const {
        return std::sqrt(x * x + y * y + z * z);
    }
    
    T norm() const {
        return magnitude();
    }
    
    // Normalized vector
    Vector3 normalized() const {
        T mag = magnitude();
        if (mag > 0) {
            return Vector3(x / mag, y / mag, z / mag);
        }
        return Vector3(0, 0, 0);
    }
    
    // Component-wise absolute value
    Vector3 cwiseAbs() const {
        return Vector3(std::abs(x), std::abs(y), std::abs(z));
    }
    
    // Component-wise max
    static Vector3 max(const Vector3& a, const Vector3& b) {
        return Vector3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
    }
    
    // Component-wise min
    static Vector3 min(const Vector3& a, const Vector3& b) {
        return Vector3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
    }
    
    // Sum of components
    T sum() const {
        return x + y + z;
    }
    
    // Convert to Vector3<int>
    Vector3<int> castInt() const {
        return Vector3<int>(static_cast<int>(x), static_cast<int>(y), static_cast<int>(z));
    }
    
    // Convert to Vector3<double>
    Vector3<double> castDouble() const {
        return Vector3<double>(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z));
    }
    
    // Access operator
    T& operator[](int index) {
        if (index == 0) return x;
        if (index == 1) return y;
        return z;
    }
    
    const T& operator[](int index) const {
        if (index == 0) return x;
        if (index == 1) return y;
        return z;
    }
    
    // String representation
    std::string toString() const {
        std::ostringstream oss;
        oss << "(" << x << ", " << y << ", " << z << ")";
        return oss.str();
    }
    
    // Static factory methods
    static Vector3 Zero() { return Vector3(0, 0, 0); }
    static Vector3 One() { return Vector3(1, 1, 1); }
};

// Integer vector typedef
using Vector3i = Vector3<int>;
// Double vector typedef
using Vector3d = Vector3<double>;

// Matrix class (simplified for our needs)
template<typename T = double>
class Matrix {
private:
    std::vector<T> data;
    int rows, cols;
    
public:
    Matrix(int rows = 0, int cols = 0) : rows(rows), cols(cols) {
        data.resize(rows * cols, T(0));
    }
    
    Matrix(int rows, int cols, const std::vector<T>& values) : rows(rows), cols(cols) {
        data = values;
        if (data.size() < rows * cols) {
            data.resize(rows * cols, T(0));
        }
    }
    
    T& operator()(int row, int col) {
        return data[row * cols + col];
    }
    
    const T& operator()(int row, int col) const {
        return data[row * cols + col];
    }
    
    int getRows() const { return rows; }
    int getCols() const { return cols; }
    
    // Matrix multiplication
    Matrix operator*(const Matrix& other) const {
        if (cols != other.rows) {
            throw std::runtime_error("Matrix dimensions mismatch for multiplication");
        }
        
        Matrix result(rows, other.cols);
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < other.cols; ++j) {
                T sum = 0;
                for (int k = 0; k < cols; ++k) {
                    sum += (*this)(i, k) * other(k, j);
                }
                result(i, j) = sum;
            }
        }
        return result;
    }
    
    // Matrix-vector multiplication
    Vector3<T> operator*(const Vector3<T>& vec) const {
        if (rows != 3 || cols != 3) {
            throw std::runtime_error("Matrix must be 3x3 for vector multiplication");
        }
        
        return Vector3<T>(
            (*this)(0, 0) * vec.x + (*this)(0, 1) * vec.y + (*this)(0, 2) * vec.z,
            (*this)(1, 0) * vec.x + (*this)(1, 1) * vec.y + (*this)(1, 2) * vec.z,
            (*this)(2, 0) * vec.x + (*this)(2, 1) * vec.y + (*this)(2, 2) * vec.z
        );
    }
    
    // Matrix transpose
    Matrix transpose() const {
        Matrix result(cols, rows);
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                result(j, i) = (*this)(i, j);
            }
        }
        return result;
    }
    
    // Identity matrix
    static Matrix identity(int size) {
        Matrix result(size, size);
        for (int i = 0; i < size; ++i) {
            result(i, i) = 1;
        }
        return result;
    }
    
    // Matrix inverse (Gauss-Jordan elimination for small matrices)
    Matrix inverse() const {
        if (rows != cols) {
            throw std::runtime_error("Matrix must be square for inverse");
        }
        
        int n = rows;
        Matrix augmented(n, 2 * n);
        
        // Create augmented matrix [A | I]
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                augmented(i, j) = (*this)(i, j);
            }
            augmented(i, i + n) = 1;
        }
        
        // Gauss-Jordan elimination
        for (int i = 0; i < n; ++i) {
            // Find pivot
            int maxRow = i;
            for (int j = i + 1; j < n; ++j) {
                if (std::abs(augmented(j, i)) > std::abs(augmented(maxRow, i))) {
                    maxRow = j;
                }
            }
            
            // Swap rows
            for (int j = 0; j < 2 * n; ++j) {
                std::swap(augmented(i, j), augmented(maxRow, j));
            }
            
            // Make pivot 1
            T pivot = augmented(i, i);
            if (std::abs(pivot) < 1e-10) {
                throw std::runtime_error("Matrix is singular");
            }
            
            for (int j = 0; j < 2 * n; ++j) {
                augmented(i, j) /= pivot;
            }
            
            // Eliminate other rows
            for (int j = 0; j < n; ++j) {
                if (j != i) {
                    T factor = augmented(j, i);
                    for (int k = 0; k < 2 * n; ++k) {
                        augmented(j, k) -= factor * augmented(i, k);
                    }
                }
            }
        }
        
        // Extract inverse
        Matrix result(n, n);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                result(i, j) = augmented(i, j + n);
            }
        }
        
        return result;
    }
    
    // Solve linear system Ax = b
    std::vector<T> solve(const std::vector<T>& b) const {
        if (rows != cols || b.size() != rows) {
            throw std::runtime_error("Invalid dimensions for linear system");
        }
        
        Matrix inv = inverse();
        std::vector<T> x(rows, 0);
        
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                x[i] += inv(i, j) * b[j];
            }
        }
        
        return x;
    }
};

// Quaternion for rotation representation (avoids gimbal lock)
template<typename T = double>
class Quaternion {
private:
    T w, x, y, z;
    
public:
    Quaternion(T w = 1, T x = 0, T y = 0, T z = 0) 
        : w(w), x(x), y(y), z(z) {}
    
    // Convert from Euler angles (in radians)
    static Quaternion fromEuler(T roll, T pitch, T yaw) {
        T cy = std::cos(yaw * 0.5);
        T sy = std::sin(yaw * 0.5);
        T cp = std::cos(pitch * 0.5);
        T sp = std::sin(pitch * 0.5);
        T cr = std::cos(roll * 0.5);
        T sr = std::sin(roll * 0.5);
        
        return Quaternion(
            cr * cp * cy + sr * sp * sy,
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy
        );
    }
    
    // Hamilton product for quaternion multiplication
    Quaternion operator*(const Quaternion& q) const {
        return Quaternion(
            w * q.w - x * q.x - y * q.y - z * q.z,
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w
        );
    }
    
    // Rotate a vector by this quaternion
    Vector3<T> rotate(const Vector3<T>& v) const {
        Quaternion vq(0, v.x, v.y, v.z);
        Quaternion result = (*this) * vq * conjugate();
        return Vector3<T>(result.x, result.y, result.z);
    }
    
    Quaternion conjugate() const {
        return Quaternion(w, -x, -y, -z);
    }
    
    // Convert to rotation matrix
    Matrix<T> toRotationMatrix() const {
        Matrix<T> R(3, 3);
        R(0, 0) = 1 - 2*(y*y + z*z);
        R(0, 1) = 2*(x*y - w*z);
        R(0, 2) = 2*(x*z + w*y);
        R(1, 0) = 2*(x*y + w*z);
        R(1, 1) = 1 - 2*(x*x + z*z);
        R(1, 2) = 2*(y*z - w*x);
        R(2, 0) = 2*(x*z - w*y);
        R(2, 1) = 2*(y*z + w*x);
        R(2, 2) = 1 - 2*(x*x + y*y);
        return R;
    }
    
    // Spherical linear interpolation (slerp)
    static Quaternion slerp(const Quaternion& q1, const Quaternion& q2, T t) {
        T dot = q1.w*q2.w + q1.x*q2.x + q1.y*q2.y + q1.z*q2.z;
        
        // If dot is negative, take the shorter path
        Quaternion q2_copy = q2;
        if (dot < 0) {
            q2_copy = Quaternion(-q2.w, -q2.x, -q2.y, -q2.z);
            dot = -dot;
        }
        
        // If quaternions are very close, use linear interpolation
        if (dot > 0.9995) {
            Quaternion result(
                q1.w + t*(q2_copy.w - q1.w),
                q1.x + t*(q2_copy.x - q1.x),
                q1.y + t*(q2_copy.y - q1.y),
                q1.z + t*(q2_copy.z - q1.z)
            );
            T norm = std::sqrt(result.w*result.w + result.x*result.x + 
                              result.y*result.y + result.z*result.z);
            return Quaternion(result.w/norm, result.x/norm, 
                            result.y/norm, result.z/norm);
        }
        
        T theta_0 = std::acos(dot);
        T theta = theta_0 * t;
        T sin_theta = std::sin(theta);
        T sin_theta_0 = std::sin(theta_0);
        
        T s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
        T s1 = sin_theta / sin_theta_0;
        
        return Quaternion(
            s0*q1.w + s1*q2_copy.w,
            s0*q1.x + s1*q2_copy.x,
            s0*q1.y + s1*q2_copy.y,
            s0*q1.z + s1*q2_copy.z
        );
    }
    
    // Getters
    T getW() const { return w; }
    T getX() const { return x; }
    T getY() const { return y; }
    T getZ() const { return z; }
};

// ============================================================================
// PART 1: SPATIAL HASHING AND CHUNK MANAGEMENT
// ============================================================================

// Spatial hash for efficient chunk lookup
class SpatialHashGrid {
private:
    double cellSize;
    std::unordered_map<uint64_t, std::vector<uint64_t>> grid;
    
    // Morton code (Z-order curve) for 3D spatial hashing
    static uint64_t mortonEncode(uint32_t x, uint32_t y, uint32_t z) {
        uint64_t answer = 0;
        for (uint64_t i = 0; i < 21; ++i) {
            answer |= ((x & (1ULL << i)) << 2*i) | 
                     ((y & (1ULL << i)) << (2*i + 1)) | 
                     ((z & (1ULL << i)) << (2*i + 2));
        }
        return answer;
    }
    
    static void mortonDecode(uint64_t code, uint32_t& x, uint32_t& y, uint32_t& z) {
        x = 0; y = 0; z = 0;
        for (uint64_t i = 0; i < 21; ++i) {
            x |= ((code >> (3*i)) & 1) << i;
            y |= ((code >> (3*i + 1)) & 1) << i;
            z |= ((code >> (3*i + 2)) & 1) << i;
        }
    }
    
public:
    SpatialHashGrid(double cellSize = 16.0) : cellSize(cellSize) {}
    
    uint64_t getCellKey(const Vector3d& position) const {
        uint32_t x = static_cast<uint32_t>(std::floor(position.x / cellSize));
        uint32_t y = static_cast<uint32_t>(std::floor(position.y / cellSize));
        uint32_t z = static_cast<uint32_t>(std::floor(position.z / cellSize));
        return mortonEncode(x, y, z);
    }
    
    void insert(uint64_t entityId, const Vector3d& position) {
        uint64_t key = getCellKey(position);
        grid[key].push_back(entityId);
    }
    
    std::vector<uint64_t> queryNearby(const Vector3d& position, double radius) const {
        std::vector<uint64_t> results;
        uint64_t baseKey = getCellKey(position);
        
        int cellRadius = static_cast<int>(std::ceil(radius / cellSize));
        uint32_t baseX, baseY, baseZ;
        mortonDecode(baseKey, baseX, baseY, baseZ);
        
        for (int dx = -cellRadius; dx <= cellRadius; ++dx) {
            for (int dy = -cellRadius; dy <= cellRadius; ++dy) {
                for (int dz = -cellRadius; dz <= cellRadius; ++dz) {
                    uint64_t key = mortonEncode(
                        baseX + dx, baseY + dy, baseZ + dz
                    );
                    auto it = grid.find(key);
                    if (it != grid.end()) {
                        results.insert(results.end(), 
                                     it->second.begin(), it->second.end());
                    }
                }
            }
        }
        return results;
    }
};

// ============================================================================
// PART 2: PROCEDURAL GENERATION WITH NOISE
// ============================================================================

// Perlin noise for terrain generation
class PerlinNoise {
private:
    std::vector<int> permutation;
    
    static double fade(double t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }
    
    static double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }
    
    static double grad(int hash, double x, double y, double z) {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
    
public:
    PerlinNoise(uint32_t seed = 42) {
        permutation.resize(512);
        std::vector<int> p(256);
        std::iota(p.begin(), p.end(), 0);
        
        std::mt19937 gen(seed);
        std::shuffle(p.begin(), p.end(), gen);
        
        for (int i = 0; i < 256; ++i) {
            permutation[i] = p[i];
            permutation[i + 256] = p[i];
        }
    }
    
    double noise(double x, double y, double z) const {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        int Z = static_cast<int>(std::floor(z)) & 255;
        
        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);
        
        double u = fade(x);
        double v = fade(y);
        double w = fade(z);
        
        int A = permutation[X] + Y;
        int AA = permutation[A] + Z;
        int AB = permutation[A + 1] + Z;
        int B = permutation[X + 1] + Y;
        int BA = permutation[B] + Z;
        int BB = permutation[B + 1] + Z;
        
        return lerp(w, lerp(v, lerp(u, grad(permutation[AA], x, y, z),
                                      grad(permutation[BA], x-1, y, z)),
                              lerp(u, grad(permutation[AB], x, y-1, z),
                                      grad(permutation[BB], x-1, y-1, z))),
                       lerp(v, lerp(u, grad(permutation[AA+1], x, y, z-1),
                                      grad(permutation[BA+1], x-1, y, z-1)),
                              lerp(u, grad(permutation[AB+1], x, y-1, z-1),
                                      grad(permutation[BB+1], x-1, y-1, z-1))));
    }
    
    // Fractal Brownian Motion for more detailed terrain
    double fbm(double x, double y, double z, int octaves = 4) const {
        double value = 0;
        double amplitude = 1;
        double frequency = 1;
        double maxValue = 0;
        
        for (int i = 0; i < octaves; ++i) {
            value += amplitude * noise(x * frequency, y * frequency, z * frequency);
            maxValue += amplitude;
            amplitude *= 0.5;
            frequency *= 2;
        }
        
        return value / maxValue;
    }
};

// ============================================================================
// PART 3: VOXEL WORLD REPRESENTATION
// ============================================================================

// Block types using enum
enum class BlockType : uint8_t {
    AIR = 0,
    STONE = 1,
    DIRT = 2,
    GRASS = 3,
    WOOD = 4,
    LEAVES = 5,
    SAND = 6,
    WATER = 7,
    IRON_ORE = 8,
    GOLD_ORE = 9,
    DIAMOND_ORE = 10,
    COAL_ORE = 11,
    BEDROCK = 12,
    CRAFTING_TABLE = 13,
    FURNACE = 14
};

// Sparse voxel octree for efficient storage
class VoxelOctree {
private:
    struct OctreeNode {
        std::array<std::unique_ptr<OctreeNode>, 8> children;
        std::optional<BlockType> block;
        bool isLeaf = true;
        
        OctreeNode() : children{} {
            for (auto& child : children) {
                child = nullptr;
            }
        }
    };
    
    std::unique_ptr<OctreeNode> root;
    int maxDepth;
    
    int getOctant(const Vector3i& min, const Vector3i& max, 
                  const Vector3i& pos) const {
        int octant = 0;
        Vector3i mid(
            (min.x + max.x) / 2,
            (min.y + max.y) / 2,
            (min.z + max.z) / 2
        );
        
        if (pos.x >= mid.x) octant |= 1;
        if (pos.y >= mid.y) octant |= 2;
        if (pos.z >= mid.z) octant |= 4;
        
        return octant;
    }
    
    void getChildBounds(const Vector3i& min, const Vector3i& max,
                       int octant, Vector3i& childMin, Vector3i& childMax) const {
        Vector3i mid(
            (min.x + max.x) / 2,
            (min.y + max.y) / 2,
            (min.z + max.z) / 2
        );
        childMin = min;
        childMax = max;
        
        if (octant & 1) childMin.x = mid.x;
        else childMax.x = mid.x;
        
        if (octant & 2) childMin.y = mid.y;
        else childMax.y = mid.y;
        
        if (octant & 4) childMin.z = mid.z;
        else childMax.z = mid.z;
    }
    
public:
    VoxelOctree(int maxDepth = 8) : maxDepth(maxDepth) {
        root = std::make_unique<OctreeNode>();
    }
    
    void setBlock(const Vector3i& pos, BlockType block) {
        OctreeNode* node = root.get();
        Vector3i min(0, 0, 0);
        Vector3i max(1 << maxDepth, 1 << maxDepth, 1 << maxDepth);
        
        for (int depth = 0; depth < maxDepth; ++depth) {
            int octant = getOctant(min, max, pos);
            
            if (!node->children[octant]) {
                node->children[octant] = std::make_unique<OctreeNode>();
                node->isLeaf = false;
            }
            
            node = node->children[octant].get();
            getChildBounds(min, max, octant, min, max);
        }
        
        node->block = block;
    }
    
    std::optional<BlockType> getBlock(const Vector3i& pos) const {
        const OctreeNode* node = root.get();
        Vector3i min(0, 0, 0);
        Vector3i max(1 << maxDepth, 1 << maxDepth, 1 << maxDepth);
        
        for (int depth = 0; depth < maxDepth; ++depth) {
            int octant = getOctant(min, max, pos);
            
            if (!node->children[octant]) {
                return std::nullopt;
            }
            
            node = node->children[octant].get();
            getChildBounds(min, max, octant, min, max);
        }
        
        return node->block;
    }
    
    // Ray casting for block selection using DDA algorithm
    std::optional<Vector3i> raycast(const Vector3d& origin, 
                                    const Vector3d& direction,
                                    double maxDistance = 10.0) const {
        // Voxel traversal using Amanatides & Woo algorithm
        Vector3i currentPos(
            static_cast<int>(std::floor(origin.x)),
            static_cast<int>(std::floor(origin.y)),
            static_cast<int>(std::floor(origin.z))
        );
        Vector3i step;
        Vector3d tDelta;
        Vector3d tMax;
        
        for (int i = 0; i < 3; ++i) {
            if (direction[i] > 0) {
                step[i] = 1;
                tDelta[i] = 1.0 / direction[i];
                tMax[i] = (std::floor(origin[i]) + 1 - origin[i]) / direction[i];
            } else if (direction[i] < 0) {
                step[i] = -1;
                tDelta[i] = -1.0 / direction[i];
                tMax[i] = (origin[i] - std::floor(origin[i])) / direction[i];
            } else {
                step[i] = 0;
                tDelta[i] = std::numeric_limits<double>::infinity();
                tMax[i] = std::numeric_limits<double>::infinity();
            }
        }
        
        double distance = 0;
        while (distance <= maxDistance) {
            if (getBlock(currentPos).value_or(BlockType::AIR) != BlockType::AIR) {
                return currentPos;
            }
            
            int axis = 0;
            if (tMax[1] < tMax[axis]) axis = 1;
            if (tMax[2] < tMax[axis]) axis = 2;
            
            distance = tMax[axis];
            currentPos[axis] += step[axis];
            tMax[axis] += tDelta[axis];
        }
        
        return std::nullopt;
    }
};

// ============================================================================
// PART 4: CRAFTING SYSTEM USING GROUP THEORY
// ============================================================================

// Crafting recipe as a tensor/matrix
class CraftingSystem {
public:
    struct Recipe {
        std::array<std::array<BlockType, 3>, 3> pattern;
        BlockType result;
        int count;
        
        Recipe() : pattern{}, result(BlockType::AIR), count(0) {
            for (auto& row : pattern) {
                row.fill(BlockType::AIR);
            }
        }
    };
    
private:
    std::vector<Recipe> recipes;
    
    // Canonical form using group theory (dihedral group D4 symmetries)
    static bool isIsomorphic(const Recipe& r1, const Recipe& r2) {
        // Check all 8 symmetries of the square (D4 group)
        for (int transform = 0; transform < 8; ++transform) {
            if (matchesTransformation(r1, r2, transform)) {
                return true;
            }
        }
        return false;
    }
    
    static bool matchesTransformation(const Recipe& r1, const Recipe& r2, int transform) {
        // Transform r2 according to the D4 group element
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                int i2 = i, j2 = j;
                
                switch (transform) {
                    case 1: i2 = 2-j; j2 = i; break;  // 90° CCW
                    case 2: i2 = 2-i; j2 = 2-j; break; // 180°
                    case 3: i2 = j; j2 = 2-i; break;  // 270° CCW
                    case 4: i2 = i; j2 = 2-j; break;  // vertical reflection
                    case 5: i2 = 2-i; j2 = j; break;  // horizontal reflection
                    case 6: i2 = j; j2 = i; break;    // main diagonal
                    case 7: i2 = 2-j; j2 = 2-i; break; // anti-diagonal
                }
                
                if (r1.pattern[i][j] != r2.pattern[i2][j2]) {
                    return false;
                }
            }
        }
        return true;
    }
    
public:
    void addRecipe(const Recipe& recipe) {
        // Check for duplicates using D4 symmetry
        for (const auto& existing : recipes) {
            if (isIsomorphic(existing, recipe)) {
                std::cout << "Recipe already exists (symmetric equivalent)\n";
                return;
            }
        }
        recipes.push_back(recipe);
    }
    
    std::optional<std::pair<BlockType, int>> craft(
        const std::array<std::array<BlockType, 3>, 3>& grid) const {
        
        Recipe input;
        input.pattern = grid;
        
        for (const auto& recipe : recipes) {
            if (isIsomorphic(recipe, input)) {
                return std::make_pair(recipe.result, recipe.count);
            }
        }
        return std::nullopt;
    }
};

// ============================================================================
// PART 5: INVENTORY SYSTEM WITH OPTIMIZATION
// ============================================================================

class InventorySystem {
private:
    struct ItemStack {
        BlockType type;
        int count;
        int maxStack = 64;
        
        ItemStack(BlockType t = BlockType::AIR, int c = 0) 
            : type(t), count(c) {}
    };
    
    std::vector<ItemStack> items;
    int capacity;
    
public:
    InventorySystem(int cap = 36) : capacity(cap) {
        items.resize(capacity);
    }
    
    // Knapsack-style optimization for inventory management
    std::vector<std::pair<BlockType, int>> optimizeStorage() {
        std::map<BlockType, int> itemCounts;
        for (const auto& item : items) {
            if (item.type != BlockType::AIR) {
                itemCounts[item.type] += item.count;
            }
        }
        
        std::vector<std::pair<BlockType, int>> itemVector(itemCounts.begin(), 
                                                           itemCounts.end());
        
        int n = itemVector.size();
        std::vector<std::vector<int>> dp(capacity + 1, 
                                          std::vector<int>(n + 1, 0));
        
        for (int i = 1; i <= n; ++i) {
            int weight = (itemVector[i-1].second + 63) / 64;
            int value = itemVector[i-1].second;
            
            for (int w = 0; w <= capacity; ++w) {
                if (weight <= w) {
                    dp[w][i] = std::max(dp[w][i-1], 
                                       dp[w-weight][i-1] + value);
                } else {
                    dp[w][i] = dp[w][i-1];
                }
            }
        }
        
        std::vector<std::pair<BlockType, int>> result;
        int w = capacity;
        for (int i = n; i > 0; --i) {
            if (dp[w][i] != dp[w][i-1]) {
                result.push_back(itemVector[i-1]);
                int weight = (itemVector[i-1].second + 63) / 64;
                w -= weight;
            }
        }
        
        return result;
    }
};

// ============================================================================
// PART 6: PHYSICS ENGINE
// ============================================================================

class PhysicsEngine {
private:
    struct AABB {
        Vector3d min, max;
        
        bool intersects(const AABB& other) const {
            return (min.x <= other.max.x && max.x >= other.min.x) &&
                   (min.y <= other.max.y && max.y >= other.min.y) &&
                   (min.z <= other.max.z && max.z >= other.min.z);
        }
        
        bool intersectsSAT(const AABB& other) const {
            Vector3d center1 = (min + max) / 2;
            Vector3d center2 = (other.min + other.max) / 2;
            Vector3d extents1 = (max - min) / 2;
            Vector3d extents2 = (other.max - other.min) / 2;
            
            Vector3d delta = center2 - center1;
            
            return std::abs(delta.x) <= extents1.x + extents2.x &&
                   std::abs(delta.y) <= extents1.y + extents2.y &&
                   std::abs(delta.z) <= extents1.z + extents2.z;
        }
    };
    
    struct RigidBody {
        Vector3d position;
        Vector3d velocity;
        Vector3d acceleration;
        Vector3d force;
        double mass;
        double restitution;
        AABB boundingBox;
        Quaternion<> orientation;
        Vector3d angularVelocity;
        
        void integrate(double dt) {
            velocity += acceleration * dt;
            position += velocity * dt;
            acceleration = force / mass;
            force = Vector3d::Zero();
            
            Quaternion<> angularVelQuat(0, angularVelocity.x, 
                                        angularVelocity.y, angularVelocity.z);
            Quaternion<> dq = angularVelQuat * orientation * 0.5;
            orientation = Quaternion<>(
                orientation.getW() + dq.getW() * dt,
                orientation.getX() + dq.getX() * dt,
                orientation.getY() + dq.getY() * dt,
                orientation.getZ() + dq.getZ() * dt
            );
            
            double norm = std::sqrt(orientation.getW() * orientation.getW() + 
                                   orientation.getX() * orientation.getX() +
                                   orientation.getY() * orientation.getY() + 
                                   orientation.getZ() * orientation.getZ());
            orientation = Quaternion<>(orientation.getW() / norm, 
                                      orientation.getX() / norm,
                                      orientation.getY() / norm, 
                                      orientation.getZ() / norm);
        }
    };
    
    std::vector<RigidBody> bodies;
    
    void resolveCollision(RigidBody& b1, RigidBody& b2) {
        Vector3d normal = b2.position - b1.position;
        double distance = normal.norm();
        normal = normal.normalized();
        
        Vector3d relativeVel = b2.velocity - b1.velocity;
        double velAlongNormal = relativeVel.dot(normal);
        
        if (velAlongNormal > 0) return;
        
        double e = std::min(b1.restitution, b2.restitution);
        double j = -(1 + e) * velAlongNormal;
        j /= 1/b1.mass + 1/b2.mass;
        
        Vector3d impulse = normal * j;
        b1.velocity -= impulse / b1.mass;
        b2.velocity += impulse / b2.mass;
    }
    
public:
    void update(double dt) {
        for (int i = 0; i < bodies.size(); ++i) {
            for (int j = i + 1; j < bodies.size(); ++j) {
                if (bodies[i].boundingBox.intersectsSAT(bodies[j].boundingBox)) {
                    resolveCollision(bodies[i], bodies[j]);
                }
            }
        }
        
        for (auto& body : bodies) {
            body.integrate(dt);
        }
    }
};

// ============================================================================
// PART 7: LIGHTING AND RENDERING
// ============================================================================

class LightingSystem {
private:
    class SphericalHarmonics {
    private:
        static constexpr int BANDS = 3;
        std::array<Vector3d, BANDS * BANDS> coefficients;
        
        double legendrePolynomial(int l, int m, double x) const {
            if (l == 0 && m == 0) return 1.0;
            if (l == 1 && m == 0) return x;
            if (l == 1 && m == 1) return -std::sqrt(1 - x*x);
            if (l == 2 && m == 0) return 0.5 * (3*x*x - 1);
            if (l == 2 && m == 1) return -3 * x * std::sqrt(1 - x*x);
            if (l == 2 && m == 2) return 3 * (1 - x*x);
            return 0;
        }
        
    public:
        void projectLight(const Vector3d& direction, const Vector3d& color) {
            double x = direction.x;
            double y = direction.y;
            double z = direction.z;
            
            int index = 0;
            for (int l = 0; l < BANDS; ++l) {
                for (int m = -l; m <= l; ++m) {
                    double basis = legendrePolynomial(l, std::abs(m), z);
                    if (m > 0) basis *= std::cos(m * std::atan2(y, x));
                    if (m < 0) basis *= std::sin(-m * std::atan2(y, x));
                    
                    coefficients[index] = coefficients[index] + color * basis;
                    index++;
                }
            }
        }
        
        Vector3d evaluate(const Vector3d& normal) const {
            Vector3d result(0, 0, 0);
            int index = 0;
            
            for (int l = 0; l < BANDS; ++l) {
                for (int m = -l; m <= l; ++m) {
                    double basis = legendrePolynomial(l, std::abs(m), normal.z);
                    if (m > 0) basis *= std::cos(m * std::atan2(normal.y, normal.x));
                    if (m < 0) basis *= std::sin(-m * std::atan2(normal.y, normal.x));
                    
                    result = result + coefficients[index] * basis;
                    index++;
                }
            }
            return result;
        }
    };
    
    SphericalHarmonics sh;
    
public:
    double calculateAmbientOcclusion(const Vector3d& position,
                                     const Vector3d& normal,
                                     const VoxelOctree& world) const {
        int samples = 16;
        int occluded = 0;
        
        for (int i = 0; i < samples; ++i) {
            double theta = 2 * M_PI * rand() / RAND_MAX;
            double phi = acos(sqrt(1 - rand() / static_cast<double>(RAND_MAX)));
            
            Vector3d direction(
                std::sin(phi) * std::cos(theta),
                std::sin(phi) * std::sin(theta),
                std::cos(phi)
            );
            
            Vector3d tangent, bitangent;
            if (std::abs(normal.x) < 0.9) {
                tangent = normal.cross(Vector3d(1, 0, 0)).normalized();
            } else {
                tangent = normal.cross(Vector3d(0, 1, 0)).normalized();
            }
            bitangent = normal.cross(tangent);
            
            Vector3d worldDir = tangent * direction.x + 
                               bitangent * direction.y + 
                               normal * direction.z;
            
            auto hit = world.raycast(position, worldDir, 2.0);
            if (hit.has_value()) {
                occluded++;
            }
        }
        
        return 1.0 - static_cast<double>(occluded) / samples;
    }
};

// ============================================================================
// PART 8: PATHFINDING AND AI
// ============================================================================

class PathfindingSystem {
private:
    struct Node {
        Vector3i position;
        double g_cost;
        double h_cost;
        double f_cost() const { return g_cost + h_cost; }
        
        bool operator>(const Node& other) const {
            return f_cost() > other.f_cost();
        }
    };
    
    double heuristic(const Vector3i& a, const Vector3i& b) const {
        Vector3i diff = (a - b).cwiseAbs();
        return std::max({diff.x, diff.y, diff.z}) + 
               0.414 * (diff.sum() - std::max({diff.x, diff.y, diff.z}));
    }
    
    std::vector<Vector3i> getNeighbors(const Vector3i& pos) const {
        std::vector<Vector3i> neighbors;
        
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    if (dx != 0 || dy != 0 || dz != 0) {
                        neighbors.push_back(Vector3i(pos.x + dx, pos.y + dy, pos.z + dz));
                    }
                }
            }
        }
        return neighbors;
    }
    
    uint64_t hash(const Vector3i& v) const {
        return static_cast<uint64_t>(v.x) << 42 | 
               static_cast<uint64_t>(v.y) << 21 | 
               static_cast<uint64_t>(v.z);
    }
    
public:
    std::vector<Vector3i> findPath(const Vector3i& start,
                                   const Vector3i& goal,
                                   const VoxelOctree& world) {
        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
        std::unordered_map<uint64_t, Vector3i> cameFrom;
        std::unordered_map<uint64_t, double> gScore;
        
        openSet.push({start, 0, heuristic(start, goal)});
        gScore[hash(start)] = 0;
        
        while (!openSet.empty()) {
            Node current = openSet.top();
            openSet.pop();
            
            if (current.position.x == goal.x && 
                current.position.y == goal.y && 
                current.position.z == goal.z) {
                std::vector<Vector3i> path;
                Vector3i currentPos = goal;
                
                while (currentPos.x != start.x || 
                       currentPos.y != start.y || 
                       currentPos.z != start.z) {
                    path.push_back(currentPos);
                    currentPos = cameFrom[hash(currentPos)];
                }
                
                path.push_back(start);
                std::reverse(path.begin(), path.end());
                return path;
            }
            
            for (const auto& neighbor : getNeighbors(current.position)) {
                auto block = world.getBlock(neighbor);
                if (!block || block.value() == BlockType::AIR) {
                    Vector3i diff = neighbor - current.position;
                    double moveCost = (diff.magnitude() > 1) ? 1.414 : 1.0;
                    double tentativeG = current.g_cost + moveCost;
                    
                    if (!gScore.count(hash(neighbor)) || 
                        tentativeG < gScore[hash(neighbor)]) {
                        cameFrom[hash(neighbor)] = current.position;
                        gScore[hash(neighbor)] = tentativeG;
                        openSet.push({neighbor, tentativeG, 
                                     heuristic(neighbor, goal)});
                    }
                }
            }
        }
        
        return {};
    }
};

// ============================================================================
// PART 9: GAME WORLD MANAGER
// ============================================================================

class GameWorld {
private:
    VoxelOctree voxelWorld;
    PerlinNoise noiseGenerator;
    SpatialHashGrid spatialHash;
    PhysicsEngine physics;
    LightingSystem lighting;
    PathfindingSystem pathfinding;
    CraftingSystem crafting;
    InventorySystem inventory;
    
    struct Chunk {
        static constexpr int SIZE = 16;
        Vector3i chunkPos;
        std::array<std::array<std::array<BlockType, SIZE>, SIZE>, SIZE> blocks;
        
        Chunk(const Vector3i& pos) : chunkPos(pos) {
            for (auto& layer1 : blocks) {
                for (auto& layer2 : layer1) {
                    layer2.fill(BlockType::AIR);
                }
            }
        }
    };
    
    std::unordered_map<uint64_t, std::unique_ptr<Chunk>> chunks;
    
    uint64_t getChunkKey(const Vector3i& chunkPos) const {
        return static_cast<uint64_t>(chunkPos.x) << 42 |
               static_cast<uint64_t>(chunkPos.y) << 21 |
               static_cast<uint64_t>(chunkPos.z);
    }
    
    void generateChunk(const Vector3i& chunkPos) {
        auto chunk = std::make_unique<Chunk>(chunkPos);
        
        for (int x = 0; x < Chunk::SIZE; ++x) {
            for (int z = 0; z < Chunk::SIZE; ++z) {
                int worldX = chunkPos.x * Chunk::SIZE + x;
                int worldZ = chunkPos.z * Chunk::SIZE + z;
                
                double height = 64 + 
                    32 * noiseGenerator.fbm(worldX * 0.01, 0, worldZ * 0.01, 4) +
                    8 * noiseGenerator.fbm(worldX * 0.05, 0, worldZ * 0.05, 3) +
                    2 * noiseGenerator.fbm(worldX * 0.1, 0, worldZ * 0.1, 2);
                
                for (int y = 0; y < Chunk::SIZE; ++y) {
                    int worldY = chunkPos.y * Chunk::SIZE + y;
                    
                    if (worldY < height - 4) {
                        chunk->blocks[x][y][z] = BlockType::STONE;
                    } else if (worldY < height - 1) {
                        chunk->blocks[x][y][z] = BlockType::DIRT;
                    } else if (worldY < height) {
                        chunk->blocks[x][y][z] = BlockType::GRASS;
                    } else if (worldY < 48) {
                        chunk->blocks[x][y][z] = BlockType::WATER;
                    } else {
                        chunk->blocks[x][y][z] = BlockType::AIR;
                    }
                    
                    if (worldY < 16) {
                        if (noiseGenerator.noise(worldX * 0.1, worldY * 0.1, 
                                                worldZ * 0.1) > 0.8) {
                            chunk->blocks[x][y][z] = BlockType::DIAMOND_ORE;
                        }
                    }
                    
                    voxelWorld.setBlock(Vector3i(worldX, worldY, worldZ),
                                       chunk->blocks[x][y][z]);
                }
            }
        }
        
        chunks[getChunkKey(chunkPos)] = std::move(chunk);
    }
    
public:
    GameWorld() {
        initializeCraftingRecipes();
    }
    
    void initializeCraftingRecipes() {
        CraftingSystem::Recipe tableRecipe;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                tableRecipe.pattern[i][j] = BlockType::WOOD;
            }
        }
        tableRecipe.result = BlockType::CRAFTING_TABLE;
        tableRecipe.count = 1;
        crafting.addRecipe(tableRecipe);
    }
    
    void update(double deltaTime) {
        physics.update(deltaTime);
    }
    
    void generateWorld() {
        int chunkRadius = 4;
        for (int x = -chunkRadius; x <= chunkRadius; ++x) {
            for (int z = -chunkRadius; z <= chunkRadius; ++z) {
                generateChunk(Vector3i(x, 0, z));
            }
        }
    }
    
    BlockType getBlock(const Vector3i& pos) const {
        return voxelWorld.getBlock(pos).value_or(BlockType::AIR);
    }
    
    std::optional<Vector3i> raycastBlocks(const Vector3d& origin,
                                          const Vector3d& direction,
                                          double maxDistance = 10.0) const {
        return voxelWorld.raycast(origin, direction, maxDistance);
    }
    
    std::vector<Vector3i> findPath(const Vector3i& start, const Vector3i& goal) {
        return pathfinding.findPath(start, goal, voxelWorld);
    }
};

// ============================================================================
// PART 10: MAIN GAME LOOP AND DEMONSTRATION
// ============================================================================

class SurvivalGame {
private:
    GameWorld world;
    bool running;
    double gameTime;
    
    struct Player {
        Vector3d position;
        Vector3d velocity;
        Quaternion<> orientation;
        double health;
        double hunger;
        double stamina;
        InventorySystem inventory;
        
        Player() : position(0, 100, 0), velocity(0, 0, 0),
                   health(100), hunger(100), stamina(100) {}
    };
    
    Player player;
    
public:
    SurvivalGame() : running(false), gameTime(0) {
        world.generateWorld();
    }
    
    void run() {
        running = true;
        std::cout << "Game started! Press Ctrl+C to exit.\n";
        
        // Simulate a few frames
        for (int frame = 0; frame < 10 && running; ++frame) {
            world.update(1.0/60.0);
            gameTime += 1.0/60.0;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        
        running = false;
    }
    
    void demonstrateFeatures() {
        std::cout << "\n=== DEMONSTRATION OF MATHEMATICAL FEATURES ===\n";
        
        // Test ray casting
        std::cout << "\n1. Ray Casting (Block Selection):\n";
        Vector3d origin(0, 100, 0);
        Vector3d direction(0, -1, 0);
        auto hit = world.raycastBlocks(origin, direction, 50.0);
        if (hit) {
            std::cout << "Hit block at: " << hit->toString() << "\n";
        }
        
        // Test pathfinding
        std::cout << "\n2. Pathfinding (A* Algorithm):\n";
        Vector3i start(0, 70, 0);
        Vector3i goal(10, 70, 10);
        auto path = world.findPath(start, goal);
        std::cout << "Path length: " << path.size() << " nodes\n";
        
        // Test quaternion rotation
        std::cout << "\n3. Quaternion Rotation:\n";
        Quaternion<> q = Quaternion<>::fromEuler(0, M_PI/4, 0);
        Vector3d v(1, 0, 0);
        Vector3d rotated = q.rotate(v);
        std::cout << "Rotated vector: " << rotated.toString() << "\n";
        
        // Test Perlin noise
        std::cout << "\n4. Perlin Noise Generation:\n";
        PerlinNoise noise;
        for (int i = 0; i < 3; ++i) {
            double value = noise.fbm(i * 0.5, i * 0.3, i * 0.7, 4);
            std::cout << "Noise value " << i << ": " << value << "\n";
        }
        
        // Test spatial hashing
        std::cout << "\n5. Spatial Hashing (Morton Code):\n";
        SpatialHashGrid grid;
        Vector3d pos1(10, 5, 3);
        Vector3d pos2(15, 7, 8);
        std::cout << "Hash key 1: " << grid.getCellKey(pos1) << "\n";
        std::cout << "Hash key 2: " << grid.getCellKey(pos2) << "\n";
    }
};

int main() {
    std::cout << "=== 3D SURVIVAL CRAFTING GAME - MATHEMATICAL FRAMEWORK ===\n";
    std::cout << "===========================================================\n\n";
    
    SurvivalGame game;
    
    // Demonstrate mathematical features
    game.demonstrateFeatures();
    
    std::cout << "\n\n=== MATHEMATICAL CONCEPTS IMPLEMENTED ===\n";
    std::cout << "1. Vector3 - Custom 3D vector operations\n";
    std::cout << "2. Matrix - Matrix operations (multiplication, inverse, solve)\n";
    std::cout << "3. Quaternion - Rotation without gimbal lock\n";
    std::cout << "4. Morton Code - Z-order curve for spatial indexing\n";
    std::cout << "5. Perlin Noise - Procedural terrain generation\n";
    std::cout << "6. Fractal Brownian Motion - Multi-octave noise\n";
    std::cout << "7. Voxel Octree - Efficient 3D world storage\n";
    std::cout << "8. DDA Algorithm - Ray casting for block selection\n";
    std::cout << "9. D4 Group Theory - Crafting recipe symmetry detection\n";
    std::cout << "10. A* Pathfinding - Navigation with heuristics\n";
    std::cout << "11. Spherical Harmonics - Lighting approximation\n";
    std::cout << "12. Ambient Occlusion - Contact shadow calculation\n";
    std::cout << "13. Knapsack DP - Inventory optimization\n";
    std::cout << "14. Impulse-based Physics - Collision response\n";
    std::cout << "15. SAT - Separating Axis Theorem for collision detection\n";
    
    std::cout << "\n=== SYSTEM COMPONENTS ===\n";
    std::cout << "✓ Custom Vector3 class (replaces Eigen::Vector3d)\n";
    std::cout << "✓ Custom Matrix class (replaces Eigen::MatrixXd)\n";
    std::cout << "✓ Custom Quaternion class\n";
    std::cout << "✓ Self-contained (no external dependencies)\n";
    std::cout << "✓ Standard C++17 only\n";
    
    return 0;
}