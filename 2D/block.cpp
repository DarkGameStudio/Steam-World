#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <functional>
#include <random>
#include <cmath>

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;
const int BLOCK_SIZE = 40;

// Block types
enum class BlockType {
    EMPTY,
    GRASS,
    DIRT,
    STONE,
    WOOD,
    LEAVES,
    SAND,
    WATER,
    LAVA,
    ICE,
    BRICK,
    GLASS,
    METAL,
    GOLD,
    DIAMOND,
    BEDROCK,
    TNT,
    CHEST,
    DOOR,
    TORCH,
    PLATFORM,
    SPIKE,
    SPRING,
    PORTAL
};

// Block properties
struct BlockProperties {
    bool isSolid;
    bool isDestructible;
    bool isTransparent;
    bool isLiquid;
    bool isFlammable;
    bool isExplosive;
    bool isInteractive;
    bool isAnimated;
    bool emitsLight;
    float hardness;
    float friction;
    float bounciness;
    Color baseColor;
    std::string name;
    
    BlockProperties() 
        : isSolid(true), isDestructible(true), isTransparent(false),
          isLiquid(false), isFlammable(false), isExplosive(false),
          isInteractive(false), isAnimated(false), emitsLight(false),
          hardness(1.0f), friction(0.5f), bounciness(0.0f),
          baseColor(GRAY), name("Block") {}
};

// Block structure
struct Block {
    BlockType type;
    BlockProperties properties;
    Vector2 position; // Grid position
    Vector2 worldPosition; // World position in pixels
    float health;
    float damage;
    bool isActive;
    float animationTime;
    int textureIndex;
    std::map<std::string, float> data; // Custom block data
    
    Block(BlockType t = BlockType::EMPTY, Vector2 gridPos = {0, 0})
        : type(t), position(gridPos), health(100), damage(0), 
          isActive(true), animationTime(0), textureIndex(0) {
        worldPosition = {gridPos.x * BLOCK_SIZE, gridPos.y * BLOCK_SIZE};
        properties = GetBlockProperties(t);
        health = properties.hardness * 100;
    }
    
