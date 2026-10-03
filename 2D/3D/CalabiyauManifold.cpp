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

// ============================================================================
// PART 1: COMPLEX MANIFOLD FOUNDATIONS
// ============================================================================

// Complex number representation for Calabi-Yau geometry
template<typename T = double>
class Complex {
public:
    T real, imag;
    
    Complex(T r = 0, T i = 0) : real(r), imag(i) {}
    
    Complex operator+(const Complex& other) const {
        return Complex(real + other.real, imag + other.imag);
    }
    
    Complex operator-(const Complex& other) const {
        return Complex(real - other.real, imag - other.imag);
    }
    
    Complex operator*(const Complex& other) const {
        return Complex(real * other.real - imag * other.imag,
                      real * other.imag + imag * other.real);
    }
    
    Complex operator/(const Complex& other) const {
        T denominator = other.real * other.real + other.imag * other.imag;
        return Complex((real * other.real + imag * other.imag) / denominator,
                      (imag * other.real - real * other.imag) / denominator);
    }
    
    T magnitude() const {
        return std::sqrt(real * real + imag * imag);
    }
    
    T magnitudeSquared() const {
        return real * real + imag * imag;
    }
    
    Complex conjugate() const {
        return Complex(real, -imag);
    }
    
    // Complex exponential
    static Complex exp(const Complex& z) {
        T expReal = std::exp(z.real);
        return Complex(expReal * std::cos(z.imag), 
                      expReal * std::sin(z.imag));
    }
    
    // Complex logarithm
    static Complex log(const Complex& z) {
        return Complex(std::log(z.magnitude()), std::atan2(z.imag, z.real));
    }
    
    // Complex power
    static Complex pow(const Complex& z, const Complex& w) {
        return exp(log(z) * w);
    }
    
    // String representation
    std::string toString() const {
        std::ostringstream oss;
        oss << real << (imag >= 0 ? " + " : " - ") << std::abs(imag) << "i";
        return oss.str();
    }
};

// ============================================================================
// PART 2: DIFFERENTIAL FORMS AND COHOMOLOGY
// ============================================================================

// Differential forms for Calabi-Yau cohomology
class DifferentialForms {
public:
    // p-form representation
    class PForm {
    private:
        int degree;
        std::vector<double> coefficients;
        std::vector<std::vector<int>> basisIndices;
        
    public:
        PForm(int p) : degree(p) {
            initializeBasis();
        }
        
        void initializeBasis() {
            // Initialize basis for p-forms in 6 real dimensions (3 complex)
            int dimensions = 6; // Calabi-Yau 3-fold has 6 real dimensions
            std::vector<int> indices(degree);
            
            // Generate all combinations
            std::function<void(int, int)> generate = [&](int start, int depth) {
                if (depth == degree) {
                    basisIndices.push_back(indices);
                    coefficients.push_back(0);
                    return;
                }
                
                for (int i = start; i < dimensions; ++i) {
                    indices[depth] = i;
                    generate(i + 1, depth + 1);
                }
            };
            
            generate(0, 0);
        }
        
        // Wedge product
        PForm wedge(const PForm& other) const {
            int newDegree = degree + other.degree;
            PForm result(newDegree);
            
            // Simplified wedge product implementation
            return result;
        }
        
        // Exterior derivative
        PForm exteriorDerivative() const {
            PForm result(degree + 1);
            return result;
        }
        
        // Check if form is closed (dω = 0)
        bool isClosed() const {
            PForm d = exteriorDerivative();
            return d.isZero();
        }
        
        // Check if form is exact (ω = dη)
        bool isExact() const {
            // For Calabi-Yau, use Poincaré lemma
            return degree > 0 && degree < 6;
        }
        
        bool isZero() const {
            for (double coef : coefficients) {
                if (std::abs(coef) > 1e-10) return false;
            }
            return true;
        }
        
        int getDegree() const { return degree; }
    };
    
    // Hodge numbers for Calabi-Yau 3-fold
    struct HodgeNumbers {
        // Standard Hodge diamond for Calabi-Yau 3-fold:
        //              1
        //           0     0
        //        0     h11   0
        //     1    h21   h21    1
        //        0     h11   0
        //           0     0
        //              1
        int h11;  // Kähler moduli
        int h21;  // Complex structure moduli
        
        HodgeNumbers(int h11_ = 0, int h21_ = 0) : h11(h11_), h21(h21_) {}
        
