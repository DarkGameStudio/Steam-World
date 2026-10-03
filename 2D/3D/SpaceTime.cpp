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
#include <type_traits>
#define M_PI 3.14
// ============================================================================
// PART 1: 12-DIMENSIONAL VECTOR IN XYZT MINKOWSKI SPACETIME
// ============================================================================

// Custom 12D Vector class for XYZT Minkowski spacetime
// Dimensions: X1, X2, X3 (spatial), T1, T2, T3 (temporal)
// Plus additional dimensions: ROW, COLUMN, DEPTH, LENGTH, WIDTH, HEIGHT
// Plus relationships: HORIZONTAL, VERTICAL, DIAGONAL, TOP, BOTTOM, SIDE

template<typename T = double>
class Vector12D {
public:
    // 12 dimensions of XYZT Minkowski spacetime
    union {
        struct {
            T x1, x2, x3;  // Spatial dimensions (X, Y, Z)
            T t1, t2, t3;  // Temporal dimensions (T)
            T row;          // Row dimension
            T column;       // Column dimension
            T depth;        // Depth dimension
            T length;       // Length dimension
            T width;        // Width dimension
            T height;       // Height dimension
        };
        std::array<T, 12> components;
    };
    
    // Constructors
    Vector12D() : components{} {}
    
    Vector12D(T x1_, T x2_, T x3_, T t1_, T t2_, T t3_,
              T row_ = 0, T column_ = 0, T depth_ = 0,
              T length_ = 0, T width_ = 0, T height_ = 0)
        : x1(x1_), x2(x2_), x3(x3_), t1(t1_), t2(t2_), t3(t3_),
          row(row_), column(column_), depth(depth_),
          length(length_), width(width_), height(height_) {}
    
    // Initialize from array
    Vector12D(const std::array<T, 12>& arr) : components(arr) {}
    
    // Operator overloads
    Vector12D operator+(const Vector12D& v) const {
        Vector12D result;
        for (int i = 0; i < 12; ++i) {
            result.components[i] = components[i] + v.components[i];
        }
        return result;
    }
    
    Vector12D operator-(const Vector12D& v) const {
        Vector12D result;
        for (int i = 0; i < 12; ++i) {
            result.components[i] = components[i] - v.components[i];
        }
        return result;
    }
    
    Vector12D operator*(T scalar) const {
        Vector12D result;
        for (int i = 0; i < 12; ++i) {
            result.components[i] = components[i] * scalar;
        }
        return result;
    }
    
    Vector12D operator/(T scalar) const {
        Vector12D result;
        for (int i = 0; i < 12; ++i) {
            result.components[i] = components[i] / scalar;
        }
        return result;
    }
    
    // Minkowski inner product (with metric signature)
    // η = diag(-1, -1, -1, +1, +1, +1, +1, +1, +1, +1, +1, +1)
    // Spatial dimensions have negative signature, temporal have positive
    T minkowskiDot(const Vector12D& v) const {
        T result = 0;
        // Spatial dimensions (negative signature)
        result -= x1 * v.x1 + x2 * v.x2 + x3 * v.x3;
        // Temporal dimensions (positive signature)
        result += t1 * v.t1 + t2 * v.t2 + t3 * v.t3;
        // Additional spatial-like dimensions (positive signature)
        result += row * v.row + column * v.column + depth * v.depth;
        result += length * v.length + width * v.width + height * v.height;
        return result;
    }
    
    // Euclidean inner product
    T euclideanDot(const Vector12D& v) const {
        T result = 0;
        for (int i = 0; i < 12; ++i) {
            result += components[i] * v.components[i];
        }
        return result;
    }
    
    // Minkowski norm (can be negative for timelike vectors)
    T minkowskiNorm() const {
        return minkowskiDot(*this);
    }
    
    // Euclidean norm
    T euclideanNorm() const {
        return std::sqrt(euclideanDot(*this));
    }
    
    // Check if vector is timelike, spacelike, or lightlike
    enum class VectorType { TIMELIKE, SPACELIKE, LIGHTLIKE };
    
    VectorType getVectorType() const {
        T norm = minkowskiNorm();
        if (norm > 0) return VectorType::TIMELIKE;
        if (norm < 0) return VectorType::SPACELIKE;
        return VectorType::LIGHTLIKE;
    }
    
    // Access operators
    T& operator[](int index) {
        return components[index];
    }
    
    const T& operator[](int index) const {
        return components[index];
    }
    
    // String representation
    std::string toString() const {
        std::ostringstream oss;
        oss << "(";
        for (int i = 0; i < 11; ++i) {
            oss << components[i] << ", ";
        }
        oss << components[11] << ")";
        return oss.str();
    }
};

// ============================================================================
// PART 2: SPATIAL RELATIONSHIP MATRIX
// ============================================================================

