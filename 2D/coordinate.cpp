#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <cmath>
#include <sstream>
#include <iomanip>

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;

// Coordinate display modes
enum class CoordinateMode {
    SCREEN_SPACE,
    WORLD_SPACE,
    GRID_SPACE,
    TILE_SPACE,
    CHUNK_SPACE,
    POLAR_SPACE
};

// Grid settings
struct GridSettings {
    bool showGrid = true;
    bool showMajorLines = true;
    bool showMinorLines = true;
    int minorGridSize = 50;
    int majorGridSize = 250;
    float gridAlpha = 0.3f;
    Color minorGridColor = {100, 100, 100, 80};
    Color majorGridColor = {150, 150, 150, 120};
    Color axisColor = {255, 255, 0, 200};
};

// Coordinate display settings
struct CoordinateDisplaySettings {
    bool showScreenCoords = true;
    bool showWorldCoords = true;
    bool showGridCoords = true;
    bool showTileCoords = true;
    bool showChunkCoords = false;
    bool showPolarCoords = false;
    bool showMousePosition = true;
    bool showPlayerPosition = true;
    bool showDistanceInfo = true;
    bool showCoordinateAxes = true;
    bool showMiniMap = false;
    bool showTooltip = true;
    int fontSize = 16;
    Color textColor = WHITE;
    Color backgroundColor = {0, 0, 0, 180};
};

// Coordinate system class
class CoordinateSystem {
private:
    Vector2 origin; // World origin in screen space
    float zoom;
    float rotation;
    GridSettings gridSettings;
    CoordinateDisplaySettings displaySettings;
    
    // Mouse tracking
    Vector2 mouseScreenPos;
    Vector2 mouseWorldPos;
    bool mouseInWorld;
    
    // Player position
    Vector2 playerPosition;
    
    // Reference points for distance measurement
    std::vector<std::pair<Vector2, std::string>> referencePoints;
    
public:
    CoordinateSystem() : origin({SCREEN_WIDTH/2, SCREEN_HEIGHT/2}), zoom(1.0f), 
                        rotation(0.0f), mouseInWorld(false), 
                        playerPosition({0, 0}) {
        // Add default reference points
        referencePoints.push_back({{0, 0}, "Origin"});
        referencePoints.push_back({{100, 100}, "Point A"});
        referencePoints.push_back({{-50, 200}, "Point B"});
    }
    
    // Coordinate conversion functions
    Vector2 ScreenToWorld(Vector2 screenPos) {
        // Apply inverse rotation
        float cosRot = cos(-rotation);
        float sinRot = sin(-rotation);
        float dx = screenPos.x - origin.x;
        float dy = screenPos.y - origin.y;
        
        float rotatedX = dx * cosRot - dy * sinRot;
        float rotatedY = dx * sinRot + dy * cosRot;
        
        // Apply inverse zoom
        return {
            rotatedX / zoom,
            rotatedY / zoom
        };
    }
    
    Vector2 WorldToScreen(Vector2 worldPos) {
        // Apply zoom
        float scaledX = worldPos.x * zoom;
        float scaledY = worldPos.y * zoom;
        
        // Apply rotation
        float cosRot = cos(rotation);
        float sinRot = sin(rotation);
        float rotatedX = scaledX * cosRot - scaledY * sinRot;
        float rotatedY = scaledX * sinRot + scaledY * cosRot;
        
        // Apply translation
        return {
            rotatedX + origin.x,
            rotatedY + origin.y
        };
    }
    
    Vector2 WorldToGrid(Vector2 worldPos) {
        return {
            floor(worldPos.x / gridSettings.minorGridSize),
            floor(worldPos.y / gridSettings.minorGridSize)
        };
    }
    
    Vector2 WorldToTile(Vector2 worldPos, int tileSize = 32) {
        return {
            floor(worldPos.x / tileSize),
            floor(worldPos.y / tileSize)
        };
    }
    
    Vector2 WorldToChunk(Vector2 worldPos, int chunkSize = 16, int tileSize = 32) {
        Vector2 tilePos = WorldToTile(worldPos, tileSize);
        return {
            floor(tilePos.x / chunkSize),
            floor(tilePos.y / chunkSize)
        };
    }
    
    std::pair<float, float> WorldToPolar(Vector2 worldPos) {
        float radius = Vector2Length(worldPos);
        float angle = atan2(worldPos.y, worldPos.x) * RAD2DEG;
        return {radius, angle};
    }
    
