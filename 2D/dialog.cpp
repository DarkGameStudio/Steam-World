#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <map>
#include <queue>
#include <functional>
#include <memory>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

// Window dimensions
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

// Dialog system types
enum class DialogEffect {
    NONE,
    TYPEWRITER,
    FADE_IN,
    BOUNCE,
    SHAKE
};

enum class Emotion {
    NEUTRAL,
    HAPPY,
    SAD,
    ANGRY,
    SURPRISED,
    THINKING,
    SCARED,
    EXCITED
};

// Dialog character structure
struct DialogCharacter {
    std::string name;
    Color nameColor;
    Texture2D portrait; // Optional portrait texture
    bool hasPortrait;
    Emotion currentEmotion;
    
    DialogCharacter(const std::string& n, Color c) 
        : name(n), nameColor(c), hasPortrait(false), currentEmotion(Emotion::NEUTRAL) {}
};

// Dialog line structure
struct DialogLine {
    DialogCharacter* speaker;
    std::string text;
    Emotion emotion;
    DialogEffect effect;
    float textSpeed; // Characters per second
    std::vector<std::pair<std::string, int>> choices; // Choice text and next dialog index
    std::function<void()> onShow; // Callback when line is shown
    std::function<void()> onComplete; // Callback when line is completed
    std::function<void(int)> onChoice; // Callback when choice is made
    
    DialogLine(DialogCharacter* s, const std::string& t, Emotion e = Emotion::NEUTRAL)
        : speaker(s), text(t), emotion(e), effect(DialogEffect::TYPEWRITER), 
          textSpeed(30.0f) {}
};

// Dialog node for branching dialogs
struct DialogNode {
    DialogLine line;
    std::vector<int> nextNodes;
    std::vector<std::string> choices;
    std::vector<int> choiceTargets;
    bool isEndNode;
    std::function<bool()> condition; // Condition to show this node
    
    DialogNode(const DialogLine& l) : line(l), isEndNode(false), condition(nullptr) {}
};

// Dialog tree structure
class DialogTree {
private:
    std::vector<DialogNode> nodes;
    int startNode;
    
public:
    DialogTree() : startNode(0) {}
    
    void AddNode(const DialogNode& node) {
        nodes.push_back(node);
    }
    
    void SetStartNode(int index) {
        startNode = index;
    }
    
    int GetStartNode() const { return startNode; }
    DialogNode& GetNode(int index) { return nodes[index]; }
    size_t GetNodeCount() const { return nodes.size(); }
    
    void ConnectNodes(int from, int to) {
        if (from < nodes.size() && to < nodes.size()) {
            nodes[from].nextNodes.push_back(to);
        }
    }
    
    void AddChoice(int from, const std::string& choiceText, int target) {
        if (from < nodes.size() && target < nodes.size()) {
            nodes[from].choices.push_back(choiceText);
            nodes[from].choiceTargets.push_back(target);
        }
    }
};

// Dialog UI Manager
class DialogUIManager {
private:
    // Dialog box dimensions
    Rectangle dialogBox;
    Rectangle portraitBox;
    Rectangle nameBox;
    Rectangle continueIndicator;
    
    // Animation
    float dialogAnimation;
    float textProgress;
    float continuePulse;
    bool dialogVisible;
    bool textComplete;
    bool waitingForChoice;
    
    // Typewriter effect
    float currentTextLength;
    float textSpeed;
    std::string currentText;
    std::string displayedText;
    
    // Choice selection
    int selectedChoice;
    std::vector<std::string> currentChoices;
    
    // Font and styling
    Font dialogFont;
    float fontSize;
    Color dialogBackground;
    Color dialogBorder;
    Color textColor;
    
    // Sound effects (optional)
    Sound typeSound;
    Sound selectSound;
    Sound advanceSound;
    bool hasSounds;
    