class SpatialRelationships {
public:
    // Define all possible spatial relationships
    enum class Relationship {
        ROW,           // Row-wise relationship
        COLUMN,        // Column-wise relationship
        DEPTH,         // Depth-wise relationship
        LENGTH,        // Length dimension relationship
        WIDTH,         // Width dimension relationship
        HEIGHT,        // Height dimension relationship
        HORIZONTAL,    // Horizontal relationship (X-axis)
        VERTICAL,      // Vertical relationship (Y-axis)
        DIAGONAL,      // Diagonal relationship (XY plane)
        ANTI_DIAGONAL, // Anti-diagonal relationship (XY plane)
        TOP,           // Top relationship (+Z)
        BOTTOM,        // Bottom relationship (-Z)
        SIDE_LEFT,     // Left side (-X)
        SIDE_RIGHT,    // Right side (+X)
        SIDE_FRONT,    // Front side (+Y)
        SIDE_BACK      // Back side (-Y)
    };
    
    // Relationship transformation matrix in 3D
    struct RelationshipTransform {
        std::array<std::array<double, 3>, 3> matrix;
        
        RelationshipTransform() {
            for (auto& row : matrix) {
                row.fill(0);
            }
            // Default: identity matrix
            matrix[0][0] = 1;
            matrix[1][1] = 1;
            matrix[2][2] = 1;
        }
    };
    
private:
    std::map<Relationship, RelationshipTransform> transforms;
    
public:
    SpatialRelationships() {
        initializeTransforms();
    }
    
    void initializeTransforms() {
        // Identity (no transformation)
        transforms[Relationship::ROW] = createIdentity();
        transforms[Relationship::COLUMN] = createIdentity();
        transforms[Relationship::DEPTH] = createIdentity();
        
        // Horizontal: rotation around Z-axis by 0° (X-axis)
        transforms[Relationship::HORIZONTAL] = createRotationZ(0);
        
        // Vertical: rotation around Z-axis by 90° (Y-axis)
        transforms[Relationship::VERTICAL] = createRotationZ(M_PI / 2);
        
        // Diagonal: rotation around Z-axis by 45°
        transforms[Relationship::DIAGONAL] = createRotationZ(M_PI / 4);
        
        // Anti-diagonal: rotation around Z-axis by -45°
        transforms[Relationship::ANTI_DIAGONAL] = createRotationZ(-M_PI / 4);
        
        // Top: reflection through XY plane (+Z direction)
        transforms[Relationship::TOP] = createReflectionZ(1);
        
        // Bottom: reflection through XY plane (-Z direction)
        transforms[Relationship::BOTTOM] = createReflectionZ(-1);
        
        // Side relationships
        transforms[Relationship::SIDE_LEFT] = createReflectionX(-1);
        transforms[Relationship::SIDE_RIGHT] = createReflectionX(1);
        transforms[Relationship::SIDE_FRONT] = createReflectionY(1);
        transforms[Relationship::SIDE_BACK] = createReflectionY(-1);
    }
    
    static RelationshipTransform createIdentity() {
        RelationshipTransform t;
        t.matrix[0][0] = 1;
        t.matrix[1][1] = 1;
        t.matrix[2][2] = 1;
        return t;
    }
    
    static RelationshipTransform createRotationZ(double angle) {
        RelationshipTransform t;
        t.matrix[0][0] = std::cos(angle);
        t.matrix[0][1] = -std::sin(angle);
        t.matrix[1][0] = std::sin(angle);
        t.matrix[1][1] = std::cos(angle);
        t.matrix[2][2] = 1;
        return t;
    }
    
    static RelationshipTransform createReflectionX(double sign) {
        RelationshipTransform t;
        t.matrix[0][0] = sign;
        t.matrix[1][1] = 1;
        t.matrix[2][2] = 1;
        return t;
    }
    
    static RelationshipTransform createReflectionY(double sign) {
        RelationshipTransform t;
        t.matrix[0][0] = 1;
        t.matrix[1][1] = sign;
        t.matrix[2][2] = 1;
        return t;
    }
    
    static RelationshipTransform createReflectionZ(double sign) {
        RelationshipTransform t;
        t.matrix[0][0] = 1;
        t.matrix[1][1] = 1;
        t.matrix[2][2] = sign;
        return t;
    }
    
public:
    // Apply relationship transformation to a 3D vector
    std::array<double, 3> applyTransform(Relationship rel, 
                                          const std::array<double, 3>& vec) {
        auto& transform = transforms[rel];
        std::array<double, 3> result{};
        
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                result[i] += transform.matrix[i][j] * vec[j];
            }
        }
        return result;
    }
    
    // Get relationship name as string
    static std::string getRelationshipName(Relationship rel) {
        switch (rel) {
            case Relationship::ROW: return "Row";
            case Relationship::COLUMN: return "Column";
            case Relationship::DEPTH: return "Depth";
            case Relationship::LENGTH: return "Length";
            case Relationship::WIDTH: return "Width";
            case Relationship::HEIGHT: return "Height";
            case Relationship::HORIZONTAL: return "Horizontal";
            case Relationship::VERTICAL: return "Vertical";
            case Relationship::DIAGONAL: return "Diagonal";
            case Relationship::ANTI_DIAGONAL: return "Anti-Diagonal";
            case Relationship::TOP: return "Top";
            case Relationship::BOTTOM: return "Bottom";
            case Relationship::SIDE_LEFT: return "Side Left";
            case Relationship::SIDE_RIGHT: return "Side Right";
            case Relationship::SIDE_FRONT: return "Side Front";
            case Relationship::SIDE_BACK: return "Side Back";
            default: return "Unknown";
        }
    }
};