    static BlockProperties GetBlockProperties(BlockType type) {
        BlockProperties props;
        
        switch (type) {
            case BlockType::EMPTY:
                props.name = "Empty";
                props.isSolid = false;
                props.isDestructible = false;
                props.isTransparent = true;
                props.baseColor = BLANK;
                break;
                
            case BlockType::GRASS:
                props.name = "Grass";
                props.isSolid = true;
                props.isDestructible = true;
                props.hardness = 0.5f;
                props.friction = 0.6f;
                props.baseColor = GREEN;
                break;
                
            case BlockType::DIRT:
                props.name = "Dirt";
                props.isSolid = true;
                props.isDestructible = true;
                props.hardness = 0.4f;
                props.friction = 0.7f;
                props.baseColor = BROWN;
                break;
                
            case BlockType::STONE:
                props.name = "Stone";
                props.isSolid = true;
                props.isDestructible = true;
                props.hardness = 2.0f;
                props.friction = 0.4f;
                props.baseColor = GRAY;
                break;
                
            case BlockType::WOOD:
                props.name = "Wood";
                props.isSolid = true;
                props.isDestructible = true;
                props.isFlammable = true;
                props.hardness = 1.0f;
                props.friction = 0.5f;
                props.baseColor = {139, 90, 43, 255};
                break;
                
            case BlockType::LEAVES:
                props.name = "Leaves";
                props.isSolid = false;
                props.isDestructible = true;
                props.isTransparent = true;
                props.isFlammable = true;
                props.hardness = 0.2f;
                props.baseColor = {34, 139, 34, 255};
                break;
                
            case BlockType::SAND:
                props.name = "Sand";
                props.isSolid = true;
                props.isDestructible = true;
                props.hardness = 0.3f;
                props.friction = 0.8f;
                props.baseColor = {240, 220, 150, 255};
                break;
                
            case BlockType::WATER:
                props.name = "Water";
                props.isSolid = false;
                props.isDestructible = false;
                props.isTransparent = true;
                props.isLiquid = true;
                props.baseColor = {50, 100, 200, 180};
                break;
                
            case BlockType::LAVA:
                props.name = "Lava";
                props.isSolid = false;
                props.isDestructible = false;
                props.isTransparent = true;
                props.isLiquid = true;
                props.emitsLight = true;
                props.baseColor = {255, 100, 0, 255};
                break;
                
            case BlockType::ICE:
                props.name = "Ice";
                props.isSolid = true;
                props.isDestructible = true;
                props.isTransparent = true;
                props.hardness = 0.6f;
                props.friction = 0.05f;
                props.baseColor = {200, 220, 255, 255};
                break;
                
            case BlockType::BRICK:
                props.name = "Brick";
                props.isSolid = true;
                props.isDestructible = true;
                props.hardness = 1.5f;
                props.friction = 0.5f;
                props.baseColor = {178, 34, 34, 255};
                break;
                
            case BlockType::GLASS:
                props.name = "Glass";
                props.isSolid = true;
                props.isDestructible = true;
                props.isTransparent = true;
                props.hardness = 0.3f;
                props.friction = 0.3f;
                props.baseColor = {200, 220, 255, 150};
                break;
                
            case BlockType::METAL:
                props.name = "Metal";
                props.isSolid = true;
                props.isDestructible = true;
                props.hardness = 3.0f;
                props.friction = 0.2f;
                props.baseColor = {180, 180, 180, 255};
                break;
                
            case BlockType::GOLD:
                props.name = "Gold";
                props.isSolid = true;
                props.isDestructible = true;
                props.hardness = 2.5f;
                props.friction = 0.3f;
                props.baseColor = GOLD;
                break;
                
            case BlockType::DIAMOND:
                props.name = "Diamond";
                props.isSolid = true;
                props.isDestructible = true;
                props.hardness = 5.0f;
                props.friction = 0.3f;
                props.baseColor = {0, 200, 255, 255};
                break;
                
            case BlockType::BEDROCK:
                props.name = "Bedrock";
                props.isSolid = true;
                props.isDestructible = false;
                props.hardness = 999.0f;
                props.friction = 0.5f;
                props.baseColor = {50, 50, 50, 255};
                break;
                
            case BlockType::TNT:
                props.name = "TNT";
                props.isSolid = true;
                props.isDestructible = true;
                props.isExplosive = true;
                props.isFlammable = true;
                props.hardness = 0.1f;
                props.baseColor = {255, 50, 50, 255};
                break;
                
            case BlockType::CHEST:
                props.name = "Chest";
                props.isSolid = true;
                props.isDestructible = true;
                props.isInteractive = true;
                props.hardness = 1.5f;
                props.baseColor = {139, 90, 43, 255};
                break;
                
            case BlockType::DOOR:
                props.name = "Door";
                props.isSolid = true;
                props.isDestructible = true;
                props.isInteractive = true;
                props.isTransparent = true;
                props.hardness = 1.0f;
                props.baseColor = {139, 90, 43, 255};
                break;
                
            case BlockType::TORCH:
                props.name = "Torch";
                props.isSolid = false;
                props.isDestructible = true;
                props.isTransparent = true;
                props.emitsLight = true;
                props.isAnimated = true;
                props.baseColor = {255, 200, 50, 255};
                break;
                
            case BlockType::PLATFORM:
                props.name = "Platform";
                props.isSolid = true;
                props.isDestructible = true;
                props.isTransparent = true;
                props.hardness = 0.8f;
                props.baseColor = {180, 180, 180, 255};
                break;
                
            case BlockType::SPIKE:
                props.name = "Spike";
                props.isSolid = false;
                props.isDestructible = false;
                props.isTransparent = true;
                props.baseColor = {150, 150, 150, 255};
                break;
                
            case BlockType::SPRING:
                props.name = "Spring";
                props.isSolid = true;
                props.isDestructible = true;
                props.isInteractive = true;
                props.bounciness = 2.0f;
                props.baseColor = {255, 150, 0, 255};
                break;
                
            case BlockType::PORTAL:
                props.name = "Portal";
                props.isSolid = false;
                props.isDestructible = false;
                props.isTransparent = true;
                props.isInteractive = true;
                props.isAnimated = true;
                props.emitsLight = true;
                props.baseColor = {150, 0, 255, 255};
                break;
                
            default:
                break;
        }
        
        return props;
    }
    
