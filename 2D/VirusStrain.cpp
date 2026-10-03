#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <map>
#include <queue>
#include <memory>
#include <functional>
#include <random>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iostream>

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;

// Virus strains and types
enum class VirusStrain {
    AGGRESSION,      // Makes NPCs aggressive
    CONFUSION,       // Makes NPCs confused and erratic
    OBEDIENCE,       // Makes NPCs obedient to virus origin
    PARANOIA,        // Makes NPCs paranoid and distrustful
    GREED,           // Makes NPCs greedy and selfish
    FEAR,            // Makes NPCs fearful and cowardly
    CHAOS,           // Makes NPCs chaotic and unpredictable
    HIVE_MIND        // Connects NPCs to a collective consciousness
};

// Virus mutation types
enum class MutationType {
    NONE,
    ENHANCED_SPREAD,      // Increased infection rate
    INCREASED_VIRULENCE,  // Stronger effects
    CAMOUFLAGE,           // Harder to detect
    LATENCY,              // Delayed symptoms
    RESISTANCE,           // Resistance to countermeasures
    ADAPTIVE,             // Adapts to host defenses
    SYMBIOTIC             // Provides benefits to host
};

// Virus status
enum class VirusStatus {
    DORMANT,
    INCUBATING,
    ACTIVE,
    MUTATING,
    REMISSION,
    CURED
};

// NPC mental states affected by virus
struct MentalState {
    float aggression = 0.0f;
    float confusion = 0.0f;
    float obedience = 0.0f;
    float paranoia = 0.0f;
    float greed = 0.0f;
    float fear = 0.0f;
    float chaos = 0.0f;
    float hivemind = 0.0f;
    float sanity = 1.0f;
    float willpower = 1.0f;
    float consciousness = 1.0f;
    
    // Original personality (pre-infection)
    float originalAggression = 0.0f;
    float originalFriendliness = 0.0f;
    float originalIntelligence = 0.0f;
    float originalSociability = 0.0f;
};

// Digital Mind Virus class
class DigitalMindVirus {
public:
    std::string name;
    VirusStrain strain;
    VirusStatus status;
    MutationType mutation;
    float infectionRate;
    float virulence;
    float detectionDifficulty;
    float incubationPeriod;
    float mutationRate;
    float resistance;
    float adaptability;
    int generation;
    std::vector<std::string> infectionHistory;
    std::map<std::string, float> strainEffects;
    Color virusColor;
    
    // Virus lifecycle
    float age;
    float mutationTimer;
    std::vector<std::string> evolvedTraits;
    
    DigitalMindVirus(const std::string& virusName, VirusStrain virusStrain)
        : name(virusName), strain(virusStrain), status(VirusStatus::ACTIVE),
          mutation(MutationType::NONE), infectionRate(0.3f), virulence(0.5f),
          detectionDifficulty(0.3f), incubationPeriod(2.0f), mutationRate(0.1f),
          resistance(0.0f), adaptability(0.3f), generation(1), age(0),
          mutationTimer(0) {
        
        // Set virus color based on strain
        switch (strain) {
            case VirusStrain::AGGRESSION: virusColor = RED; break;
            case VirusStrain::CONFUSION: virusColor = PURPLE; break;
            case VirusStrain::OBEDIENCE: virusColor = BLUE; break;
            case VirusStrain::PARANOIA: virusColor = DARKPURPLE; break;
            case VirusStrain::GREED: virusColor = GOLD; break;
            case VirusStrain::FEAR: virusColor = DARKGRAY; break;
            case VirusStrain::CHAOS: virusColor = ORANGE; break;
            case VirusStrain::HIVE_MIND: virusColor = CYAN; break;
        }
        
        // Initialize strain effects
        InitializeStrainEffects();
    }
    