// ============================================================================
// PART 3: 12D MINKOWSKI SPACETIME TENSOR
// ============================================================================

class MinkowskiSpacetime {
private:
    // Metric tensor for 12D Minkowski spacetime
    // η_μν = diag(-1, -1, -1, +1, +1, +1, +1, +1, +1, +1, +1, +1)
    std::array<std::array<double, 12>, 12> metricTensor;
    
    // Connection coefficients (Christoffel symbols) for flat spacetime
    std::array<std::array<std::array<double, 12>, 12>, 12> christoffelSymbols;
    
public:
    MinkowskiSpacetime() {
        initializeMetric();
        initializeChristoffelSymbols();
    }
    
    void initializeMetric() {
        // Initialize all to zero
        for (auto& row : metricTensor) {
            row.fill(0);
        }
        
        // Spatial dimensions (indices 0-2): -1
        for (int i = 0; i < 3; ++i) {
            metricTensor[i][i] = -1;
        }
        
        // Temporal dimensions (indices 3-5): +1
        for (int i = 3; i < 6; ++i) {
            metricTensor[i][i] = 1;
        }
        
        // Additional dimensions (indices 6-11): +1
        for (int i = 6; i < 12; ++i) {
            metricTensor[i][i] = 1;
        }
    }
    
    void initializeChristoffelSymbols() {
        // For flat Minkowski spacetime, all Christoffel symbols are zero
        for (auto& matrix : christoffelSymbols) {
            for (auto& row : matrix) {
                row.fill(0);
            }
        }
    }
    
    // Get metric tensor element
    double getMetric(int mu, int nu) const {
        return metricTensor[mu][nu];
    }
    
    // Lower an index (covariant)
    Vector12D<> lowerIndex(const Vector12D<>& vec) const {
        Vector12D<> result;
        for (int mu = 0; mu < 12; ++mu) {
            for (int nu = 0; nu < 12; ++nu) {
                result[mu] += metricTensor[mu][nu] * vec[nu];
            }
        }
        return result;
    }
    
    // Raise an index (contravariant)
    Vector12D<> raiseIndex(const Vector12D<>& vec) const {
        // For Minkowski metric, raising and lowering are the same operation
        return lowerIndex(vec);
    }
    
    // Calculate proper time along a worldline
    double calculateProperTime(const std::vector<Vector12D<>>& worldline) const {
        double properTime = 0;
        
        for (size_t i = 1; i < worldline.size(); ++i) {
            Vector12D<> dx = worldline[i] - worldline[i-1];
            double ds2 = dx.minkowskiDot(dx);
            
            if (ds2 > 0) { // Timelike separation
                properTime += std::sqrt(ds2);
            }
        }
        
        return properTime;
    }
    
    // Calculate spacetime interval
    double calculateInterval(const Vector12D<>& a, const Vector12D<>& b) const {
        Vector12D<> dx = b - a;
        return dx.minkowskiDot(dx);
    }
    
    // Check causality
    enum class CausalRelation {
        TIMELIKE_SEPARATED,   // Can influence each other
        SPACELIKE_SEPARATED,  // Cannot influence each other
        LIGHTLIKE_SEPARATED   // On the light cone
    };
    
    CausalRelation checkCausality(const Vector12D<>& a, const Vector12D<>& b) const {
        double interval = calculateInterval(a, b);
        
        if (interval > 0) return CausalRelation::TIMELIKE_SEPARATED;
        if (interval < 0) return CausalRelation::SPACELIKE_SEPARATED;
        return CausalRelation::LIGHTLIKE_SEPARATED;
    }
};

// ============================================================================
// PART 4: 12D LORENTZ TRANSFORMATIONS
// ============================================================================

class LorentzTransform12D {
private:
    std::array<std::array<double, 12>, 12> transformationMatrix;
    
public:
    LorentzTransform12D() {
        // Initialize as identity
        for (int i = 0; i < 12; ++i) {
            for (int j = 0; j < 12; ++j) {
                transformationMatrix[i][j] = (i == j) ? 1.0 : 0.0;
            }
        }
    }
    
    // Boost along X1 axis
    static LorentzTransform12D boostX1(double velocity) {
        LorentzTransform12D transform;
        double c = 1.0; // Speed of light in natural units
        double gamma = 1.0 / std::sqrt(1.0 - velocity * velocity / (c * c));
        double beta = velocity / c;
        
        // Boost matrix for X1 and T1 dimensions
        transform.transformationMatrix[0][0] = gamma;
        transform.transformationMatrix[0][3] = -gamma * beta;
        transform.transformationMatrix[3][0] = -gamma * beta;
        transform.transformationMatrix[3][3] = gamma;
        
        return transform;
    }
    