    void TakeDamage(float amount) {
        if (!properties.isDestructible) return;
        
        damage += amount;
        if (damage >= health) {
            isActive = false;
        }
    }
    
    void Update(float dt) {
        if (!isActive) return;
        
        animationTime += dt;
        
        // Update block-specific behaviors
        switch (type) {
            case BlockType::TNT:
                if (damage > 0) {
                    // TNT explodes when damaged
                    Explode();
                }
                break;
                
            case BlockType::TORCH:
                // Animate torch flame
                break;
                
            case BlockType::PORTAL:
                // Animate portal
                break;
                
            default:
                break;
        }
    }
    
    void Explode() {
        // Handle explosion logic
        isActive = false;
        // In a real game, this would trigger an explosion effect
    }
    
    void Draw() {
        if (!isActive) return;
        
        Color drawColor = properties.baseColor;
        
        // Apply damage tint
        if (damage > 0) {
            float damagePercent = damage / health;
            drawColor = ColorLerp(drawColor, RED, damagePercent * 0.5f);
        }
        
        // Draw block based on type
        switch (type) {
            case BlockType::EMPTY:
                // Don't draw empty blocks
                return;
                
            case BlockType::GRASS:
                DrawRectangleV(worldPosition, {BLOCK_SIZE, BLOCK_SIZE}, drawColor);
                // Draw grass top
                DrawRectangleV(worldPosition, {BLOCK_SIZE, BLOCK_SIZE * 0.3f}, GREEN);
                break;
                
            case BlockType::WOOD:
                DrawRectangleV(worldPosition, {BLOCK_SIZE, BLOCK_SIZE}, drawColor);
                // Draw wood grain
                for (int i = 0; i < 3; i++) {
                    DrawLine(worldPosition.x, worldPosition.y + i * 10,
                            worldPosition.x + BLOCK_SIZE, worldPosition.y + i * 10,
                            {101, 67, 33, 255});
                }
                break;
                
            case BlockType::BRICK:
                DrawRectangleV(worldPosition, {BLOCK_SIZE, BLOCK_SIZE}, drawColor);
                // Draw brick pattern
                DrawLine(worldPosition.x, worldPosition.y + BLOCK_SIZE/2,
                        worldPosition.x + BLOCK_SIZE, worldPosition.y + BLOCK_SIZE/2,
                        DARKGRAY);
                DrawLine(worldPosition.x + BLOCK_SIZE/2, worldPosition.y,
                        worldPosition.x + BLOCK_SIZE/2, worldPosition.y + BLOCK_SIZE/2,
                        DARKGRAY);
                DrawLine(worldPosition.x + BLOCK_SIZE/4, worldPosition.y + BLOCK_SIZE/2,
                        worldPosition.x + BLOCK_SIZE/4, worldPosition.y + BLOCK_SIZE,
                        DARKGRAY);
                DrawLine(worldPosition.x + BLOCK_SIZE * 3/4, worldPosition.y + BLOCK_SIZE/2,
                        worldPosition.x + BLOCK_SIZE * 3/4, worldPosition.y + BLOCK_SIZE,
                        DARKGRAY);
                break;
                
            case BlockType::GLASS:
                DrawRectangleV(worldPosition, {BLOCK_SIZE, BLOCK_SIZE}, drawColor);
                // Draw glass shine
                DrawLine(worldPosition.x + BLOCK_SIZE * 0.2f, worldPosition.y,
                        worldPosition.x + BLOCK_SIZE * 0.3f, worldPosition.y + BLOCK_SIZE,
                        ColorAlpha(WHITE, 0.3f));
                break;
                
            case BlockType::TORCH:
                DrawRectangleV({worldPosition.x + BLOCK_SIZE * 0.35f, worldPosition.y + BLOCK_SIZE * 0.3f},
                              {BLOCK_SIZE * 0.3f, BLOCK_SIZE * 0.7f}, BROWN);
                // Animated flame
                float flicker = sin(animationTime * 10) * 5;
                DrawCircleV({worldPosition.x + BLOCK_SIZE/2, worldPosition.y + BLOCK_SIZE * 0.25f + flicker},
                          8, {255, 200, 50, 255});
                DrawCircleV({worldPosition.x + BLOCK_SIZE/2, worldPosition.y + BLOCK_SIZE * 0.25f + flicker},
                          4, {255, 255, 150, 255});
                break;
                
            case BlockType::SPIKE:
                DrawTriangle(
                    {worldPosition.x, worldPosition.y + BLOCK_SIZE},
                    {worldPosition.x + BLOCK_SIZE/2, worldPosition.y},
                    {worldPosition.x + BLOCK_SIZE, worldPosition.y + BLOCK_SIZE},
                    drawColor
                );
                break;
                
            case BlockType::SPRING:
                DrawRectangleV(worldPosition, {BLOCK_SIZE, BLOCK_SIZE * 0.7f}, DARKGRAY);
                DrawRectangleV({worldPosition.x, worldPosition.y + BLOCK_SIZE * 0.7f},
                              {BLOCK_SIZE, BLOCK_SIZE * 0.3f}, drawColor);
                break;
                
            case BlockType::PORTAL:
                // Animated portal
                float pulse = (sin(animationTime * 3) + 1) / 2;
                DrawRectangleV(worldPosition, {BLOCK_SIZE, BLOCK_SIZE}, 
                              ColorAlpha(drawColor, 0.3f + pulse * 0.3f));
                DrawRectangleLinesEx({worldPosition.x, worldPosition.y, BLOCK_SIZE, BLOCK_SIZE},
                                    3, ColorAlpha(drawColor, 0.5f + pulse * 0.5f));
                break;
                
            default:
                DrawRectangleV(worldPosition, {BLOCK_SIZE, BLOCK_SIZE}, drawColor);
                break;
        }
        
        // Draw block outline
        if (properties.isSolid) {
            DrawRectangleLines(worldPosition.x, worldPosition.y, BLOCK_SIZE, BLOCK_SIZE, 
                             ColorAlpha(BLACK, 0.3f));
        }
    }
};