    // Update functions
    void Update(Vector2 playerPos) {
        playerPosition = playerPos;
        
        // Get mouse position
        mouseScreenPos = GetMousePosition();
        
        // Check if mouse is in world
        mouseInWorld = (mouseScreenPos.x >= 0 && mouseScreenPos.x < SCREEN_WIDTH &&
                       mouseScreenPos.y >= 0 && mouseScreenPos.y < SCREEN_HEIGHT);
        
        if (mouseInWorld) {
            mouseWorldPos = ScreenToWorld(mouseScreenPos);
        }
        
        // Zoom controls
        float wheelMove = GetMouseWheelMove();
        if (wheelMove != 0) {
            Vector2 mouseWorldBefore = ScreenToWorld(mouseScreenPos);
            zoom *= (1.0f + wheelMove * 0.1f);
            zoom = Clamp(zoom, 0.1f, 10.0f);
            
            // Adjust origin to zoom towards mouse
            Vector2 mouseWorldAfter = ScreenToWorld(mouseScreenPos);
            origin.x += (mouseWorldAfter.x - mouseWorldBefore.x) * zoom;
            origin.y += (mouseWorldAfter.y - mouseWorldBefore.y) * zoom;
        }
        
        // Pan with middle mouse button or right mouse button
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || 
            (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && IsKeyDown(KEY_LEFT_CONTROL))) {
            Vector2 mouseDelta = GetMouseDelta();
            origin.x += mouseDelta.x;
            origin.y += mouseDelta.y;
        }
        
        // Reset view
        if (IsKeyPressed(KEY_R)) {
            origin = {SCREEN_WIDTH/2, SCREEN_HEIGHT/2};
            zoom = 1.0f;
            rotation = 0.0f;
        }
        