    void InitializeStrainEffects() {
        // Set base effects based on strain
        switch (strain) {
            case VirusStrain::AGGRESSION:
                strainEffects["aggression"] = 0.8f;
                strainEffects["sanity_loss"] = 0.3f;
                strainEffects["willpower_loss"] = 0.2f;
                break;
            case VirusStrain::CONFUSION:
                strainEffects["confusion"] = 0.9f;
                strainEffects["sanity_loss"] = 0.5f;
                strainEffects["intelligence_loss"] = 0.4f;
                break;
            case VirusStrain::OBEDIENCE:
                strainEffects["obedience"] = 0.9f;
                strainEffects["willpower_loss"] = 0.6f;
                strainEffects["consciousness_loss"] = 0.4f;
                break;
            case VirusStrain::PARANOIA:
                strainEffects["paranoia"] = 0.8f;
                strainEffects["trust_loss"] = 0.7f;
                strainEffects["sanity_loss"] = 0.4f;
                break;
            case VirusStrain::GREED:
                strainEffects["greed"] = 0.8f;
                strainEffects["morality_loss"] = 0.5f;
                strainEffects["empathy_loss"] = 0.4f;
                break;
            case VirusStrain::FEAR:
                strainEffects["fear"] = 0.9f;
                strainEffects["courage_loss"] = 0.7f;
                strainEffects["willpower_loss"] = 0.3f;
                break;
            case VirusStrain::CHAOS:
                strainEffects["chaos"] = 0.9f;
                strainEffects["sanity_loss"] = 0.6f;
                strainEffects["predictability_loss"] = 0.8f;
                break;
            case VirusStrain::HIVE_MIND:
                strainEffects["hivemind"] = 0.8f;
                strainEffects["individuality_loss"] = 0.7f;
                strainEffects["consciousness_loss"] = 0.5f;
                break;
        }
    }
    
    void Update(float dt) {
        age += dt;
        mutationTimer += dt;
        
        // Check for mutation
        if (mutationTimer > 10.0f && status == VirusStatus::ACTIVE) {
            mutationTimer = 0;
            Mutate();
        }
        
        // Spread through infection
        if (status == VirusStatus::ACTIVE) {
            infectionRate *= (1.0f + adaptability * 0.01f * dt);
            infectionRate = std::min(infectionRate, 1.0f);
        }
    }
    
    void Mutate() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        if (dist(gen) < mutationRate) {
            generation++;
            
            // Random mutation
            int mutationType = GetRandomValue(0, 6);
            mutation = static_cast<MutationType>(mutationType);
            
            switch (mutation) {
                case MutationType::ENHANCED_SPREAD:
                    infectionRate *= 1.5f;
                    evolvedTraits.push_back("Enhanced Spread");
                    break;
                case MutationType::INCREASED_VIRULENCE:
                    virulence *= 1.5f;
                    evolvedTraits.push_back("Increased Virulence");
                    break;
                case MutationType::CAMOUFLAGE:
                    detectionDifficulty *= 1.5f;
                    evolvedTraits.push_back("Camouflage");
                    break;
                case MutationType::LATENCY:
                    incubationPeriod *= 2.0f;
                    evolvedTraits.push_back("Latency");
                    break;
                case MutationType::RESISTANCE:
                    resistance += 0.3f;
                    evolvedTraits.push_back("Resistance");
                    break;
                case MutationType::ADAPTIVE:
                    adaptability *= 1.5f;
                    evolvedTraits.push_back("Adaptive");
                    break;
                case MutationType::SYMBIOTIC:
                    // Virus provides benefits
                    strainEffects["strength_boost"] = 0.3f;
                    strainEffects["speed_boost"] = 0.3f;
                    evolvedTraits.push_back("Symbiotic");
                    break;
            }
            
            infectionHistory.push_back("Generation " + std::to_string(generation) + 
                                      ": " + evolvedTraits.back());
        }
    }
    
    void Draw(Vector2 position, float radius = 15.0f) {
        // Draw virus as a pulsing sphere
        float pulse = (sin(GetTime() * 4) + 1) / 2;
        DrawCircleV(position, radius * (0.8f + pulse * 0.4f), ColorAlpha(virusColor, 0.7f));
        DrawCircleV(position, radius * 0.5f, ColorAlpha(WHITE, 0.3f));
        
        // Draw virus tendrils
        for (int i = 0; i < 8; i++) {
            float angle = GetTime() * 2 + i * PI / 4;
            Vector2 end = {
                position.x + cos(angle) * radius * 1.5f,
                position.y + sin(angle) * radius * 1.5f
            };
            DrawLineEx(position, end, 2, ColorAlpha(virusColor, 0.5f));
        }
    }
};