// Block manager class
class BlockManager {
private:
    std::vector<std::vector<std::unique_ptr<Block>>> blocks;
    int gridWidth;
    int gridHeight;
    
    // Block selection for building
    BlockType selectedBlockType;
    bool buildMode;
    
public:
    BlockManager(int width, int height) : gridWidth(width), gridHeight(height),
                                          selectedBlockType(BlockType::GRASS),
                                          buildMode(false) {
        // Initialize grid
        blocks.resize(gridHeight);
        for (int y = 0; y < gridHeight; y++) {
            blocks[y].resize(gridWidth);
            for (int x = 0; x < gridWidth; x++) {
                blocks[y][x] = std::make_unique<Block>(BlockType::EMPTY, Vector2{x, y});
            }
        }
    }
    
    void SetBlock(int x, int y, BlockType type) {
        if (x < 0 || x >= gridWidth || y < 0 || y >= gridHeight) return;
        
        blocks[y][x] = std::make_unique<Block>(type, Vector2{x, y});
    }
    
    Block* GetBlock(int x, int y) {
        if (x < 0 || x >= gridWidth || y < 0 || y >= gridHeight) return nullptr;
        return blocks[y][x].get();
    }
    
    void RemoveBlock(int x, int y) {
        if (x < 0 || x >= gridWidth || y < 0 || y >= gridHeight) return;
        blocks[y][x] = std::make_unique<Block>(BlockType::EMPTY, Vector2{x, y});
    }
    
