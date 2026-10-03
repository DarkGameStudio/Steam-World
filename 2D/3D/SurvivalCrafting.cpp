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
#include "Eigen/Dense"
#include "Eigen/Sparse"
#include "Eigen/Geometry"

// ============================================================================
// PART 1: CORE MATHEMATICAL STRUCTURES
// ============================================================================

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
    Eigen::Vector3d rotate(const Eigen::Vector3d& v) const {
        Quaternion vq(0, v.x(), v.y(), v.z());
        Quaternion result = (*this) * vq * conjugate();
        return Eigen::Vector3d(result.x, result.y, result.z);
    }
    
    Quaternion conjugate() const {
        return Quaternion(w, -x, -y, -z);
    }
    
    // Convert to rotation matrix
    Eigen::Matrix3d toRotationMatrix() const {
        Eigen::Matrix3d R;
        R << 1 - 2*(y*y + z*z), 2*(x*y - w*z), 2*(x*z + w*y),
             2*(x*y + w*z), 1 - 2*(x*x + z*z), 2*(y*z - w*x),
             2*(x*z - w*y), 2*(y*z + w*x), 1 - 2*(x*x + y*y);
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
};

// ============================================================================
// PART 2: SPATIAL HASHING AND CHUNK MANAGEMENT
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
    
    uint64_t getCellKey(const Eigen::Vector3d& position) const {
        uint32_t x = static_cast<uint32_t>(std::floor(position.x() / cellSize));
        uint32_t y = static_cast<uint32_t>(std::floor(position.y() / cellSize));
        uint32_t z = static_cast<uint32_t>(std::floor(position.z() / cellSize));
        return mortonEncode(x, y, z);
    }
    
    void insert(uint64_t entityId, const Eigen::Vector3d& position) {
        uint64_t key = getCellKey(position);
        grid[key].push_back(entityId);
    }
    
    std::vector<uint64_t> queryNearby(const Eigen::Vector3d& position, double radius) const {
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
// PART 3: PROCEDURAL GENERATION WITH NOISE
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
// PART 4: VOXEL WORLD REPRESENTATION
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
    
    int getOctant(const Eigen::Vector3i& min, const Eigen::Vector3i& max, 
                  const Eigen::Vector3i& pos) const {
        int octant = 0;
        Eigen::Vector3i mid = (min + max) / 2;
        
        if (pos.x() >= mid.x()) octant |= 1;
        if (pos.y() >= mid.y()) octant |= 2;
        if (pos.z() >= mid.z()) octant |= 4;
        
        return octant;
    }
    
    void getChildBounds(const Eigen::Vector3i& min, const Eigen::Vector3i& max,
                       int octant, Eigen::Vector3i& childMin, Eigen::Vector3i& childMax) const {
        Eigen::Vector3i mid = (min + max) / 2;
        childMin = min;
        childMax = max;
        
        if (octant & 1) childMin.x() = mid.x();
        else childMax.x() = mid.x();
        
        if (octant & 2) childMin.y() = mid.y();
        else childMax.y() = mid.y();
        
        if (octant & 4) childMin.z() = mid.z();
        else childMax.z() = mid.z();
    }
    
public:
    VoxelOctree(int maxDepth = 8) : maxDepth(maxDepth) {
        root = std::make_unique<OctreeNode>();
    }
    
    void setBlock(const Eigen::Vector3i& pos, BlockType block) {
        OctreeNode* node = root.get();
        Eigen::Vector3i min(0, 0, 0);
        Eigen::Vector3i max(1 << maxDepth, 1 << maxDepth, 1 << maxDepth);
        
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
    
    std::optional<BlockType> getBlock(const Eigen::Vector3i& pos) const {
        const OctreeNode* node = root.get();
        Eigen::Vector3i min(0, 0, 0);
        Eigen::Vector3i max(1 << maxDepth, 1 << maxDepth, 1 << maxDepth);
        
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
    std::optional<Eigen::Vector3i> raycast(const Eigen::Vector3d& origin, 
                                           const Eigen::Vector3d& direction,
                                           double maxDistance = 10.0) const {
        // Voxel traversal using Amanatides & Woo algorithm
        Eigen::Vector3i currentPos = origin.cast<int>();
        Eigen::Vector3i step;
        Eigen::Vector3d tDelta;
        Eigen::Vector3d tMax;
        
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
// PART 5: CRAFTING SYSTEM USING GROUP THEORY
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
        // 0: identity, 1: rotate 90°, 2: rotate 180°, 3: rotate 270°,
        // 4: reflect vertical, 5: reflect horizontal, 
        // 6: reflect main diagonal, 7: reflect anti-diagonal
        
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
    
    // Optimize crafting using matrix operations
    Eigen::MatrixXi recipeToMatrix(const Recipe& recipe) const {
        Eigen::MatrixXi matrix(3, 3);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                matrix(i, j) = static_cast<int>(recipe.pattern[i][j]);
            }
        }
        return matrix;
    }
};

// ============================================================================
// PART 6: INVENTORY SYSTEM WITH OPTIMIZATION
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
        // Dynamic programming approach to maximize item storage
        std::map<BlockType, int> itemCounts;
        for (const auto& item : items) {
            if (item.type != BlockType::AIR) {
                itemCounts[item.type] += item.count;
            }
        }
        
        // Convert to vector for DP
        std::vector<std::pair<BlockType, int>> itemVector(itemCounts.begin(), 
                                                           itemCounts.end());
        
        // 0/1 knapsack with stack sizes
        int n = itemVector.size();
        std::vector<std::vector<int>> dp(capacity + 1, 
                                          std::vector<int>(n + 1, 0));
        
        for (int i = 1; i <= n; ++i) {
            int weight = (itemVector[i-1].second + 63) / 64; // Number of stacks needed
            int value = itemVector[i-1].second; // Total item count
            
            for (int w = 0; w <= capacity; ++w) {
                if (weight <= w) {
                    dp[w][i] = std::max(dp[w][i-1], 
                                       dp[w-weight][i-1] + value);
                } else {
                    dp[w][i] = dp[w][i-1];
                }
            }
        }
        
        // Backtrack to find optimal arrangement
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
    
    // Use simplex algorithm for resource allocation
    Eigen::VectorXd optimizeResourceAllocation(
        const Eigen::MatrixXd& constraints,
        const Eigen::VectorXd& objectives) {
        
        // Simplex method implementation (simplified)
        int m = constraints.rows();
        int n = constraints.cols();
        
        Eigen::MatrixXd tableau(m + 1, n + m + 1);
        tableau.setZero();
        
        // Set up initial tableau
        tableau.topLeftCorner(m, n) = constraints;
        tableau.topRightCorner(m, m) = Eigen::MatrixXd::Identity(m, m);
        tableau.topRightCorner(m, 1) = constraints * Eigen::VectorXd::Ones(n);
        tableau.bottomLeftCorner(1, n) = -objectives.transpose();
        
        // Simplex iterations (simplified for demonstration)
        for (int iter = 0; iter < 100; ++iter) {
            // Find entering variable
            int entering = -1;
            double minCoeff = 0;
            for (int j = 0; j < n + m; ++j) {
                if (tableau(m, j) < minCoeff) {
                    minCoeff = tableau(m, j);
                    entering = j;
                }
            }
            
            if (entering == -1) break; // Optimal solution found
            
            // Find leaving variable
            int leaving = -1;
            double minRatio = std::numeric_limits<double>::infinity();
            for (int i = 0; i < m; ++i) {
                if (tableau(i, entering) > 0) {
                    double ratio = tableau(i, n + m) / tableau(i, entering);
                    if (ratio < minRatio) {
                        minRatio = ratio;
                        leaving = i;
                    }
                }
            }
            
            if (leaving == -1) break; // Unbounded
            
            // Pivot
            double pivot = tableau(leaving, entering);
            tableau.row(leaving) /= pivot;
            
            for (int i = 0; i < m + 1; ++i) {
                if (i != leaving) {
                    tableau.row(i) -= tableau(i, entering) * tableau.row(leaving);
                }
            }
        }
        
        // Extract solution
        Eigen::VectorXd solution(n);
        for (int j = 0; j < n; ++j) {
            int basicRow = -1;
            for (int i = 0; i < m; ++i) {
                if (std::abs(tableau(i, j) - 1) < 1e-6) {
                    basicRow = i;
                    break;
                }
            }
            solution(j) = (basicRow != -1) ? tableau(basicRow, n + m) : 0;
        }
        
        return solution;
    }
};

