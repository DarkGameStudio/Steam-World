#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <cmath>
#include <algorithm>
#include <string>
#include <random>
#include <iostream>
#include <sstream>

const int SCREEN_WIDTH = 1080;
const int SCREEN_HEIGHT = 1920;
const int GRID_SIZE = 40;
const int GRID_WIDTH = SCREEN_WIDTH / GRID_SIZE;
const int GRID_HEIGHT = SCREEN_HEIGHT / GRID_SIZE;

// Node types for the grid
enum class NodeType {
    EMPTY,
    WALL,
    GOAL,
    HAZARD,
    COLLECTIBLE
};

// Grid node structure
struct GridNode {
    NodeType type = NodeType::EMPTY;
    bool visited = false;
    float gCost = 0;
    float hCost = 0;
    float fCost = 0;
    Vector2 parent = {-1, -1};
};

// State machine for character
enum class CharacterState {
    IDLE,
    EXPLORING,
    SEEKING_GOAL,
    COLLECTING,
    AVOIDING_HAZARD,
    LEARNING,
    RESTING
};

// Memory types for self-improvement
struct Memory {
    Vector2 position;
    float reward;
    float timestamp;
    int visits;
};

// Character class with self-proving capabilities
class SelfProvingCharacter {
private:
    Vector2 position;
    Vector2 velocity;
    CharacterState currentState;
    CharacterState previousState;
    
    // Grid for pathfinding
    std::vector<std::vector<GridNode>> grid;
    
    // A* pathfinding
    std::vector<Vector2> currentPath;
    size_t pathIndex;
    
    // Self-improvement systems
    std::vector<Memory> memories;
    std::map<std::string, float> skillLevels;
    float confidence;
    float energy;
    float learningRate;
    int experiencePoints;
    int level;
    
    // Statistics
    int goalsReached;
    int hazardsAvoided;
    int itemsCollected;
    float totalDistanceTraveled;
    float successRate;
    
    // Decision making
    Vector2 targetPosition;
    bool hasTarget;
    float decisionTimer;
    
    // Animation
    float animationTime;
    Color characterColor;
    
    // Random generation
    std::mt19937 rng;
    
    // Performance tracking
    std::vector<float> recentPerformances;
    float averagePerformance;
    
    // Personality traits
    float curiosity;
    float cautiousness;
    float persistence;
    
    // Helper methods
    float CalculateDistance(Vector2 a, Vector2 b) {
        return Vector2Distance(a, b);
    }
    
    float CalculateHeuristic(Vector2 a, Vector2 b) {
        // Manhattan distance for grid-based movement
        return abs(a.x - b.x) + abs(a.y - b.y);
    }
    
    bool IsValidGridPosition(int x, int y) {
        return x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT;
    }
    
    bool IsWalkable(int x, int y) {
        if (!IsValidGridPosition(x, y)) return false;
        return grid[y][x].type != NodeType::WALL;
    }
    