    void DamageBlock(int x, int y, float damage) {
        Block* block = GetBlock(x, y);
        if (block) {
            block->TakeDamage(damage);
            if (!block->isActive) {
                RemoveBlock(x, y);
            }
        }
    }
    
    void Update(float dt) {
        for (int y = 0; y < gridHeight; y++) {
            for (int x = 0; x < gridWidth; x++) {
                if (blocks[y][x]) {
                    blocks[y][x]->Update(dt);
                }
            }
        }
    }
    
    void Draw() {
        for (int y = 0; y < gridHeight; y++) {
            for (int x = 0; x < gridWidth; x++) {
                if (blocks[y][x]) {
                    blocks[y][x]->Draw();
                }
            }
        }
    }
    
    void HandleInput(Vector2 mousePos) {
        // Convert mouse position to grid coordinates
        int gridX = (int)(mousePos.x / BLOCK_SIZE);
        int gridY = (int)(mousePos.y / BLOCK_SIZE);
        
        // Build mode
        if (IsKeyPressed(KEY_B)) {
            buildMode = !buildMode;
        }
        
        // Select block type
        if (IsKeyPressed(KEY_1)) selectedBlockType = BlockType::GRASS;
        if (IsKeyPressed(KEY_2)) selectedBlockType = BlockType::DIRT;
        if (IsKeyPressed(KEY_3)) selectedBlockType = BlockType::STONE;
        if (IsKeyPressed(KEY_4)) selectedBlockType = BlockType::WOOD;
        if (IsKeyPressed(KEY_5)) selectedBlockType = BlockType::BRICK;
        if (IsKeyPressed(KEY_6)) selectedBlockType = BlockType::GLASS;
        if (IsKeyPressed(KEY_7)) selectedBlockType = BlockType::METAL;
        if (IsKeyPressed(KEY_8)) selectedBlockType = BlockType::GOLD;
        if (IsKeyPressed(KEY_9)) selectedBlockType = BlockType::DIAMOND;
        if (IsKeyPressed(KEY_0)) selectedBlockType = BlockType::TORCH;
        
        // Mouse interactions
        if (buildMode) {
            // Left click to place block
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                SetBlock(gridX, gridY, selectedBlockType);
            }
            // Right click to remove block
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                RemoveBlock(gridX, gridY);
            }
        } else {
            // Left click to damage block
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                DamageBlock(gridX, gridY, 50.0f * GetFrameTime());
            }
        }
    }
    
    void DrawUI() {
        // Draw build mode indicator
        Color modeColor = buildMode ? GREEN : RED;
        DrawText(buildMode ? "BUILD MODE" : "DESTROY MODE", 
                SCREEN_WIDTH - 200, 20, 20, modeColor);
        
        // Draw selected block info
        BlockProperties props = Block::GetBlockProperties(selectedBlockType);
        DrawText(TextFormat("Selected: %s", props.name.c_str()), 
                SCREEN_WIDTH - 200, 50, 16, WHITE);
        
        // Draw block preview
        DrawRectangle(SCREEN_WIDTH - 60, 80, BLOCK_SIZE, BLOCK_SIZE, props.baseColor);
        DrawRectangleLines(SCREEN_WIDTH - 60, 80, BLOCK_SIZE, BLOCK_SIZE, BLACK);
        
        // Draw block properties
        DrawText(TextFormat("Solid: %s", props.isSolid ? "Yes" : "No"), 
                SCREEN_WIDTH - 200, 130, 14, props.isSolid ? GREEN : RED);
        DrawText(TextFormat("Destructible: %s", props.isDestructible ? "Yes" : "No"), 
                SCREEN_WIDTH - 200, 150, 14, props.isDestructible ? GREEN : RED);
        DrawText(TextFormat("Hardness: %.1f", props.hardness), 
                SCREEN_WIDTH - 200, 170, 14, WHITE);
        DrawText(TextFormat("Friction: %.2f", props.friction), 
                SCREEN_WIDTH - 200, 190, 14, WHITE);
        
        // Draw controls
        DrawText("Controls:", 10, 10, 20, YELLOW);
        DrawText("1-9,0: Select Block", 10, 40, 16, WHITE);
        DrawText("B: Toggle Build Mode", 10, 60, 16, WHITE);
        DrawText("Left Click: Place/Damage Block", 10, 80, 16, WHITE);
        DrawText("Right Click: Remove Block", 10, 100, 16, WHITE);
    }
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Block Structure System");
    SetTargetFPS(60);
    
    // Create block manager
    int gridWidth = SCREEN_WIDTH / BLOCK_SIZE;
    int gridHeight = SCREEN_HEIGHT / BLOCK_SIZE;
    BlockManager blockManager(gridWidth, gridHeight);
    
    // Generate some initial blocks
    for (int x = 0; x < gridWidth; x++) {
        for (int y = gridHeight - 5; y < gridHeight; y++) {
            if (y == gridHeight - 5) {
                blockManager.SetBlock(x, y, BlockType::GRASS);
            } else if (y < gridHeight - 2) {
                blockManager.SetBlock(x, y, BlockType::DIRT);
            } else {
                blockManager.SetBlock(x, y, BlockType::STONE);
            }
        }
    }
    
    // Add some trees
    for (int i = 0; i < 10; i++) {
        int treeX = GetRandomValue(5, gridWidth - 5);
        int treeBaseY = gridHeight - 6;
        
        blockManager.SetBlock(treeX, treeBaseY - 1, BlockType::WOOD);
        blockManager.SetBlock(treeX, treeBaseY - 2, BlockType::WOOD);
        blockManager.SetBlock(treeX, treeBaseY - 3, BlockType::LEAVES);
        blockManager.SetBlock(treeX - 1, treeBaseY - 3, BlockType::LEAVES);
        blockManager.SetBlock(treeX + 1, treeBaseY - 3, BlockType::LEAVES);
        blockManager.SetBlock(treeX, treeBaseY - 4, BlockType::LEAVES);
    }
    
    // Add some special blocks
    blockManager.SetBlock(5, gridHeight - 6, BlockType::CHEST);
    blockManager.SetBlock(10, gridHeight - 6, BlockType::TORCH);
    blockManager.SetBlock(15, gridHeight - 6, BlockType::SPIKE);
    blockManager.SetBlock(20, gridHeight - 6, BlockType::SPRING);
    blockManager.SetBlock(25, gridHeight - 6, BlockType::PORTAL);
    
    // Game loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Update
        blockManager.Update(dt);
        
        // Handle input
        Vector2 mousePos = GetMousePosition();
        blockManager.HandleInput(mousePos);
        
        // Draw
        BeginDrawing();
        ClearBackground({50, 50, 70, 255});
        
        // Draw grid
        for (int x = 0; x <= gridWidth; x++) {
            DrawLine(x * BLOCK_SIZE, 0, x * BLOCK_SIZE, SCREEN_HEIGHT, ColorAlpha(GRAY, 0.2f));
        }
        for (int y = 0; y <= gridHeight; y++) {
            DrawLine(0, y * BLOCK_SIZE, SCREEN_WIDTH, y * BLOCK_SIZE, ColorAlpha(GRAY, 0.2f));
        }
        
        // Draw blocks
        blockManager.Draw();
        
        // Draw UI
        blockManager.DrawUI();
        
        // Draw hover highlight
        int mouseGridX = (int)(mousePos.x / BLOCK_SIZE);
        int mouseGridY = (int)(mousePos.y / BLOCK_SIZE);
        if (mouseGridX >= 0 && mouseGridX < gridWidth && 
            mouseGridY >= 0 && mouseGridY < gridHeight) {
            DrawRectangleLines(mouseGridX * BLOCK_SIZE, mouseGridY * BLOCK_SIZE, 
                              BLOCK_SIZE, BLOCK_SIZE, YELLOW);
        }
        
        // Draw title
        DrawText("Block Structure System", SCREEN_WIDTH/2 - 130, 10, 25, WHITE);
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}