// ============================================================================
// PART 7: PHYSICS ENGINE
// ============================================================================

class PhysicsEngine {
private:
    struct AABB {
        Eigen::Vector3d min, max;
        
        bool intersects(const AABB& other) const {
            return (min.x() <= other.max.x() && max.x() >= other.min.x()) &&
                   (min.y() <= other.max.y() && max.y() >= other.min.y()) &&
                   (min.z() <= other.max.z() && max.z() >= other.min.z());
        }
        
        // Separating Axis Theorem for collision detection
        bool intersectsSAT(const AABB& other) const {
            Eigen::Vector3d center1 = (min + max) / 2;
            Eigen::Vector3d center2 = (other.min + other.max) / 2;
            Eigen::Vector3d extents1 = (max - min) / 2;
            Eigen::Vector3d extents2 = (other.max - other.min) / 2;
            
            Eigen::Vector3d delta = center2 - center1;
            
            return std::abs(delta.x()) <= extents1.x() + extents2.x() &&
                   std::abs(delta.y()) <= extents1.y() + extents2.y() &&
                   std::abs(delta.z()) <= extents1.z() + extents2.z();
        }
    };
    
    struct RigidBody {
        Eigen::Vector3d position;
        Eigen::Vector3d velocity;
        Eigen::Vector3d acceleration;
        Eigen::Vector3d force;
        double mass;
        double restitution; // Bounciness
        AABB boundingBox;
        Quaternion<> orientation;
        Eigen::Vector3d angularVelocity;
        