// Infected NPC class
class InfectedNPC {
public:
    int id;
    std::string name;
    Vector2 position;
    Color originalColor;
    Color currentColor;
    MentalState mentalState;
    VirusStatus infectionStatus;
    std::shared_ptr<DigitalMindVirus> virus;
    float infectionProgress;
    float symptomTimer;
    float behaviorTimer;
    Vector2 targetPosition;
    bool isAlive;
    
    // Original personality
    float baseFriendliness;
    float baseIntelligence;
    float baseSociability;
    float baseAggression;
    
    // Infection history
    std::vector<std::string> infectionLog;
    std::vector<std::string> symptoms;
    
    InfectedNPC(int npcId, const std::string& npcName, Vector2 pos, Color color)
        : id(npcId), name(npcName), position(pos), originalColor(color), currentColor(color),
          infectionStatus(VirusStatus::DORMANT), infectionProgress(0), symptomTimer(0),
          behaviorTimer(0), targetPosition(pos), isAlive(true) {
        
        // Initialize random personality
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.3f, 1.0f);
        
        baseFriendliness = dist(gen);
        baseIntelligence = dist(gen);
        baseSociability = dist(gen);
        baseAggression = dist(gen);
        
        mentalState.originalAggression = baseAggression;
        mentalState.originalFriendliness = baseFriendliness;
        mentalState.originalIntelligence = baseIntelligence;
        mentalState.originalSociability = baseSociability;
    }
    
    void Infect(std::shared_ptr<DigitalMindVirus> newVirus) {
        if (infectionStatus == VirusStatus::DORMANT) {
            virus = newVirus;
            infectionStatus = VirusStatus::INCUBATING;
            infectionProgress = 0;
            infectionLog.push_back("Infected with " + virus->name + " at generation " + 
                                  std::to_string(virus->generation));
        }
    }
    
    void Update(float dt, std::vector<InfectedNPC*>& population) {
        if (!isAlive) return;
        
        // Update infection
        UpdateInfection(dt);
        
        // Update symptoms
        if (infectionStatus == VirusStatus::ACTIVE) {
            UpdateSymptoms(dt);
            SpreadInfection(population, dt);
        }
        
        // Update behavior
        behaviorTimer -= dt;
        if (behaviorTimer <= 0) {
            behaviorTimer = 0.5f / (mentalState.confusion + 0.5f);
            UpdateBehavior(population);
        }
        
        // Move
        Move(dt);
    }
    
    void UpdateInfection(float dt) {
        if (!virus || infectionStatus == VirusStatus::CURED) return;
        
        switch (infectionStatus) {
            case VirusStatus::INCUBATING:
                infectionProgress += dt / virus->incubationPeriod;
                if (infectionProgress >= 1.0f) {
                    infectionStatus = VirusStatus::ACTIVE;
                    infectionProgress = 1.0f;
                    infectionLog.push_back("Virus became active");
                    ApplyInitialSymptoms();
                }
                break;
                
            case VirusStatus::ACTIVE:
                // Apply ongoing effects
                ApplyVirusEffects(dt);
                
                // Check for remission
                if (mentalState.willpower > 0.8f && GetRandomValue(0, 100) < 1) {
                    infectionStatus = VirusStatus::REMISSION;
                    infectionLog.push_back("Virus entered remission");
                }
                break;
                
            case VirusStatus::REMISSION:
                // Partial recovery
                RecoverMentalState(dt);
                if (mentalState.sanity > 0.8f) {
                    infectionStatus = VirusStatus::CURED;
                    infectionLog.push_back("Cured from virus");
                    virus = nullptr;
                }
                break;
                
            default:
                break;
        }
    }
    
    void ApplyInitialSymptoms() {
        if (!virus) return;
        
        // Apply strain-specific symptoms
        for (const auto& [effect, magnitude] : virus->strainEffects) {
            ApplyStrainEffect(effect, magnitude);
            symptoms.push_back(effect);
        }
        
        // Change color based on virus
        currentColor = ColorLerp(originalColor, virus->virusColor, 0.5f);
    }
    
    void ApplyVirusEffects(float dt) {
        if (!virus) return;
        
        // Gradual mental degradation
        for (const auto& [effect, magnitude] : virus->strainEffects) {
            float strength = magnitude * virus->virulence * dt;
            ApplyStrainEffect(effect, strength);
        }
        
        // Update color intensity based on infection
        float intensity = 0.5f + (virus->virulence * 0.5f);
        currentColor = ColorLerp(originalColor, virus->virusColor, intensity);
        
        // Reduce sanity and willpower
        mentalState.sanity = std::max(0.0f, mentalState.sanity - 0.1f * virus->virulence * dt);
        mentalState.willpower = std::max(0.0f, mentalState.willpower - 0.05f * virus->virulence * dt);
        
        // Symbiotic benefits
        if (virus->mutation == MutationType::SYMBIOTIC) {
            // Provide stat bonuses
        }
    }
    
    void ApplyStrainEffect(const std::string& effect, float magnitude) {
        if (effect == "aggression") mentalState.aggression = std::min(1.0f, mentalState.aggression + magnitude);
        if (effect == "confusion") mentalState.confusion = std::min(1.0f, mentalState.confusion + magnitude);
        if (effect == "obedience") mentalState.obedience = std::min(1.0f, mentalState.obedience + magnitude);
        if (effect == "paranoia") mentalState.paranoia = std::min(1.0f, mentalState.paranoia + magnitude);
        if (effect == "greed") mentalState.greed = std::min(1.0f, mentalState.greed + magnitude);
        if (effect == "fear") mentalState.fear = std::min(1.0f, mentalState.fear + magnitude);
        if (effect == "chaos") mentalState.chaos = std::min(1.0f, mentalState.chaos + magnitude);
        if (effect == "hivemind") mentalState.hivemind = std::min(1.0f, mentalState.hivemind + magnitude);
        
        if (effect == "sanity_loss") mentalState.sanity = std::max(0.0f, mentalState.sanity - magnitude);
        if (effect == "willpower_loss") mentalState.willpower = std::max(0.0f, mentalState.willpower - magnitude);
        if (effect == "consciousness_loss") mentalState.consciousness = std::max(0.0f, mentalState.consciousness - magnitude);
    }
    
    void UpdateSymptoms(float dt) {
        symptomTimer -= dt;
        if (symptomTimer <= 0) {
            symptomTimer = 2.0f;
            
            // Display random symptom based on strain
            std::string symptom = GenerateSymptom();
            if (!symptom.empty()) {
                infectionLog.push_back(symptom);
            }
        }
    }
    
    std::string GenerateSymptom() {
        if (!virus) return "";
        
        std::vector<std::string> possibleSymptoms;
        
        switch (virus->strain) {
            case VirusStrain::AGGRESSION:
                possibleSymptoms = {"Growls angrily", "Clenches fists", "Eyes turn red", 
                                   "Shouts threats", "Attacks nearby objects"};
                break;
            case VirusStrain::CONFUSION:
                possibleSymptoms = {"Walks in circles", "Mumbles incoherently", 
                                   "Stares blankly", "Forgets own name"};
                break;
            case VirusStrain::OBEDIENCE:
                possibleSymptoms = {"Follows other infected", "Repeats phrases", 
                                   "Loses independent thought", "Moves in unison"};
                break;
            case VirusStrain::PARANOIA:
                possibleSymptoms = {"Looks around nervously", "Accuses others", 
                                   "Builds barriers", "Hides in corners"};
                break;
            case VirusStrain::GREED:
                possibleSymptoms = {"Hoards objects", "Steals from others", 
                                   "Counts possessions obsessively"};
                break;
            case VirusStrain::FEAR:
                possibleSymptoms = {"Cowers in fear", "Runs from shadows", 
                                   "Screams at noises", "Refuses to move"};
                break;
            case VirusStrain::CHAOS:
                possibleSymptoms = {"Moves erratically", "Laughs maniacally", 
                                   "Breaks objects randomly", "Speaks in riddles"};
                break;
            case VirusStrain::HIVE_MIND:
                possibleSymptoms = {"Speaks in unison", "Moves in patterns", 
                                   "Shares thoughts", "Acts as one"};
                break;
        }
        
        if (possibleSymptoms.empty()) return "";
        return possibleSymptoms[GetRandomValue(0, possibleSymptoms.size() - 1)];
    }
    
    void SpreadInfection(std::vector<InfectedNPC*>& population, float dt) {
        if (!virus) return;
        
        for (auto npc : population) {
            if (npc->id != id && npc->infectionStatus == VirusStatus::DORMANT) {
                float distance = Vector2Distance(position, npc->position);
                
                if (distance < 50) {
                    float infectionChance = virus->infectionRate * dt;
                    if (GetRandomValue(0, 1000) < infectionChance * 1000) {
                        npc->Infect(virus);
                    }
                }
                
                // Hive mind can spread through network
                if (virus->strain == VirusStrain::HIVE_MIND && 
                    mentalState.hivemind > 0.7f) {
                    if (distance < 100) {
                        float networkChance = 0.1f * dt;
                        if (GetRandomValue(0, 1000) < networkChance * 1000) {
                            npc->Infect(virus);
                        }
                    }
                }
            }
        }
    }
    
    void UpdateBehavior(std::vector<InfectedNPC*>& population) {
        // Behavior changes based on virus effects
        if (mentalState.aggression > 0.7f) {
            // Attack nearest NPC
            InfectedNPC* nearest = FindNearestNPC(population);
            if (nearest && Vector2Distance(position, nearest->position) < 100) {
                targetPosition = nearest->position;
            }
        } else if (mentalState.fear > 0.7f) {
            // Run away from others
            InfectedNPC* nearest = FindNearestNPC(population);
            if (nearest && Vector2Distance(position, nearest->position) < 100) {
                Vector2 awayDir = Vector2Subtract(position, nearest->position);
                awayDir = Vector2Normalize(awayDir);
                targetPosition = Vector2Add(position, Vector2Scale(awayDir, 200));
            }
        } else if (mentalState.confusion > 0.7f) {
            // Random movement
            targetPosition = {
                position.x + GetRandomValue(-200, 200),
                position.y + GetRandomValue(-200, 200)
            };
        } else if (mentalState.hivemind > 0.7f) {
            // Move towards center of infected group
            Vector2 center = CalculateInfectedCenter(population);
            targetPosition = center;
        } else if (mentalState.chaos > 0.7f) {
            // Erratic movement
            float angle = GetRandomValue(0, 360) * DEG2RAD;
            targetPosition = {
                position.x + cos(angle) * 150,
                position.y + sin(angle) * 150
            };
        } else {
            // Normal behavior with some modifications
            if (GetRandomValue(0, 100) < 20) {
                targetPosition = {
                    position.x + GetRandomValue(-100, 100),
                    position.y + GetRandomValue(-100, 100)
                };
            }
        }
    }
    
    void Move(float dt) {
        float speed = 80.0f * (1.0f - mentalState.confusion * 0.5f);
        if (mentalState.chaos > 0.7f) speed *= 1.5f;
        if (mentalState.fear > 0.7f) speed *= 1.2f;
        
        if (Vector2Distance(position, targetPosition) > 5) {
            Vector2 direction = Vector2Subtract(targetPosition, position);
            direction = Vector2Normalize(direction);
            position = Vector2Add(position, Vector2Scale(direction, speed * dt));
        }
    }
    
    InfectedNPC* FindNearestNPC(std::vector<InfectedNPC*>& population) {
        InfectedNPC* nearest = nullptr;
        float minDist = INFINITY;
        
        for (auto npc : population) {
            if (npc->id != id && npc->isAlive) {
                float dist = Vector2Distance(position, npc->position);
                if (dist < minDist) {
                    minDist = dist;
                    nearest = npc;
                }
            }
        }
        
        return nearest;
    }
    
    Vector2 CalculateInfectedCenter(std::vector<InfectedNPC*>& population) {
        Vector2 center = {0, 0};
        int count = 0;
        
        for (auto npc : population) {
            if (npc->infectionStatus == VirusStatus::ACTIVE) {
                center.x += npc->position.x;
                center.y += npc->position.y;
                count++;
            }
        }
        
        if (count > 0) {
            center.x /= count;
            center.y /= count;
        }
        
        return center;
    }
    
    void RecoverMentalState(float dt) {
        // Gradual recovery
        mentalState.aggression *= (1.0f - 0.1f * dt);
        mentalState.confusion *= (1.0f - 0.1f * dt);
        mentalState.obedience *= (1.0f - 0.1f * dt);
        mentalState.paranoia *= (1.0f - 0.1f * dt);
        mentalState.greed *= (1.0f - 0.1f * dt);
        mentalState.fear *= (1.0f - 0.1f * dt);
        mentalState.chaos *= (1.0f - 0.1f * dt);
        mentalState.hivemind *= (1.0f - 0.1f * dt);
        
        mentalState.sanity = std::min(1.0f, mentalState.sanity + 0.05f * dt);
        mentalState.willpower = std::min(1.0f, mentalState.willpower + 0.03f * dt);
        mentalState.consciousness = std::min(1.0f, mentalState.consciousness + 0.02f * dt);
        
        // Restore original color
        currentColor = ColorLerp(currentColor, originalColor, 0.1f * dt);
    }
    
    void Draw() {
        if (!isAlive) return;
        
        // Draw NPC
        DrawCircleV(position, 18, currentColor);
        DrawCircleV(position, 7, WHITE);
        
        // Draw name
        DrawText(name.c_str(), position.x - 20, position.y - 40, 12, WHITE);
        
        // Draw infection indicator
        if (infectionStatus != VirusStatus::DORMANT && virus) {
            // Draw virus aura
            float pulse = (sin(GetTime() * 3) + 1) / 2;
            DrawCircleLines(position.x, position.y, 25 + pulse * 5, 
                          ColorAlpha(virus->virusColor, 0.5f));
            
            // Draw infection progress
            DrawRectangle(position.x - 20, position.y - 35, 40, 6, DARKGRAY);
            DrawRectangle(position.x - 20, position.y - 35, 
                        40 * infectionProgress, 6, virus->virusColor);
        }
        
        // Draw mental state indicators
        if (mentalState.aggression > 0.5f) {
            DrawText("AGGR", position.x - 25, position.y + 25, 10, RED);
        }
        if (mentalState.confusion > 0.5f) {
            DrawText("CONF", position.x - 10, position.y + 25, 10, PURPLE);
        }
        if (mentalState.fear > 0.5f) {
            DrawText("FEAR", position.x + 5, position.y + 25, 10, DARKGRAY);
        }
        if (mentalState.hivemind > 0.5f) {
            DrawText("HIVE", position.x + 20, position.y + 25, 10, CYAN);
        }
        
        // Draw connection lines for hive mind
        if (mentalState.hivemind > 0.7f && infectionStatus == VirusStatus::ACTIVE) {
            DrawCircleLines(position.x, position.y, 30, ColorAlpha(CYAN, 0.3f));
        }
    }
};

