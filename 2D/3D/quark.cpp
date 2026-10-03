#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <map>

// Enum for quark flavors
enum class QuarkFlavor {
    UP,
    DOWN,
    CHARM,
    STRANGE,
    TOP,
    BOTTOM
};

// Enum for quark generations
enum class Generation {
    FIRST,
    SECOND,
    THIRD
};

// Enum for electric charge type
enum class ChargeSign {
    POSITIVE,
    NEGATIVE
};

// Class representing a quark
class Quark {
private:
    QuarkFlavor flavor;
    std::string name;
    std::string symbol;
    Generation generation;
    double massGeV;           // Mass in GeV/c²
    double electricCharge;    // Electric charge in units of elementary charge
    double spin;              // Spin in units of ħ
    int baryonNumber;         // Baryon number
    double isospin;           // Isospin
    double strangeness;       // Strangeness quantum number
    double charm;             // Charm quantum number
    double bottomness;        // Bottomness quantum number
    double topness;           // Topness quantum number
    std::string colorCharge;  // Color charge (red, green, blue)
    double weakIsospin;       // Weak isospin
    double weakHypercharge;   // Weak hypercharge

public:
    // Constructor
    Quark(QuarkFlavor f, std::string n, std::string sym, Generation gen,
          double mass, double charge, double sp, double isospin_val,
          double strangeness_val, double charm_val, double bottom_val,
          double top_val, double weak_isospin_val, double weak_hyper_val)
        : flavor(f), name(n), symbol(sym), generation(gen), massGeV(mass),
          electricCharge(charge), spin(sp), baryonNumber(1.0/3.0),
          isospin(isospin_val), strangeness(strangeness_val),
          charm(charm_val), bottomness(bottom_val), topness(top_val),
          colorCharge("RGB (can be red, green, or blue)"),
          weakIsospin(weak_isospin_val), weakHypercharge(weak_hyper_val) {}

    // Getters
    std::string getName() const { return name; }
    std::string getSymbol() const { return symbol; }
    Generation getGeneration() const { return generation; }
    double getMass() const { return massGeV; }
    double getElectricCharge() const { return electricCharge; }
    double getSpin() const { return spin; }
    double getBaryonNumber() const { return baryonNumber; }
    double getIsospin() const { return isospin; }
    double getStrangeness() const { return strangeness; }
    double getCharm() const { return charm; }
    double getBottomness() const { return bottomness; }
    double getTopness() const { return topness; }
    std::string getColorCharge() const { return colorCharge; }
    double getWeakIsospin() const { return weakIsospin; }
    double getWeakHypercharge() const { return weakHypercharge; }

    // Helper method to get generation as string
    std::string getGenerationString() const {
        switch(generation) {
            case Generation::FIRST: return "First";
            case Generation::SECOND: return "Second";
            case Generation::THIRD: return "Third";
            default: return "Unknown";
        }
    }

    // Display quark properties
    void displayProperties() const {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "Quark: " << name << " (" << symbol << ")" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        std::cout << std::left << std::setw(25) << "Generation:" 
                  << getGenerationString() << std::endl;
        std::cout << std::left << std::setw(25) << "Mass:" 
                  << massGeV << " GeV/c²" << std::endl;
        std::cout << std::left << std::setw(25) << "Electric Charge:" 
                  << electricCharge << " e" << std::endl;
        std::cout << std::left << std::setw(25) << "Spin:" 
                  << spin << " ħ" << std::endl;
        std::cout << std::left << std::setw(25) << "Baryon Number:" 
                  << baryonNumber << std::endl;
        std::cout << std::left << std::setw(25) << "Isospin:" 
                  << isospin << std::endl;
        std::cout << std::left << std::setw(25) << "Strangeness:" 
                  << strangeness << std::endl;
        std::cout << std::left << std::setw(25) << "Charm:" 
                  << charm << std::endl;
        std::cout << std::left << std::setw(25) << "Bottomness:" 
                  << bottomness << std::endl;
        std::cout << std::left << std::setw(25) << "Topness:" 
                  << topness << std::endl;
        std::cout << std::left << std::setw(25) << "Color Charge:" 
                  << colorCharge << std::endl;
        std::cout << std::left << std::setw(25) << "Weak Isospin:" 
                  << weakIsospin << std::endl;
        std::cout << std::left << std::setw(25) << "Weak Hypercharge:" 
                  << weakHypercharge << std::endl;
    }
};