        void integrate(double dt) {
            // Semi-implicit Euler integration
            velocity += acceleration * dt;
            position += velocity * dt;
            acceleration = force / mass;
            force.setZero();
            
            // Update orientation
            Quaternion<> angularVelQuat(0, angularVelocity.x(), 
                                        angularVelocity.y(), angularVelocity.z());
            Quaternion<> dq = angularVelQuat * orientation * 0.5;
            orientation = Quaternion<>(
                orientation.w + dq.w * dt,
                orientation.x + dq.x * dt,
                orientation.y + dq.y * dt,
                orientation.z + dq.z * dt
            );
            // Normalize quaternion
            double norm = std::sqrt(orientation.w * orientation.w + 
                                   orientation.x * orientation.x +
                                   orientation.y * orientation.y + 
                                   orientation.z * orientation.z);
            orientation = Quaternion<>(orientation.w / norm, 
                                      orientation.x / norm,
                                      orientation.y / norm, 
                                      orientation.z / norm);
        }
    };
    
    std::vector<RigidBody> bodies;
    
    // Collision response using impulse-based method
    void resolveCollision(RigidBody& b1, RigidBody& b2) {
        Eigen::Vector3d normal = b2.position - b1.position;
        double distance = normal.norm();
        normal.normalize();
        
        // Relative velocity
        Eigen::Vector3d relativeVel = b2.velocity - b1.velocity;
        double velAlongNormal = relativeVel.dot(normal);
        
        // Don't resolve if velocities are separating
        if (velAlongNormal > 0) return;
        
        // Calculate impulse
        double e = std::min(b1.restitution, b2.restitution);
        double j = -(1 + e) * velAlongNormal;
        j /= 1/b1.mass + 1/b2.mass;
        
        Eigen::Vector3d impulse = j * normal;
        b1.velocity -= impulse / b1.mass;
        b2.velocity += impulse / b2.mass;
    }
    
public:
    void update(double dt) {
        // Broad phase collision detection using sweep and prune
        std::vector<std::pair<double, int>> xAxis;
        for (int i = 0; i < bodies.size(); ++i) {
            xAxis.push_back({bodies[i].boundingBox.min.x(), i});
            xAxis.push_back({bodies[i].boundingBox.max.x(), i + 10000});
        }
        
        std::sort(xAxis.begin(), xAxis.end());
        
        // Narrow phase
        for (int i = 0; i < bodies.size(); ++i) {
            for (int j = i + 1; j < bodies.size(); ++j) {
                if (bodies[i].boundingBox.intersectsSAT(bodies[j].boundingBox)) {
                    resolveCollision(bodies[i], bodies[j]);
                }
            }
        }
        
        // Integrate
        for (auto& body : bodies) {
            body.integrate(dt);
        }
    }
    
    // Constraint solving using Lagrange multipliers
    Eigen::VectorXd solveConstraints(const Eigen::MatrixXd& J,
                                     const Eigen::VectorXd& b,
                                     const Eigen::MatrixXd& M) {
        // Solve for constraint forces using:
        // (J * M^{-1} * J^T) * λ = -b
        Eigen::MatrixXd A = J * M.inverse() * J.transpose();
        Eigen::VectorXd lambda = A.ldlt().solve(-b);
        
        // Calculate constraint forces
        Eigen::VectorXd constraintForces = J.transpose() * lambda;
        return constraintForces;
    }
};

// ============================================================================
// PART 8: LIGHTING AND RENDERING
// ============================================================================

