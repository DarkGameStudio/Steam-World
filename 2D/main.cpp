#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <unordered_map>
#include <random>
#include <cmath>
#include <string>

// Custom hash for pair to use as map key
struct PairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

// Game constants
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int TILE_SIZE = 32;
const int CHUNK_SIZE = 16; // 16x16 tiles per chunk
const float GRAVITY = 980.0f;
const float PLAYER_SPEED = 200.0f;
const float JUMP_FORCE = -400.0f;

// Tile types
enum class TileType {
    EMPTY = 0,
    GRASS = 1,
    DIRT = 2,
    STONE = 3,
    PLATFORM = 4
};

// Object types
enum class ObjectType {
    NONE = 0,
    TREE = 1,
    ROCK = 2,
    CRYSTAL = 3,
    ENEMY = 4,
    COIN = 5
};

// Chunk structure
struct Chunk {
    std::vector<TileType> tiles;
    bool generated = false;
    
    Chunk() : tiles(CHUNK_SIZE * CHUNK_SIZE, TileType::EMPTY) {}
};

// Game object structure
struct GameObject {
    Vector2 position;
    ObjectType type;
    bool active = true;
    float animationTime = 0;
};

// Player structure
struct Player {
    Vector2 position;
    Vector2 velocity;
    float width = 24;
    float height = 32;
    bool onGround = false;
    int coins = 0;
};

// Particle system
struct Particle {
    Vector2 position;
    Vector2 velocity;
    float life;
    float maxLife;
    Color color;
    bool active = true;
};

// Game class
class InfiniteGame {
private:
    std::unordered_map<std::pair<int, int>, Chunk, PairHash> chunks;
    std::vector<GameObject> objects;
    std::vector<Particle> particles;
    Player player;
    Camera2D camera;
    std::mt19937 rng;
    
    int worldSeed;
    float gameTime = 0;
    
    // Helper functions
    std::pair<int, int> GetChunkCoords(float worldX, float worldY) {
        return {
            (int)floor(worldX / (TILE_SIZE * CHUNK_SIZE)),
            (int)floor(worldY / (TILE_SIZE * CHUNK_SIZE))
        };
    }
    
    std::pair<int, int> GetTileCoords(float worldX, float worldY) {
        return {
            (int)floor(worldX / TILE_SIZE),
            (int)floor(worldY / TILE_SIZE)
        };
    }
    
    // Noise function for terrain generation
    float Noise(int x, int y, int seed) {
        int n = x + y * 57 + seed * 131;
        n = (n << 13) ^ n;
        return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
    }
    
    float SmoothNoise(int x, int y, int seed) {
        float corners = (Noise(x-1, y-1, seed) + Noise(x+1, y-1, seed) + 
                        Noise(x-1, y+1, seed) + Noise(x+1, y+1, seed)) / 16.0f;
        float sides = (Noise(x-1, y, seed) + Noise(x+1, y, seed) + 
                      Noise(x, y-1, seed) + Noise(x, y+1, seed)) / 8.0f;
        float center = Noise(x, y, seed) / 4.0f;
        return corners + sides + center;
    }
    
    // Generate terrain height at x position
    int GetTerrainHeight(int worldX) {
        float height = 20; // Base height in tiles
        
        // Add multiple octaves of noise
        height += SmoothNoise(worldX / 30, 0, worldSeed) * 8.0f;
        height += SmoothNoise(worldX / 15, 1, worldSeed + 100) * 4.0f;
        height += SmoothNoise(worldX / 7, 2, worldSeed + 200) * 2.0f;
        
        // Add occasional mountains
        int mountainNoise = (int)(SmoothNoise(worldX / 100, 3, worldSeed + 300) * 10.0f);
        if (mountainNoise > 7) {
            height += mountainNoise * 0.8f;
        }
        
        return (int)height;
    }
    