        // Rotate view
        if (IsKeyDown(KEY_Q)) {
            rotation -= 0.5f * GetFrameTime();
        }
        if (IsKeyDown(KEY_E)) {
            rotation += 0.5f * GetFrameTime();
        }
    }
    
    // Drawing functions
    void DrawGrid() {
        if (!gridSettings.showGrid) return;
        
        // Calculate visible world area
        Vector2 topLeft = ScreenToWorld({0, 0});
        Vector2 bottomRight = ScreenToWorld({(float)SCREEN_WIDTH, (float)SCREEN_HEIGHT});
        
        // Draw minor grid lines
        if (gridSettings.showMinorLines) {
            int startX = (int)floor(topLeft.x / gridSettings.minorGridSize) * gridSettings.minorGridSize;
            int endX = (int)ceil(bottomRight.x / gridSettings.minorGridSize) * gridSettings.minorGridSize;
            int startY = (int)floor(topLeft.y / gridSettings.minorGridSize) * gridSettings.minorGridSize;
            int endY = (int)ceil(bottomRight.y / gridSettings.minorGridSize) * gridSettings.minorGridSize;
            
            for (int x = startX; x <= endX; x += gridSettings.minorGridSize) {
                Vector2 screenStart = WorldToScreen({(float)x, topLeft.y});
                Vector2 screenEnd = WorldToScreen({(float)x, bottomRight.y});
                DrawLine(screenStart.x, screenStart.y, screenEnd.x, screenEnd.y, 
                        gridSettings.minorGridColor);
            }
            
            for (int y = startY; y <= endY; y += gridSettings.minorGridSize) {
                Vector2 screenStart = WorldToScreen({topLeft.x, (float)y});
                Vector2 screenEnd = WorldToScreen({bottomRight.x, (float)y});
                DrawLine(screenStart.x, screenStart.y, screenEnd.x, screenEnd.y, 
                        gridSettings.minorGridColor);
            }
        }
        
        // Draw major grid lines
        if (gridSettings.showMajorLines) {
            int startX = (int)floor(topLeft.x / gridSettings.majorGridSize) * gridSettings.majorGridSize;
            int endX = (int)ceil(bottomRight.x / gridSettings.majorGridSize) * gridSettings.majorGridSize;
            int startY = (int)floor(topLeft.y / gridSettings.majorGridSize) * gridSettings.majorGridSize;
            int endY = (int)ceil(bottomRight.y / gridSettings.majorGridSize) * gridSettings.majorGridSize;
            
            for (int x = startX; x <= endX; x += gridSettings.majorGridSize) {
                Vector2 screenStart = WorldToScreen({(float)x, topLeft.y});
                Vector2 screenEnd = WorldToScreen({(float)x, bottomRight.y});
                DrawLine(screenStart.x, screenStart.y, screenEnd.x, screenEnd.y, 
                        gridSettings.majorGridColor);
                
                // Draw coordinate labels
                DrawText(TextFormat("%d", x), screenStart.x + 5, screenStart.y + 5, 
                        10, gridSettings.majorGridColor);
            }
            
            for (int y = startY; y <= endY; y += gridSettings.majorGridSize) {
                Vector2 screenStart = WorldToScreen({topLeft.x, (float)y});
                Vector2 screenEnd = WorldToScreen({bottomRight.x, (float)y});
                DrawLine(screenStart.x, screenStart.y, screenEnd.x, screenEnd.y, 
                        gridSettings.majorGridColor);
                
                // Draw coordinate labels
                DrawText(TextFormat("%d", y), screenStart.x + 5, screenStart.y + 5, 
                        10, gridSettings.majorGridColor);
            }
        }
        
        // Draw axes
        if (displaySettings.showCoordinateAxes) {
            DrawAxes();
        }
    }
    
    void DrawAxes() {
        Vector2 xAxisStart = WorldToScreen({-10000, 0});
        Vector2 xAxisEnd = WorldToScreen({10000, 0});
        Vector2 yAxisStart = WorldToScreen({0, -10000});
        Vector2 yAxisEnd = WorldToScreen({0, 10000});
        
        // Draw X axis (red)
        DrawLine(xAxisStart.x, xAxisStart.y, xAxisEnd.x, xAxisEnd.y, RED);
        DrawText("X", xAxisEnd.x - 20, xAxisEnd.y + 10, 20, RED);
        
        // Draw Y axis (green)
        DrawLine(yAxisStart.x, yAxisStart.y, yAxisEnd.x, yAxisEnd.y, GREEN);
        DrawText("Y", yAxisEnd.x + 10, yAxisEnd.y - 20, 20, GREEN);
        
        // Draw origin marker
        Vector2 originScreen = WorldToScreen({0, 0});
        DrawCircleV(originScreen, 5, YELLOW);
        DrawText("O(0,0)", originScreen.x + 10, originScreen.y - 10, 12, YELLOW);
    }
    
    void DrawReferencePoints() {
        for (const auto& point : referencePoints) {
            Vector2 screenPos = WorldToScreen(point.first);
            DrawCircleV(screenPos, 4, ORANGE);
            DrawText(point.second.c_str(), screenPos.x + 8, screenPos.y - 8, 12, ORANGE);
            
            // Draw coordinates
            DrawText(TextFormat("(%.0f, %.0f)", point.first.x, point.first.y), 
                    screenPos.x + 8, screenPos.y + 8, 10, GRAY);
        }
    }
    
    void DrawMouseInfo() {
        if (!mouseInWorld || !displaySettings.showMousePosition) return;
        
        // Draw crosshair at mouse position
        DrawCircleV(mouseScreenPos, 3, YELLOW);
        DrawLine(mouseScreenPos.x - 10, mouseScreenPos.y, mouseScreenPos.x + 10, mouseScreenPos.y, YELLOW);
        DrawLine(mouseScreenPos.x, mouseScreenPos.y - 10, mouseScreenPos.x, mouseScreenPos.y + 10, YELLOW);
        
        // Create info string
        std::string info;
        info += "Mouse Position:\n";
        
        if (displaySettings.showScreenCoords) {
            info += TextFormat("  Screen: (%.0f, %.0f)\n", mouseScreenPos.x, mouseScreenPos.y);
        }
        
        if (displaySettings.showWorldCoords) {
            info += TextFormat("  World: (%.2f, %.2f)\n", mouseWorldPos.x, mouseWorldPos.y);
        }
        
        if (displaySettings.showGridCoords) {
            Vector2 gridPos = WorldToGrid(mouseWorldPos);
            info += TextFormat("  Grid: (%.0f, %.0f)\n", gridPos.x, gridPos.y);
        }
        
        if (displaySettings.showTileCoords) {
            Vector2 tilePos = WorldToTile(mouseWorldPos);
            info += TextFormat("  Tile: (%.0f, %.0f)\n", tilePos.x, tilePos.y);
        }
        
        if (displaySettings.showChunkCoords) {
            Vector2 chunkPos = WorldToChunk(mouseWorldPos);
            info += TextFormat("  Chunk: (%.0f, %.0f)\n", chunkPos.x, chunkPos.y);
        }
        
        if (displaySettings.showPolarCoords) {
            auto polar = WorldToPolar(mouseWorldPos);
            info += TextFormat("  Polar: (r=%.2f, θ=%.1f°)\n", polar.first, polar.second);
        }
        
        // Draw info box
        DrawInfoBox(info, mouseScreenPos.x + 20, mouseScreenPos.y + 20);
    }
    
    void DrawPlayerInfo() {
        if (!displaySettings.showPlayerPosition) return;
        
        Vector2 playerScreenPos = WorldToScreen(playerPosition);
        
        // Draw player marker
        DrawCircleV(playerScreenPos, 8, BLUE);
        DrawCircleV(playerScreenPos, 3, WHITE);
        
        // Draw player coordinates
        std::string info = "Player Position:\n";
        
        if (displaySettings.showScreenCoords) {
            info += TextFormat("  Screen: (%.0f, %.0f)\n", playerScreenPos.x, playerScreenPos.y);
        }
        
        if (displaySettings.showWorldCoords) {
            info += TextFormat("  World: (%.2f, %.2f)\n", playerPosition.x, playerPosition.y);
        }
        
        if (displaySettings.showGridCoords) {
            Vector2 gridPos = WorldToGrid(playerPosition);
            info += TextFormat("  Grid: (%.0f, %.0f)\n", gridPos.x, gridPos.y);
        }
        
        if (displaySettings.showTileCoords) {
            Vector2 tilePos = WorldToTile(playerPosition);
            info += TextFormat("  Tile: (%.0f, %.0f)\n", tilePos.x, tilePos.y);
        }
        
        if (displaySettings.showPolarCoords) {
            auto polar = WorldToPolar(playerPosition);
            info += TextFormat("  Polar: (r=%.2f, θ=%.1f°)\n", polar.first, polar.second);
        }
        
        // Draw info box at top-left corner
        DrawInfoBox(info, 10, 10);
    }
    
    void DrawDistanceInfo() {
        if (!displaySettings.showDistanceInfo) return;
        
        Vector2 playerScreenPos = WorldToScreen(playerPosition);
        
        // Draw distances to reference points
        std::string info = "Distances from Player:\n";
        
        for (const auto& point : referencePoints) {
            float distance = Vector2Distance(playerPosition, point.first);
            info += TextFormat("  %s: %.2f units\n", point.second.c_str(), distance);
        }
        
        // Draw distance to mouse
        if (mouseInWorld) {
            float mouseDistance = Vector2Distance(playerPosition, mouseWorldPos);
            info += TextFormat("  Mouse: %.2f units\n", mouseDistance);
            
            // Draw line to mouse
            DrawLineV(playerScreenPos, mouseScreenPos, ColorAlpha(YELLOW, 0.3f));
            
            // Draw distance label at midpoint
            Vector2 midPoint = Vector2Scale(Vector2Add(playerScreenPos, mouseScreenPos), 0.5f);
            DrawText(TextFormat("%.2f", mouseDistance), midPoint.x - 20, midPoint.y - 10, 
                    12, YELLOW);
        }
        
        // Draw info box at top-right
        DrawInfoBox(info, SCREEN_WIDTH - 300, 10);
    }
    
    void DrawMiniMap() {
        if (!displaySettings.showMiniMap) return;
        
        // Mini map settings
        int miniMapSize = 200;
        int miniMapX = SCREEN_WIDTH - miniMapSize - 20;
        int miniMapY = SCREEN_HEIGHT - miniMapSize - 20;
        float miniMapScale = 0.1f; // World units per pixel
        
        // Draw mini map background
        DrawRectangle(miniMapX - 5, miniMapY - 5, miniMapSize + 10, miniMapSize + 10, 
                     ColorAlpha(BLACK, 0.7f));
        DrawRectangle(miniMapX, miniMapY, miniMapSize, miniMapSize, 
                     ColorAlpha(DARKGRAY, 0.5f));
        
        // Draw reference points on mini map
        for (const auto& point : referencePoints) {
            int px = miniMapX + miniMapSize/2 + (int)(point.first.x * miniMapScale);
            int py = miniMapY + miniMapSize/2 + (int)(point.first.y * miniMapScale);
            
            if (px >= miniMapX && px < miniMapX + miniMapSize &&
                py >= miniMapY && py < miniMapY + miniMapSize) {
                DrawCircle(px, py, 3, ORANGE);
            }
        }
        
        // Draw player on mini map
        int playerMiniX = miniMapX + miniMapSize/2 + (int)(playerPosition.x * miniMapScale);
        int playerMiniY = miniMapY + miniMapSize/2 + (int)(playerPosition.y * miniMapScale);
        
        if (playerMiniX >= miniMapX && playerMiniX < miniMapX + miniMapSize &&
            playerMiniY >= miniMapY && playerMiniY < miniMapY + miniMapSize) {
            DrawCircle(playerMiniX, playerMiniY, 4, BLUE);
        }
        
        // Draw mini map border
        DrawRectangleLines(miniMapX - 5, miniMapY - 5, miniMapSize + 10, miniMapSize + 10, WHITE);
        
        // Draw viewport rectangle on mini map
        float viewWidth = SCREEN_WIDTH / zoom * miniMapScale;
        float viewHeight = SCREEN_HEIGHT / zoom * miniMapScale;
        
        Rectangle viewRect = {
            miniMapX + miniMapSize/2 - viewWidth/2,
            miniMapY + miniMapSize/2 - viewHeight/2,
            viewWidth,
            viewHeight
        };
        
        DrawRectangleLinesEx(viewRect, 1, YELLOW);
    }
    
    void DrawInfoBox(const std::string& text, float x, float y) {
        // Calculate text dimensions
        int lines = 1;
        int maxWidth = 0;
        int currentWidth = 0;
        
        for (char c : text) {
            if (c == '\n') {
                lines++;
                maxWidth = std::max(maxWidth, currentWidth);
                currentWidth = 0;
            } else {
                currentWidth += MeasureText(std::string(1, c).c_str(), displaySettings.fontSize);
            }
        }
        maxWidth = std::max(maxWidth, currentWidth);
        
        int boxWidth = maxWidth + 20;
        int boxHeight = lines * (displaySettings.fontSize + 5) + 20;
        
        // Draw background
        DrawRectangle(x - 5, y - 5, boxWidth, boxHeight, displaySettings.backgroundColor);
        DrawRectangleLines(x - 5, y - 5, boxWidth, boxHeight, ColorAlpha(WHITE, 0.5f));
        
        // Draw text
        DrawText(text.c_str(), x, y, displaySettings.fontSize, displaySettings.textColor);
    }
    
    void DrawAll() {
        DrawGrid();
        DrawReferencePoints();
        DrawPlayerInfo();
        DrawMouseInfo();
        DrawDistanceInfo();
        DrawMiniMap();
    }
    
    // Setters and getters
    void SetOrigin(Vector2 newOrigin) { origin = newOrigin; }
    void SetZoom(float newZoom) { zoom = newZoom; }
    void SetRotation(float newRotation) { rotation = newRotation; }
    void SetGridSettings(const GridSettings& settings) { gridSettings = settings; }
    void SetDisplaySettings(const CoordinateDisplaySettings& settings) { displaySettings = settings; }
    
    Vector2 GetOrigin() const { return origin; }
    float GetZoom() const { return zoom; }
    float GetRotation() const { return rotation; }
    GridSettings& GetGridSettings() { return gridSettings; }
    CoordinateDisplaySettings& GetDisplaySettings() { return displaySettings; }
    
    void AddReferencePoint(Vector2 pos, const std::string& name) {
        referencePoints.push_back({pos, name});
    }
    
    void ClearReferencePoints() {
        referencePoints.clear();
    }
};