    // Skip functionality
    bool skipEnabled;
    bool skipping;
    
public:
    DialogUIManager() 
        : dialogVisible(false), textComplete(false), waitingForChoice(false),
          currentTextLength(0), textSpeed(30.0f), fontSize(20),
          selectedChoice(0), skipEnabled(true), skipping(false) {
        
        // Initialize dialog box
        dialogBox = {50, SCREEN_HEIGHT - 200, SCREEN_WIDTH - 100, 150};
        portraitBox = {70, SCREEN_HEIGHT - 280, 100, 100};
        nameBox = {70, SCREEN_HEIGHT - 290, 200, 40};
        continueIndicator = {SCREEN_WIDTH - 80, SCREEN_HEIGHT - 60, 20, 20};
        
        // Colors
        dialogBackground = Color{20, 20, 40, 230};
        dialogBorder = Color{100, 100, 180, 255};
        textColor = WHITE;
        
        // Animation
        dialogAnimation = 0;
        textProgress = 0;
        continuePulse = 0;
        
        // Load font (you can use default font)
        dialogFont = GetFontDefault();
        
        // Initialize sounds as not loaded
        hasSounds = false;
    }
    
    void LoadSounds() {
        // Load sound files (commented out as they may not exist)
        // typeSound = LoadSound("sounds/type.wav");
        // selectSound = LoadSound("sounds/select.wav");
        // advanceSound = LoadSound("sounds/advance.wav");
        // hasSounds = true;
    }
    
    void ShowDialog(const DialogLine& line) {
        dialogVisible = true;
        textComplete = false;
        waitingForChoice = false;
        currentText = line.text;
        displayedText = "";
        currentTextLength = 0;
        textSpeed = line.textSpeed;
        selectedChoice = 0;
        
        // Apply dialog effect
        switch (line.effect) {
            case DialogEffect::TYPEWRITER:
                dialogAnimation = 0;
                break;
            case DialogEffect::FADE_IN:
                dialogAnimation = 0;
                break;
            case DialogEffect::BOUNCE:
                dialogAnimation = 0;
                break;
            case DialogEffect::SHAKE:
                dialogAnimation = 0;
                break;
            default:
                dialogAnimation = 1;
                break;
        }
        
        // Set choices if any
        currentChoices = line.choices.empty() ? std::vector<std::string>() : 
                       std::vector<std::string>(line.choices.size());
        for (size_t i = 0; i < line.choices.size(); i++) {
            currentChoices[i] = line.choices[i].first;
        }
        
        if (!currentChoices.empty()) {
            waitingForChoice = true;
        }
        
        // Call onShow callback
        if (line.onShow) {
            line.onShow();
        }
    }
    
    void HideDialog() {
        dialogVisible = false;
        textComplete = false;
        waitingForChoice = false;
        currentChoices.clear();
    }
    