class LightingSystem {
private:
    // Spherical harmonics for global illumination approximation
    class SphericalHarmonics {
    private:
        static constexpr int BANDS = 3; // 9 coefficients
        std::array<Eigen::Vector3d, BANDS * BANDS> coefficients;
        
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
        void projectLight(const Eigen::Vector3d& direction, 
                         const Eigen::Vector3d& color) {
            double x = direction.x();
            double y = direction.y();
            double z = direction.z();
            
            // Project onto SH basis
            int index = 0;
            for (int l = 0; l < BANDS; ++l) {
                for (int m = -l; m <= l; ++m) {
                    double basis = legendrePolynomial(l, std::abs(m), z);
                    if (m > 0) basis *= std::cos(m * std::atan2(y, x));
                    if (m < 0) basis *= std::sin(-m * std::atan2(y, x));
                    
                    coefficients[index++] += color * basis;
                }
            }
        }
        
        Eigen::Vector3d evaluate(const Eigen::Vector3d& normal) const {
            Eigen::Vector3d result(0, 0, 0);
            int index = 0;
            
            for (int l = 0; l < BANDS; ++l) {
                for (int m = -l; m <= l; ++m) {
                    double basis = legendrePolynomial(l, std::abs(m), normal.z());
                    if (m > 0) basis *= std::cos(m * std::atan2(normal.y(), normal.x()));
                    if (m < 0) basis *= std::sin(-m * std::atan2(normal.y(), normal.x()));
                    
                    result += coefficients[index++] * basis;
                }
            }
            return result;
        }
    };
    
    SphericalHarmonics sh;
    
public:
    // Ambient occlusion using hemisphere sampling
    double calculateAmbientOcclusion(const Eigen::Vector3d& position,
                                     const Eigen::Vector3d& normal,
                                     const VoxelOctree& world) const {
        int samples = 16;
        int occluded = 0;
        
        for (int i = 0; i < samples; ++i) {
            // Generate sample direction in hemisphere
            double theta = 2 * M_PI * rand() / RAND_MAX;
            double phi = acos(sqrt(1 - rand() / static_cast<double>(RAND_MAX)));
            
            Eigen::Vector3d direction(
                std::sin(phi) * std::cos(theta),
                std::sin(phi) * std::sin(theta),
                std::cos(phi)
            );
            
            // Transform to be aligned with normal
            Eigen::Vector3d tangent, bitangent;
            if (std::abs(normal.x()) < 0.9) {
                tangent = normal.cross(Eigen::Vector3d(1, 0, 0)).normalized();
            } else {
                tangent = normal.cross(Eigen::Vector3d(0, 1, 0)).normalized();
            }
            bitangent = normal.cross(tangent);
            
            Eigen::Vector3d worldDir = tangent * direction.x() + 
                                       bitangent * direction.y() + 
                                       normal * direction.z();
            
            // Ray march to check occlusion
            auto hit = world.raycast(position, worldDir, 2.0);
            if (hit.has_value()) {
                occluded++;
            }
        }
        
        return 1.0 - static_cast<double>(occluded) / samples;
    }
    
    // Radiosity calculation for indirect lighting
    void calculateRadiosity(const std::vector<Eigen::Vector3d>& patches,
                           const std::vector<Eigen::Vector3d>& normals,
                           std::vector<Eigen::Vector3d>& illumination) {
        int n = patches.size();
        
        // Form factor matrix
        Eigen::MatrixXd formFactors(n, n);
        
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i != j) {
                    Eigen::Vector3d diff = patches[j] - patches[i];
                    double distance = diff.norm();
                    double cosThetaI = normals[i].dot(diff) / distance;
                    double cosThetaJ = -normals[j].dot(diff) / distance;
                    
                    if (cosThetaI > 0 && cosThetaJ > 0) {
                        formFactors(i, j) = cosThetaI * cosThetaJ / 
                                           (M_PI * distance * distance);
                    } else {
                        formFactors(i, j) = 0;
                    }
                } else {
                    formFactors(i, j) = 0;
                }
            }
        }
        
        // Solve radiosity equation: B = E + ρ * F * B
        Eigen::MatrixXd I = Eigen::MatrixXd::Identity(n, n);
        double rho = 0.5; // Average reflectivity
        Eigen::MatrixXd A = I - rho * formFactors;
        
        Eigen::VectorXd emission(n);
        for (int i = 0; i < n; ++i) {
            emission(i) = illumination[i].norm();
        }
        
        Eigen::VectorXd solution = A.ldlt().solve(emission);
        
        for (int i = 0; i < n; ++i) {
            illumination[i] *= solution(i) / emission(i);
        }
    }
};

// ============================================================================
// PART 9: PATHFINDING AND AI
// ============================================================================