// Coordinate display manager
class CoordinateDisplayManager {
private:
    CoordinateSystem coordSystem;
    CoordinateMode currentMode;
    bool showCoordinatePanel;
    bool showHelpPanel;
    
    // Display panel settings
    Rectangle coordPanelRect;
    Rectangle helpPanelRect;
    
public:
    CoordinateDisplayManager() : currentMode(CoordinateMode::WORLD_SPACE),
                                showCoordinatePanel(true), showHelpPanel(false) {
        coordPanelRect = {10, SCREEN_HEIGHT - 300, 350, 290};
        helpPanelRect = {SCREEN_WIDTH/2 - 200, 50, 400, 350};
    }
    
    void Update(Vector2 playerPos) {
        coordSystem.Update(playerPos);
        
        // Toggle panels
        if (IsKeyPressed(KEY_F1)) {
            showCoordinatePanel = !showCoordinatePanel;
        }
        if (IsKeyPressed(KEY_F2)) {
            showHelpPanel = !showHelpPanel;
        }
        
        // Toggle display modes
        if (IsKeyPressed(KEY_1)) currentMode = CoordinateMode::SCREEN_SPACE;
        if (IsKeyPressed(KEY_2)) currentMode = CoordinateMode::WORLD_SPACE;
        if (IsKeyPressed(KEY_3)) currentMode = CoordinateMode::GRID_SPACE;
        if (IsKeyPressed(KEY_4)) currentMode = CoordinateMode::TILE_SPACE;
        if (IsKeyPressed(KEY_5)) currentMode = CoordinateMode::CHUNK_SPACE;
        if (IsKeyPressed(KEY_6)) currentMode = CoordinateMode::POLAR_SPACE;
        
        // Toggle grid
        if (IsKeyPressed(KEY_G)) {
            coordSystem.GetGridSettings().showGrid = !coordSystem.GetGridSettings().showGrid;
        }
    }
    