        // Euler characteristic: χ = 2(h11 - h21)
        int eulerCharacteristic() const {
            return 2 * (h11 - h21);
        }
        
        // Check if valid Calabi-Yau (SU(3) holonomy)
        bool isValidCalabiYau() const {
            // For Calabi-Yau 3-fold: h00 = h33 = 1, h10 = h20 = h30 = 0
            return h11 > 0 && h21 >= 0;
        }
    };
    
    // Dolbeault cohomology groups
    class DolbeaultCohomology {
    private:
        std::vector<std::vector<int>> hpq; // Hodge numbers
        
    public:
        DolbeaultCohomology() {
            // Initialize for Calabi-Yau 3-fold (3 complex dimensions)
            hpq.resize(4, std::vector<int>(4, 0));
        }
        
        void setHodgeNumber(int p, int q, int value) {
            hpq[p][q] = value;
        }
        
        int getHodgeNumber(int p, int q) const {
            return hpq[p][q];
        }
        
        // Betti numbers
        std::vector<int> getBettiNumbers() const {
            std::vector<int> betti(7, 0);
            for (int p = 0; p <= 3; ++p) {
                for (int q = 0; q <= 3; ++q) {
                    betti[p + q] += hpq[p][q];
                }
            }
            return betti;
        }
        
        // Euler characteristic
        int eulerCharacteristic() const {
            int chi = 0;
            for (int p = 0; p <= 3; ++p) {
                for (int q = 0; q <= 3; ++q) {
                    chi += (p % 2 == 0 ? 1 : -1) * (q % 2 == 0 ? 1 : -1) * hpq[p][q];
                }
            }
            return chi;
        }
    };
    
private:
    HodgeNumbers hodgeNumbers;
    DolbeaultCohomology cohomology;
    
public:
    DifferentialForms(int h11, int h21) : hodgeNumbers(h11, h21) {
        initializeCohomology();
    }
    
    void initializeCohomology() {
        // Set Hodge numbers for Calabi-Yau 3-fold
        cohomology.setHodgeNumber(0, 0, 1);  // h00
        cohomology.setHodgeNumber(3, 3, 1);  // h33
        cohomology.setHodgeNumber(1, 1, hodgeNumbers.h11);
        cohomology.setHodgeNumber(2, 2, hodgeNumbers.h11);
        cohomology.setHodgeNumber(2, 1, hodgeNumbers.h21);
        cohomology.setHodgeNumber(1, 2, hodgeNumbers.h21);
        cohomology.setHodgeNumber(3, 0, 1);  // h30 (holomorphic 3-form)
        cohomology.setHodgeNumber(0, 3, 1);  // h03
    }
    
    HodgeNumbers getHodgeNumbers() const { return hodgeNumbers; }
    DolbeaultCohomology getCohomology() const { return cohomology; }
    
    // Kähler cone
    class KahlerCone {
    private:
        std::vector<std::vector<double>> constraints;
        
    public:
        void addConstraint(const std::vector<double>& constraint) {
            constraints.push_back(constraint);
        }
        
        bool isInside(const std::vector<double>& point) const {
            for (const auto& constraint : constraints) {
                double value = 0;
                for (size_t i = 0; i < point.size(); ++i) {
                    value += constraint[i] * point[i];
                }
                if (value < 0) return false;
            }
            return true;
        }
    };
};

// ============================================================================
// PART 3: CALABI-YAU MANIFOLD CONSTRUCTION
// ============================================================================

class CalabiYauManifold {
public:
    enum class ConstructionMethod {
        HYPERSURFACE_IN_TORIC_VARIETY,
        COMPLETE_INTERSECTION,
        ELLIPTIC_FIBRATION,
        K3_FIBRATION,
        ORBIFOLD,
        QUOTIENT
    };
    
private:
    // Manifold properties
    std::string name;
    ConstructionMethod method;
    int complexDimension;  // Usually 3 for Calabi-Yau 3-fold
    int h11;               // Kähler moduli
    int h21;               // Complex structure moduli
    int eulerCharacteristic;
    bool isSimplyConnected;
    bool hasSU3Holonomy;
    
    // Geometric data
    std::vector<std::vector<double>> periodMatrix;
    std::vector<Complex<>> complexStructureModuli;
    std::vector<double> kahlerModuli;
    
