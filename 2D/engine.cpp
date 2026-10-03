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
#include <fstream>

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;

// Neural Network for NPC evolution
class NeuralNetwork {
private:
    std::vector<std::vector<float>> weights1;
    std::vector<float> bias1;
    std::vector<std::vector<float>> weights2;
    std::vector<float> bias2;
    int inputSize;
    int hiddenSize;
    int outputSize;
    
public:
    NeuralNetwork(int input, int hidden, int output) 
        : inputSize(input), hiddenSize(hidden), outputSize(output) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, 0.5f);
        
        // Initialize weights
        weights1.resize(inputSize, std::vector<float>(hiddenSize));
        bias1.resize(hiddenSize);
        weights2.resize(hiddenSize, std::vector<float>(outputSize));
        bias2.resize(outputSize);
        
        for (int i = 0; i < inputSize; i++) {
            for (int j = 0; j < hiddenSize; j++) {
                weights1[i][j] = dist(gen);
            }
        }
        
        for (int i = 0; i < hiddenSize; i++) {
            bias1[i] = dist(gen);
            for (int j = 0; j < outputSize; j++) {
                weights2[i][j] = dist(gen);
            }
        }
        
        for (int i = 0; i < outputSize; i++) {
            bias2[i] = dist(gen);
        }
    }
    
    std::vector<float> Forward(const std::vector<float>& input) {
        std::vector<float> hidden(hiddenSize);
        std::vector<float> output(outputSize);
        
        // Hidden layer
        for (int j = 0; j < hiddenSize; j++) {
            float sum = bias1[j];
            for (int i = 0; i < inputSize && i < input.size(); i++) {
                sum += input[i] * weights1[i][j];
            }
            hidden[j] = tanh(sum);
        }
        
        // Output layer
        for (int j = 0; j < outputSize; j++) {
            float sum = bias2[j];
            for (int i = 0; i < hiddenSize; i++) {
                sum += hidden[i] * weights2[i][j];
            }
            output[j] = tanh(sum);
        }
        
        return output;
    }
    
    void Mutate(float mutationRate, float mutationAmount) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::normal_distribution<float> mutationDist(0.0f, mutationAmount);
        
        // Mutate weights1
        for (int i = 0; i < inputSize; i++) {
            for (int j = 0; j < hiddenSize; j++) {
                if (dist(gen) < mutationRate) {
                    weights1[i][j] += mutationDist(gen);
                }
            }
        }
        
        // Mutate bias1
        for (int i = 0; i < hiddenSize; i++) {
            if (dist(gen) < mutationRate) {
                bias1[i] += mutationDist(gen);
            }
        }
        
        // Mutate weights2
        for (int i = 0; i < hiddenSize; i++) {
            for (int j = 0; j < outputSize; j++) {
                if (dist(gen) < mutationRate) {
                    weights2[i][j] += mutationDist(gen);
                }
            }
        }
        
        // Mutate bias2
        for (int i = 0; i < outputSize; i++) {
            if (dist(gen) < mutationRate) {
                bias2[i] += mutationDist(gen);
            }
        }
    }
    
    NeuralNetwork Crossover(const NeuralNetwork& other) {
        NeuralNetwork child(inputSize, hiddenSize, outputSize);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        // Crossover weights1
        for (int i = 0; i < inputSize; i++) {
            for (int j = 0; j < hiddenSize; j++) {
                if (dist(gen) < 0.5f) {
                    child.weights1[i][j] = weights1[i][j];
                } else {
                    child.weights1[i][j] = other.weights1[i][j];
                }
            }
        }
        
        // Crossover bias1
        for (int i = 0; i < hiddenSize; i++) {
            if (dist(gen) < 0.5f) {
                child.bias1[i] = bias1[i];
            } else {
                child.bias1[i] = other.bias1[i];
            }
        }
        
        // Crossover weights2
        for (int i = 0; i < hiddenSize; i++) {
            for (int j = 0; j < outputSize; j++) {
                if (dist(gen) < 0.5f) {
                    child.weights2[i][j] = weights2[i][j];
                } else {
                    child.weights2[i][j] = other.weights2[i][j];
                }
            }
        }
        
        // Crossover bias2
        for (int i = 0; i < outputSize; i++) {
            if (dist(gen) < 0.5f) {
                child.bias2[i] = bias2[i];
            } else {
                child.bias2[i] = other.bias2[i];
            }
        }
        
        return child;
    }
    
    void SaveToFile(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return;
        
        file.write((char*)&inputSize, sizeof(int));
        file.write((char*)&hiddenSize, sizeof(int));
        file.write((char*)&outputSize, sizeof(int));
        
        for (auto& row : weights1) {
            file.write((char*)row.data(), row.size() * sizeof(float));
        }
        file.write((char*)bias1.data(), bias1.size() * sizeof(float));
        for (auto& row : weights2) {
            file.write((char*)row.data(), row.size() * sizeof(float));
        }
        file.write((char*)bias2.data(), bias2.size() * sizeof(float));
        
        file.close();
    }
    
    void LoadFromFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return;
        
        file.read((char*)&inputSize, sizeof(int));
        file.read((char*)&hiddenSize, sizeof(int));
        file.read((char*)&outputSize, sizeof(int));
        
        weights1.resize(inputSize, std::vector<float>(hiddenSize));
        bias1.resize(hiddenSize);
        weights2.resize(hiddenSize, std::vector<float>(outputSize));
        bias2.resize(outputSize);
        
        for (auto& row : weights1) {
            file.read((char*)row.data(), row.size() * sizeof(float));
        }
        file.read((char*)bias1.data(), bias1.size() * sizeof(float));
        for (auto& row : weights2) {
            file.read((char*)row.data(), row.size() * sizeof(float));
        }
        file.read((char*)bias2.data(), bias2.size() * sizeof(float));
        
        file.close();
    }
};