    // Generate a chunk
    void GenerateChunk(int chunkX, int chunkY) {
        auto key = std::make_pair(chunkX, chunkY);
        auto& chunk = chunks[key];
        
        if (chunk.generated) return;
        
        // Generate tiles
        for (int localY = 0; localY < CHUNK_SIZE; localY++) {
            for (int localX = 0; localX < CHUNK_SIZE; localX++) {
                int worldX = chunkX * CHUNK_SIZE + localX;
                int worldY = chunkY * CHUNK_SIZE + localY;
                int terrainHeight = GetTerrainHeight(worldX);
                
                int index = localY * CHUNK_SIZE + localX;
                
                if (worldY > terrainHeight) {
                    // Underground
                    if (worldY < terrainHeight + 4) {
                        chunk.tiles[index] = TileType::DIRT;
                    } else if (worldY < terrainHeight + 12) {
                        chunk.tiles[index] = TileType::STONE;
                    } else {
                        chunk.tiles[index] = TileType::STONE;
                    }
                } else if (worldY == terrainHeight) {
                    chunk.tiles[index] = TileType::GRASS;
                } else if (worldY == terrainHeight - 1 && 
                          (int)(SmoothNoise(worldX, worldY, worldSeed + 400) * 10) > 6) {
                    // Floating platforms
                    chunk.tiles[index] = TileType::PLATFORM;
                } else {
                    chunk.tiles[index] = TileType::EMPTY;
                }
            }
        }
        
        // Generate objects in this chunk
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        for (int localY = 0; localY < CHUNK_SIZE; localY++) {
            for (int localX = 0; localX < CHUNK_SIZE; localX++) {
                int worldX = chunkX * CHUNK_SIZE + localX;
                int worldY = chunkY * CHUNK_SIZE + localY;
                int terrainHeight = GetTerrainHeight(worldX);
                
                if (worldY == terrainHeight - 1) { // Surface
                    float rand = dist(rng);
                    
                    if (rand < 0.15) {
                        // Spawn tree
                        GameObject obj;
                        obj.position = {(float)(worldX * TILE_SIZE + TILE_SIZE/2), 
                                       (float)(terrainHeight * TILE_SIZE - TILE_SIZE)};
                        obj.type = ObjectType::TREE;
                        objects.push_back(obj);
                    } else if (rand < 0.25) {
                        // Spawn rock
                        GameObject obj;
                        obj.position = {(float)(worldX * TILE_SIZE + TILE_SIZE/2), 
                                       (float)(terrainHeight * TILE_SIZE - TILE_SIZE/2)};
                        obj.type = ObjectType::ROCK;
                        objects.push_back(obj);
                    } else if (rand < 0.30) {
                        // Spawn crystal
                        GameObject obj;
                        obj.position = {(float)(worldX * TILE_SIZE + TILE_SIZE/2), 
                                       (float)(terrainHeight * TILE_SIZE - TILE_SIZE/2)};
                        obj.type = ObjectType::CRYSTAL;
                        objects.push_back(obj);
                    } else if (rand < 0.35) {
                        // Spawn coin
                        GameObject obj;
                        obj.position = {(float)(worldX * TILE_SIZE + TILE_SIZE/2), 
                                       (float)(terrainHeight * TILE_SIZE - TILE_SIZE * 2)};
                        obj.type = ObjectType::COIN;
                        objects.push_back(obj);
                    } else if (rand < 0.40 && worldX > 50) {
                        // Spawn enemy
                        GameObject obj;
                        obj.position = {(float)(worldX * TILE_SIZE), 
                                       (float)(terrainHeight * TILE_SIZE - TILE_SIZE)};
                        obj.type = ObjectType::ENEMY;
                        objects.push_back(obj);
                    }
                }
            }
        }
        
        chunk.generated = true;
    }
    