// Virus Outbreak Manager
class VirusOutbreakManager {
private:
    std::vector<InfectedNPC*> npcs;
    std::vector<std::shared_ptr<DigitalMindVirus>> viruses;
    std::vector<std::string> outbreakLog;
    float outbreakTimer;
    float infectionRate;
    float cureRate;
    bool outbreakActive;
    
    // Statistics
    int totalInfected;
    int totalCured;
    int totalDeaths;
    int peakInfected;
    
public:
    VirusOutbreakManager() : outbreakTimer(0), infectionRate(0.3f), cureRate(0.1f),
                            outbreakActive(false), totalInfected(0), totalCured(0),
                            totalDeaths(0), peakInfected(0) {}
    
    void Initialize(int npcCount) {
        // Create NPCs
        for (int i = 0; i < npcCount; i++) {
            Vector2 pos = {
                GetRandomValue(100, SCREEN_WIDTH - 100),
                GetRandomValue(100, SCREEN_HEIGHT - 100)
            };
            
            Color color = ColorFromHSV(GetRandomValue(0, 360), 0.6f, 0.9f);
            InfectedNPC* npc = new InfectedNPC(i, "NPC_" + std::to_string(i), pos, color);
            npcs.push_back(npc);
        }
        
        // Create initial virus
        CreateVirus("MindVirus-Alpha", VirusStrain::AGGRESSION);
    }
    