// Evolution traits and genes
struct Genes {
    float speed;
    float strength;
    float intelligence;
    float sociability;
    float adaptability;
    float aggression;
    float curiosity;
    float resilience;
    
    Genes() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.1f, 1.0f);
        
        speed = dist(gen);
        strength = dist(gen);
        intelligence = dist(gen);
        sociability = dist(gen);
        adaptability = dist(gen);
        aggression = dist(gen);
        curiosity = dist(gen);
        resilience = dist(gen);
    }
    
    void Mutate(float rate) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::normal_distribution<float> mutationDist(0.0f, 0.1f);
        
        if (dist(gen) < rate) speed = Clamp(speed + mutationDist(gen), 0.1f, 1.0f);
        if (dist(gen) < rate) strength = Clamp(strength + mutationDist(gen), 0.1f, 1.0f);
        if (dist(gen) < rate) intelligence = Clamp(intelligence + mutationDist(gen), 0.1f, 1.0f);
        if (dist(gen) < rate) sociability = Clamp(sociability + mutationDist(gen), 0.1f, 1.0f);
        if (dist(gen) < rate) adaptability = Clamp(adaptability + mutationDist(gen), 0.1f, 1.0f);
        if (dist(gen) < rate) aggression = Clamp(aggression + mutationDist(gen), 0.1f, 1.0f);
        if (dist(gen) < rate) curiosity = Clamp(curiosity + mutationDist(gen), 0.1f, 1.0f);
        if (dist(gen) < rate) resilience = Clamp(resilience + mutationDist(gen), 0.1f, 1.0f);
    }
    
    float Clamp(float value, float min, float max) {
        return std::max(min, std::min(max, value));
    }
};

// Evolved NPC class
class EvolvedNPC {
public:
    int id;
    std::string name;
    Vector2 position;
    Color color;
    int generation;
    float age;
    float health;
    float energy;
    float fitness;
    bool isAlive;
    
    Genes genes;
    NeuralNetwork brain;
    std::vector<float> memory;
    std::map<std::string, float> skills;
    std::vector<std::string> learnedBehaviors;
    
    // Evolution tracking
    float survivalTime;
    int interactions;
    int successfulInteractions;
    int resourcesCollected;
    int offspringCount;
    
    // Behavior state
    Vector2 targetPosition;
    float decisionTimer;
    float socialTimer;
    int socialTarget;
    
    EvolvedNPC(int npcId, const std::string& npcName, Vector2 pos, Color npcColor, int gen = 0)
        : id(npcId), name(npcName), position(pos), color(npcColor), generation(gen),
          age(0), health(100), energy(100), fitness(0), isAlive(true),
          brain(8, 16, 4), survivalTime(0), interactions(0),
          successfulInteractions(0), resourcesCollected(0), offspringCount(0),
          decisionTimer(0), socialTimer(0), socialTarget(-1) {
        
        targetPosition = position;
        memory.resize(10, 0.0f);
        
        // Initialize skills based on genes
        skills["movement"] = genes.speed * 10;
        skills["combat"] = genes.strength * 10;
        skills["learning"] = genes.intelligence * 10;
        skills["social"] = genes.sociability * 10;
        skills["survival"] = genes.resilience * 10;
    }
    