    // Check collision with tiles
    bool CheckTileCollision(Vector2 position, float width, float height, TileType& tileType) {
        int leftTile = (int)floor((position.x - width/2) / TILE_SIZE);
        int rightTile = (int)floor((position.x + width/2) / TILE_SIZE);
        int topTile = (int)floor((position.y - height/2) / TILE_SIZE);
        int bottomTile = (int)floor((position.y + height/2) / TILE_SIZE);
        
        for (int y = topTile; y <= bottomTile; y++) {
            for (int x = leftTile; x <= rightTile; x++) {
                auto chunkKey = std::make_pair(
                    (int)floor((float)x / CHUNK_SIZE), 
                    (int)floor((float)y / CHUNK_SIZE)
                );
                
                if (chunks.find(chunkKey) != chunks.end()) {
                    int localX = ((x % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
                    int localY = ((y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
                    TileType tile = chunks[chunkKey].tiles[localY * CHUNK_SIZE + localX];
                    
                    if (tile != TileType::EMPTY && tile != TileType::PLATFORM) {
                        tileType = tile;
                        return true;
                    }
                }
            }
        }
        return false;
    }
    
    // Spawn particles
    void SpawnParticles(Vector2 position, Color color, int count) {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        std::uniform_real_distribution<float> speedDist(50.0f, 150.0f);
        
        for (int i = 0; i < count; i++) {
            Particle p;
            p.position = position;
            p.velocity = {dist(rng) * speedDist(rng), dist(rng) * speedDist(rng)};
            p.life = 0.5f;
            p.maxLife = 0.5f;
            p.color = color;
            particles.push_back(p);
        }
    }

public:
    InfiniteGame() : rng(std::random_device{}()) {
        worldSeed = std::random_device{}();
        
        // Initialize player
        player.position = {SCREEN_WIDTH/2.0f, 100.0f};
        player.velocity = {0, 0};
        
        // Initialize camera
        camera.target = player.position;
        camera.offset = {SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f};
        camera.rotation = 0.0f;
        camera.zoom = 1.0f;
        
        // Generate initial chunks
        UpdateChunks();
    }
    
    // Update chunks around player
    void UpdateChunks() {
        auto centerChunk = GetChunkCoords(player.position.x, player.position.y);
        int viewDistance = 3;
        
        for (int cy = -viewDistance; cy <= viewDistance; cy++) {
            for (int cx = -viewDistance; cx <= viewDistance; cx++) {
                GenerateChunk(centerChunk.first + cx, centerChunk.second + cy);
            }
        }
    }
    
    void Update() {
        gameTime += GetFrameTime();
        float dt = GetFrameTime();
        
        // Player input
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
            player.velocity.x = -PLAYER_SPEED;
        } else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
            player.velocity.x = PLAYER_SPEED;
        } else {
            player.velocity.x = 0;
        }
        
        if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) && player.onGround) {
            player.velocity.y = JUMP_FORCE;
            player.onGround = false;
        }
        
        // Apply gravity
        player.velocity.y += GRAVITY * dt;
        
        // Move player with collision
        Vector2 newPos = player.position;
        newPos.x += player.velocity.x * dt;
        
        TileType tileType;
        if (!CheckTileCollision(newPos, player.width, player.height, tileType)) {
            player.position.x = newPos.x;
        }
        
        newPos = player.position;
        newPos.y += player.velocity.y * dt;
        
        if (!CheckTileCollision(newPos, player.width, player.height, tileType)) {
            player.position.y = newPos.y;
            player.onGround = false;
        } else {
            if (player.velocity.y > 0) {
                player.onGround = true;
            }
            player.velocity.y = 0;
        }
        
        // Update objects
        for (auto& obj : objects) {
            if (!obj.active) continue;
            
            obj.animationTime += dt;
            
            // Check collision with player
            float distToPlayer = Vector2Distance(obj.position, player.position);
            
            if (distToPlayer < 30) {
                switch (obj.type) {
                    case ObjectType::COIN:
                        player.coins++;
                        obj.active = false;
                        SpawnParticles(obj.position, GOLD, 10);
                        break;
                    case ObjectType::CRYSTAL:
                        player.coins += 5;
                        obj.active = false;
                        SpawnParticles(obj.position, PURPLE, 15);
                        break;
                    case ObjectType::ENEMY:
                        // Simple enemy AI - move towards player
                        Vector2 direction = Vector2Subtract(player.position, obj.position);
                        direction = Vector2Normalize(direction);
                        obj.position.x += direction.x * 50.0f * dt;
                        break;
                    default:
                        break;
                }
            }
            
            // Simple enemy animation
            if (obj.type == ObjectType::ENEMY) {
                obj.position.x += sin(gameTime * 2) * 0.5f;
            }
        }
        
        // Update particles
        for (auto& p : particles) {
            if (!p.active) continue;
            
            p.life -= dt;
            if (p.life <= 0) {
                p.active = false;
                continue;
            }
            
            p.velocity.y += GRAVITY * 0.5f * dt;
            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
        }
        
        // Remove inactive particles
        particles.erase(
            std::remove_if(particles.begin(), particles.end(), 
                          [](const Particle& p) { return !p.active; }),
            particles.end()
        );
        
        // Update camera
        camera.target = Vector2Lerp(camera.target, player.position, 5.0f * dt);
        
        // Update chunks
        UpdateChunks();
        
        // Reset if player falls too far
        if (player.position.y > 2000) {
            player.position = {SCREEN_WIDTH/2.0f, 100.0f};
            player.velocity = {0, 0};
        }
    }
    