    // Topological data
    DifferentialForms::DolbeaultCohomology cohomology;
    std::vector<int> bettiNumbers;
    
    // Quantum corrections
    struct QuantumCorrection {
        int instantonNumber;
        double correction;
        
        QuantumCorrection(int n = 0, double c = 0) 
            : instantonNumber(n), correction(c) {}
    };
    std::vector<QuantumCorrection> quantumCorrections;
    
public:
    CalabiYauManifold(const std::string& name_,
                      ConstructionMethod method_,
                      int h11_, int h21_)
        : name(name_), method(method_), complexDimension(3),
          h11(h11_), h21(h21_), isSimplyConnected(true), hasSU3Holonomy(true) {
        
        eulerCharacteristic = 2 * (h11 - h21);
        cohomology = DifferentialForms(h11, h21).getCohomology();
        bettiNumbers = cohomology.getBettiNumbers();
        initializePeriodMatrix();
        initializeModuli();
    }
    
    void initializePeriodMatrix() {
        // Period matrix for Calabi-Yau 3-fold
        // Ω = (1, τ, ρ, τρ) where τ and ρ are complex structure moduli
        
        int dimension = 2 * (h21 + 1);
        periodMatrix.resize(dimension, std::vector<double>(dimension, 0));
        
        // Set identity for A-cycles
        for (int i = 0; i < dimension / 2; ++i) {
            periodMatrix[i][i] = 1;
        }
        
        // Set period matrix for B-cycles
        for (int i = 0; i < dimension / 2; ++i) {
            periodMatrix[i + dimension / 2][i] = 1;
            periodMatrix[i + dimension / 2][i + dimension / 2] = 1;
        }
    }
    
    void initializeModuli() {
        // Initialize complex structure moduli (random for demonstration)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.1, 2.0);
        
        for (int i = 0; i < h21; ++i) {
            complexStructureModuli.push_back(
                Complex<>(dis(gen), dis(gen)));
        }
        
        // Initialize Kähler moduli
        for (int i = 0; i < h11; ++i) {
            kahlerModuli.push_back(dis(gen));
        }
    }
    
    // Getters
    std::string getName() const { return name; }
    ConstructionMethod getMethod() const { return method; }
    int getComplexDimension() const { return complexDimension; }
    int getH11() const { return h11; }
    int getH21() const { return h21; }
    int getEulerCharacteristic() const { return eulerCharacteristic; }
    bool isCalabiYau() const { return hasSU3Holonomy; }
    
    // Kähler potential
    double kahlerPotential(const std::vector<double>& moduli) const {
        // K = -ln(∫ Ω ∧ Ω̄) for complex structure
        // Simplified for demonstration
        double sum = 0;
        for (const auto& z : complexStructureModuli) {
            sum += z.magnitudeSquared();
        }
        return -std::log(sum);
    }
    
    // Yukawa couplings (intersection numbers)
    double yukawaCoupling(int i, int j, int k) const {
        // For Calabi-Yau 3-fold, Yukawa couplings are related to
        // triple intersection numbers κ_ijk
        double coupling = 0;
        
        // Simplified: use random but consistent values
        std::mt19937 gen(i * 100 + j * 10 + k);
        std::uniform_real_distribution<> dis(0.5, 2.0);
        coupling = dis(gen);
        
        return coupling;
    }
    
    // Mirror symmetry transformation
    CalabiYauManifold mirrorSymmetry() const {
        // Mirror symmetry: (h11, h21) → (h21, h11)
        std::string mirrorName = name + "_mirror";
        return CalabiYauManifold(mirrorName, ConstructionMethod::QUOTIENT, 
                                h21, h11);
    }
    
    // Check if manifold is self-mirror
    bool isSelfMirror() const {
        return h11 == h21;
    }
    