    // A* pathfinding algorithm
    std::vector<Vector2> FindPath(Vector2 start, Vector2 goal) {
        Vector2 startGrid = {floor(start.x / GRID_SIZE), floor(start.y / GRID_SIZE)};
        Vector2 goalGrid = {floor(goal.x / GRID_SIZE), floor(goal.y / GRID_SIZE)};
        
        // Reset grid for new pathfinding
        for (auto& row : grid) {
            for (auto& node : row) {
                node.visited = false;
                node.gCost = 0;
                node.hCost = 0;
                node.fCost = 0;
                node.parent = {-1, -1};
            }
        }
        
        // Priority queue for open set
        auto compare = [](const std::pair<float, Vector2>& a, const std::pair<float, Vector2>& b) {
            return a.first > b.first;
        };
        std::priority_queue<std::pair<float, Vector2>, 
                           std::vector<std::pair<float, Vector2>>, 
                           decltype(compare)> openSet(compare);
        
        openSet.push({0, startGrid});
        grid[(int)startGrid.y][(int)startGrid.x].visited = true;
        
        std::vector<Vector2> directions = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}, 
                                          {-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
        
        while (!openSet.empty()) {
            Vector2 current = openSet.top().second;
            openSet.pop();
            
            if (current.x == goalGrid.x && current.y == goalGrid.y) {
                // Reconstruct path
                std::vector<Vector2> path;
                Vector2 currentPathNode = current;
                
                while (currentPathNode.x != -1 && currentPathNode.y != -1) {
                    path.push_back({currentPathNode.x * GRID_SIZE + GRID_SIZE/2, 
                                   currentPathNode.y * GRID_SIZE + GRID_SIZE/2});
                    currentPathNode = grid[(int)currentPathNode.y][(int)currentPathNode.x].parent;
                }
                
                std::reverse(path.begin(), path.end());
                return path;
            }
            
            for (const auto& dir : directions) {
                int newX = (int)current.x + (int)dir.x;
                int newY = (int)current.y + (int)dir.y;
                
                if (!IsWalkable(newX, newY)) continue;
                
                Vector2 neighbor = {(float)newX, (float)newY};
                float tentativeGCost = grid[(int)current.y][(int)current.x].gCost + 
                                      CalculateDistance(current, neighbor);
                
                if (!grid[newY][newX].visited || tentativeGCost < grid[newY][newX].gCost) {
                    grid[newY][newX].visited = true;
                    grid[newY][newX].parent = current;
                    grid[newY][newX].gCost = tentativeGCost;
                    grid[newY][newX].hCost = CalculateHeuristic(neighbor, goalGrid);
                    grid[newY][newX].fCost = grid[newY][newX].gCost + grid[newY][newX].hCost;
                    openSet.push({grid[newY][newX].fCost, neighbor});
                }
            }
        }
        
        return {}; // No path found
    }
    