    void Draw() {
        BeginDrawing();
        ClearBackground(SKYBLUE);
        
        BeginMode2D(camera);
        
        // Draw visible chunks
        int startChunkX = (int)floor((camera.target.x - SCREEN_WIDTH/2) / (TILE_SIZE * CHUNK_SIZE));
        int endChunkX = (int)floor((camera.target.x + SCREEN_WIDTH/2) / (TILE_SIZE * CHUNK_SIZE));
        int startChunkY = (int)floor((camera.target.y - SCREEN_HEIGHT/2) / (TILE_SIZE * CHUNK_SIZE));
        int endChunkY = (int)floor((camera.target.y + SCREEN_HEIGHT/2) / (TILE_SIZE * CHUNK_SIZE));
        
        for (int cy = startChunkY; cy <= endChunkY; cy++) {
            for (int cx = startChunkX; cx <= endChunkX; cx++) {
                auto chunkKey = std::make_pair(cx, cy);
                if (chunks.find(chunkKey) == chunks.end()) continue;
                
                auto& chunk = chunks[chunkKey];
                
                for (int localY = 0; localY < CHUNK_SIZE; localY++) {
                    for (int localX = 0; localX < CHUNK_SIZE; localX++) {
                        TileType tile = chunk.tiles[localY * CHUNK_SIZE + localX];
                        if (tile == TileType::EMPTY) continue;
                        
                        float worldX = (cx * CHUNK_SIZE + localX) * TILE_SIZE;
                        float worldY = (cy * CHUNK_SIZE + localY) * TILE_SIZE;
                        
                        Color color;
                        switch (tile) {
                            case TileType::GRASS:
                                color = GREEN;
                                break;
                            case TileType::DIRT:
                                color = BROWN;
                                break;
                            case TileType::STONE:
                                color = GRAY;
                                break;
                            case TileType::PLATFORM:
                                color = LIGHTGRAY;
                                break;
                            default:
                                color = BLANK;
                                break;
                        }
                        
                        DrawRectangle((int)worldX, (int)worldY, TILE_SIZE, TILE_SIZE, color);
                        
                        // Draw grid lines for debugging
                        if (IsKeyDown(KEY_G)) {
                            DrawRectangleLines((int)worldX, (int)worldY, TILE_SIZE, TILE_SIZE, ColorAlpha(BLACK, 0.2f));
                        }
                    }
                }
            }
        }
        
        // Draw objects
        for (const auto& obj : objects) {
            if (!obj.active) continue;
            
            switch (obj.type) {
                case ObjectType::TREE:
                    DrawRectangle((int)obj.position.x - 4, (int)obj.position.y - 20, 8, 20, BROWN);
                    DrawCircle((int)obj.position.x, (int)obj.position.y - 25, 12, DARKGREEN);
                    break;
                case ObjectType::ROCK:
                    DrawCircle((int)obj.position.x, (int)obj.position.y, 8, DARKGRAY);
                    break;
                case ObjectType::CRYSTAL:
                    DrawTriangle(
                        {(float)obj.position.x, (float)obj.position.y - 10},
                        {(float)obj.position.x - 6, (float)obj.position.y + 5},
                        {(float)obj.position.x + 6, (float)obj.position.y + 5},
                        PURPLE
                    );
                    break;
                case ObjectType::ENEMY:
                    DrawCircle((int)obj.position.x, (int)obj.position.y - 10, 10, RED);
                    DrawRectangle((int)obj.position.x - 8, (int)obj.position.y - 5, 16, 10, RED);
                    break;
                case ObjectType::COIN:
                    DrawCircle((int)obj.position.x, (int)obj.position.y, 6, GOLD);
                    DrawCircle((int)obj.position.x - 2, (int)obj.position.y - 2, 2, YELLOW);
                    break;
                default:
                    break;
            }
        }
        
        // Draw particles
        for (const auto& p : particles) {
            float alpha = p.life / p.maxLife;
            DrawCircle((int)p.position.x, (int)p.position.y, 3, ColorAlpha(p.color, alpha));
        }
        
        // Draw player
        DrawRectangleRec(
            (Rectangle){player.position.x - player.width/2, 
                       player.position.y - player.height/2, 
                       player.width, player.height},
            BLUE
        );
        
        EndMode2D();
        
        // Draw UI
        DrawText(TextFormat("Coins: %d", player.coins), 10, 10, 20, BLACK);
        DrawText(TextFormat("Position: %.0f, %.0f", player.position.x, player.position.y), 10, 40, 20, BLACK);
        DrawText("Controls: A/D or Arrow Keys to move, Space to jump, G to show grid", 10, SCREEN_HEIGHT - 30, 20, BLACK);
        
        EndDrawing();
    }
    
    void Run() {
        while (!WindowShouldClose()) {
            Update();
            Draw();
        }
    }
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Infinite 2D Game Generator");
    SetTargetFPS(60);
    
    InfiniteGame game;
    game.Run();
    
    CloseWindow();
    return 0;
}