class PathfindingSystem {
private:
    // A* algorithm with binary heap optimization
    struct Node {
        Eigen::Vector3i position;
        double g_cost; // Cost from start
        double h_cost; // Heuristic to goal
        double f_cost() const { return g_cost + h_cost; }
        
        bool operator>(const Node& other) const {
            return f_cost() > other.f_cost();
        }
    };
    
    double heuristic(const Eigen::Vector3i& a, const Eigen::Vector3i& b) const {
        // Octile distance (diagonal movement allowed)
        Eigen::Vector3i diff = (a - b).cwiseAbs();
        return std::max({diff.x(), diff.y(), diff.z()}) + 
               0.414 * (diff.sum() - std::max({diff.x(), diff.y(), diff.z()}));
    }
    
    std::vector<Eigen::Vector3i> getNeighbors(const Eigen::Vector3i& pos) const {
        std::vector<Eigen::Vector3i> neighbors;
        
        // 26-connected neighborhood (all adjacent cubes)
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    if (dx != 0 || dy != 0 || dz != 0) {
                        neighbors.push_back(pos + Eigen::Vector3i(dx, dy, dz));
                    }
                }
            }
        }
        return neighbors;
    }
    
public:
    std::vector<Eigen::Vector3i> findPath(const Eigen::Vector3i& start,
                                          const Eigen::Vector3i& goal,
                                          const VoxelOctree& world) {
        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
        std::unordered_map<uint64_t, Eigen::Vector3i> cameFrom;
        std::unordered_map<uint64_t, double> gScore;
        
        auto hash = [](const Eigen::Vector3i& v) {
            return static_cast<uint64_t>(v.x()) << 42 | 
                   static_cast<uint64_t>(v.y()) << 21 | 
                   static_cast<uint64_t>(v.z());
        };
        
        openSet.push({start, 0, heuristic(start, goal)});
        gScore[hash(start)] = 0;
        
        while (!openSet.empty()) {
            Node current = openSet.top();
            openSet.pop();
            
            if (current.position == goal) {
                // Reconstruct path
                std::vector<Eigen::Vector3i> path;
                Eigen::Vector3i currentPos = goal;
                
                while (currentPos != start) {
                    path.push_back(currentPos);
                    currentPos = cameFrom[hash(currentPos)];
                }
                
                path.push_back(start);
                std::reverse(path.begin(), path.end());
                return path;
            }
            
            for (const auto& neighbor : getNeighbors(current.position)) {
                // Check if walkable
                auto block = world.getBlock(neighbor);
                if (!block || block.value() == BlockType::AIR) {
                    double tentativeG = current.g_cost + 
                        ((neighbor - current.position).norm() > 1 ? 1.414 : 1.0);
                    
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
        
        return {}; // No path found
    }
    
    // Flow field pathfinding for group movement
    class FlowField {
    private:
        std::vector<std::vector<std::vector<Eigen::Vector3d>>> flowVectors;
        int width, height, depth;
        
    public:
        FlowField(int w, int h, int d) : width(w), height(h), depth(d) {
            flowVectors.resize(w, std::vector<std::vector<Eigen::Vector3d>>(
                h, std::vector<Eigen::Vector3d>(d, Eigen::Vector3d::Zero())));
        }
        
        void generate(const Eigen::Vector3i& goal, const VoxelOctree& world) {
            // Dijkstra's algorithm to generate flow field
            std::queue<Eigen::Vector3i> queue;
            std::vector<std::vector<std::vector<int>>> distance(
                width, std::vector<std::vector<int>>(
                    height, std::vector<int>(depth, INT_MAX)));
            
            queue.push(goal);
            distance[goal.x()][goal.y()][goal.z()] = 0;
            
            while (!queue.empty()) {
                Eigen::Vector3i current = queue.front();
                queue.pop();
                
                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dz = -1; dz <= 1; ++dz) {
                            Eigen::Vector3i neighbor = current + 
                                Eigen::Vector3i(dx, dy, dz);
                            
                            if (neighbor.x() >= 0 && neighbor.x() < width &&
                                neighbor.y() >= 0 && neighbor.y() < height &&
                                neighbor.z() >= 0 && neighbor.z() < depth) {
                                
                                auto block = world.getBlock(neighbor);
                                if (block && block.value() != BlockType::AIR) {
                                    continue;
                                }
                                
                                int newDist = distance[current.x()][current.y()][current.z()] + 1;
                                if (newDist < distance[neighbor.x()][neighbor.y()][neighbor.z()]) {
                                    distance[neighbor.x()][neighbor.y()][neighbor.z()] = newDist;
                                    queue.push(neighbor);
                                }
                            }
                        }
                    }
                }
            }
            
            // Generate flow vectors from distance field
            for (int x = 0; x < width; ++x) {
                for (int y = 0; y < height; ++y) {
                    for (int z = 0; z < depth; ++z) {
                        Eigen::Vector3i current(x, y, z);
                        int minDist = distance[x][y][z];
                        Eigen::Vector3d bestDir(0, 0, 0);
                        
                        for (int dx = -1; dx <= 1; ++dx) {
                            for (int dy = -1; dy <= 1; ++dy) {
                                for (int dz = -1; dz <= 1; ++dz) {
                                    int nx = x + dx, ny = y + dy, nz = z + dz;
                                    if (nx >= 0 && nx < width &&
                                        ny >= 0 && ny < height &&
                                        nz >= 0 && nz < depth) {
                                        if (distance[nx][ny][nz] < minDist) {
                                            minDist = distance[nx][ny][nz];
                                            bestDir = Eigen::Vector3d(dx, dy, dz).normalized();
                                        }
                                    }
                                }
                            }
                        }
                        
                        flowVectors[x][y][z] = bestDir;
                    }
                }
            }
        }
        
        Eigen::Vector3d getFlowDirection(const Eigen::Vector3i& position) const {
            return flowVectors[position.x()][position.y()][position.z()];
        }
    };
};