    // Find nearest node of specific type
    Vector2 FindNearestNode(NodeType type) {
        Vector2 nearest = {-1, -1};
        float minDist = INFINITY;
        
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                if (grid[y][x].type == type) {
                    Vector2 nodePos = {(float)(x * GRID_SIZE + GRID_SIZE/2), 
                                      (float)(y * GRID_SIZE + GRID_SIZE/2)};
                    float dist = CalculateDistance(position, nodePos);
                    
                    if (dist < minDist) {
                        minDist = dist;
                        nearest = nodePos;
                    }
                }
            }
        }
        
        return nearest;
    }
    
    // Store memory of important events
    void StoreMemory(Vector2 pos, float reward) {
        Memory mem;
        mem.position = pos;
        mem.reward = reward;
        mem.timestamp = GetTime();
        mem.visits = 1;
        
        // Check if similar memory exists
        for (auto& existing : memories) {
            if (CalculateDistance(existing.position, pos) < 50) {
                existing.visits++;
                existing.reward = (existing.reward + reward) / 2;
                return;
            }
        }
        
        memories.push_back(mem);
    }
    
    // Update skill levels based on performance
    void UpdateSkills() {
        // Update skills based on recent performance
        if (goalsReached > 0) {
            skillLevels["navigation"] += learningRate * 0.1f;
        }
        if (hazardsAvoided > 0) {
            skillLevels["survival"] += learningRate * 0.15f;
        }
        if (itemsCollected > 0) {
            skillLevels["gathering"] += learningRate * 0.12f;
        }
        
        // Cap skill levels
        for (auto& skill : skillLevels) {
            skill.second = std::min(skill.second, 10.0f);
        }
        
        // Update confidence based on success rate
        if (goalsReached + hazardsAvoided > 0) {
            successRate = (float)goalsReached / (goalsReached + hazardsAvoided);
            confidence += (successRate - 0.5f) * learningRate * 0.2f;
            confidence = std::max(0.0f, std::min(1.0f, confidence));
        }
    }
    
    // Make decisions based on current state and environment
    void MakeDecision() {
        decisionTimer -= GetFrameTime();
        if (decisionTimer > 0) return;
        
        decisionTimer = 0.5f; // Make decision every 0.5 seconds
        
        // Check energy levels
        if (energy < 20) {
            ChangeState(CharacterState::RESTING);
            return;
        }
        
        // Check for nearby hazards
        Vector2 nearestHazard = FindNearestNode(NodeType::HAZARD);
        if (nearestHazard.x != -1 && CalculateDistance(position, nearestHazard) < 100) {
            ChangeState(CharacterState::AVOIDING_HAZARD);
            return;
        }
        
        // Check for collectibles
        Vector2 nearestCollectible = FindNearestNode(NodeType::COLLECTIBLE);
        if (nearestCollectible.x != -1) {
            if (skillLevels["gathering"] > 5 || curiosity > 0.7) {
                ChangeState(CharacterState::COLLECTING);
                return;
            }
        }
        
        // Check for goals
        Vector2 nearestGoal = FindNearestNode(NodeType::GOAL);
        if (nearestGoal.x != -1) {
            ChangeState(CharacterState::SEEKING_GOAL);
            return;
        }
        
        // Explore if nothing else to do
        if (curiosity > 0.3) {
            ChangeState(CharacterState::EXPLORING);
        } else {
            ChangeState(CharacterState::IDLE);
        }
    }
    
    // Execute current state actions
    void ExecuteState() {
        switch (currentState) {
            case CharacterState::IDLE:
                // Stand still, occasionally look around
                velocity = {0, 0};
                break;
                
            case CharacterState::EXPLORING:
                if (!hasTarget || pathIndex >= currentPath.size()) {
                    // Choose random exploration target
                    std::uniform_int_distribution<int> distX(0, GRID_WIDTH - 1);
                    std::uniform_int_distribution<int> distY(0, GRID_HEIGHT - 1);
                    
                    targetPosition = {(float)(distX(rng) * GRID_SIZE), 
                                     (float)(distY(rng) * GRID_SIZE)};
                    currentPath = FindPath(position, targetPosition);
                    pathIndex = 0;
                    hasTarget = !currentPath.empty();
                }
                break;
                
            case CharacterState::SEEKING_GOAL:
                if (!hasTarget || pathIndex >= currentPath.size()) {
                    targetPosition = FindNearestNode(NodeType::GOAL);
                    if (targetPosition.x != -1) {
                        currentPath = FindPath(position, targetPosition);
                        pathIndex = 0;
                        hasTarget = !currentPath.empty();
                    }
                }
                break;
                
            case CharacterState::COLLECTING:
                if (!hasTarget || pathIndex >= currentPath.size()) {
                    targetPosition = FindNearestNode(NodeType::COLLECTIBLE);
                    if (targetPosition.x != -1) {
                        currentPath = FindPath(position, targetPosition);
                        pathIndex = 0;
                        hasTarget = !currentPath.empty();
                    }
                }
                break;
                
            case CharacterState::AVOIDING_HAZARD:
                Vector2 hazard = FindNearestNode(NodeType::HAZARD);
                if (hazard.x != -1) {
                    // Move away from hazard
                    Vector2 awayDir = Vector2Subtract(position, hazard);
                    awayDir = Vector2Normalize(awayDir);
                    
                    targetPosition = Vector2Add(position, Vector2Scale(awayDir, 200));
                    currentPath = FindPath(position, targetPosition);
                    pathIndex = 0;
                    hasTarget = !currentPath.empty();
                }
                break;
                
            case CharacterState::LEARNING:
                // Analyze memories and improve skills
                UpdateSkills();
                StoreMemory(position, 0.5f);
                energy -= 1.0f;
                ChangeState(CharacterState::RESTING);
                break;
                
            case CharacterState::RESTING:
                // Rest and regain energy
                energy += 2.0f;
                if (energy > 100) {
                    energy = 100;
                    ChangeState(CharacterState::IDLE);
                }
                break;
        }
        
        // Move along path
        if (hasTarget && pathIndex < currentPath.size()) {
            Vector2 nextWaypoint = currentPath[pathIndex];
            Vector2 direction = Vector2Subtract(nextWaypoint, position);
            float distance = Vector2Length(direction);
            
            if (distance < 5) {
                pathIndex++;
            } else {
                direction = Vector2Normalize(direction);
                float speed = 150.0f * (energy / 100.0f);
                velocity = Vector2Scale(direction, speed);
            }
        }
        
        // Update position
        position = Vector2Add(position, Vector2Scale(velocity, GetFrameTime()));
        totalDistanceTraveled += Vector2Length(Vector2Scale(velocity, GetFrameTime()));
        
        // Check if reached goal
        if (currentState == CharacterState::SEEKING_GOAL && 
            CalculateDistance(position, FindNearestNode(NodeType::GOAL)) < 20) {
            goalsReached++;
            experiencePoints += 100;
            StoreMemory(position, 1.0f);
            ChangeState(CharacterState::LEARNING);
            
            // Level up check
            if (experiencePoints >= level * 1000) {
                level++;
                learningRate *= 1.1f;
                characterColor = ColorFromHSV((level * 50) % 360, 0.8f, 1.0f);
            }
        }
        
        // Energy consumption
        energy -= Vector2Length(velocity) * 0.01f;
        energy = std::max(0.0f, energy);
    }
    
    void ChangeState(CharacterState newState) {
        previousState = currentState;
        currentState = newState;
        hasTarget = false;
        pathIndex = 0;
        currentPath.clear();
    }
    
    std::string StateToString(CharacterState state) {
        switch (state) {
            case CharacterState::IDLE: return "Idle";
            case CharacterState::EXPLORING: return "Exploring";
            case CharacterState::SEEKING_GOAL: return "Seeking Goal";
            case CharacterState::COLLECTING: return "Collecting";
            case CharacterState::AVOIDING_HAZARD: return "Avoiding Hazard";
            case CharacterState::LEARNING: return "Learning";
            case CharacterState::RESTING: return "Resting";
            default: return "Unknown";
        }
    }