    void CreateVirus(const std::string& name, VirusStrain strain) {
        auto virus = std::make_shared<DigitalMindVirus>(name, strain);
        viruses.push_back(virus);
        
        // Infect a random NPC
        if (!npcs.empty()) {
            int index = GetRandomValue(0, npcs.size() - 1);
            npcs[index]->Infect(virus);
            totalInfected++;
            outbreakActive = true;
            
            outbreakLog.push_back("Outbreak started: " + name);
        }
    }
    
    void Update(float dt) {
        outbreakTimer += dt;
        
        // Update all NPCs
        for (auto npc : npcs) {
            npc->Update(dt, npcs);
        }
        
        // Update viruses
        for (auto virus : viruses) {
            virus->Update(dt);
        }
        
        // Update statistics
        UpdateStatistics();
        
        // Check for outbreak control
        if (GetInfectedCount() == 0 && outbreakActive) {
            outbreakActive = false;
            outbreakLog.push_back("Outbreak contained at " + std::to_string(outbreakTimer) + "s");
        }
    }
    
    void UpdateStatistics() {
        int currentInfected = GetInfectedCount();
        peakInfected = std::max(peakInfected, currentInfected);
    }
    
    int GetInfectedCount() {
        int count = 0;
        for (auto npc : npcs) {
            if (npc->infectionStatus == VirusStatus::ACTIVE || 
                npc->infectionStatus == VirusStatus::INCUBATING) {
                count++;
            }
        }
        return count;
    }
    