// Class to manage all quarks
class QuarkManager {
private:
    std::vector<Quark> quarks;

public:
    // Constructor initializes all six quarks
    QuarkManager() {
        // Up quark
        quarks.emplace_back(
            QuarkFlavor::UP, "Up", "u", Generation::FIRST,
            0.0022,   // Mass ~2.2 MeV/c²
            2.0/3.0,  // Electric charge +2/3
            0.5,      // Spin 1/2
            0.5,      // Isospin +1/2
            0.0,      // Strangeness
            0.0,      // Charm
            0.0,      // Bottomness
            0.0,      // Topness
            0.5,      // Weak isospin
            1.0/3.0   // Weak hypercharge
        );

        // Down quark
        quarks.emplace_back(
            QuarkFlavor::DOWN, "Down", "d", Generation::FIRST,
            0.0047,   // Mass ~4.7 MeV/c²
            -1.0/3.0, // Electric charge -1/3
            0.5,      // Spin 1/2
            -0.5,     // Isospin -1/2
            0.0,      // Strangeness
            0.0,      // Charm
            0.0,      // Bottomness
            0.0,      // Topness
            -0.5,     // Weak isospin
            1.0/3.0   // Weak hypercharge
        );

        // Charm quark
        quarks.emplace_back(
            QuarkFlavor::CHARM, "Charm", "c", Generation::SECOND,
            1.27,     // Mass ~1.27 GeV/c²
            2.0/3.0,  // Electric charge +2/3
            0.5,      // Spin 1/2
            0.0,      // Isospin
            0.0,      // Strangeness
            1.0,      // Charm
            0.0,      // Bottomness
            0.0,      // Topness
            0.5,      // Weak isospin
            1.0/3.0   // Weak hypercharge
        );

        // Strange quark
        quarks.emplace_back(
            QuarkFlavor::STRANGE, "Strange", "s", Generation::SECOND,
            0.093,    // Mass ~93 MeV/c²
            -1.0/3.0, // Electric charge -1/3
            0.5,      // Spin 1/2
            0.0,      // Isospin
            -1.0,     // Strangeness
            0.0,      // Charm
            0.0,      // Bottomness
            0.0,      // Topness
            -0.5,     // Weak isospin
            1.0/3.0   // Weak hypercharge
        );

        // Top quark
        quarks.emplace_back(
            QuarkFlavor::TOP, "Top", "t", Generation::THIRD,
            172.76,   // Mass ~172.76 GeV/c²
            2.0/3.0,  // Electric charge +2/3
            0.5,      // Spin 1/2
            0.0,      // Isospin
            0.0,      // Strangeness
            0.0,      // Charm
            0.0,      // Bottomness
            1.0,      // Topness
            0.5,      // Weak isospin
            1.0/3.0   // Weak hypercharge
        );

        // Bottom quark
        quarks.emplace_back(
            QuarkFlavor::BOTTOM, "Bottom", "b", Generation::THIRD,
            4.18,     // Mass ~4.18 GeV/c²
            -1.0/3.0, // Electric charge -1/3
            0.5,      // Spin 1/2
            0.0,      // Isospin
            0.0,      // Strangeness
            0.0,      // Charm
            -1.0,     // Bottomness
            0.0,      // Topness
            -0.5,     // Weak isospin
            1.0/3.0   // Weak hypercharge
        );
    }

    // Display all quarks
    void displayAllQuarks() const {
        std::cout << "\n" << std::string(70, '*') << std::endl;
        std::cout << "ALL QUARKS IN THE STANDARD MODEL" << std::endl;
        std::cout << std::string(70, '*') << std::endl;
        for (const auto& quark : quarks) {
            quark.displayProperties();
        }
    }