// ============================================================================
// PART 10: GAME WORLD MANAGER
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
    
    // Chunk management
    struct Chunk {
        static constexpr int SIZE = 16;
        Eigen::Vector3i chunkPos;
        std::array<std::array<std::array<BlockType, SIZE>, SIZE>, SIZE> blocks;
        
        Chunk(const Eigen::Vector3i& pos) : chunkPos(pos) {
            for (auto& layer1 : blocks) {
                for (auto& layer2 : layer1) {
                    layer2.fill(BlockType::AIR);
                }
            }
        }
    };
    
    std::unordered_map<uint64_t, std::unique_ptr<Chunk>> chunks;
    
    uint64_t getChunkKey(const Eigen::Vector3i& chunkPos) const {
        return static_cast<uint64_t>(chunkPos.x()) << 42 |
               static_cast<uint64_t>(chunkPos.y()) << 21 |
               static_cast<uint64_t>(chunkPos.z());
    }
    
    void generateChunk(const Eigen::Vector3i& chunkPos) {
        auto chunk = std::make_unique<Chunk>(chunkPos);
        
        for (int x = 0; x < Chunk::SIZE; ++x) {
            for (int z = 0; z < Chunk::SIZE; ++z) {
                int worldX = chunkPos.x() * Chunk::SIZE + x;
                int worldZ = chunkPos.z() * Chunk::SIZE + z;
                
                // Generate terrain height using multiple noise octaves
                double height = 64 + 
                    32 * noiseGenerator.fbm(worldX * 0.01, 0, worldZ * 0.01, 4) +
                    8 * noiseGenerator.fbm(worldX * 0.05, 0, worldZ * 0.05, 3) +
                    2 * noiseGenerator.fbm(worldX * 0.1, 0, worldZ * 0.1, 2);
                
                for (int y = 0; y < Chunk::SIZE; ++y) {
                    int worldY = chunkPos.y() * Chunk::SIZE + y;
                    
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
                    
                    // Ore generation
                    if (worldY < 16) {
                        if (noiseGenerator.noise(worldX * 0.1, worldY * 0.1, 
                                                worldZ * 0.1) > 0.8) {
                            chunk->blocks[x][y][z] = BlockType::DIAMOND_ORE;
                        }
                    }
                    
                    // Update voxel octree
                    voxelWorld.setBlock(Eigen::Vector3i(worldX, worldY, worldZ),
                                       chunk->blocks[x][y][z]);
                }
            }
        }
        
        chunks[getChunkKey(chunkPos)] = std::move(chunk);
    }
    