    void Update(float dt) {
        if (!dialogVisible) return;
        
        // Update animations
        dialogAnimation = std::min(dialogAnimation + dt * 3, 1.0f);
        continuePulse += dt * 4;
        
        // Typewriter effect
        if (!textComplete && !waitingForChoice) {
            if (skipping || IsKeyDown(KEY_SPACE)) {
                // Skip to complete text
                currentTextLength = currentText.length();
                displayedText = currentText;
                textComplete = true;
            } else {
                // Normal typewriter speed
                currentTextLength += textSpeed * dt;
                if (currentTextLength >= currentText.length()) {
                    currentTextLength = currentText.length();
                    textComplete = true;
                }
                displayedText = currentText.substr(0, (int)currentTextLength);
            }
        }
        
        // Choice navigation
        if (waitingForChoice && !currentChoices.empty()) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                selectedChoice = (selectedChoice - 1 + currentChoices.size()) % currentChoices.size();
                if (hasSounds) PlaySound(selectSound);
            }
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                selectedChoice = (selectedChoice + 1) % currentChoices.size();
                if (hasSounds) PlaySound(selectSound);
            }
        }
    }
    
    void Draw(const DialogCharacter* speaker = nullptr, Emotion emotion = Emotion::NEUTRAL) {
        if (!dialogVisible) return;
        
        // Apply dialog effect to animation
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float offsetX = 0;
        float offsetY = 0;
        
        // Dialog box animation
        if (dialogAnimation < 1) {
            scaleY = dialogAnimation;
            scaleX = 0.5f + dialogAnimation * 0.5f;
        }
        
        // Draw dialog box background with animation
        Rectangle animDialogBox = dialogBox;
        animDialogBox.height *= scaleY;
        animDialogBox.width *= scaleX;
        animDialogBox.x += (dialogBox.width - animDialogBox.width) / 2;
        animDialogBox.y += (dialogBox.height - animDialogBox.height);
        
        DrawRectangleRec(animDialogBox, dialogBackground);
        DrawRectangleLinesEx(animDialogBox, 2, dialogBorder);
        
        // Draw portrait frame
        if (speaker && speaker->hasPortrait) {
            DrawRectangleRec(portraitBox, ColorAlpha(BLACK, 0.8f));
            DrawRectangleLinesEx(portraitBox, 2, dialogBorder);
            DrawTexturePro(speaker->portrait, 
                          {0, 0, (float)speaker->portrait.width, (float)speaker->portrait.height},
                          portraitBox, {0, 0}, 0, WHITE);
        }
        
        // Draw speaker name
        if (speaker) {
            DrawRectangleRec(nameBox, ColorAlpha(BLACK, 0.9f));
            DrawRectangleLinesEx(nameBox, 1, speaker->nameColor);
            
            Color nameColor = speaker->nameColor;
            switch (emotion) {
                case Emotion::ANGRY:
                    nameColor = RED;
                    break;
                case Emotion::SAD:
                    nameColor = BLUE;
                    break;
                case Emotion::HAPPY:
                    nameColor = YELLOW;
                    break;
                case Emotion::SURPRISED:
                    nameColor = ORANGE;
                    break;
                default:
                    break;
            }
            
            DrawTextEx(dialogFont, speaker->name.c_str(), 
                      {nameBox.x + 10, nameBox.y + 8}, fontSize, 1, nameColor);
        }
        
        // Draw dialog text with word wrap
        float textX = dialogBox.x + 20;
        float textY = dialogBox.y + 10;
        float maxTextWidth = dialogBox.width - 40;
        
        // Simple word wrap
        std::string remainingText = displayedText;
        std::string currentLine;
        float lineHeight = fontSize + 5;
        
        std::istringstream words(remainingText);
        std::string word;
        
        while (words >> word) {
            std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
            float testWidth = MeasureTextEx(dialogFont, testLine.c_str(), fontSize, 1).x;
            
            if (testWidth > maxTextWidth) {
                // Draw current line
                DrawTextEx(dialogFont, currentLine.c_str(), {textX, textY}, fontSize, 1, textColor);
                textY += lineHeight;
                currentLine = word;
            } else {
                currentLine = testLine;
            }
        }
        
        // Draw last line
        if (!currentLine.empty()) {
            DrawTextEx(dialogFont, currentLine.c_str(), {textX, textY}, fontSize, 1, textColor);
        }
        
        // Draw choices
        if (waitingForChoice && !currentChoices.empty()) {
            float choiceY = dialogBox.y - currentChoices.size() * 30 - 10;
            
            for (size_t i = 0; i < currentChoices.size(); i++) {
                Color choiceColor = (i == selectedChoice) ? YELLOW : WHITE;
                Color bgColor = (i == selectedChoice) ? ColorAlpha(BLUE, 0.5f) : ColorAlpha(BLACK, 0.7f);
                
                Rectangle choiceBox = {dialogBox.x, choiceY + i * 30, 
                                      dialogBox.width, 25};
                DrawRectangleRec(choiceBox, bgColor);
                
                if (i == selectedChoice) {
                    DrawTextEx(dialogFont, ">", 
                              {choiceBox.x + 5, choiceBox.y + 2}, fontSize, 1, choiceColor);
                }
                
                DrawTextEx(dialogFont, currentChoices[i].c_str(), 
                          {choiceBox.x + 25, choiceBox.y + 2}, fontSize, 1, choiceColor);
            }
        }
        
        // Draw continue indicator
        if (textComplete && !waitingForChoice) {
            float pulse = (sin(continuePulse) + 1) / 2;
            DrawTriangle(
                {continueIndicator.x, continueIndicator.y},
                {continueIndicator.x + 15, continueIndicator.y + 7},
                {continueIndicator.x, continueIndicator.y + 15},
                ColorAlpha(WHITE, 0.5f + pulse * 0.5f)
            );
        }
    }
    
    bool IsDialogVisible() const { return dialogVisible; }
    bool IsTextComplete() const { return textComplete; }
    bool IsWaitingForChoice() const { return waitingForChoice; }
    int GetSelectedChoice() const { return selectedChoice; }
    
    void SetTextSpeed(float speed) { textSpeed = speed; }
    void SetSkipEnabled(bool enabled) { skipEnabled = enabled; }
    void SetSkipping(bool skip) { skipping = skip; }
};