    // Display quarks by generation
    void displayQuarksByGeneration(Generation gen) const {
        std::cout << "\n" << std::string(70, '*') << std::endl;
        std::string genName;
        switch(gen) {
            case Generation::FIRST: genName = "FIRST GENERATION QUARKS"; break;
            case Generation::SECOND: genName = "SECOND GENERATION QUARKS"; break;
            case Generation::THIRD: genName = "THIRD GENERATION QUARKS"; break;
        }
        std::cout << genName << std::endl;
        std::cout << std::string(70, '*') << std::endl;
        
        for (const auto& quark : quarks) {
            if (quark.getGeneration() == gen) {
                quark.displayProperties();
            }
        }
    }

    // Display summary table
    void displaySummaryTable() const {
        std::cout << "\n" << std::string(80, '*') << std::endl;
        std::cout << "QUARK SUMMARY TABLE" << std::endl;
        std::cout << std::string(80, '*') << std::endl;
        std::cout << std::left << std::setw(10) << "Flavor" 
                  << std::setw(8) << "Symbol"
                  << std::setw(12) << "Generation"
                  << std::setw(15) << "Mass (GeV/c²)"
                  << std::setw(15) << "Charge (e)"
                  << std::setw(10) << "Spin (ħ)"
                  << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        for (const auto& quark : quarks) {
            std::cout << std::left << std::setw(10) << quark.getName()
                      << std::setw(8) << quark.getSymbol()
                      << std::setw(12) << quark.getGenerationString()
                      << std::setw(15) << quark.getMass()
                      << std::setw(15) << quark.getElectricCharge()
                      << std::setw(10) << quark.getSpin()
                      << std::endl;
        }
    }

    // Get quark by flavor
    Quark* getQuark(QuarkFlavor flavor) {
        for (auto& quark : quarks) {
            // We need to add a getter for flavor or compare by name
            std::string targetName;
            switch(flavor) {
                case QuarkFlavor::UP: targetName = "Up"; break;
                case QuarkFlavor::DOWN: targetName = "Down"; break;
                case QuarkFlavor::CHARM: targetName = "Charm"; break;
                case QuarkFlavor::STRANGE: targetName = "Strange"; break;
                case QuarkFlavor::TOP: targetName = "Top"; break;
                case QuarkFlavor::BOTTOM: targetName = "Bottom"; break;
            }
            if (quark.getName() == targetName) {
                return &quark;
            }
        }
        return nullptr;
    }
};

// Main function to demonstrate the quark system
int main() {
    QuarkManager manager;
    
    // Display all quarks
    manager.displayAllQuarks();
    
    // Display summary table
    manager.displaySummaryTable();
    
    // Display quarks by generation
    std::cout << "\n\n" << std::string(70, '=') << std::endl;
    std::cout << "QUARKS ORGANIZED BY GENERATION" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    manager.displayQuarksByGeneration(Generation::FIRST);
    manager.displayQuarksByGeneration(Generation::SECOND);
    manager.displayQuarksByGeneration(Generation::THIRD);
    
    // Additional information about quark combinations
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "IMPORTANT QUARK FACTS" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "• Quarks combine to form hadrons:" << std::endl;
    std::cout << "  - Baryons (3 quarks): Proton (uud), Neutron (udd)" << std::endl;
    std::cout << "  - Mesons (quark-antiquark): Pion (π⁺ = ud̄), Kaon (K⁺ = us̄)" << std::endl;
    std::cout << "\n• Quarks interact via all four fundamental forces:" << std::endl;
    std::cout << "  - Strong force (color charge)" << std::endl;
    std::cout << "  - Weak force (flavor changing)" << std::endl;
    std::cout << "  - Electromagnetic force (electric charge)" << std::endl;
    std::cout << "  - Gravitational force (mass)" << std::endl;
    std::cout << "\n• Quarks are never found in isolation (color confinement)" << std::endl;
    std::cout << "• Top quark is the heaviest known elementary particle" << std::endl;
    
    return 0;
}