public:
    SelfProvingCharacter() : rng(std::random_device{}()) {
        position = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};
        velocity = {0, 0};
        currentState = CharacterState::IDLE;
        previousState = CharacterState::IDLE;
        
        // Initialize grid
        grid.resize(GRID_HEIGHT, std::vector<GridNode>(GRID_WIDTH));
        
        // Generate random obstacles and objectives
        std::uniform_int_distribution<int> distX(0, GRID_WIDTH - 1);
        std::uniform_int_distribution<int> distY(0, GRID_HEIGHT - 1);
        std::uniform_real_distribution<float> chance(0.0f, 1.0f);
        
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                float rand = chance(rng);
                
                if (rand < 0.1) {
                    grid[y][x].type = NodeType::WALL;
                } else if (rand < 0.2) {
                    grid[y][x].type = NodeType::GOAL;
                } else if (rand < 0.3) {
                    grid[y][x].type = NodeType::HAZARD;
                } else if (rand < 0.4) {
                    grid[y][x].type = NodeType::COLLECTIBLE;
                }
            }
        }
        
        // Ensure starting position is clear
        int startX = (int)position.x / GRID_SIZE;
        int startY = (int)position.y / GRID_SIZE;
        grid[startY][startX].type = NodeType::EMPTY;
        
        // Initialize character attributes
        energy = 100;
        confidence = 0.5f;
        learningRate = 0.1f;
        experiencePoints = 0;
        level = 1;
        goalsReached = 0;
        hazardsAvoided = 0;
        itemsCollected = 0;
        totalDistanceTraveled = 0;
        successRate = 0;
        decisionTimer = 0;
        hasTarget = false;
        pathIndex = 0;
        animationTime = 0;
        characterColor = BLUE;
        
        // Initialize personality traits
        curiosity = 0.7f + chance(rng) * 0.3f;
        cautiousness = 0.5f + chance(rng) * 0.5f;
        persistence = 0.6f + chance(rng) * 0.4f;
        
        // Initialize skills
        skillLevels["navigation"] = 1.0f;
        skillLevels["survival"] = 1.0f;
        skillLevels["gathering"] = 1.0f;
        skillLevels["exploration"] = 1.0f;
    }
    
    void Update() {
        animationTime += GetFrameTime();
        
        // Make decisions and execute actions
        MakeDecision();
        ExecuteState();
        
        // Check for nearby items
        Vector2 gridPos = {floor(position.x / GRID_SIZE), floor(position.y / GRID_SIZE)};
        
        if (IsValidGridPosition((int)gridPos.x, (int)gridPos.y)) {
            NodeType currentNode = grid[(int)gridPos.y][(int)gridPos.x].type;
            
            if (currentNode == NodeType::COLLECTIBLE) {
                itemsCollected++;
                experiencePoints += 50;
                grid[(int)gridPos.y][(int)gridPos.x].type = NodeType::EMPTY;
                StoreMemory(position, 0.8f);
            } else if (currentNode == NodeType::HAZARD) {
                hazardsAvoided++;
                energy -= 10;
                grid[(int)gridPos.y][(int)gridPos.x].type = NodeType::EMPTY;
                StoreMemory(position, -0.5f);
            }
        }
        
        // Periodic learning
        if (GetTime() > 30 && fmod(GetTime(), 30) < 0.1f) {
            ChangeState(CharacterState::LEARNING);
        }
    }
    
    void Draw() {
        // Draw path
        if (hasTarget && !currentPath.empty()) {
            for (size_t i = pathIndex; i < currentPath.size() - 1; i++) {
                DrawLineV(currentPath[i], currentPath[i + 1], ColorAlpha(YELLOW, 0.5f));
            }
        }
        
        // Draw character
        DrawCircleV(position, 15, characterColor);
        
        // Draw energy bar above character
        DrawRectangle((int)position.x - 20, (int)position.y - 30, 40, 8, DARKGRAY);
        DrawRectangle((int)position.x - 20, (int)position.y - 30, (int)(40 * energy / 100), 8, GREEN);
        
        // Draw level indicator
        DrawText(TextFormat("LVL %d", level), (int)position.x - 10, (int)position.y - 45, 10, WHITE);
        
        // Draw state indicator
        DrawText(StateToString(currentState).c_str(), (int)position.x - 20, (int)position.y + 25, 10, WHITE);
    }
    
    void DrawHUD() {
        // Draw character information panel
        DrawRectangle(10, 10, 300, 250, ColorAlpha(BLACK, 0.7f));
        
        DrawText("SELF-PROVING CHARACTER", 20, 20, 16, WHITE);
        DrawText(TextFormat("State: %s", StateToString(currentState).c_str()), 20, 45, 14, YELLOW);
        DrawText(TextFormat("Level: %d (XP: %d)", level, experiencePoints), 20, 65, 14, WHITE);
        DrawText(TextFormat("Energy: %.1f%%", energy), 20, 85, 14, GREEN);
        DrawText(TextFormat("Confidence: %.2f", confidence), 20, 105, 14, WHITE);
        DrawText(TextFormat("Success Rate: %.2f", successRate), 20, 125, 14, WHITE);
        
        DrawText("--- STATISTICS ---", 20, 150, 12, GRAY);
        DrawText(TextFormat("Goals Reached: %d", goalsReached), 20, 170, 12, WHITE);
        DrawText(TextFormat("Hazards Avoided: %d", hazardsAvoided), 20, 190, 12, WHITE);
        DrawText(TextFormat("Items Collected: %d", itemsCollected), 20, 210, 12, WHITE);
        DrawText(TextFormat("Distance: %.0f", totalDistanceTraveled), 20, 230, 12, WHITE);
        
        // Draw skills
        DrawRectangle(320, 10, 200, 150, ColorAlpha(BLACK, 0.7f));
        DrawText("SKILLS", 330, 20, 16, WHITE);
        
        int yOffset = 45;
        for (const auto& skill : skillLevels) {
            DrawText(TextFormat("%s: %.1f", skill.first.c_str(), skill.second), 
                    330, yOffset, 12, CYAN);
            yOffset += 20;
        }
        
        // Draw personality traits
        DrawRectangle(530, 10, 200, 100, ColorAlpha(BLACK, 0.7f));
        DrawText("PERSONALITY", 540, 20, 14, WHITE);
        DrawText(TextFormat("Curiosity: %.2f", curiosity), 540, 45, 12, WHITE);
        DrawText(TextFormat("Cautiousness: %.2f", cautiousness), 540, 65, 12, WHITE);
        DrawText(TextFormat("Persistence: %.2f", persistence), 540, 85, 12, WHITE);
    }
    
    Vector2 GetPosition() const { return position; }
};