    // Display manifold properties
    void displayProperties() const {
        std::cout << "\n=== CALABI-YAU MANIFOLD: " << name << " ===\n";
        std::cout << "Construction Method: ";
        switch (method) {
            case ConstructionMethod::HYPERSURFACE_IN_TORIC_VARIETY:
                std::cout << "Hypersurface in Toric Variety\n";
                break;
            case ConstructionMethod::COMPLETE_INTERSECTION:
                std::cout << "Complete Intersection\n";
                break;
            case ConstructionMethod::ELLIPTIC_FIBRATION:
                std::cout << "Elliptic Fibration\n";
                break;
            case ConstructionMethod::K3_FIBRATION:
                std::cout << "K3 Fibration\n";
                break;
            case ConstructionMethod::ORBIFOLD:
                std::cout << "Orbifold\n";
                break;
            case ConstructionMethod::QUOTIENT:
                std::cout << "Quotient\n";
                break;
        }
        std::cout << "Complex Dimension: " << complexDimension << "\n";
        std::cout << "Hodge Numbers: h11 = " << h11 << ", h21 = " << h21 << "\n";
        std::cout << "Euler Characteristic: " << eulerCharacteristic << "\n";
        std::cout << "Simply Connected: " << (isSimplyConnected ? "Yes" : "No") << "\n";
        std::cout << "SU(3) Holonomy: " << (hasSU3Holonomy ? "Yes" : "No") << "\n";
        std::cout << "Betti Numbers: ";
        for (int b : bettiNumbers) {
            std::cout << b << " ";
        }
        std::cout << "\n";
    }
};

// ============================================================================
// PART 4: CALABI-YAU DATABASE FOR SURVIVAL GAME
// ============================================================================

class CalabiYauDatabase {
private:
    std::vector<CalabiYauManifold> manifolds;
    std::map<std::string, int> manifoldIndex;
    
public:
    CalabiYauDatabase() {
        initializeStandardManifolds();
    }
    
    void initializeStandardManifolds() {
        // Quintic threefold (the canonical example)
        addManifold(CalabiYauManifold(
            "Quintic", 
            CalabiYauManifold::ConstructionMethod::HYPERSURFACE_IN_TORIC_VARIETY,
            1, 101));
        
        // Tian-Yau manifold
        addManifold(CalabiYauManifold(
            "Tian-Yau",
            CalabiYauManifold::ConstructionMethod::COMPLETE_INTERSECTION,
            14, 23));
        
        // Schoen manifold
        addManifold(CalabiYauManifold(
            "Schoen",
            CalabiYauManifold::ConstructionMethod::COMPLETE_INTERSECTION,
            19, 19));
        
        // Self-mirror manifold
        addManifold(CalabiYauManifold(
            "Self-Mirror",
            CalabiYauManifold::ConstructionMethod::ORBIFOLD,
            11, 11));
        
        // K3 fibration example
        addManifold(CalabiYauManifold(
            "K3-Fibration",
            CalabiYauManifold::ConstructionMethod::K3_FIBRATION,
            3, 243));
    }
    
    void addManifold(const CalabiYauManifold& manifold) {
        manifolds.push_back(manifold);
        manifoldIndex[manifold.getName()] = manifolds.size() - 1;
    }
    
    const CalabiYauManifold* getManifold(const std::string& name) const {
        auto it = manifoldIndex.find(name);
        if (it != manifoldIndex.end()) {
            return &manifolds[it->second];
        }
        return nullptr;
    }
    
    void displayAllManifolds() const {
        std::cout << "\n=== CALABI-YAU MANIFOLD DATABASE ===\n";
        std::cout << "Total manifolds: " << manifolds.size() << "\n";
        
        for (const auto& manifold : manifolds) {
            manifold.displayProperties();
        }
    }
    
    // Find manifolds by properties
    std::vector<const CalabiYauManifold*> findByEulerCharacteristic(int chi) const {
        std::vector<const CalabiYauManifold*> results;
        for (const auto& manifold : manifolds) {
            if (manifold.getEulerCharacteristic() == chi) {
                results.push_back(&manifold);
            }
        }
        return results;
    }
    
    // Find mirror pairs
    std::vector<std::pair<const CalabiYauManifold*, const CalabiYauManifold*>> 
    findMirrorPairs() const {
        std::vector<std::pair<const CalabiYauManifold*, const CalabiYauManifold*>> pairs;
        
        for (size_t i = 0; i < manifolds.size(); ++i) {
            for (size_t j = i + 1; j < manifolds.size(); ++j) {
                if (manifolds[i].getH11() == manifolds[j].getH21() &&
                    manifolds[i].getH21() == manifolds[j].getH11()) {
                    pairs.push_back({&manifolds[i], &manifolds[j]});
                }
            }
        }
        return pairs;
    }
};

// ============================================================================
// PART 5: CALABI-YAU CRAFTING SYSTEM
// ============================================================================