// Dialog Manager - manages dialog flow
class DialogManager {
private:
    DialogUIManager uiManager;
    std::map<std::string, DialogCharacter*> characters;
    DialogTree* currentTree;
    int currentNodeIndex;
    bool dialogActive;
    
    // Dialog state
    std::function<void()> onDialogEnd;
    std::queue<std::pair<DialogCharacter*, std::string>> dialogQueue;
    
public:
    DialogManager() : currentTree(nullptr), currentNodeIndex(0), dialogActive(false) {}
    
    void RegisterCharacter(DialogCharacter* character) {
        characters[character->name] = character;
    }
    
    void StartDialog(DialogTree* tree, std::function<void()> onEnd = nullptr) {
        currentTree = tree;
        currentNodeIndex = tree->GetStartNode();
        dialogActive = true;
        onDialogEnd = onEnd;
        
        ShowCurrentNode();
    }
    
    void ShowCurrentNode() {
        if (!currentTree || !dialogActive) return;
        
        DialogNode& node = currentTree->GetNode(currentNodeIndex);
        
        // Check condition
        if (node.condition && !node.condition()) {
            AdvanceDialog();
            return;
        }
        
        uiManager.ShowDialog(node.line);
    }
    
    void Update(float dt) {
        if (!dialogActive) return;
        
        uiManager.Update(dt);
        
        // Handle input
        if (uiManager.IsWaitingForChoice()) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                MakeChoice(uiManager.GetSelectedChoice());
            }
        } else if (uiManager.IsTextComplete()) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                // Call onComplete callback
                DialogNode& node = currentTree->GetNode(currentNodeIndex);
                if (node.line.onComplete) {
                    node.line.onComplete();
                }
                
                AdvanceDialog();
            }
        }
    }
    
    void MakeChoice(int choiceIndex) {
        if (!currentTree || !dialogActive) return;
        
        DialogNode& node = currentTree->GetNode(currentNodeIndex);
        
        if (choiceIndex < node.choiceTargets.size()) {
            // Call onChoice callback
            if (node.line.onChoice) {
                node.line.onChoice(choiceIndex);
            }
            
            currentNodeIndex = node.choiceTargets[choiceIndex];
            ShowCurrentNode();
        }
    }
    
    void AdvanceDialog() {
        if (!currentTree || !dialogActive) return;
        
        DialogNode& node = currentTree->GetNode(currentNodeIndex);
        
        if (node.isEndNode || node.nextNodes.empty()) {
            EndDialog();
            return;
        }
        
        // Choose next node
        if (node.nextNodes.size() == 1) {
            currentNodeIndex = node.nextNodes[0];
        } else {
            // Random or conditional selection
            std::vector<int> validNodes;
            for (int nextNode : node.nextNodes) {
                if (!currentTree->GetNode(nextNode).condition || 
                    currentTree->GetNode(nextNode).condition()) {
                    validNodes.push_back(nextNode);
                }
            }
            
            if (validNodes.empty()) {
                EndDialog();
                return;
            }
            
            int randomIndex = GetRandomValue(0, validNodes.size() - 1);
            currentNodeIndex = validNodes[randomIndex];
        }
        
        ShowCurrentNode();
    }
    
    void EndDialog() {
        dialogActive = false;
        uiManager.HideDialog();
        
        if (onDialogEnd) {
            onDialogEnd();
        }
    }
    
    void Draw() {
        if (!dialogActive) return;
        
        DialogNode& node = currentTree->GetNode(currentNodeIndex);
        uiManager.Draw(node.line.speaker, node.line.emotion);
    }
    
    bool IsDialogActive() const { return dialogActive; }
    
    void QueueDialog(DialogCharacter* speaker, const std::string& text) {
        dialogQueue.push({speaker, text});
    }
    
    void ProcessDialogQueue() {
        if (!dialogActive && !dialogQueue.empty()) {
            auto [speaker, text] = dialogQueue.front();
            dialogQueue.pop();
            
            // Create simple one-line dialog
            DialogTree* tree = new DialogTree();
            DialogNode node(DialogLine(speaker, text));
            node.isEndNode = true;
            tree->AddNode(node);
            tree->SetStartNode(0);
            
            StartDialog(tree, [tree]() { delete tree; });
        }
    }
};