    void Update(float dt, std::vector<EvolvedNPC*>& population, 
                std::vector<Vector2>& resources, std::vector<Vector2>& threats) {
        if (!isAlive) return;
        
        age += dt;
        survivalTime += dt;
        
        // Energy consumption
        energy -= (0.5f + genes.speed * 0.5f + genes.strength * 0.3f) * dt;
        if (energy <= 0) {
            health -= 10 * dt;
            if (health <= 0) {
                isAlive = false;
                return;
            }
        }
        
        // Health regeneration
        if (energy > 50) {
            health = std::min(100.0f, health + genes.resilience * 2 * dt);
        }
        
        // Make decisions using neural network
        decisionTimer -= dt;
        if (decisionTimer <= 0) {
            decisionTimer = 0.5f / (genes.intelligence + 0.1f);
            MakeDecision(population, resources, threats);
        }
        
        // Move towards target
        Move(dt);
        
        // Social interactions
        socialTimer -= dt;
        if (socialTimer <= 0 && genes.sociability > 0.5f) {
            socialTimer = 3.0f / genes.sociability;
            TrySocialInteraction(population);
        }
        
        // Collect resources
        CollectResources(resources);
        
        // Avoid threats
        AvoidThreats(threats);
        
        // Update memory
        UpdateMemory();
        
        // Calculate fitness
        CalculateFitness();
    }
    
    void MakeDecision(std::vector<EvolvedNPC*>& population, 
                      std::vector<Vector2>& resources, std::vector<Vector2>& threats) {
        // Prepare neural network input
        std::vector<float> input = {
            energy / 100.0f,
            health / 100.0f,
            genes.sociability,
            genes.curiosity,
            FindNearestResource(resources).x / SCREEN_WIDTH,
            FindNearestResource(resources).y / SCREEN_HEIGHT,
            FindNearestThreat(threats).x / SCREEN_WIDTH,
            FindNearestThreat(threats).y / SCREEN_HEIGHT
        };
        
        // Get neural network output
        std::vector<float> output = brain.Forward(input);
        
        // Interpret output
        if (output[0] > 0.3f) {
            // Move towards nearest resource
            Vector2 nearestResource = FindNearestResource(resources);
            if (nearestResource.x != -1) {
                targetPosition = nearestResource;
            }
        } else if (output[1] > 0.3f) {
            // Move towards nearest threat (aggressive behavior)
            Vector2 nearestThreat = FindNearestThreat(threats);
            if (nearestThreat.x != -1) {
                targetPosition = nearestThreat;
            }
        } else if (output[2] > 0.3f) {
            // Random exploration
            targetPosition = {
                position.x + GetRandomValue(-200, 200),
                position.y + GetRandomValue(-200, 200)
            };
        } else if (output[3] > 0.3f) {
            // Move towards population center (social behavior)
            Vector2 center = CalculatePopulationCenter(population);
            targetPosition = center;
        }
    }
    
    void Move(float dt) {
        float speed = genes.speed * 100.0f * (energy / 100.0f);
        
        if (Vector2Distance(position, targetPosition) > 5) {
            Vector2 direction = Vector2Subtract(targetPosition, position);
            direction = Vector2Normalize(direction);
            position = Vector2Add(position, Vector2Scale(direction, speed * dt));
        }
    }
    
    void TrySocialInteraction(std::vector<EvolvedNPC*>& population) {
        if (population.empty()) return;
        
        // Find nearest NPC
        EvolvedNPC* nearest = nullptr;
        float minDist = 100.0f;
        
        for (auto npc : population) {
            if (npc->id != id && npc->isAlive) {
                float dist = Vector2Distance(position, npc->position);
                if (dist < minDist) {
                    minDist = dist;
                    nearest = npc;
                }
            }
        }
        
        if (nearest && minDist < 50) {
            interactions++;
            socialTarget = nearest->id;
            
            // Learn from interaction
            if (nearest->genes.intelligence > genes.intelligence) {
                // Learn from smarter NPC
                skills["learning"] += 0.1f;
                learnedBehaviors.push_back("Learned from " + nearest->name);
            }
            
            // Successful interaction based on compatibility
            float compatibility = 1.0f - abs(genes.sociability - nearest->genes.sociability);
            if (compatibility > 0.5f) {
                successfulInteractions++;
                fitness += 0.5f;
                
                // Share knowledge
                ExchangeKnowledge(nearest);
            }
        }
    }
    