    // Boost along X2 axis
    static LorentzTransform12D boostX2(double velocity) {
        LorentzTransform12D transform;
        double c = 1.0;
        double gamma = 1.0 / std::sqrt(1.0 - velocity * velocity / (c * c));
        double beta = velocity / c;
        
        transform.transformationMatrix[1][1] = gamma;
        transform.transformationMatrix[1][4] = -gamma * beta;
        transform.transformationMatrix[4][1] = -gamma * beta;
        transform.transformationMatrix[4][4] = gamma;
        
        return transform;
    }
    
    // Boost along X3 axis
    static LorentzTransform12D boostX3(double velocity) {
        LorentzTransform12D transform;
        double c = 1.0;
        double gamma = 1.0 / std::sqrt(1.0 - velocity * velocity / (c * c));
        double beta = velocity / c;
        
        transform.transformationMatrix[2][2] = gamma;
        transform.transformationMatrix[2][5] = -gamma * beta;
        transform.transformationMatrix[5][2] = -gamma * beta;
        transform.transformationMatrix[5][5] = gamma;
        
        return transform;
    }
    
    // General boost
    static LorentzTransform12D boost(const Vector12D<>& velocity) {
        LorentzTransform12D transform;
        double c = 1.0;
        
        // Apply boosts sequentially
        transform = transform * boostX1(velocity.x1);
        transform = transform * boostX2(velocity.x2);
        transform = transform * boostX3(velocity.x3);
        
        return transform;
    }
    
    // Apply transformation to a vector
    Vector12D<> apply(const Vector12D<>& vec) const {
        Vector12D<> result;
        
        for (int i = 0; i < 12; ++i) {
            result[i] = 0;
            for (int j = 0; j < 12; ++j) {
                result[i] += transformationMatrix[i][j] * vec[j];
            }
        }
        
        return result;
    }
    
    // Compose transformations (matrix multiplication)
    LorentzTransform12D operator*(const LorentzTransform12D& other) const {
        LorentzTransform12D result;
        
        for (int i = 0; i < 12; ++i) {
            for (int j = 0; j < 12; ++j) {
                result.transformationMatrix[i][j] = 0;
                for (int k = 0; k < 12; ++k) {
                    result.transformationMatrix[i][j] += 
                        transformationMatrix[i][k] * other.transformationMatrix[k][j];
                }
            }
        }
        
        return result;
    }
    
    // Get transformation matrix element
    double getElement(int i, int j) const {
        return transformationMatrix[i][j];
    }
};

// ============================================================================
// PART 5: SPATIAL RELATIONSHIP DETECTOR
// ============================================================================

class SpatialRelationshipDetector {
private:
    SpatialRelationships relationships;
    MinkowskiSpacetime spacetime;
    
public:
    // Detect relationship between two objects in 12D spacetime
    struct RelationshipInfo {
        SpatialRelationships::Relationship primaryRelation;
        double strength;
        Vector12D<> directionVector;
        double spacetimeInterval;
        MinkowskiSpacetime::CausalRelation causalRelation;
    };
    
    RelationshipInfo detectRelationship(const Vector12D<>& obj1, 
                                        const Vector12D<>& obj2) {
        RelationshipInfo info;
        
        // Calculate difference vector
        Vector12D<> diff = obj2 - obj1;
        info.directionVector = diff;
        
        // Calculate spacetime interval
        info.spacetimeInterval = spacetime.calculateInterval(obj1, obj2);
        
        // Check causality
        info.causalRelation = spacetime.checkCausality(obj1, obj2);
        
        // Determine primary spatial relationship
        info.primaryRelation = determinePrimaryRelation(diff);
        
        // Calculate relationship strength
        info.strength = calculateRelationshipStrength(diff);
        
        return info;
    }
    
private:
    SpatialRelationships::Relationship determinePrimaryRelation(
        const Vector12D<>& diff) {
        
        // Extract 3D spatial components
        std::array<double, 3> spatialVec = {diff.x1, diff.x2, diff.x3};
        
        // Calculate magnitude in different orientations
        double horizontalMag = std::abs(spatialVec[0]); // X-axis
        double verticalMag = std::abs(spatialVec[1]);   // Y-axis
        double depthMag = std::abs(spatialVec[2]);      // Z-axis
        
        // Diagonal components
        double diagXY = std::abs(spatialVec[0] - spatialVec[1]);
        double diagXZ = std::abs(spatialVec[0] - spatialVec[2]);
        double diagYZ = std::abs(spatialVec[1] - spatialVec[2]);
        
        // Determine dominant relationship
        double maxMag = std::max({horizontalMag, verticalMag, depthMag, 
                                  diagXY, diagXZ, diagYZ});
        
        if (maxMag == horizontalMag) {
            return (spatialVec[0] > 0) ? 
                SpatialRelationships::Relationship::SIDE_RIGHT :
                SpatialRelationships::Relationship::SIDE_LEFT;
        } else if (maxMag == verticalMag) {
            return (spatialVec[1] > 0) ? 
                SpatialRelationships::Relationship::SIDE_FRONT :
                SpatialRelationships::Relationship::SIDE_BACK;
        } else if (maxMag == depthMag) {
            return (spatialVec[2] > 0) ? 
                SpatialRelationships::Relationship::TOP :
                SpatialRelationships::Relationship::BOTTOM;
        } else if (maxMag == diagXY) {
            return SpatialRelationships::Relationship::DIAGONAL;
        } else if (maxMag == diagXZ || maxMag == diagYZ) {
            return SpatialRelationships::Relationship::ANTI_DIAGONAL;
        }
        
        return SpatialRelationships::Relationship::ROW;
    }
    