// Example game character
class GameCharacter {
public:
    Vector2 position;
    Color color;
    std::string name;
    bool canInteract;
    float interactionRadius;
    
    GameCharacter(Vector2 pos, Color c, std::string n) 
        : position(pos), color(c), name(n), canInteract(true), interactionRadius(50) {}
    
    void Draw() {
        DrawCircleV(position, 20, color);
        DrawCircleV(position, 5, WHITE);
        
        // Draw name
        DrawText(name.c_str(), position.x - 20, position.y - 40, 12, WHITE);
        
        // Draw interaction indicator
        if (canInteract) {
            DrawCircleLines(position.x, position.y, interactionRadius, ColorAlpha(YELLOW, 0.3f));
            DrawText("!", position.x + 25, position.y - 10, 20, YELLOW);
        }
    }
    
    bool IsPlayerNear(Vector2 playerPos) {
        return Vector2Distance(position, playerPos) < interactionRadius;
    }
};

// Example player
class Player {
public:
    Vector2 position;
    float speed;
    Color color;
    
    Player() : speed(200), color(BLUE) {
        position = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};
    }
    
    void Update(float dt) {
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) position.x -= speed * dt;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) position.x += speed * dt;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) position.y -= speed * dt;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) position.y += speed * dt;
    }
    
    void Draw() {
        DrawCircleV(position, 15, color);
        DrawCircleV(position, 5, WHITE);
    }
};