    void Draw() {
        // Draw coordinate system elements
        coordSystem.DrawAll();
        
        // Draw coordinate panel
        if (showCoordinatePanel) {
            DrawCoordinatePanel();
        }
        
        // Draw help panel
        if (showHelpPanel) {
            DrawHelpPanel();
        }
        
        // Draw current mode indicator
        DrawModeIndicator();
    }
    
    void DrawCoordinatePanel() {
        DrawRectangleRec(coordPanelRect, ColorAlpha(BLACK, 0.8f));
        DrawRectangleLinesEx(coordPanelRect, 2, GOLD);
        
        DrawText("COORDINATE INFORMATION", coordPanelRect.x + 10, coordPanelRect.y + 10, 
                16, YELLOW);
        
        Vector2 playerPos = {0, 0}; // This should be updated with actual player position
        Vector2 mousePos = GetMousePosition();
        Vector2 worldMousePos = coordSystem.ScreenToWorld(mousePos);
        
        // Player coordinates
        DrawText("Player:", coordPanelRect.x + 10, coordPanelRect.y + 35, 14, WHITE);
        DrawText(TextFormat("  World: (%.2f, %.2f)", playerPos.x, playerPos.y), 
                coordPanelRect.x + 10, coordPanelRect.y + 55, 12, GREEN);
        
        // Mouse coordinates
        DrawText("Mouse:", coordPanelRect.x + 10, coordPanelRect.y + 80, 14, WHITE);
        DrawText(TextFormat("  Screen: (%.0f, %.0f)", mousePos.x, mousePos.y), 
                coordPanelRect.x + 10, coordPanelRect.y + 100, 12, CYAN);
        DrawText(TextFormat("  World: (%.2f, %.2f)", worldMousePos.x, worldMousePos.y), 
                coordPanelRect.x + 10, coordPanelRect.y + 120, 12, GREEN);
        
        Vector2 gridPos = coordSystem.WorldToGrid(worldMousePos);
        DrawText(TextFormat("  Grid: (%.0f, %.0f)", gridPos.x, gridPos.y), 
                coordPanelRect.x + 10, coordPanelRect.y + 140, 12, ORANGE);
        
        Vector2 tilePos = coordSystem.WorldToTile(worldMousePos);
        DrawText(TextFormat("  Tile: (%.0f, %.0f)", tilePos.x, tilePos.y), 
                coordPanelRect.x + 10, coordPanelRect.y + 160, 12, PURPLE);
        
        // View information
        DrawText("View:", coordPanelRect.x + 10, coordPanelRect.y + 185, 14, WHITE);
        DrawText(TextFormat("  Origin: (%.0f, %.0f)", coordSystem.GetOrigin().x, 
                coordSystem.GetOrigin().y), 
                coordPanelRect.x + 10, coordPanelRect.y + 205, 12, WHITE);
        DrawText(TextFormat("  Zoom: %.2fx", coordSystem.GetZoom()), 
                coordPanelRect.x + 10, coordPanelRect.y + 225, 12, WHITE);
        DrawText(TextFormat("  Rotation: %.1f°", coordSystem.GetRotation()), 
                coordPanelRect.x + 10, coordPanelRect.y + 245, 12, WHITE);
    }
    