public:
    GameWorld() {
        // Initialize crafting recipes
        initializeCraftingRecipes();
    }
    
    void initializeCraftingRecipes() {
        // Crafting table recipe
        CraftingSystem::Recipe tableRecipe;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                tableRecipe.pattern[i][j] = BlockType::WOOD;
            }
        }
        tableRecipe.result = BlockType::CRAFTING_TABLE;
        tableRecipe.count = 1;
        crafting.addRecipe(tableRecipe);
        
        // Furnace recipe
        CraftingSystem::Recipe furnaceRecipe;
        furnaceRecipe.pattern[0][0] = BlockType::STONE;
        furnaceRecipe.pattern[0][1] = BlockType::STONE;
        furnaceRecipe.pattern[0][2] = BlockType::STONE;
        furnaceRecipe.pattern[1][0] = BlockType::STONE;
        furnaceRecipe.pattern[1][2] = BlockType::STONE;
        furnaceRecipe.pattern[2][0] = BlockType::STONE;
        furnaceRecipe.pattern[2][1] = BlockType::STONE;
        furnaceRecipe.pattern[2][2] = BlockType::STONE;
        furnaceRecipe.result = BlockType::FURNACE;
        furnaceRecipe.count = 1;
        crafting.addRecipe(furnaceRecipe);
    }
    
    void update(double deltaTime) {
        physics.update(deltaTime);
        // Update spatial hash
        // Update lighting
        // Update AI
    }
    
    void generateWorld() {
        // Generate chunks in a radius around origin
        int chunkRadius = 4;
        for (int x = -chunkRadius; x <= chunkRadius; ++x) {
            for (int z = -chunkRadius; z <= chunkRadius; ++z) {
                generateChunk(Eigen::Vector3i(x, 0, z));
            }
        }
    }
    
    BlockType getBlock(const Eigen::Vector3i& pos) const {
        return voxelWorld.getBlock(pos).value_or(BlockType::AIR);
    }
    
    void setBlock(const Eigen::Vector3i& pos, BlockType block) {
        voxelWorld.setBlock(pos, block);
        
        // Update chunk storage
        Eigen::Vector3i chunkPos = pos / Chunk::SIZE;
        auto it = chunks.find(getChunkKey(chunkPos));
        
        if (it != chunks.end()) {
            Eigen::Vector3i localPos(
                std::abs(pos.x() % Chunk::SIZE),
                std::abs(pos.y() % Chunk::SIZE),
                std::abs(pos.z() % Chunk::SIZE)
            );
            it->second->blocks[localPos.x()][localPos.y()][localPos.z()] = block;
        }
    }
    
    // Raycast for block interaction
    std::optional<Eigen::Vector3i> raycastBlocks(const Eigen::Vector3d& origin,
                                                 const Eigen::Vector3d& direction,
                                                 double maxDistance = 10.0) const {
        return voxelWorld.raycast(origin, direction, maxDistance);
    }
    
    // Pathfinding API
    std::vector<Eigen::Vector3i> findPath(const Eigen::Vector3i& start,
                                          const Eigen::Vector3i& goal) {
        return pathfinding.findPath(start, goal, voxelWorld);
    }
    
    // Crafting API
    std::optional<std::pair<BlockType, int>> craft(
        const std::array<std::array<BlockType, 3>, 3>& grid) {
        return crafting.craft(grid);
    }
};

// ============================================================================
// PART 11: MAIN GAME LOOP
// ============================================================================

class SurvivalGame {
private:
    GameWorld world;
    bool running;
    double gameTime;
    
    // Player state
    struct Player {
        Eigen::Vector3d position;
        Eigen::Vector3d velocity;
        Quaternion<> orientation;
        double health;
        double hunger;
        double stamina;
        InventorySystem inventory;
        
        Player() : position(0, 100, 0), velocity(0, 0, 0),
                   health(100), hunger(100), stamina(100) {}
    };
    
    Player player;
    
    void handleInput() {
        // Simplified input handling
        // In real implementation, this would process keyboard/mouse input
    }
    
    void updatePlayer(double dt) {
        // Apply gravity
        player.velocity.y() -= 9.81 * dt;
        
        // Update position
        player.position += player.velocity * dt;
        
        // Check block collisions
        Eigen::Vector3i feetPos = (player.position - Eigen::Vector3d(0, 1, 0)).cast<int>();
        if (world.getBlock(feetPos) != BlockType::AIR) {
            player.position.y() = feetPos.y() + 2;
            player.velocity.y() = 0;
        }
        
        // Hunger and stamina depletion
        player.hunger -= 0.01 * dt;
        if (player.velocity.norm() > 5.0) {
            player.stamina -= 0.1 * dt;
        }
        
        // Health regeneration/damage
        if (player.hunger > 80 && player.health < 100) {
            player.health += 0.5 * dt;
        } else if (player.hunger < 10) {
            player.health -= 0.5 * dt;
        }
    }
    
    void render() {
        // Simplified rendering
        // In real implementation, this would use OpenGL/Vulkan/DirectX
        std::cout << "Rendering frame at time: " << gameTime << std::endl;
    }
    
public:
    SurvivalGame() : running(false), gameTime(0) {
        world.generateWorld();
    }
    
