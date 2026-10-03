#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <unordered_map>
#include <queue>
#include <memory>
#include <random>
#include <cmath>
#include <algorithm>
#include <thread>
#include <mutex>
#include <future>
#include <iostream>

// Window dimensions
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

// Map generation settings
const int TILE_SIZE = 32;
const int CHUNK_SIZE = 16; // 16x16 tiles per chunk
const int VIEW_DISTANCE = 4; // Chunks visible in each direction
const int MAX_CACHE_SIZE = 100; // Maximum chunks to keep in memory

// Biome types
enum class BiomeType {
    PLAINS,
    FOREST,
    DESERT,
    MOUNTAIN,
    SNOW,
    SWAMP,
    VOLCANIC
};

// Tile types
enum class TileType {
    EMPTY = 0,
    GRASS = 1,
    DIRT = 2,
    STONE = 3,
    SAND = 4,
    SNOW = 5,
    WATER = 6,
    LAVA = 7,
    WOOD = 8,
    LEAVES = 9
};

// Structure for chunk coordinates
struct ChunkCoord {
    int x, y;
    
    bool operator==(const ChunkCoord& other) const {
        return x == other.x && y == other.y;
    }
    
    bool operator<(const ChunkCoord& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

// Hash function for ChunkCoord
struct ChunkCoordHash {
    std::size_t operator()(const ChunkCoord& coord) const {
        return std::hash<int>()(coord.x) ^ (std::hash<int>()(coord.y) << 1);
    }
};

// Chunk data structure
struct Chunk {
    std::vector<TileType> tiles;
    bool generated = false;
    bool generating = false;
    int priority = 0;
    float lastAccessTime = 0;
    BiomeType biome = BiomeType::PLAINS;
    
    Chunk() : tiles(CHUNK_SIZE * CHUNK_SIZE, TileType::EMPTY) {}
};

// Noise generator class
class NoiseGenerator {
private:
    std::vector<int> permutation;
    
    float Fade(float t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }
    
    float Lerp(float a, float b, float t) {
        return a + t * (b - a);
    }
    
    float Gradient(int hash, float x, float y) {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
    
public:
    NoiseGenerator(unsigned int seed = 12345) {
        permutation.resize(512);
        std::vector<int> p(256);
        
        for (int i = 0; i < 256; i++) {
            p[i] = i;
        }
        
        std::mt19937 rng(seed);
        std::shuffle(p.begin(), p.end(), rng);
        
        for (int i = 0; i < 512; i++) {
            permutation[i] = p[i & 255];
        }
    }
    
    // 2D Perlin noise
    float PerlinNoise(float x, float y) {
        int X = (int)floor(x) & 255;
        int Y = (int)floor(y) & 255;
        
        x -= floor(x);
        y -= floor(y);
        
        float u = Fade(x);
        float v = Fade(y);
        
        int A = permutation[X] + Y;
        int B = permutation[X + 1] + Y;
        
        return Lerp(
            Lerp(Gradient(permutation[A], x, y), Gradient(permutation[B], x - 1, y), u),
            Lerp(Gradient(permutation[A + 1], x, y - 1), Gradient(permutation[B + 1], x - 1, y - 1), u),
            v
        );
    }
    
    // Fractal Brownian Motion
    float FBM(float x, float y, int octaves, float persistence = 0.5f, float lacunarity = 2.0f) {
        float total = 0;
        float frequency = 1;
        float amplitude = 1;
        float maxValue = 0;
        
        for (int i = 0; i < octaves; i++) {
            total += PerlinNoise(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }
        
        return total / maxValue;
    }
    
    // Ridged noise for mountains
    float RidgedNoise(float x, float y, int octaves) {
        return 1.0f - abs(FBM(x, y, octaves, 0.5f, 2.0f));
    }
    
    // Voronoi-like pattern for biome distribution
    float VoronoiNoise(float x, float y, float scale) {
        float cx = floor(x / scale) * scale;
        float cy = floor(y / scale) * scale;
        
        float minDist = scale * 2;
        float secondMin = scale * 2;
        
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                float px = cx + i * scale + scale * 0.5f;
                float py = cy + j * scale + scale * 0.5f;
                float dist = sqrt(pow(x - px, 2) + pow(y - py, 2));
                
                if (dist < minDist) {
                    secondMin = minDist;
                    minDist = dist;
                } else if (dist < secondMin) {
                    secondMin = dist;
                }
            }
        }
        
        return secondMin - minDist;
    }
};

// JIT Map Generator class
class JITMapGenerator {
private:
    std::unordered_map<ChunkCoord, std::shared_ptr<Chunk>, ChunkCoordHash> chunks;
    std::priority_queue<std::pair<int, ChunkCoord>> generationQueue;
    std::mutex chunkMutex;
    std::vector<std::future<void>> generationFutures;
    
    NoiseGenerator noiseGen;
    std::mt19937 rng;
    
    Vector2 playerPosition;
    int worldSeed;
    float gameTime;
    
    // Performance metrics
    int chunksGenerated;
    int chunksLoaded;
    float generationTime;
    int cacheHits;
    int cacheMisses;
    
    // Helper functions
    ChunkCoord GetChunkCoord(float worldX, float worldY) {
        return {
            (int)floor(worldX / (TILE_SIZE * CHUNK_SIZE)),
            (int)floor(worldY / (TILE_SIZE * CHUNK_SIZE))
        };
    }
    
    ChunkCoord GetChunkCoordFromTile(int tileX, int tileY) {
        return {
            (int)floor((float)tileX / CHUNK_SIZE),
            (int)floor((float)tileY / CHUNK_SIZE)
        };
    }
    
    float GetBiomeNoise(float x, float y) {
        // Combine multiple noise functions for biome determination
        float temperature = noiseGen.FBM(x * 0.001f, y * 0.001f, 4) * 0.5f + 0.5f;
        float moisture = noiseGen.FBM(x * 0.002f + 100, y * 0.002f + 100, 4) * 0.5f + 0.5f;
        float elevation = noiseGen.FBM(x * 0.003f + 200, y * 0.003f + 200, 5) * 0.5f + 0.5f;
        
        // Determine biome based on these factors
        if (elevation > 0.7f) {
            if (temperature < 0.3f) return 0.0f; // Snow
            return 0.3f; // Mountain
        } else if (elevation > 0.6f) {
            if (temperature > 0.7f && moisture < 0.3f) return 0.6f; // Volcanic
            return 0.3f; // Mountain
        } else if (temperature > 0.7f && moisture < 0.3f) {
            return 0.15f; // Desert
        } else if (moisture > 0.7f) {
            if (temperature > 0.6f) return 0.45f; // Swamp
            return 0.35f; // Forest
        } else if (temperature < 0.3f) {
            return 0.0f; // Snow
        } else {
            return 0.2f; // Plains
        }
    }
    
    BiomeType DetermineBiome(float noise) {
        if (noise < 0.1f) return BiomeType::SNOW;
        if (noise < 0.25f) return BiomeType::DESERT;
        if (noise < 0.4f) return BiomeType::PLAINS;
        if (noise < 0.55f) return BiomeType::MOUNTAIN;
        if (noise < 0.65f) return BiomeType::SWAMP;
        if (noise < 0.8f) return BiomeType::FOREST;
        return BiomeType::VOLCANIC;
    }
    
    float GetElevation(int worldX, int worldY) {
        float base = noiseGen.FBM(worldX * 0.01f, worldY * 0.01f, 4);
        float detail = noiseGen.FBM(worldX * 0.05f, worldY * 0.05f, 3) * 0.3f;
        float mountainMask = noiseGen.RidgedNoise(worldX * 0.005f, worldY * 0.005f, 4);
        
        return base + detail + mountainMask * 0.5f;
    }
    
    TileType GetTileTypeForBiome(BiomeType biome, float elevation, float moisture, int worldX, int worldY) {
        switch (biome) {
            case BiomeType::PLAINS:
                if (elevation < 0.3f) return TileType::WATER;
                if (elevation < 0.35f) return TileType::SAND;
                if (elevation > 0.8f) return TileType::STONE;
                return TileType::GRASS;
                
            case BiomeType::FOREST:
                if (elevation < 0.3f) return TileType::WATER;
                if (elevation < 0.35f) return TileType::DIRT;
                if (elevation > 0.75f) return TileType::STONE;
                // Random trees
                if (noiseGen.PerlinNoise(worldX * 0.1f, worldY * 0.1f) > 0.6f) {
                    return TileType::WOOD;
                }
                return TileType::GRASS;
                
            case BiomeType::DESERT:
                if (elevation < 0.3f) return TileType::WATER;
                if (elevation > 0.7f) return TileType::STONE;
                return TileType::SAND;
                
            case BiomeType::MOUNTAIN:
                if (elevation < 0.4f) return TileType::GRASS;
                if (elevation < 0.5f) return TileType::DIRT;
                return TileType::STONE;
                
            case BiomeType::SNOW:
                if (elevation < 0.3f) return TileType::WATER;
                if (elevation > 0.8f) return TileType::STONE;
                return TileType::SNOW;
                
            case BiomeType::SWAMP:
                if (elevation < 0.4f) return TileType::WATER;
                if (elevation < 0.5f) return TileType::DIRT;
                return TileType::GRASS;
                
            case BiomeType::VOLCANIC:
                if (elevation > 0.7f) return TileType::LAVA;
                if (elevation > 0.6f) return TileType::STONE;
                return TileType::DIRT;
                
            default:
                return TileType::GRASS;
        }
    }
    
    void GenerateChunkAsync(ChunkCoord coord, int priority) {
        auto chunk = std::make_shared<Chunk>();
        chunk->generating = true;
        chunk->priority = priority;
        
        // Store chunk immediately for reference
        {
            std::lock_guard<std::mutex> lock(chunkMutex);
            chunks[coord] = chunk;
        }
        
        // Generate asynchronously
        generationFutures.push_back(std::async(std::launch::async, [this, coord, chunk]() {
            GenerateChunkData(coord, chunk);
        }));
    }
    
    void GenerateChunkData(ChunkCoord coord, std::shared_ptr<Chunk> chunk) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Determine biome for this chunk
        float biomeNoise = GetBiomeNoise(
            coord.x * CHUNK_SIZE * TILE_SIZE,
            coord.y * CHUNK_SIZE * TILE_SIZE
        );
        chunk->biome = DetermineBiome(biomeNoise);
        
        // Generate tiles
        for (int localY = 0; localY < CHUNK_SIZE; localY++) {
            for (int localX = 0; localX < CHUNK_SIZE; localX++) {
                int worldX = coord.x * CHUNK_SIZE + localX;
                int worldY = coord.y * CHUNK_SIZE + localY;
                
                float elevation = GetElevation(worldX, worldY);
                float moisture = noiseGen.FBM(worldX * 0.01f, worldY * 0.01f, 3) * 0.5f + 0.5f;
                
                TileType tile = GetTileTypeForBiome(chunk->biome, elevation, moisture, worldX, worldY);
                chunk->tiles[localY * CHUNK_SIZE + localX] = tile;
            }
        }
        
        chunk->generated = true;
        chunk->generating = false;
        chunk->lastAccessTime = gameTime;
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        
        {
            std::lock_guard<std::mutex> lock(chunkMutex);
            chunksGenerated++;
            generationTime += duration.count() / 1000000.0f;
        }
    }
    
    void UpdateGenerationQueue(Vector2 playerPos) {
        ChunkCoord centerChunk = GetChunkCoord(playerPos.x, playerPos.y);
        
        // Clear old queue
        while (!generationQueue.empty()) {
            generationQueue.pop();
        }
        
        // Add chunks to generation queue based on distance from player
        for (int cy = -VIEW_DISTANCE; cy <= VIEW_DISTANCE; cy++) {
            for (int cx = -VIEW_DISTANCE; cx <= VIEW_DISTANCE; cx++) {
                ChunkCoord coord = {centerChunk.x + cx, centerChunk.y + cy};
                
                // Calculate priority based on distance
                int distance = abs(cx) + abs(cy);
                int priority = VIEW_DISTANCE * 2 - distance;
                
                // Check if chunk exists and needs generation
                bool needsGeneration = false;
                {
                    std::lock_guard<std::mutex> lock(chunkMutex);
                    auto it = chunks.find(coord);
                    if (it == chunks.end() || (!it->second->generated && !it->second->generating)) {
                        needsGeneration = true;
                    } else if (it->second->generated) {
                        it->second->lastAccessTime = gameTime;
                    }
                }
                
                if (needsGeneration) {
                    generationQueue.push({priority, coord});
                }
            }
        }
    }
    
    void ProcessGenerationQueue() {
        // Process highest priority chunks first
        int chunksToGenerate = 2; // Limit chunks per frame
        int generated = 0;
        
        while (!generationQueue.empty() && generated < chunksToGenerate) {
            auto [priority, coord] = generationQueue.top();
            generationQueue.pop();
            
            // Check if chunk still needs generation
            bool shouldGenerate = false;
            {
                std::lock_guard<std::mutex> lock(chunkMutex);
                auto it = chunks.find(coord);
                if (it == chunks.end() || (!it->second->generated && !it->second->generating)) {
                    shouldGenerate = true;
                }
            }
            
            if (shouldGenerate) {
                GenerateChunkAsync(coord, priority);
                generated++;
            }
        }
    }
    
    void CleanupOldChunks() {
        // Remove chunks that are too far from player
        ChunkCoord centerChunk = GetChunkCoord(playerPosition.x, playerPosition.y);
        
        std::lock_guard<std::mutex> lock(chunkMutex);
        
        // If we have too many chunks, remove the oldest/farthest ones
        if (chunks.size() > MAX_CACHE_SIZE) {
            std::vector<ChunkCoord> toRemove;
            
            for (const auto& [coord, chunk] : chunks) {
                int distance = abs(coord.x - centerChunk.x) + abs(coord.y - centerChunk.y);
                if (distance > VIEW_DISTANCE * 2 && chunk->generated && !chunk->generating) {
                    toRemove.push_back(coord);
                }
            }
            
            for (const auto& coord : toRemove) {
                chunks.erase(coord);
                cacheMisses++;
            }
        }
    }
    
public:
    JITMapGenerator(unsigned int seed = 12345) 
        : noiseGen(seed), rng(seed), worldSeed(seed) {
        playerPosition = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
        gameTime = 0;
        chunksGenerated = 0;
        chunksLoaded = 0;
        generationTime = 0;
        cacheHits = 0;
        cacheMisses = 0;
        
        // Initial generation
        UpdateGenerationQueue(playerPosition);
        ProcessGenerationQueue();
    }
    
    void Update(Vector2 playerPos, float dt) {
        playerPosition = playerPos;
        gameTime += dt;
        
        // Update generation queue based on new position
        UpdateGenerationQueue(playerPos);
        
        // Process high-priority chunks
        ProcessGenerationQueue();
        
        // Clean up old chunks periodically
        if (chunks.size() > MAX_CACHE_SIZE) {
            CleanupOldChunks();
        }
        
        // Clean up completed futures
        generationFutures.erase(
            std::remove_if(generationFutures.begin(), generationFutures.end(),
                [](std::future<void>& fut) {
                    return fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
                }),
            generationFutures.end()
        );
    }
    
    TileType GetTile(int worldX, int worldY) {
        ChunkCoord coord = GetChunkCoordFromTile(worldX, worldY);
        
        std::lock_guard<std::mutex> lock(chunkMutex);
        auto it = chunks.find(coord);
        
        if (it != chunks.end() && it->second->generated) {
            int localX = ((worldX % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
            int localY = ((worldY % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
            cacheHits++;
            return it->second->tiles[localY * CHUNK_SIZE + localX];
        }
        
        return TileType::EMPTY;
    }
    
    void DrawVisibleChunks(Camera2D camera) {
        // Calculate visible chunk range
        Vector2 topLeft = GetScreenToWorld2D({0, 0}, camera);
        Vector2 bottomRight = GetScreenToWorld2D({(float)SCREEN_WIDTH, (float)SCREEN_HEIGHT}, camera);
        
        ChunkCoord startChunk = GetChunkCoord(topLeft.x, topLeft.y);
        ChunkCoord endChunk = GetChunkCoord(bottomRight.x, bottomRight.y);
        
        std::lock_guard<std::mutex> lock(chunkMutex);
        
        for (int cy = startChunk.y; cy <= endChunk.y; cy++) {
            for (int cx = startChunk.x; cx <= endChunk.x; cx++) {
                ChunkCoord coord = {cx, cy};
                auto it = chunks.find(coord);
                
                if (it == chunks.end()) continue;
                
                auto& chunk = it->second;
                if (!chunk->generated) continue;
                
                // Draw chunk tiles
                for (int localY = 0; localY < CHUNK_SIZE; localY++) {
                    for (int localX = 0; localX < CHUNK_SIZE; localX++) {
                        TileType tile = chunk->tiles[localY * CHUNK_SIZE + localX];
                        if (tile == TileType::EMPTY) continue;
                        
                        float worldX = (coord.x * CHUNK_SIZE + localX) * TILE_SIZE;
                        float worldY = (coord.y * CHUNK_SIZE + localY) * TILE_SIZE;
                        
                        Color color;
                        switch (tile) {
                            case TileType::GRASS:
                                color = (chunk->biome == BiomeType::SWAMP) ? 
                                       Color{50, 120, 50, 255} : Color{50, 180, 50, 255};
                                break;
                            case TileType::DIRT:
                                color = Color{139, 90, 43, 255};
                                break;
                            case TileType::STONE:
                                color = Color{128, 128, 128, 255};
                                break;
                            case TileType::SAND:
                                color = Color{240, 220, 150, 255};
                                break;
                            case TileType::SNOW:
                                color = Color{250, 250, 250, 255};
                                break;
                            case TileType::WATER:
                                color = Color{50, 100, 200, 200};
                                break;
                            case TileType::LAVA:
                                color = Color{255, 100, 0, 255};
                                break;
                            case TileType::WOOD:
                                color = Color{101, 67, 33, 255};
                                break;
                            case TileType::LEAVES:
                                color = Color{34, 139, 34, 255};
                                break;
                            default:
                                color = MAGENTA;
                                break;
                        }
                        
                        DrawRectangle((int)worldX, (int)worldY, TILE_SIZE, TILE_SIZE, color);
                    }
                }
                
                // Draw chunk border (debug)
                if (IsKeyDown(KEY_B)) {
                    DrawRectangleLines(
                        coord.x * CHUNK_SIZE * TILE_SIZE,
                        coord.y * CHUNK_SIZE * TILE_SIZE,
                        CHUNK_SIZE * TILE_SIZE,
                        CHUNK_SIZE * TILE_SIZE,
                        ColorAlpha(RED, 0.5f)
                    );
                    
                    // Draw biome label
                    DrawText(TextFormat("%d,%d", coord.x, coord.y),
                            coord.x * CHUNK_SIZE * TILE_SIZE + 5,
                            coord.y * CHUNK_SIZE * TILE_SIZE + 5,
                            10, BLACK);
                }
            }
        }
    }
    
    void DrawDebugInfo() {
        DrawRectangle(10, 10, 280, 180, ColorAlpha(BLACK, 0.7f));
        
        DrawText("JIT MAP GENERATOR", 20, 20, 16, WHITE);
        DrawText(TextFormat("Chunks in memory: %zu", chunks.size()), 20, 45, 14, WHITE);
        DrawText(TextFormat("Chunks generated: %d", chunksGenerated), 20, 65, 14, WHITE);
        DrawText(TextFormat("Cache hits: %d", cacheHits), 20, 85, 14, GREEN);
        DrawText(TextFormat("Cache misses: %d", cacheMisses), 20, 105, 14, RED);
        DrawText(TextFormat("Gen time: %.3fs", generationTime), 20, 125, 14, WHITE);
        DrawText(TextFormat("Avg gen time: %.3fms", 
                chunksGenerated > 0 ? (generationTime / chunksGenerated) * 1000 : 0), 
                20, 145, 14, WHITE);
        DrawText(TextFormat("Pending futures: %zu", generationFutures.size()), 
                20, 165, 14, YELLOW);
    }
    
    BiomeType GetBiomeAt(Vector2 position) {
        ChunkCoord coord = GetChunkCoord(position.x, position.y);
        
        std::lock_guard<std::mutex> lock(chunkMutex);
        auto it = chunks.find(coord);
        if (it != chunks.end() && it->second->generated) {
            return it->second->biome;
        }
        return BiomeType::PLAINS;
    }
    
    const char* GetBiomeName(BiomeType biome) {
        switch (biome) {
            case BiomeType::PLAINS: return "Plains";
            case BiomeType::FOREST: return "Forest";
            case BiomeType::DESERT: return "Desert";
            case BiomeType::MOUNTAIN: return "Mountain";
            case BiomeType::SNOW: return "Snow";
            case BiomeType::SWAMP: return "Swamp";
            case BiomeType::VOLCANIC: return "Volcanic";
            default: return "Unknown";
        }
    }
};

// Player class
class Player {
public:
    Vector2 position;
    Vector2 velocity;
    float speed = 200.0f;
    
    Player() {
        position = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
        velocity = {0, 0};
    }
    
    void Update(float dt) {
        // Simple movement
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) position.x -= speed * dt;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) position.x += speed * dt;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) position.y -= speed * dt;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) position.y += speed * dt;
        
        // Sprint
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            position.x += velocity.x * speed * 2 * dt;
            position.y += velocity.y * speed * 2 * dt;
        }
    }
    