    double calculateRelationshipStrength(const Vector12D<>& diff) {
        // Calculate Euclidean distance in 12D
        double distance = diff.euclideanNorm();
        
        // Normalize to [0, 1] range
        return 1.0 / (1.0 + distance);
    }
};

// ============================================================================
// PART 6: 12D TENSOR FIELD
// ============================================================================

class TensorField12D {
public:
    // Rank-2 tensor field in 12D spacetime
    class Rank2Tensor {
    private:
        std::array<std::array<double, 12>, 12> components;
        
    public:
        Rank2Tensor() {
            for (auto& row : components) {
                row.fill(0);
            }
        }
        
        double& operator()(int mu, int nu) {
            return components[mu][nu];
        }
        
        const double& operator()(int mu, int nu) const {
            return components[mu][nu];
        }
        
        // Tensor contraction
        double contract(const Rank2Tensor& other) const {
            double result = 0;
            for (int mu = 0; mu < 12; ++mu) {
                for (int nu = 0; nu < 12; ++nu) {
                    result += components[mu][nu] * other.components[mu][nu];
                }
            }
            return result;
        }
        
        // Tensor trace
        double trace() const {
            double result = 0;
            for (int mu = 0; mu < 12; ++mu) {
                result += components[mu][mu];
            }
            return result;
        }
        
        // Symmetric part
        Rank2Tensor symmetricPart() const {
            Rank2Tensor result;
            for (int mu = 0; mu < 12; ++mu) {
                for (int nu = 0; nu < 12; ++nu) {
                    result(mu, nu) = 0.5 * (components[mu][nu] + components[nu][mu]);
                }
            }
            return result;
        }
        
        // Antisymmetric part
        Rank2Tensor antisymmetricPart() const {
            Rank2Tensor result;
            for (int mu = 0; mu < 12; ++mu) {
                for (int nu = 0; nu < 12; ++nu) {
                    result(mu, nu) = 0.5 * (components[mu][nu] - components[nu][mu]);
                }
            }
            return result;
        }
    };
    
private:
    std::map<std::array<int, 12>, Rank2Tensor> field;
    
public:
    // Set tensor at a point in 12D spacetime
    void setTensor(const Vector12D<>& position, const Rank2Tensor& tensor) {
        std::array<int, 12> key;
        for (int i = 0; i < 12; ++i) {
            key[i] = static_cast<int>(position[i]);
        }
        field[key] = tensor;
    }
    