    void ExchangeKnowledge(EvolvedNPC* other) {
        // Simple knowledge sharing
        if (other->skills["learning"] > skills["learning"]) {
            skills["learning"] += (other->skills["learning"] - skills["learning"]) * 0.1f;
        }
        
        if (other->skills["survival"] > skills["survival"]) {
            skills["survival"] += (other->skills["survival"] - skills["survival"]) * 0.1f;
        }
    }
    
    void CollectResources(std::vector<Vector2>& resources) {
        for (auto it = resources.begin(); it != resources.end();) {
            if (Vector2Distance(position, *it) < 20) {
                resourcesCollected++;
                energy = std::min(100.0f, energy + 30.0f);
                fitness += 2.0f;
                it = resources.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void AvoidThreats(std::vector<Vector2>& threats) {
        for (const auto& threat : threats) {
            float dist = Vector2Distance(position, threat);
            if (dist < 50) {
                // Move away from threat
                Vector2 awayDir = Vector2Subtract(position, threat);
                awayDir = Vector2Normalize(awayDir);
                targetPosition = Vector2Add(position, Vector2Scale(awayDir, 150));
                
                // Take damage if too close
                if (dist < 20) {
                    health -= 20 * GetFrameTime();
                    if (health <= 0) {
                        isAlive = false;
                    }
                }
            }
        }
    }
    
    void UpdateMemory() {
        // Shift memory
        for (int i = memory.size() - 1; i > 0; i--) {
            memory[i] = memory[i - 1];
        }
        
        // Store current state
        memory[0] = (energy + health) / 200.0f;
    }
    
    void CalculateFitness() {
        fitness = survivalTime * 0.1f + 
                 successfulInteractions * 2.0f + 
                 resourcesCollected * 3.0f + 
                 offspringCount * 10.0f + 
                 skills["learning"] + 
                 skills["survival"];
    }
    
    Vector2 FindNearestResource(const std::vector<Vector2>& resources) {
        Vector2 nearest = {-1, -1};
        float minDist = INFINITY;
        
        for (const auto& resource : resources) {
            float dist = Vector2Distance(position, resource);
            if (dist < minDist) {
                minDist = dist;
                nearest = resource;
            }
        }
        
        return nearest;
    }
    
    Vector2 FindNearestThreat(const std::vector<Vector2>& threats) {
        Vector2 nearest = {-1, -1};
        float minDist = INFINITY;
        
        for (const auto& threat : threats) {
            float dist = Vector2Distance(position, threat);
            if (dist < minDist) {
                minDist = dist;
                nearest = threat;
            }
        }
        
        return nearest;
    }
    
    Vector2 CalculatePopulationCenter(const std::vector<EvolvedNPC*>& population) {
        Vector2 center = {0, 0};
        int count = 0;
        
        for (auto npc : population) {
            if (npc->isAlive) {
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
    
    EvolvedNPC* Reproduce(EvolvedNPC* partner) {
        if (!isAlive || !partner->isAlive || energy < 50 || partner->energy < 50) {
            return nullptr;
        }
        
        offspringCount++;
        partner->offspringCount++;
        
        // Create child with combined genes
        EvolvedNPC* child = new EvolvedNPC(
            id * 100 + offspringCount,
            name + " Jr.",
            {position.x + GetRandomValue(-20, 20), position.y + GetRandomValue(-20, 20)},
            ColorLerp(color, partner->color, 0.5f),
            generation + 1
        );
        
        // Crossover genes
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        child->genes.speed = dist(gen) < 0.5f ? genes.speed : partner->genes.speed;
        child->genes.strength = dist(gen) < 0.5f ? genes.strength : partner->genes.strength;
        child->genes.intelligence = dist(gen) < 0.5f ? genes.intelligence : partner->genes.intelligence;
        child->genes.sociability = dist(gen) < 0.5f ? genes.sociability : partner->genes.sociability;
        child->genes.adaptability = dist(gen) < 0.5f ? genes.adaptability : partner->genes.adaptability;
        child->genes.aggression = dist(gen) < 0.5f ? genes.aggression : partner->genes.aggression;
        child->genes.curiosity = dist(gen) < 0.5f ? genes.curiosity : partner->genes.curiosity;
        child->genes.resilience = dist(gen) < 0.5f ? genes.resilience : partner->genes.resilience;
        
        // Mutate child's genes
        child->genes.Mutate(0.1f);
        
        // Crossover neural networks
        child->brain = brain.Crossover(partner->brain);
        child->brain.Mutate(0.05f, 0.1f);
        
        // Reduce parent energy
        energy -= 25;
        partner->energy -= 25;
        
        return child;
    }
    
    void Draw() {
        if (!isAlive) return;
        
        // Draw NPC
        DrawCircleV(position, 15, color);
        DrawCircleV(position, 5, WHITE);
        
        // Draw generation indicator
        DrawText(TextFormat("G%d", generation), position.x - 10, position.y - 30, 10, WHITE);
        
        // Draw energy bar
        DrawRectangle(position.x - 15, position.y - 25, 30, 5, DARKGRAY);
        DrawRectangle(position.x - 15, position.y - 25, 30 * (energy / 100), 5, GREEN);
        
        // Draw health bar
        DrawRectangle(position.x - 15, position.y - 20, 30, 5, DARKGRAY);
        DrawRectangle(position.x - 15, position.y - 20, 30 * (health / 100), 5, RED);
        
        // Draw fitness indicator
        if (fitness > 50) {
            DrawText("*", position.x + 15, position.y - 15, 15, GOLD);
        }
    }
};

// Evolution Engine
class EvolutionEngine {
private:
    std::vector<EvolvedNPC*> population;
    std::vector<Vector2> resources;
    std::vector<Vector2> threats;
    int generation;
    float generationTimer;
    float evolutionRate;
    
    // Statistics
    float averageFitness;
    float maxFitness;
    float averageLifespan;
    int totalBirths;
    int totalDeaths;
    
    // Selection and reproduction
    float reproductionThreshold;
    float mutationRate;
    
public:
    EvolutionEngine() : generation(1), generationTimer(0), evolutionRate(0.1f),
                       averageFitness(0), maxFitness(0), averageLifespan(0),
                       totalBirths(0), totalDeaths(0), reproductionThreshold(0.7f),
                       mutationRate(0.1f) {}
    
    void Initialize(int initialPopulation) {
        // Create initial population
        for (int i = 0; i < initialPopulation; i++) {
            Vector2 pos = {
                GetRandomValue(100, SCREEN_WIDTH - 100),
                GetRandomValue(100, SCREEN_HEIGHT - 100)
            };
            
            Color color = ColorFromHSV(GetRandomValue(0, 360), 0.7f, 1.0f);
            EvolvedNPC* npc = new EvolvedNPC(i, "NPC_" + std::to_string(i), pos, color, 1);
            population.push_back(npc);
        }
        
        // Create initial resources
        for (int i = 0; i < 50; i++) {
            resources.push_back({
                GetRandomValue(50, SCREEN_WIDTH - 50),
                GetRandomValue(50, SCREEN_HEIGHT - 50)
            });
        }
        
        // Create initial threats
        for (int i = 0; i < 5; i++) {
            threats.push_back({
                GetRandomValue(100, SCREEN_WIDTH - 100),
                GetRandomValue(100, SCREEN_HEIGHT - 100)
            });
        }
    }
    
    void Update(float dt) {
        generationTimer += dt;
        
        // Update all NPCs
        for (auto npc : population) {
            npc->Update(dt, population, resources, threats);
        }
        
        // Remove dead NPCs
        RemoveDead();
        
        // Reproduce
        Reproduce();
        
        // Respawn resources
        RespawnResources();
        
        // Move threats
        MoveThreats(dt);
        
        // Update statistics
        UpdateStatistics();
        
        // Check for generation evolution
        if (generationTimer > 30.0f) { // Every 30 seconds
            EvolveGeneration();
            generationTimer = 0;
        }
    }
    
    void RemoveDead() {
        for (auto it = population.begin(); it != population.end();) {
            if (!(*it)->isAlive) {
                totalDeaths++;
                averageLifespan += (*it)->age;
                delete *it;
                it = population.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void Reproduce() {
        std::vector<EvolvedNPC*> newChildren;
        
        for (size_t i = 0; i < population.size() && newChildren.size() < 10; i++) {
            for (size_t j = i + 1; j < population.size() && newChildren.size() < 10; j++) {
                EvolvedNPC* npc1 = population[i];
                EvolvedNPC* npc2 = population[j];
                
                if (npc1->energy > 50 && npc2->energy > 50 &&
                    Vector2Distance(npc1->position, npc2->position) < 100) {
                    
                    // Check fitness for reproduction
                    float combinedFitness = (npc1->fitness + npc2->fitness) / 2;
                    if (combinedFitness > reproductionThreshold * 10) {
                        EvolvedNPC* child = npc1->Reproduce(npc2);
                        if (child) {
                            newChildren.push_back(child);
                            totalBirths++;
                            
                            if (newChildren.size() >= 10) break;
                        }
                    }
                }
            }
        }
        
        // Add children to population
        for (auto child : newChildren) {
            population.push_back(child);
        }
    }
    
    void RespawnResources() {
        if (resources.size() < 30) {
            int toAdd = 50 - resources.size();
            for (int i = 0; i < toAdd; i++) {
                resources.push_back({
                    GetRandomValue(50, SCREEN_WIDTH - 50),
                    GetRandomValue(50, SCREEN_HEIGHT - 50)
                });
            }
        }
    }
    
    void MoveThreats(float dt) {
        for (auto& threat : threats) {
            // Simple threat movement
            threat.x += sin(GetTime() + threat.y) * 50 * dt;
            threat.y += cos(GetTime() + threat.x) * 50 * dt;
            
            // Keep in bounds
            threat.x = Clamp(threat.x, 0, SCREEN_WIDTH);
            threat.y = Clamp(threat.y, 0, SCREEN_HEIGHT);
        }
    }
    
    void UpdateStatistics() {
        if (population.empty()) return;
        
        averageFitness = 0;
        maxFitness = 0;
        
        for (auto npc : population) {
            averageFitness += npc->fitness;
            maxFitness = std::max(maxFitness, npc->fitness);
        }
        
        averageFitness /= population.size();
    }
    
    void EvolveGeneration() {
        if (population.empty()) return;
        
        generation++;
        
        // Apply evolution pressure
        float fitnessThreshold = averageFitness * 1.2f;
        
        for (auto npc : population) {
            if (npc->fitness > fitnessThreshold) {
                // High fitness - less mutation
                npc->genes.Mutate(mutationRate * 0.5f);
                npc->brain.Mutate(mutationRate * 0.5f, 0.05f);
            } else {
                // Low fitness - more mutation
                npc->genes.Mutate(mutationRate * 2.0f);
                npc->brain.Mutate(mutationRate * 2.0f, 0.2f);
            }
        }
    }
    
    void Draw() {
        // Draw resources
        for (const auto& resource : resources) {
            DrawCircleV(resource, 5, GREEN);
            DrawCircleV(resource, 2, DARKGREEN);
        }
        
        // Draw threats
        for (const auto& threat : threats) {
            DrawCircleV(threat, 10, RED);
            DrawCircleV(threat, 5, ORANGE);
        }
        
        // Draw population
        for (auto npc : population) {
            npc->Draw();
        }
    }
    
    void DrawUI() {
        // Statistics panel
        DrawRectangle(10, 10, 300, 200, ColorAlpha(BLACK, 0.8f));
        DrawRectangleLines(10, 10, 300, 200, GOLD);
        
        DrawText("EVOLUTION ENGINE", 20, 20, 16, YELLOW);
        DrawText(TextFormat("Generation: %d", generation), 20, 45, 14, WHITE);
        DrawText(TextFormat("Population: %zu", population.size()), 20, 65, 14, WHITE);
        DrawText(TextFormat("Avg Fitness: %.2f", averageFitness), 20, 85, 14, GREEN);
        DrawText(TextFormat("Max Fitness: %.2f", maxFitness), 20, 105, 14, GOLD);
        DrawText(TextFormat("Births: %d", totalBirths), 20, 125, 14, CYAN);
        DrawText(TextFormat("Deaths: %d", totalDeaths), 20, 145, 14, RED);
        DrawText(TextFormat("Avg Lifespan: %.1fs", 
                totalDeaths > 0 ? averageLifespan / totalDeaths : 0), 
                20, 165, 14, WHITE);
        
        // Controls
        DrawText("Controls:", SCREEN_WIDTH - 200, 10, 16, WHITE);
        DrawText("SPACE: Force Evolution", SCREEN_WIDTH - 200, 35, 14, GRAY);
        DrawText("R: Reset Population", SCREEN_WIDTH - 200, 55, 14, GRAY);
        DrawText("S: Save Evolution", SCREEN_WIDTH - 200, 75, 14, GRAY);
        DrawText("L: Load Evolution", SCREEN_WIDTH - 200, 95, 14, GRAY);
        
        // Generation progress
        DrawText(TextFormat("Next Evolution: %.0fs", 30.0f - generationTimer), 
                10, SCREEN_HEIGHT - 30, 14, WHITE);
        
        // Draw progress bar
        DrawRectangle(10, SCREEN_HEIGHT - 15, 200, 10, DARKGRAY);
        DrawRectangle(10, SCREEN_HEIGHT - 15, 200 * (generationTimer / 30.0f), 10, GOLD);
    }
    
    void ForceEvolution() {
        EvolveGeneration();
        generationTimer = 0;
    }
    
    void ResetPopulation() {
        // Clear population
        for (auto npc : population) {
            delete npc;
        }
        population.clear();
        
        // Reset statistics
        generation = 1;
        totalBirths = 0;
        totalDeaths = 0;
        averageLifespan = 0;
        
        // Initialize new population
        Initialize(10);
    }
    
    void SaveEvolution(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return;
        
        // Save generation and statistics
        file.write((char*)&generation, sizeof(int));
        file.write((char*)&totalBirths, sizeof(int));
        file.write((char*)&totalDeaths, sizeof(int));
        
        // Save population size
        size_t popSize = population.size();
        file.write((char*)&popSize, sizeof(size_t));
        
        // Save each NPC's brain
        for (auto npc : population) {
            file.write((char*)&npc->generation, sizeof(int));
            file.write((char*)&npc->genes, sizeof(Genes));
            npc->brain.SaveToFile(filename + "_brain_" + std::to_string(npc->id));
        }
        
        file.close();
    }
    
    void LoadEvolution(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return;
        
        // Clear current population
        for (auto npc : population) {
            delete npc;
        }
        population.clear();
        
        // Load generation and statistics
        file.read((char*)&generation, sizeof(int));
        file.read((char*)&totalBirths, sizeof(int));
        file.read((char*)&totalDeaths, sizeof(int));
        
        // Load population size
        size_t popSize;
        file.read((char*)&popSize, sizeof(size_t));
        
        // Load each NPC
        for (size_t i = 0; i < popSize; i++) {
            int npcGen;
            file.read((char*)&npcGen, sizeof(int));
            
            Genes genes;
            file.read((char*)&genes, sizeof(Genes));
            
            Vector2 pos = {
                GetRandomValue(100, SCREEN_WIDTH - 100),
                GetRandomValue(100, SCREEN_HEIGHT - 100)
            };
            
            Color color = ColorFromHSV(GetRandomValue(0, 360), 0.7f, 1.0f);
            EvolvedNPC* npc = new EvolvedNPC(i, "NPC_" + std::to_string(i), pos, color, npcGen);
            npc->genes = genes;
            npc->brain.LoadFromFile(filename + "_brain_" + std::to_string(i));
            population.push_back(npc);
        }
        
        file.close();
    }
    
    float Clamp(float value, float min, float max) {
        return std::max(min, std::min(max, value));
    }
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Self-Evolving Engine");
    SetTargetFPS(60);
    
    // Create evolution engine
    EvolutionEngine engine;
    engine.Initialize(10);
    
    // Game loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Update
        engine.Update(dt);
        
        // Handle input
        if (IsKeyPressed(KEY_SPACE)) {
            engine.ForceEvolution();
        }
        if (IsKeyPressed(KEY_R)) {
            engine.ResetPopulation();
        }
        if (IsKeyPressed(KEY_S)) {
            engine.SaveEvolution("evolution_data.bin");
            std::cout << "Evolution saved!" << std::endl;
        }
        if (IsKeyPressed(KEY_L)) {
            engine.LoadEvolution("evolution_data.bin");
            std::cout << "Evolution loaded!" << std::endl;
        }
        
        // Draw
        BeginDrawing();
        ClearBackground(Color{20, 20, 30, 255});
        
        // Draw title
        DrawText("Self-Evolving Engine", SCREEN_WIDTH/2 - 150, 10, 30, WHITE);
        
        // Draw evolution elements
        engine.Draw();
        
        // Draw UI
        engine.DrawUI();
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}