    void Draw() {
        DrawCircleV(position, 10, RED);
        DrawCircleV(position, 5, YELLOW);
    }
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "JIT Map Generator - Procedural 2D World");
    SetTargetFPS(60);
    
    // Create JIT map generator with random seed
    std::random_device rd;
    JITMapGenerator mapGenerator(rd());
    
    Player player;
    Camera2D camera = {0};
    camera.target = player.position;
    camera.offset = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Update
        player.Update(dt);
        mapGenerator.Update(player.position, dt);
        
        // Camera follows player
        camera.target = Vector2Lerp(camera.target, player.position, 5.0f * dt);
        
        // Draw
        BeginDrawing();
        ClearBackground(Color{20, 20, 30, 255});
        
        BeginMode2D(camera);
        
        // Draw map
        mapGenerator.DrawVisibleChunks(camera);
        player.Draw();
        
        EndMode2D();
        
        // Draw UI
        mapGenerator.DrawDebugInfo();
        
        // Draw biome info
        BiomeType currentBiome = mapGenerator.GetBiomeAt(player.position);
        DrawText(TextFormat("Current Biome: %s", mapGenerator.GetBiomeName(currentBiome)),
                SCREEN_WIDTH - 250, 10, 20, WHITE);
        
        // Draw controls
        DrawText("WASD/Arrows: Move | Shift: Sprint | B: Show Chunk Borders | ESC: Exit",
                SCREEN_WIDTH / 2 - 300, SCREEN_HEIGHT - 30, 16, WHITE);
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}