// Example dialog creation
DialogTree* CreateExampleDialog(DialogCharacter* npc1, DialogCharacter* npc2) {
    DialogTree* tree = new DialogTree();
    
    // Node 0: Greeting with choices
    DialogNode node0(DialogLine(npc1, "Hello, traveler! Would you like to hear about our village?", Emotion::HAPPY));
    tree->AddNode(node0);
    
    // Node 1: Yes response
    DialogNode node1(DialogLine(npc1, "Wonderful! Our village has a rich history...", Emotion::EXCITED));
    tree->AddNode(node1);
    
    // Node 2: No response
    DialogNode node2(DialogLine(npc1, "Oh, that's too bad. Maybe another time.", Emotion::SAD));
    node2.isEndNode = true;
    tree->AddNode(node2);
    
    // Node 3: Continue story
    DialogNode node3(DialogLine(npc1, "We were founded 300 years ago by great explorers.", Emotion::NEUTRAL));
    tree->AddNode(node3);
    
    // Node 4: Another character joins
    DialogNode node4(DialogLine(npc2, "Ah, telling stories again? Let me tell you about the dragon!", Emotion::SURPRISED));
    tree->AddNode(node4);
    
    // Node 5: Dragon story
    DialogNode node5(DialogLine(npc2, "A mighty dragon once lived in the mountains nearby...", Emotion::SCARED));
    tree->AddNode(node5);
    
    // Node 6: Choice point
    DialogNode node6(DialogLine(npc1, "Would you like to help us with our dragon problem?", Emotion::THINKING));
    tree->AddNode(node6);
    
    // Node 7: Accept quest
    DialogNode node7(DialogLine(npc1, "Thank you, brave hero! The dragon is in the northern mountains.", Emotion::HAPPY));
    node7.isEndNode = true;
    tree->AddNode(node7);
    
    // Node 8: Decline quest
    DialogNode node8(DialogLine(npc1, "I understand. It's a dangerous task.", Emotion::SAD));
    node8.isEndNode = true;
    tree->AddNode(node8);
    
    // Connect nodes
    tree->SetStartNode(0);
    
    // Add choices to node 0
    tree->AddChoice(0, "Yes, tell me more!", 1);
    tree->AddChoice(0, "No, I'm in a hurry.", 2);
    
    // Connect story nodes
    tree->ConnectNodes(1, 3);
    tree->ConnectNodes(3, 4);
    tree->ConnectNodes(4, 5);
    tree->ConnectNodes(5, 6);
    
    // Add choices to node 6
    tree->AddChoice(6, "I'll help you!", 7);
    tree->AddChoice(6, "I'm not ready for this.", 8);
    
    return tree;
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "2D Dialog System");
    SetTargetFPS(60);
    
    // Create dialog characters
    DialogCharacter villager("Elder Thomas", GREEN);
    DialogCharacter warrior("Captain Sarah", RED);
    DialogCharacter merchant("Merchant Bob", ORANGE);
    
    // Create dialog manager
    DialogManager dialogManager;
    dialogManager.RegisterCharacter(&villager);
    dialogManager.RegisterCharacter(&warrior);
    dialogManager.RegisterCharacter(&merchant);
    
    // Create game characters
    GameCharacter npc1({SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2}, GREEN, "Elder Thomas");
    GameCharacter npc2({SCREEN_WIDTH/2 + 200, SCREEN_HEIGHT/2}, RED, "Captain Sarah");
    GameCharacter npc3({SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 200}, ORANGE, "Merchant Bob");
    
    // Create player
    Player player;
    player.position = {SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 150};
    
    // Create dialog tree
    DialogTree* dialogTree = CreateExampleDialog(&villager, &warrior);
    
    // Game loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Update
        player.Update(dt);
        dialogManager.Update(dt);
        
        // Check for interactions
        if (!dialogManager.IsDialogActive()) {
            if (IsKeyPressed(KEY_E)) {
                if (npc1.IsPlayerNear(player.position)) {
                    dialogManager.StartDialog(dialogTree);
                } else if (npc2.IsPlayerNear(player.position)) {
                    dialogManager.QueueDialog(&warrior, "Ready for training, soldier?");
                } else if (npc3.IsPlayerNear(player.position)) {
                    dialogManager.QueueDialog(&merchant, "Fresh goods! Get your fresh goods here!");
                }
            }
        }
        
        // Draw
        BeginDrawing();
        ClearBackground(Color{30, 30, 50, 255});
        
        // Draw world
        DrawText("Dialog System Demo", SCREEN_WIDTH/2 - 100, 50, 30, WHITE);
        
        // Draw NPCs
        npc1.Draw();
        npc2.Draw();
        npc3.Draw();
        
        // Draw player
        player.Draw();
        
        // Draw dialog
        dialogManager.Draw();
        
        // Draw UI hints
        if (!dialogManager.IsDialogActive()) {
            DrawText("Press E to interact with NPCs", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT - 50, 20, WHITE);
        } else {
            DrawText("Press ENTER/SPACE to advance dialog", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT - 50, 20, YELLOW);
        }
        
        EndDrawing();
    }
    
    delete dialogTree;
    CloseWindow();
    return 0;
}