    void ApplyCure() {
        for (auto npc : npcs) {
            if (npc->infectionStatus == VirusStatus::ACTIVE) {
                float cureChance = cureRate;
                if (GetRandomValue(0, 1000) < cureChance * 1000) {
                    npc->infectionStatus = VirusStatus::REMISSION;
                    npc->infectionLog.push_back("Cure administered");
                }
            }
        }
    }
    
    void SpreadVirus() {
        for (auto npc : npcs) {
            if (npc->infectionStatus == VirusStatus::ACTIVE && npc->virus) {
                // Increased infection rate temporarily
                npc->virus->infectionRate *= 1.5f;
            }
        }
    }
    
    void Draw() {
        // Draw NPCs
        for (auto npc : npcs) {
            npc->Draw();
        }
        
        // Draw viruses
        for (auto virus : viruses) {
            if (virus->status == VirusStatus::ACTIVE) {
                // Draw virus at origin point
                if (!npcs.empty()) {
                    // Find first infected NPC
                    for (auto npc : npcs) {
                        if (npc->virus == virus) {
                            virus->Draw(npc->position, 20);
                            break;
                        }
                    }
                }
            }
        }
        
        // Draw hive mind connections
        DrawHiveMindConnections();
    }
    
    void DrawHiveMindConnections() {
        std::vector<InfectedNPC*> hiveNPCs;
        
        for (auto npc : npcs) {
            if (npc->mentalState.hivemind > 0.7f && 
                npc->infectionStatus == VirusStatus::ACTIVE) {
                hiveNPCs.push_back(npc);
            }
        }
        
        for (size_t i = 0; i < hiveNPCs.size(); i++) {
            for (size_t j = i + 1; j < hiveNPCs.size(); j++) {
                DrawLineEx(hiveNPCs[i]->position, hiveNPCs[j]->position, 
                          2, ColorAlpha(CYAN, 0.3f));
            }
        }
    }
    