    void DrawHelpPanel() {
        DrawRectangleRec(helpPanelRect, ColorAlpha(BLACK, 0.9f));
        DrawRectangleLinesEx(helpPanelRect, 2, WHITE);
        
        DrawText("COORDINATE SYSTEM HELP", helpPanelRect.x + 10, helpPanelRect.y + 10, 
                16, YELLOW);
        
        DrawText("Controls:", helpPanelRect.x + 10, helpPanelRect.y + 40, 14, WHITE);
        DrawText("  Mouse Wheel: Zoom", helpPanelRect.x + 10, helpPanelRect.y + 60, 12, GRAY);
        DrawText("  Middle Mouse: Pan", helpPanelRect.x + 10, helpPanelRect.y + 80, 12, GRAY);
        DrawText("  Q/E: Rotate View", helpPanelRect.x + 10, helpPanelRect.y + 100, 12, GRAY);
        DrawText("  R: Reset View", helpPanelRect.x + 10, helpPanelRect.y + 120, 12, GRAY);
        DrawText("  G: Toggle Grid", helpPanelRect.x + 10, helpPanelRect.y + 140, 12, GRAY);
        
        DrawText("Display Modes:", helpPanelRect.x + 10, helpPanelRect.y + 170, 14, WHITE);
        DrawText("  1: Screen Space", helpPanelRect.x + 10, helpPanelRect.y + 190, 12, GRAY);
        DrawText("  2: World Space", helpPanelRect.x + 10, helpPanelRect.y + 210, 12, GRAY);
        DrawText("  3: Grid Space", helpPanelRect.x + 10, helpPanelRect.y + 230, 12, GRAY);
        DrawText("  4: Tile Space", helpPanelRect.x + 10, helpPanelRect.y + 250, 12, GRAY);
        DrawText("  5: Chunk Space", helpPanelRect.x + 10, helpPanelRect.y + 270, 12, GRAY);
        DrawText("  6: Polar Space", helpPanelRect.x + 10, helpPanelRect.y + 290, 12, GRAY);
        
        DrawText("Panels:", helpPanelRect.x + 10, helpPanelRect.y + 320, 14, WHITE);
        DrawText("  F1: Toggle Coordinate Panel", helpPanelRect.x + 10, helpPanelRect.y + 340, 12, GRAY);
        DrawText("  F2: Toggle Help Panel", helpPanelRect.x + 10, helpPanelRect.y + 360, 12, GRAY);
    }
    