    // Get tensor at a point
    std::optional<Rank2Tensor> getTensor(const Vector12D<>& position) const {
        std::array<int, 12> key;
        for (int i = 0; i < 12; ++i) {
            key[i] = static_cast<int>(position[i]);
        }
        
        auto it = field.find(key);
        if (it != field.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    // Calculate field divergence
    Vector12D<> calculateDivergence(const Vector12D<>& position) const {
        Vector12D<> divergence;
        const double h = 0.001; // Small step for numerical differentiation
        
        for (int mu = 0; mu < 12; ++mu) {
            Vector12D<> posPlus = position;
            Vector12D<> posMinus = position;
            posPlus[mu] += h;
            posMinus[mu] -= h;
            
            auto tensorPlus = getTensor(posPlus);
            auto tensorMinus = getTensor(posMinus);
            
            if (tensorPlus && tensorMinus) {
                for (int nu = 0; nu < 12; ++nu) {
                    divergence[nu] += (tensorPlus->operator()(mu, nu) - 
                                      tensorMinus->operator()(mu, nu)) / (2 * h);
                }
            }
        }
        
        return divergence;
    }
};

// ============================================================================
// PART 7: DIMENSIONAL ANALYSIS SYSTEM
// ============================================================================

class DimensionalAnalysis {
public:
    // Define all 12 dimensions with their meanings
    enum class Dimension {
        X1,     // First spatial dimension (length)
        X2,     // Second spatial dimension (width)
        X3,     // Third spatial dimension (height)
        T1,     // First temporal dimension (time)
        T2,     // Second temporal dimension (imaginary time)
        T3,     // Third temporal dimension (proper time)
        ROW,    // Row dimension (for matrices/grids)
        COLUMN, // Column dimension (for matrices/grids)
        DEPTH,  // Depth dimension (3D space)
        LENGTH, // Length dimension (measurement)
        WIDTH,  // Width dimension (measurement)
        HEIGHT  // Height dimension (measurement)
    };
    
    struct DimensionalFormula {
        std::array<int, 12> powers; // Power of each dimension
        
        DimensionalFormula() {
            powers.fill(0);
        }
        
        // Multiply two formulas (add powers)
        DimensionalFormula operator*(const DimensionalFormula& other) const {
            DimensionalFormula result;
            for (int i = 0; i < 12; ++i) {
                result.powers[i] = powers[i] + other.powers[i];
            }
            return result;
        }
        
        // Divide two formulas (subtract powers)
        DimensionalFormula operator/(const DimensionalFormula& other) const {
            DimensionalFormula result;
            for (int i = 0; i < 12; ++i) {
                result.powers[i] = powers[i] - other.powers[i];
            }
            return result;
        }
        
        // Check if dimensionless
        bool isDimensionless() const {
            for (int i = 0; i < 12; ++i) {
                if (powers[i] != 0) return false;
            }
            return true;
        }
    };
    
private:
    std::map<std::string, DimensionalFormula> physicalQuantities;
    
public:
    DimensionalAnalysis() {
        initializeCommonQuantities();
    }
    
    void initializeCommonQuantities() {
        // Length [L]
        DimensionalFormula length;
        length.powers[static_cast<int>(Dimension::X1)] = 1;
        physicalQuantities["length"] = length;
        physicalQuantities["distance"] = length;
        physicalQuantities["position"] = length;
        
        // Width [W]
        DimensionalFormula width;
        width.powers[static_cast<int>(Dimension::X2)] = 1;
        physicalQuantities["width"] = width;
        
        // Height [H]
        DimensionalFormula height;
        height.powers[static_cast<int>(Dimension::X3)] = 1;
        physicalQuantities["height"] = height;
        
        // Time [T]
        DimensionalFormula time;
        time.powers[static_cast<int>(Dimension::T1)] = 1;
        physicalQuantities["time"] = time;
        physicalQuantities["duration"] = time;
        
        // Velocity [L/T]
        DimensionalFormula velocity = length / time;
        physicalQuantities["velocity"] = velocity;
        physicalQuantities["speed"] = velocity;
        
        // Acceleration [L/T²]
        DimensionalFormula acceleration = velocity / time;
        physicalQuantities["acceleration"] = acceleration;
        
        // Volume [L³]
        DimensionalFormula volume = length * length * length;
        physicalQuantities["volume"] = volume;
        
        // Area [L²]
        DimensionalFormula area = length * length;
        physicalQuantities["area"] = area;
    }
    
    // Check dimensional consistency
    bool checkConsistency(const std::string& quantity, 
                          const DimensionalFormula& formula) {
        auto it = physicalQuantities.find(quantity);
        if (it == physicalQuantities.end()) {
            return false;
        }
        
        for (int i = 0; i < 12; ++i) {
            if (it->second.powers[i] != formula.powers[i]) {
                return false;
            }
        }
        return true;
    }
    
    // Get dimensional formula
    DimensionalFormula getFormula(const std::string& quantity) const {
        auto it = physicalQuantities.find(quantity);
        if (it != physicalQuantities.end()) {
            return it->second;
        }
        return DimensionalFormula();
    }
};

// ============================================================================
// PART 8: VISUALIZATION AND ANALYSIS TOOLS
// ============================================================================

class SpacetimeVisualizer {
public:
    struct SpacetimePoint {
        Vector12D<> position;
        std::string label;
        double intensity;
        
        SpacetimePoint(const Vector12D<>& pos, 
                      const std::string& lbl = "", 
                      double inten = 1.0)
            : position(pos), label(lbl), intensity(inten) {}
    };
    
private:
    std::vector<SpacetimePoint> points;
    SpatialRelationshipDetector detector;
    
public:
    void addPoint(const Vector12D<>& position, 
                  const std::string& label = "", 
                  double intensity = 1.0) {
        points.emplace_back(position, label, intensity);
    }
    
    // Analyze relationships between all points
    void analyzeRelationships() {
        std::cout << "\n=== SPACETIME RELATIONSHIP ANALYSIS ===\n";
        
        for (size_t i = 0; i < points.size(); ++i) {
            for (size_t j = i + 1; j < points.size(); ++j) {
                auto relationship = detector.detectRelationship(
                    points[i].position, points[j].position);
                
                std::cout << "\nRelationship between " 
                          << (points[i].label.empty() ? "Point " + std::to_string(i) 
                                                      : points[i].label)
                          << " and "
                          << (points[j].label.empty() ? "Point " + std::to_string(j) 
                                                      : points[j].label)
                          << ":\n";
                std::cout << "  Primary Relation: " 
                          << SpatialRelationships::getRelationshipName(
                              relationship.primaryRelation) << "\n";
                std::cout << "  Strength: " << relationship.strength << "\n";
                std::cout << "  Spacetime Interval: " 
                          << relationship.spacetimeInterval << "\n";
                
                switch (relationship.causalRelation) {
                    case MinkowskiSpacetime::CausalRelation::TIMELIKE_SEPARATED:
                        std::cout << "  Causal Relation: Timelike (can interact)\n";
                        break;
                    case MinkowskiSpacetime::CausalRelation::SPACELIKE_SEPARATED:
                        std::cout << "  Causal Relation: Spacelike (cannot interact)\n";
                        break;
                    case MinkowskiSpacetime::CausalRelation::LIGHTLIKE_SEPARATED:
                        std::cout << "  Causal Relation: Lightlike (on light cone)\n";
                        break;
                }
            }
        }
    }
    
    // Project 12D points to 3D for visualization
    std::vector<std::array<double, 3>> projectTo3D(int xDim, int yDim, int zDim) {
        std::vector<std::array<double, 3>> projected;
        
        for (const auto& point : points) {
            projected.push_back({
                point.position[xDim],
                point.position[yDim],
                point.position[zDim]
            });
        }
        
        return projected;
    }
    
    // Print all points
    void printPoints() {
        std::cout << "\n=== SPACETIME POINTS ===\n";
        for (size_t i = 0; i < points.size(); ++i) {
            std::cout << "Point " << i 
                      << (points[i].label.empty() ? "" : " (" + points[i].label + ")")
                      << ": " << points[i].position.toString() << "\n";
        }
    }
};

// ============================================================================
// PART 9: MAIN DEMONSTRATION
// ============================================================================

int main() {
    std::cout << "===========================================================\n";
    std::cout << "12-DIMENSIONAL XYZT MINKOWSKI SPACETIME FRAMEWORK\n";
    std::cout << "===========================================================\n\n";
    
    // 1. Create 12D vectors
    std::cout << "1. CREATING 12D SPACETIME VECTORS\n";
    std::cout << "=================================\n";
    
    Vector12D<> origin(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    Vector12D<> pointA(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
    Vector12D<> pointB(-1, 1, -2, 3, 2, 1, 0, 0, 0, 1, 2, 3);
    
    std::cout << "Origin: " << origin.toString() << "\n";
    std::cout << "Point A: " << pointA.toString() << "\n";
    std::cout << "Point B: " << pointB.toString() << "\n\n";
    
    // 2. Test Minkowski operations
    std::cout << "2. MINKOWSKI OPERATIONS\n";
    std::cout << "=======================\n";
    
    double interval = pointA.minkowskiDot(pointB);
    std::cout << "Minkowski inner product (A·B): " << interval << "\n";
    
    double euclideanDist = (pointA - pointB).euclideanNorm();
    std::cout << "Euclidean distance |A-B|: " << euclideanDist << "\n";
    
    std::cout << "Vector A type: ";
    switch (pointA.getVectorType()) {
        case Vector12D<>::VectorType::TIMELIKE:
            std::cout << "Timelike\n";
            break;
        case Vector12D<>::VectorType::SPACELIKE:
            std::cout << "Spacelike\n";
            break;
        case Vector12D<>::VectorType::LIGHTLIKE:
            std::cout << "Lightlike\n";
            break;
    }
    std::cout << "\n";
    
    // 3. Test spatial relationships
    std::cout << "3. SPATIAL RELATIONSHIPS\n";
    std::cout << "========================\n";
    
    SpatialRelationships relationships;
    std::array<double, 3> testVec = {1, 1, 1};
    
    std::cout << "Original vector: (" 
              << testVec[0] << ", " << testVec[1] << ", " << testVec[2] << ")\n\n";
    
    // Apply different transformations
    auto relationships_list = {
        SpatialRelationships::Relationship::HORIZONTAL,
        SpatialRelationships::Relationship::VERTICAL,
        SpatialRelationships::Relationship::DIAGONAL,
        SpatialRelationships::Relationship::TOP,
        SpatialRelationships::Relationship::BOTTOM
    };
    
    for (auto rel : relationships_list) {
        auto transformed = relationships.applyTransform(rel, testVec);
        std::cout << SpatialRelationships::getRelationshipName(rel) 
                  << ": (" << transformed[0] << ", " 
                  << transformed[1] << ", " << transformed[2] << ")\n";
    }
    std::cout << "\n";
    
    // 4. Test Minkowski spacetime
    std::cout << "4. MINKOWSKI SPACETIME\n";
    std::cout << "======================\n";
    
    MinkowskiSpacetime spacetime;
    
    std::cout << "Metric tensor diagonal elements:\n";
    for (int i = 0; i < 12; ++i) {
        std::cout << "  η[" << i << "][" << i << "] = " 
                  << spacetime.getMetric(i, i) << "\n";
    }
    std::cout << "\n";
    
    double spacetimeInterval = spacetime.calculateInterval(pointA, pointB);
    std::cout << "Spacetime interval between A and B: " 
              << spacetimeInterval << "\n";
    
    auto causal = spacetime.checkCausality(pointA, pointB);
    std::cout << "Causal relation: ";
    switch (causal) {
        case MinkowskiSpacetime::CausalRelation::TIMELIKE_SEPARATED:
            std::cout << "Timelike separated\n";
            break;
        case MinkowskiSpacetime::CausalRelation::SPACELIKE_SEPARATED:
            std::cout << "Spacelike separated\n";
            break;
        case MinkowskiSpacetime::CausalRelation::LIGHTLIKE_SEPARATED:
            std::cout << "Lightlike separated\n";
            break;
    }
    std::cout << "\n";
    
    // 5. Test Lorentz transformations
    std::cout << "5. LORENTZ TRANSFORMATIONS\n";
    std::cout << "==========================\n";
    
    double velocity = 0.5; // 50% speed of light
    auto boost = LorentzTransform12D::boostX1(velocity);
    
    Vector12D<> spacetimeEvent(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    Vector12D<> boostedEvent = boost.apply(spacetimeEvent);
    
    std::cout << "Original event: " << spacetimeEvent.toString() << "\n";
    std::cout << "Boosted event (v=0.5c along X1): " 
              << boostedEvent.toString() << "\n";
    std::cout << "\n";
    
    // 6. Test relationship detection
    std::cout << "6. RELATIONSHIP DETECTION\n";
    std::cout << "=========================\n";
    
    SpacetimeVisualizer visualizer;
    
    visualizer.addPoint(origin, "Origin");
    visualizer.addPoint(Vector12D<>(10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), 
                       "Right of Origin");
    visualizer.addPoint(Vector12D<>(0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0), 
                       "Above Origin");
    visualizer.addPoint(Vector12D<>(0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0), 
                       "Future of Origin");
    visualizer.addPoint(Vector12D<>(5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0), 
                       "Diagonal from Origin");
    
    visualizer.printPoints();
    visualizer.analyzeRelationships();
    
    // 7. Test tensor operations
    std::cout << "\n7. TENSOR OPERATIONS\n";
    std::cout << "====================\n";
    
    TensorField12D::Rank2Tensor tensor1;
    TensorField12D::Rank2Tensor tensor2;
    
    // Create some tensors
    for (int i = 0; i < 12; ++i) {
        tensor1(i, i) = 1.0;  // Identity tensor
        tensor2(i, i) = 2.0;  // 2*Identity tensor
    }
    
    double contraction = tensor1.contract(tensor2);
    double trace1 = tensor1.trace();
    double trace2 = tensor2.trace();
    
    std::cout << "Tensor contraction: " << contraction << "\n";
    std::cout << "Trace of tensor 1: " << trace1 << "\n";
    std::cout << "Trace of tensor 2: " << trace2 << "\n";
    std::cout << "\n";
    
    // 8. Dimensional analysis
    std::cout << "8. DIMENSIONAL ANALYSIS\n";
    std::cout << "=======================\n";
    
    DimensionalAnalysis analysis;
    
    auto lengthFormula = analysis.getFormula("length");
    auto velocityFormula = analysis.getFormula("velocity");
    auto accelerationFormula = analysis.getFormula("acceleration");
    
    std::cout << "Length formula powers: [";
    for (int i = 0; i < 12; ++i) {
        std::cout << lengthFormula.powers[i];
        if (i < 11) std::cout << ", ";
    }
    std::cout << "]\n";
    
    std::cout << "Velocity formula powers: [";
    for (int i = 0; i < 12; ++i) {
        std::cout << velocityFormula.powers[i];
        if (i < 11) std::cout << ", ";
    }
    std::cout << "]\n";
    
    std::cout << "\n===========================================================\n";
    std::cout << "12D SPACETIME FRAMEWORK SUMMARY\n";
    std::cout << "===========================================================\n";
    std::cout << "Dimensions:\n";
    std::cout << "  Spatial (X1, X2, X3): 3 dimensions\n";
    std::cout << "  Temporal (T1, T2, T3): 3 dimensions\n";
    std::cout << "  Grid (Row, Column, Depth): 3 dimensions\n";
    std::cout << "  Measurement (Length, Width, Height): 3 dimensions\n";
    std::cout << "Total: 12 dimensions\n";
    std::cout << "\n";
    std::cout << "Spatial Relationships:\n";
    std::cout << "  - Row, Column, Depth\n";
    std::cout << "  - Length, Width, Height\n";
    std::cout << "  - Horizontal, Vertical, Diagonal\n";
    std::cout << "  - Anti-Diagonal\n";
    std::cout << "  - Top, Bottom\n";
    std::cout << "  - Side (Left, Right, Front, Back)\n";
    std::cout << "\n";
    std::cout << "Mathematical Structures:\n";
    std::cout << "  ✓ Minkowski Metric Tensor (12×12)\n";
    std::cout << "  ✓ Lorentz Transformations\n";
    std::cout << "  ✓ Christoffel Symbols (flat spacetime)\n";
    std::cout << "  ✓ Tensor Fields\n";
    std::cout << "  ✓ Dimensional Analysis\n";
    std::cout << "  ✓ Spatial Relationship Detection\n";
    std::cout << "  ✓ Causality Analysis\n";
    
    return 0;
}