// Grid rendering class
class GridRenderer {
private:
    std::vector<std::vector<GridNode>> grid;
    
public:
    GridRenderer(const std::vector<std::vector<GridNode>>& g) : grid(g) {}
    
    void Draw() {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                Rectangle rect = {(float)(x * GRID_SIZE), (float)(y * GRID_SIZE), 
                                 GRID_SIZE, GRID_SIZE};
                
                switch (grid[y][x].type) {
                    case NodeType::WALL:
                        DrawRectangleRec(rect, DARKGRAY);
                        break;
                    case NodeType::GOAL:
                        DrawRectangleRec(rect, ColorAlpha(GOLD, 0.5f));
                        DrawCircle(x * GRID_SIZE + GRID_SIZE/2, y * GRID_SIZE + GRID_SIZE/2, 
                                  GRID_SIZE/3, GOLD);
                        break;
                    case NodeType::HAZARD:
                        DrawRectangleRec(rect, ColorAlpha(RED, 0.3f));
                        DrawTriangle(
                            {(float)(x * GRID_SIZE + GRID_SIZE/2), (float)(y * GRID_SIZE + 5)},
                            {(float)(x * GRID_SIZE + 5), (float)(y * GRID_SIZE + GRID_SIZE - 5)},
                            {(float)(x * GRID_SIZE + GRID_SIZE - 5), (float)(y * GRID_SIZE + GRID_SIZE - 5)},
                            RED
                        );
                        break;
                    case NodeType::COLLECTIBLE:
                        DrawRectangleRec(rect, ColorAlpha(BLUE, 0.2f));
                        DrawCircle(x * GRID_SIZE + GRID_SIZE/2, y * GRID_SIZE + GRID_SIZE/2, 
                                  GRID_SIZE/4, BLUE);
                        break;
                    default:
                        if ((x + y) % 2 == 0) {
                            DrawRectangleRec(rect, ColorAlpha(WHITE, 0.1f));
                        }
                        break;
                }
                
                // Draw grid lines
                DrawRectangleLines(x * GRID_SIZE, y * GRID_SIZE, GRID_SIZE, GRID_SIZE, 
                                  ColorAlpha(BLACK, 0.1f));
            }
        }
    }
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Self-Proving AI Character");
    SetTargetFPS(60);
    
    SelfProvingCharacter character;
    
    // We need access to the grid for rendering
    // This is a simplified version - in a real implementation, 
    // you'd want better encapsulation
    
    while (!WindowShouldClose()) {
        // Update
        character.Update();
        
        // Draw
        BeginDrawing();
        ClearBackground(DARKGREEN);
        
        // Draw grid (simplified - you'd need to expose the grid from the character)
        // For this demo, we'll just draw the character and HUD
        
        // Draw some visual elements
        DrawCircleV(character.GetPosition(), 20, ColorAlpha(YELLOW, 0.5f));
        
        character.Draw();
        character.DrawHUD();
        
        // Draw instructions
        DrawText("Watch the AI character learn and improve!", 
                SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT - 50, 20, WHITE);
        DrawText("Press ESC to exit", SCREEN_WIDTH - 200, SCREEN_HEIGHT - 30, 16, GRAY);
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}