    void run() {
        running = true;
        const double timeStep = 1.0 / 60.0; // 60 FPS
        double accumulator = 0;
        auto lastTime = std::chrono::high_resolution_clock::now();
        
        while (running) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            double frameTime = std::chrono::duration<double>(currentTime - lastTime).count();
            lastTime = currentTime;
            
            accumulator += frameTime;
            
            while (accumulator >= timeStep) {
                handleInput();
                updatePlayer(timeStep);
                world.update(timeStep);
                gameTime += timeStep;
                accumulator -= timeStep;
            }
            
            render();
            
            // Simple frame rate limiting
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
    
    void stop() {
        running = false;
    }
};

// ============================================================================
// PART 12: DEMONSTRATION AND TESTING
// ============================================================================

int main() {
    std::cout << "=== 3D SURVIVAL CRAFTING GAME - MATHEMATICAL FRAMEWORK ===\n\n";
    
    // Test quaternion rotations
    std::cout << "Testing Quaternion Rotations:\n";
    Quaternion<> q = Quaternion<>::fromEuler(0, M_PI/4, 0); // 45° pitch
    Eigen::Vector3d v(1, 0, 0);
    Eigen::Vector3d rotated = q.rotate(v);
    std::cout << "Rotated vector: (" << rotated.x() << ", " 
              << rotated.y() << ", " << rotated.z() << ")\n\n";
    
    // Test Perlin noise
    std::cout << "Testing Perlin Noise:\n";
    PerlinNoise noise;
    for (int i = 0; i < 3; ++i) {
        double value = noise.fbm(i * 0.5, i * 0.3, i * 0.7, 4);
        std::cout << "Noise value " << i << ": " << value << "\n";
    }
    std::cout << "\n";
    
    // Test spatial hash
    std::cout << "Testing Spatial Hash:\n";
    SpatialHashGrid grid;
    Eigen::Vector3d pos1(10, 5, 3);
    Eigen::Vector3d pos2(15, 7, 8);
    uint64_t key1 = grid.getCellKey(pos1);
    uint64_t key2 = grid.getCellKey(pos2);
    std::cout << "Hash key 1: " << key1 << "\n";
    std::cout << "Hash key 2: " << key2 << "\n\n";
    
    // Test crafting system
    std::cout << "Testing Crafting System:\n";
    CraftingSystem crafting;
    CraftingSystem::Recipe recipe;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            recipe.pattern[i][j] = BlockType::WOOD;
        }
    }
    recipe.result = BlockType::CRAFTING_TABLE;
    crafting.addRecipe(recipe);
    
    std::array<std::array<BlockType, 3>, 3> testGrid;
    for (auto& row : testGrid) {
        row.fill(BlockType::WOOD);
    }
    
    auto result = crafting.craft(testGrid);
    if (result) {
        std::cout << "Crafting successful! Result: " 
                  << static_cast<int>(result->first) << "\n";
    }
    std::cout << "\n";
    
    // Test pathfinding
    std::cout << "Testing Pathfinding:\n";
    PathfindingSystem pathfinder;
    Eigen::Vector3i start(0, 64, 0);
    Eigen::Vector3i goal(10, 64, 10);
    std::cout << "Path from " << start.transpose() << " to " 
              << goal.transpose() << "\n";
    
    // Create and run game
    std::cout << "\n=== Initializing Survival Game ===\n";
    SurvivalGame game;
    
    std::cout << "\n=== Framework Summary ===\n";
    std::cout << "1. Voxel Octree - Spatial data structure for world storage\n";
    std::cout << "2. Perlin Noise - Procedural terrain generation\n";
    std::cout << "3. Spatial Hash - Efficient entity lookup\n";
    std::cout << "4. Quaternions - Rotation representation\n";
    std::cout << "5. D4 Group Theory - Crafting recipe optimization\n";
    std::cout << "6. A* Pathfinding - Navigation\n";
    std::cout << "7. Flow Fields - Group movement\n";
    std::cout << "8. Spherical Harmonics - Global illumination\n";
    std::cout << "9. Radiosity - Indirect lighting\n";
    std::cout << "10. Simplex Algorithm - Resource optimization\n";
    std::cout << "11. Knapsack DP - Inventory management\n";
    std::cout << "12. Impulse-based Physics - Collision response\n";
    std::cout << "13. Lagrange Multipliers - Constraint solving\n";
    std::cout << "14. Morton Code - Spatial indexing\n";
    std::cout << "15. DDA Algorithm - Ray casting\n";
    
    return 0;
}