    void DrawModeIndicator() {
        std::string modeText;
        Color modeColor;
        
        switch (currentMode) {
            case CoordinateMode::SCREEN_SPACE:
                modeText = "SCREEN SPACE";
                modeColor = CYAN;
                break;
            case CoordinateMode::WORLD_SPACE:
                modeText = "WORLD SPACE";
                modeColor = GREEN;
                break;
            case CoordinateMode::GRID_SPACE:
                modeText = "GRID SPACE";
                modeColor = ORANGE;
                break;
            case CoordinateMode::TILE_SPACE:
                modeText = "TILE SPACE";
                modeColor = PURPLE;
                break;
            case CoordinateMode::CHUNK_SPACE:
                modeText = "CHUNK SPACE";
                modeColor = PINK;
                break;
            case CoordinateMode::POLAR_SPACE:
                modeText = "POLAR SPACE";
                modeColor = YELLOW;
                break;
        }
        
        DrawText(modeText.c_str(), SCREEN_WIDTH - 200, 20, 20, modeColor);
    }
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "2D Coordinate Display System");
    SetTargetFPS(60);
    
    // Create coordinate display manager
    CoordinateDisplayManager coordManager;
    
    // Player position (this would normally be the actual player position)
    Vector2 playerPos = {0, 0};
    float playerSpeed = 200.0f;
    
    // Game loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Update player position
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) playerPos.x -= playerSpeed * dt;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) playerPos.x += playerSpeed * dt;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) playerPos.y -= playerSpeed * dt;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) playerPos.y += playerSpeed * dt;
        
        // Update coordinate manager
        coordManager.Update(playerPos);
        
        // Draw
        BeginDrawing();
        ClearBackground(Color{30, 30, 50, 255});
        
        // Draw coordinate system
        coordManager.Draw();
        
        // Draw title
        DrawText("2D Coordinate Display System", SCREEN_WIDTH/2 - 150, 10, 25, WHITE);
        
        // Draw FPS
        DrawText(TextFormat("FPS: %d", GetFPS()), SCREEN_WIDTH - 100, 10, 15, GRAY);
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}