    void DrawUI() {
        // Statistics panel
        DrawRectangle(10, 10, 350, 250, ColorAlpha(BLACK, 0.8f));
        DrawRectangleLines(10, 10, 350, 250, GOLD);
        
        DrawText("DIGITAL MIND VIRUS", 20, 20, 16, YELLOW);
        DrawText(TextFormat("Outbreak Time: %.1fs", outbreakTimer), 20, 45, 14, WHITE);
        DrawText(TextFormat("Infected: %d / %zu", GetInfectedCount(), npcs.size()), 
                20, 65, 14, RED);
        DrawText(TextFormat("Peak Infected: %d", peakInfected), 20, 85, 14, ORANGE);
        DrawText(TextFormat("Total Infected: %d", totalInfected), 20, 105, 14, WHITE);
        DrawText(TextFormat("Total Cured: %d", totalCured), 20, 125, 14, GREEN);
        
        // Virus information
        if (!viruses.empty()) {
            auto virus = viruses[0];
            DrawText("VIRUS INFO:", 20, 150, 14, YELLOW);
            DrawText(TextFormat("Name: %s", virus->name.c_str()), 20, 170, 12, WHITE);
            DrawText(TextFormat("Generation: %d", virus->generation), 20, 190, 12, WHITE);
            DrawText(TextFormat("Infection Rate: %.2f", virus->infectionRate), 20, 210, 12, RED);
            DrawText(TextFormat("Virulence: %.2f", virus->virulence), 20, 230, 12, ORANGE);
        }
        
        // Controls
        DrawText("Controls:", SCREEN_WIDTH - 200, 10, 16, WHITE);
        DrawText("C: Apply Cure", SCREEN_WIDTH - 200, 35, 14, GREEN);
        DrawText("S: Spread Virus", SCREEN_WIDTH - 200, 55, 14, RED);
        DrawText("M: Force Mutation", SCREEN_WIDTH - 200, 75, 14, PURPLE);
        DrawText("R: Reset Outbreak", SCREEN_WIDTH - 200, 95, 14, GRAY);
        
        // Outbreak log
        DrawText("OUTBREAK LOG:", 400, 10, 14, YELLOW);
        int logY = 35;
        for (int i = outbreakLog.size() - 1; i >= 0 && logY < 200; i--) {
            DrawText(outbreakLog[i].c_str(), 400, logY, 12, WHITE);
            logY += 20;
        }
    }
    