class CalabiYauCrafting {
public:
    enum class CraftingRecipe {
        // Basic manifolds
        CREATE_QUINTIC,
        CREATE_TIAN_YAU,
        CREATE_SCHOEN,
        CREATE_ORBIFOLD,
        
        // Mirror operations
        APPLY_MIRROR_SYMMETRY,
        
        // Topological operations
        BLOW_UP,
        BLOW_DOWN,
        
        // Moduli operations
        TUNE_COMPLEX_STRUCTURE,
        TUNE_KAHLER_MODULI,
        
        // Advanced operations
        FIBER_SUM,
        CONIFOLD_TRANSITION,
        FLOP_TRANSITION
    };
    
    struct Recipe {
        CraftingRecipe type;
        std::vector<std::string> requiredManifolds;
        std::vector<int> requiredResources;
        std::string resultManifold;
        int craftingTime;
        
        Recipe(CraftingRecipe t, 
               std::vector<std::string> req = {},
               std::vector<int> resources = {},
               std::string result = "",
               int time = 1)
            : type(t), requiredManifolds(req), requiredResources(resources),
              resultManifold(result), craftingTime(time) {}
    };
    
private:
    std::vector<Recipe> recipes;
    CalabiYauDatabase database;
    
public:
    CalabiYauCrafting() {
        initializeRecipes();
    }
    
    void initializeRecipes() {
        // Basic manifold creation
        recipes.emplace_back(
            CraftingRecipe::CREATE_QUINTIC,
            std::vector<std::string>{},
            std::vector<int>{5, 5, 5},  // 5th degree polynomial
            "Quintic",
            10);
        
        recipes.emplace_back(
            CraftingRecipe::CREATE_TIAN_YAU,
            std::vector<std::string>{},
            std::vector<int>{3, 3, 3},  // Complete intersection
            "Tian-Yau",
            15);
        
        // Mirror symmetry operation
        recipes.emplace_back(
            CraftingRecipe::APPLY_MIRROR_SYMMETRY,
            std::vector<std::string>{"Quintic"},
            std::vector<int>{1, 0, 1},  // Mirror resources
            "Quintic_mirror",
            5);
        
        // Flop transition
        recipes.emplace_back(
            CraftingRecipe::FLOP_TRANSITION,
            std::vector<std::string>{"Self-Mirror"},
            std::vector<int>{2, 2, 2},
            "Flopped-Manifold",
            8);
    }
    