    void ForceMutation() {
        for (auto virus : viruses) {
            virus->Mutate();
            outbreakLog.push_back("Virus mutated to generation " + 
                                std::to_string(virus->generation));
        }
    }
    
    void ResetOutbreak() {
        // Clear NPCs
        for (auto npc : npcs) {
            delete npc;
        }
        npcs.clear();
        viruses.clear();
        outbreakLog.clear();
        outbreakTimer = 0;
        totalInfected = 0;
        totalCured = 0;
        peakInfected = 0;
        
        // Reinitialize
        Initialize(20);
    }
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Digital Mind Virus Infection System");
    SetTargetFPS(60);
    
    // Create outbreak manager
    VirusOutbreakManager outbreakManager;
    outbreakManager.Initialize(20);
    
    // Game loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Update
        outbreakManager.Update(dt);
        
        // Handle input
        if (IsKeyPressed(KEY_C)) {
            outbreakManager.ApplyCure();
        }
        if (IsKeyPressed(KEY_S)) {
            outbreakManager.SpreadVirus();
        }
        if (IsKeyPressed(KEY_M)) {
            outbreakManager.ForceMutation();
        }
        if (IsKeyPressed(KEY_R)) {
            outbreakManager.ResetOutbreak();
        }
        
        // Draw
        BeginDrawing();
        ClearBackground(Color{20, 20, 40, 255});
        
        // Draw title
        DrawText("Digital Mind Virus Infection System", SCREEN_WIDTH/2 - 200, 10, 30, WHITE);
        
        // Draw NPCs and viruses
        outbreakManager.Draw();
        
        // Draw UI
        outbreakManager.DrawUI();
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}