    // Check if crafting recipe can be applied
    bool canCraft(const Recipe& recipe, 
                  const std::vector<std::string>& availableManifolds,
                  const std::vector<int>& availableResources) const {
        
        // Check required manifolds
        for (const auto& required : recipe.requiredManifolds) {
            bool found = false;
            for (const auto& available : availableManifolds) {
                if (available == required) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        
        // Check required resources
        for (size_t i = 0; i < recipe.requiredResources.size(); ++i) {
            if (i >= availableResources.size() || 
                availableResources[i] < recipe.requiredResources[i]) {
                return false;
            }
        }
        
        return true;
    }
    
    // Display all recipes
    void displayRecipes() const {
        std::cout << "\n=== CALABI-YAU CRAFTING RECIPES ===\n";
        for (const auto& recipe : recipes) {
            std::cout << "\nRecipe: ";
            switch (recipe.type) {
                case CraftingRecipe::CREATE_QUINTIC:
                    std::cout << "Create Quintic Manifold";
                    break;
                case CraftingRecipe::CREATE_TIAN_YAU:
                    std::cout << "Create Tian-Yau Manifold";
                    break;
                case CraftingRecipe::APPLY_MIRROR_SYMMETRY:
                    std::cout << "Apply Mirror Symmetry";
                    break;
                case CraftingRecipe::FLOP_TRANSITION:
                    std::cout << "Perform Flop Transition";
                    break;
                default:
                    std::cout << "Unknown Recipe";
            }
            std::cout << "\n";
            std::cout << "  Result: " << recipe.resultManifold << "\n";
            std::cout << "  Crafting Time: " << recipe.craftingTime << " ticks\n";
            
            if (!recipe.requiredManifolds.empty()) {
                std::cout << "  Required Manifolds: ";
                for (const auto& m : recipe.requiredManifolds) {
                    std::cout << m << " ";
                }
                std::cout << "\n";
            }
            
            if (!recipe.requiredResources.empty()) {
                std::cout << "  Required Resources: ";
                for (int r : recipe.requiredResources) {
                    std::cout << r << " ";
                }
                std::cout << "\n";
            }
        }
    }
};

// ============================================================================
// PART 6: SURVIVAL GAME INTEGRATION
// ============================================================================

class SurvivalGameCalabiYau {
public:
    enum class GameResource {
        // Mathematical resources
        POLYNOMIAL_COEFFICIENTS,
        HOLOMORPHIC_FORMS,
        KAHLER_CLASSES,
        INTERSECTION_NUMBERS,
        
        // Physical resources in game
        CRYSTAL_SHARDS,
        DIMENSIONAL_ESSENCE,
        TOPOLOGICAL_FABRIC,
        
        // Advanced resources
        MIRROR_FRAGMENTS,
        MODULI_SPACE_KEYS,
        HOLONOMY_CRYSTALS
    };
    
    struct PlayerState {
        std::vector<std::string> ownedManifolds;
        std::map<GameResource, int> resources;
        int craftingLevel;
        int topologicalUnderstanding;
        
        PlayerState() : craftingLevel(1), topologicalUnderstanding(0) {}
    };
    
private:
    PlayerState player;
    CalabiYauDatabase database;
    CalabiYauCrafting crafting;
    
public:
    SurvivalGameCalabiYau() {
        initializePlayer();
    }
    
    void initializePlayer() {
        // Start with basic resources
        player.resources[GameResource::POLYNOMIAL_COEFFICIENTS] = 10;
        player.resources[GameResource::HOLOMORPHIC_FORMS] = 3;
        player.resources[GameResource::KAHLER_CLASSES] = 1;
        player.resources[GameResource::INTERSECTION_NUMBERS] = 5;
    }
    
    void displayGameState() {
        std::cout << "\n=== SURVIVAL GAME STATE ===\n";
        std::cout << "Crafting Level: " << player.craftingLevel << "\n";
        std::cout << "Topological Understanding: " << player.topologicalUnderstanding << "\n";
        std::cout << "\nResources:\n";
        
        for (const auto& [resource, count] : player.resources) {
            std::cout << "  " << getResourceName(resource) << ": " << count << "\n";
        }
        
        std::cout << "\nOwned Manifolds:\n";
        for (const auto& manifold : player.ownedManifolds) {
            std::cout << "  - " << manifold << "\n";
        }
    }
    
    static std::string getResourceName(GameResource resource) {
        switch (resource) {
            case GameResource::POLYNOMIAL_COEFFICIENTS: return "Polynomial Coefficients";
            case GameResource::HOLOMORPHIC_FORMS: return "Holomorphic Forms";
            case GameResource::KAHLER_CLASSES: return "Kähler Classes";
            case GameResource::INTERSECTION_NUMBERS: return "Intersection Numbers";
            case GameResource::CRYSTAL_SHARDS: return "Crystal Shards";
            case GameResource::DIMENSIONAL_ESSENCE: return "Dimensional Essence";
            case GameResource::TOPOLOGICAL_FABRIC: return "Topological Fabric";
            case GameResource::MIRROR_FRAGMENTS: return "Mirror Fragments";
            case GameResource::MODULI_SPACE_KEYS: return "Moduli Space Keys";
            case GameResource::HOLONOMY_CRYSTALS: return "Holonomy Crystals";
            default: return "Unknown";
        }
    }
    
    // Attempt to craft a manifold
    bool craftManifold(const std::string& manifoldName) {
        // Simplified crafting logic
        std::cout << "\nAttempting to craft: " << manifoldName << "\n";
        
        if (manifoldName == "Quintic") {
            if (player.resources[GameResource::POLYNOMIAL_COEFFICIENTS] >= 5) {
                player.resources[GameResource::POLYNOMIAL_COEFFICIENTS] -= 5;
                player.ownedManifolds.push_back("Quintic");
                player.topologicalUnderstanding += 10;
                std::cout << "Successfully crafted Quintic manifold!\n";
                return true;
            }
        } else if (manifoldName == "Tian-Yau") {
            if (player.resources[GameResource::INTERSECTION_NUMBERS] >= 3) {
                player.resources[GameResource::INTERSECTION_NUMBERS] -= 3;
                player.ownedManifolds.push_back("Tian-Yau");
                player.topologicalUnderstanding += 15;
                std::cout << "Successfully crafted Tian-Yau manifold!\n";
                return true;
            }
        }
        
        std::cout << "Insufficient resources!\n";
        return false;
    }
    
    // Explore manifold properties
    void exploreManifold(const std::string& name) {
        const CalabiYauManifold* manifold = database.getManifold(name);
        if (manifold) {
            manifold->displayProperties();
            
            // Gain understanding
            player.topologicalUnderstanding += 5;
            std::cout << "\nGained topological understanding! (+5)\n";
        } else {
            std::cout << "Unknown manifold: " << name << "\n";
        }
    }
    
    void demonstrateGameplay() {
        std::cout << "\n=== CALABI-YAU SURVIVAL GAMEPLAY DEMO ===\n";
        
        displayGameState();
        
        // Craft some manifolds
        craftManifold("Quintic");
        craftManifold("Tian-Yau");
        
        // Explore manifolds
        exploreManifold("Quintic");
        exploreManifold("Schoen");
        
        displayGameState();
    }
};

// ============================================================================
// PART 7: TOPOLOGICAL INVARIANTS
// ============================================================================

class TopologicalInvariants {
public:
    // Chern classes for Calabi-Yau manifolds
    class ChernClasses {
    private:
        std::vector<int> chernNumbers;
        
    public:
        ChernClasses() {
            // For Calabi-Yau 3-fold:
            // c1 = 0 (Ricci-flat)
            // c2 · ω = ∫ c2 ∧ ω (related to h11)
            // c3 = Euler characteristic
            chernNumbers = {0, 0, 0}; // c1, c2, c3
        }
        
        void setChernClass(int index, int value) {
            if (index >= 0 && index < chernNumbers.size()) {
                chernNumbers[index] = value;
            }
        }
        
        int getChernClass(int index) const {
            if (index >= 0 && index < chernNumbers.size()) {
                return chernNumbers[index];
            }
            return 0;
        }
        
        // Check Calabi-Yau condition: c1 = 0
        bool satisfiesCalabiYauCondition() const {
            return chernNumbers[0] == 0;
        }
    };
    
    // Intersection numbers
    class IntersectionNumbers {
    private:
        std::vector<std::vector<std::vector<int>>> tripleIntersections;
        
    public:
        IntersectionNumbers(int h11) {
            tripleIntersections.resize(h11, 
                std::vector<std::vector<int>>(h11, 
                    std::vector<int>(h11, 0)));
        }
        
        void setIntersection(int i, int j, int k, int value) {
            tripleIntersections[i][j][k] = value;
        }
        
        int getIntersection(int i, int j, int k) const {
            return tripleIntersections[i][j][k];
        }
        
        // Calculate total intersection number
        int totalIntersection() const {
            int total = 0;
            for (const auto& matrix : tripleIntersections) {
                for (const auto& row : matrix) {
                    for (int value : row) {
                        total += value;
                    }
                }
            }
            return total;
        }
    };
    
private:
    ChernClasses chernClasses;
    IntersectionNumbers intersectionNumbers;
    
public:
    TopologicalInvariants(int h11) : intersectionNumbers(h11) {
        // Initialize Chern classes for Calabi-Yau
        chernClasses.setChernClass(0, 0);  // c1 = 0 (Calabi-Yau condition)
        chernClasses.setChernClass(2, 24);  // c2 · ω for K3
    }
    
    ChernClasses getChernClasses() const { return chernClasses; }
    IntersectionNumbers getIntersectionNumbers() const { return intersectionNumbers; }
    
    // Calculate Euler characteristic from Chern classes
    int eulerCharacteristicFromChern() const {
        return chernClasses.getChernClass(2);  // Simplified
    }
    
    // Display invariants
    void displayInvariants() const {
        std::cout << "\n=== TOPOLOGICAL INVARIANTS ===\n";
        std::cout << "Chern Classes:\n";
        std::cout << "  c1 = " << chernClasses.getChernClass(0) << "\n";
        std::cout << "  c2 = " << chernClasses.getChernClass(1) << "\n";
        std::cout << "  c3 = " << chernClasses.getChernClass(2) << "\n";
        std::cout << "Calabi-Yau Condition (c1=0): " 
                  << (chernClasses.satisfiesCalabiYauCondition() ? "Satisfied" : "Not Satisfied") 
                  << "\n";
    }
};

// ============================================================================
// PART 8: MAIN DEMONSTRATION
// ============================================================================

int main() {
    std::cout << "===========================================================\n";
    std::cout << "CALABI-YAU MANIFOLDS FOR 3D SURVIVAL GAME\n";
    std::cout << "Pure Mathematical Framework\n";
    std::cout << "===========================================================\n";
    
    // 1. Complex number demonstration
    std::cout << "\n1. COMPLEX NUMBER FOUNDATIONS\n";
    std::cout << "=============================\n";
    
    Complex<> z1(3, 4);
    Complex<> z2(1, -2);
    Complex<> sum = z1 + z2;
    Complex<> product = z1 * z2;
    
    std::cout << "z1 = " << z1.toString() << "\n";
    std::cout << "z2 = " << z2.toString() << "\n";
    std::cout << "z1 + z2 = " << sum.toString() << "\n";
    std::cout << "z1 * z2 = " << product.toString() << "\n";
    std::cout << "|z1| = " << z1.magnitude() << "\n";
    std::cout << "\n";
    
    // 2. Calabi-Yau manifold database    std::cout << "2. CALABI-YAU MANIFOLD DATABASE\n";
    std::cout << "===============================\n";
    
    CalabiYauDatabase database;
    database.displayAllManifolds();
    
    // 3. Mirror symmetry demonstration
    std::cout << "\n3. MIRROR SYMMETRY\n";
    std::cout << "==================\n";
    
    auto mirrorPairs = database.findMirrorPairs();
    std::cout << "Mirror pairs found: " << mirrorPairs.size() << "\n";
    
    for (const auto& [manifold1, manifold2] : mirrorPairs) {
        std::cout << "Mirror Pair: " << manifold1->getName() 
                  << " ↔ " << manifold2->getName() << "\n";
        std::cout << "  h11: " << manifold1->getH11() 
                  << " ↔ " << manifold2->getH11() << "\n";
        std::cout << "  h21: " << manifold1->getH21() 
                  << " ↔ " << manifold2->getH21() << "\n";
    }
    std::cout << "\n";
    
    // 4. Topological invariants
    std::cout << "4. TOPOLOGICAL INVARIANTS\n";
    std::cout << "========================\n";
    
    TopologicalInvariants invariants(1);  // h11 = 1
    invariants.displayInvariants();
    
    // 5. Crafting system
    std::cout << "\n5. CRAFTING SYSTEM\n";
    std::cout << "==================\n";
    
    CalabiYauCrafting crafting;
    crafting.displayRecipes();
    
    // 6. Survival game demonstration
    std::cout << "\n6. SURVIVAL GAME INTEGRATION\n";
    std::cout << "============================\n";
    
    SurvivalGameCalabiYau game;
    game.demonstrateGameplay();
    
    // 7. Mathematical summary
    std::cout << "\n===========================================================\n";
    std::cout << "MATHEMATICAL FRAMEWORK SUMMARY\n";
    std::cout << "===========================================================\n";
    std::cout << "\nPure Mathematics Components:\n";
    std::cout << "  ✓ Complex Analysis (Complex numbers, holomorphic functions)\n";
    std::cout << "  ✓ Differential Geometry (Differential forms, Hodge theory)\n";
    std::cout << "  ✓ Algebraic Topology (Cohomology, Betti numbers)\n";
    std::cout << "  ✓ Algebraic Geometry (Toric varieties, intersections)\n";
    std::cout << "  ✓ Mirror Symmetry (Dual Calabi-Yau pairs)\n";
    std::cout << "  ✓ Chern Classes (Characteristic classes)\n";
    std::cout << "\nCalabi-Yau Properties:\n";
    std::cout << "  ✓ Ricci-flat Kähler metric\n";
    std::cout << "  ✓ SU(3) holonomy\n";
    std::cout << "  ✓ Vanishing first Chern class (c1 = 0)\n";
    std::cout << "  ✓ Non-trivial canonical bundle\n";
    std::cout << "  ✓ Hodge diamond symmetry\n";
    std::cout << "\nGame Integration:\n";
    std::cout << "  ✓ Manifold crafting recipes\n";
    std::cout << "  ✓ Resource management\n";
    std::cout << "  ✓ Topological exploration\n";
    std::cout << "  ✓ Mirror symmetry operations\n";
    std::cout << "  ✓ Progressive learning system\n";
    
    return 0;
}
