#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <memory>
#include <functional>
#include <queue>
#include <algorithm>
#include <random>
#include <sstream>
#include <iostream>

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;

// Forward declarations
class ConversationNode;
class ConversationGraph;
class NPC;
class DialogManager;

// Conversation context - passed between nodes
struct ConversationContext {
    std::map<std::string, int> intVariables;
    std::map<std::string, float> floatVariables;
    std::map<std::string, std::string> stringVariables;
    std::map<std::string, bool> boolVariables;
    std::vector<std::string> flags;
    
    // Player state
    int playerReputation = 0;
    int playerGold = 100;
    int playerLevel = 1;
    
    // Relationship tracking
    std::map<std::string, int> npcRelationships;
    
    void SetFlag(const std::string& flag) {
        if (std::find(flags.begin(), flags.end(), flag) == flags.end()) {
            flags.push_back(flag);
        }
    }
    
    bool HasFlag(const std::string& flag) const {
        return std::find(flags.begin(), flags.end(), flag) != flags.end();
    }
    
    int GetRelationship(const std::string& npcName) const {
        auto it = npcRelationships.find(npcName);
        return it != npcRelationships.end() ? it->second : 0;
    }
    
    void ModifyRelationship(const std::string& npcName, int amount) {
        npcRelationships[npcName] += amount;
    }
};

// Emotion enum for character expressions
enum class Emotion {
    NEUTRAL,
    HAPPY,
    SAD,
    ANGRY,
    SURPRISED,
    THINKING,
    SCARED,
    EXCITED,
    CONFUSED,
    FLIRTATIOUS
};

// Node types
enum class NodeType {
    DIALOG,
    CHOICE,
    CONDITION,
    ACTION,
    EVENT,
    RANDOM,
    END
};

// Base conversation node
class ConversationNode {
protected:
    int id;
    std::string text;
    Emotion emotion;
    NPC* speaker;
    NodeType type;
    std::vector<int> nextNodes;
    std::function<bool(const ConversationContext&)> condition;
    std::function<void(ConversationContext&)> action;
    
public:
    ConversationNode(int nodeId, const std::string& nodeText, NPC* nodeSpeaker, 
                    Emotion nodeEmotion = Emotion::NEUTRAL, NodeType nodeType = NodeType::DIALOG)
        : id(nodeId), text(nodeText), emotion(nodeEmotion), speaker(nodeSpeaker), type(nodeType) {}
    
    virtual ~ConversationNode() = default;
    
    int GetId() const { return id; }
    const std::string& GetText() const { return text; }
    Emotion GetEmotion() const { return emotion; }
    NPC* GetSpeaker() const { return speaker; }
    NodeType GetType() const { return type; }
    const std::vector<int>& GetNextNodes() const { return nextNodes; }
    
    void AddNextNode(int nodeId) {
        nextNodes.push_back(nodeId);
    }
    
    void SetCondition(std::function<bool(const ConversationContext&)> cond) {
        condition = cond;
    }
    
    void SetAction(std::function<void(ConversationContext&)> act) {
        action = act;
    }
    
    virtual bool CanExecute(const ConversationContext& context) const {
        if (condition) {
            return condition(context);
        }
        return true;
    }
    
    virtual void Execute(ConversationContext& context) const {
        if (action) {
            action(context);
        }
    }
    
    virtual int GetNextNode(const ConversationContext& context) const {
        if (nextNodes.empty()) return -1;
        if (nextNodes.size() == 1) return nextNodes[0];
        
        // Default: random selection
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, nextNodes.size() - 1);
        return nextNodes[dis(gen)];
    }
};

// Dialog node - simple text display
class DialogNode : public ConversationNode {
public:
    DialogNode(int nodeId, const std::string& nodeText, NPC* nodeSpeaker, 
               Emotion nodeEmotion = Emotion::NEUTRAL)
        : ConversationNode(nodeId, nodeText, nodeSpeaker, nodeEmotion, NodeType::DIALOG) {}
};

// Choice node - presents options to player
class ChoiceNode : public ConversationNode {
private:
    std::vector<std::string> choices;
    std::vector<int> choiceTargets;
    
public:
    ChoiceNode(int nodeId, const std::string& prompt, NPC* nodeSpeaker,
               Emotion nodeEmotion = Emotion::THINKING)
        : ConversationNode(nodeId, prompt, nodeSpeaker, nodeEmotion, NodeType::CHOICE) {}
    
    void AddChoice(const std::string& choiceText, int targetNode) {
        choices.push_back(choiceText);
        choiceTargets.push_back(targetNode);
    }
    
    const std::vector<std::string>& GetChoices() const { return choices; }
    const std::vector<int>& GetChoiceTargets() const { return choiceTargets; }
    
    int GetChoiceTarget(int choiceIndex) const {
        if (choiceIndex >= 0 && choiceIndex < choiceTargets.size()) {
            return choiceTargets[choiceIndex];
        }
        return -1;
    }
    
    int GetNextNode(const ConversationContext& context) const override {
        // For choice nodes, the next node is determined by player selection
        if (!choiceTargets.empty()) {
            return choiceTargets[0]; // Default to first choice
        }
        return -1;
    }
};

// Condition node - checks conditions and routes accordingly
class ConditionNode : public ConversationNode {
private:
    int trueTarget;
    int falseTarget;
    
public:
    ConditionNode(int nodeId, NPC* nodeSpeaker)
        : ConversationNode(nodeId, "", nodeSpeaker, Emotion::NEUTRAL, NodeType::CONDITION),
          trueTarget(-1), falseTarget(-1) {}
    
    void SetTargets(int trueNode, int falseNode) {
        trueTarget = trueNode;
        falseTarget = falseNode;
    }
    
    int GetNextNode(const ConversationContext& context) const override {
        if (CanExecute(context)) {
            return trueTarget;
        }
        return falseTarget;
    }
};

// Action node - performs actions without displaying text
class ActionNode : public ConversationNode {
public:
    ActionNode(int nodeId, NPC* nodeSpeaker, std::function<void(ConversationContext&)> action)
        : ConversationNode(nodeId, "", nodeSpeaker, Emotion::NEUTRAL, NodeType::ACTION) {
        SetAction(action);
    }
};

// Random node - randomly selects from multiple targets
class RandomNode : public ConversationNode {
private:
    std::vector<int> randomTargets;
    std::vector<float> weights;
    
public:
    RandomNode(int nodeId, NPC* nodeSpeaker)
        : ConversationNode(nodeId, "", nodeSpeaker, Emotion::NEUTRAL, NodeType::RANDOM) {}
    
    void AddTarget(int targetNode, float weight = 1.0f) {
        randomTargets.push_back(targetNode);
        weights.push_back(weight);
    }
    
    int GetNextNode(const ConversationContext& context) const override {
        if (randomTargets.empty()) return -1;
        
        // Weighted random selection
        std::random_device rd;
        std::mt19937 gen(rd());
        std::discrete_distribution<> dis(weights.begin(), weights.end());
        return randomTargets[dis(gen)];
    }
};

// End node - terminates conversation
class EndNode : public ConversationNode {
public:
    EndNode(int nodeId, NPC* nodeSpeaker)
        : ConversationNode(nodeId, "", nodeSpeaker, Emotion::NEUTRAL, NodeType::END) {}
    
    int GetNextNode(const ConversationContext& context) const override {
        return -1;
    }
};

// NPC class
class NPC {
public:
    std::string name;
    Color color;
    Vector2 position;
    std::string role;
    int id;
    
    // Relationship with player
    int relationship;
    
    // Knowledge and memory
    std::vector<std::string> knownTopics;
    std::map<std::string, bool> memories;
    
    NPC(int npcId, const std::string& npcName, Color npcColor, Vector2 pos, const std::string& npcRole)
        : id(npcId), name(npcName), color(npcColor), position(pos), role(npcRole), relationship(0) {}
    
    void LearnTopic(const std::string& topic) {
        if (std::find(knownTopics.begin(), knownTopics.end(), topic) == knownTopics.end()) {
            knownTopics.push_back(topic);
        }
    }
    
    bool KnowsTopic(const std::string& topic) const {
        return std::find(knownTopics.begin(), knownTopics.end(), topic) != knownTopics.end();
    }
    
    void Remember(const std::string& memory, bool value = true) {
        memories[memory] = value;
    }
    
    bool GetMemory(const std::string& memory) const {
        auto it = memories.find(memory);
        return it != memories.end() ? it->second : false;
    }
    
    void Draw() const {
        DrawCircleV(position, 20, color);
        DrawCircleV(position, 5, WHITE);
        DrawText(name.c_str(), position.x - 20, position.y - 40, 12, WHITE);
        DrawText(role.c_str(), position.x - 20, position.y - 25, 10, GRAY);
    }
    
    bool IsNear(Vector2 target, float radius = 50.0f) const {
        return Vector2Distance(position, target) < radius;
    }
};

// Conversation graph
class ConversationGraph {
private:
    std::unordered_map<int, std::shared_ptr<ConversationNode>> nodes;
    int startNodeId;
    std::string graphName;
    
public:
    ConversationGraph(const std::string& name = "") : startNodeId(0), graphName(name) {}
    
    void AddNode(std::shared_ptr<ConversationNode> node) {
        nodes[node->GetId()] = node;
    }
    
    void SetStartNode(int nodeId) {
        startNodeId = nodeId;
    }
    
    int GetStartNode() const { return startNodeId; }
    
    std::shared_ptr<ConversationNode> GetNode(int nodeId) const {
        auto it = nodes.find(nodeId);
        if (it != nodes.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    size_t GetNodeCount() const { return nodes.size(); }
    const std::string& GetName() const { return graphName; }
    
    // Utility functions for building graphs
    void ConnectNodes(int fromId, int toId) {
        auto fromNode = GetNode(fromId);
        if (fromNode) {
            fromNode->AddNextNode(toId);
        }
    }
};

// Dialog manager - manages conversation flow
class DialogManager {
private:
    std::vector<NPC*> npcs;
    std::map<std::string, ConversationGraph*> conversationGraphs;
    ConversationGraph* activeGraph;
    int currentNodeId;
    ConversationContext context;
    
    // UI state
    bool conversationActive;
    float textProgress;
    std::string displayedText;
    std::string currentText;
    float textSpeed;
    bool textComplete;
    
    // Choice handling
    bool showChoices;
    std::vector<std::string> currentChoices;
    std::vector<int> currentChoiceTargets;
    int selectedChoice;
    
    // Callbacks
    std::function<void()> onConversationStart;
    std::function<void()> onConversationEnd;
    std::function<void(NPC*, const std::string&)> onLineDisplayed;
    std::function<void(const std::string&, int)> onChoiceSelected;
    std::function<void(const ConversationContext&)> onContextChanged;
    
public:
    DialogManager() : activeGraph(nullptr), currentNodeId(-1), 
                      conversationActive(false), textProgress(0), textSpeed(30.0f),
                      textComplete(false), showChoices(false), selectedChoice(0) {}
    
    void RegisterNPC(NPC* npc) {
        npcs.push_back(npc);
    }
    
    void RegisterConversationGraph(const std::string& name, ConversationGraph* graph) {
        conversationGraphs[name] = graph;
    }
    
    void StartConversation(const std::string& graphName) {
        auto it = conversationGraphs.find(graphName);
        if (it == conversationGraphs.end()) return;
        
        activeGraph = it->second;
        currentNodeId = activeGraph->GetStartNode();
        conversationActive = true;
        textComplete = false;
        showChoices = false;
        selectedChoice = 0;
        
        if (onConversationStart) onConversationStart();
        
        ProcessCurrentNode();
    }
    
    void ProcessCurrentNode() {
        if (!conversationActive || !activeGraph) return;
        
        auto node = activeGraph->GetNode(currentNodeId);
        if (!node) {
            EndConversation();
            return;
        }
        
        // Execute node action
        node->Execute(context);
        
        // Handle different node types
        switch (node->GetType()) {
            case NodeType::DIALOG:
            case NodeType::CHOICE:
                currentText = node->GetText();
                textProgress = 0;
                displayedText = "";
                textComplete = false;
                
                if (node->GetType() == NodeType::CHOICE) {
                    auto choiceNode = std::dynamic_pointer_cast<ChoiceNode>(node);
                    currentChoices = choiceNode->GetChoices();
                    currentChoiceTargets = choiceNode->GetChoiceTargets();
                    showChoices = false; // Show after text completes
                }
                
                if (onLineDisplayed && node->GetSpeaker()) {
                    onLineDisplayed(node->GetSpeaker(), currentText);
                }
                break;
                
            case NodeType::CONDITION:
            case NodeType::ACTION:
            case NodeType::RANDOM:
                // These nodes are processed immediately
                AdvanceToNextNode();
                break;
                
            case NodeType::END:
                EndConversation();
                break;
        }
        
        if (onContextChanged) onContextChanged(context);
    }
    
    void AdvanceToNextNode() {
        if (!conversationActive || !activeGraph) return;
        
        auto node = activeGraph->GetNode(currentNodeId);
        if (!node) {
            EndConversation();
            return;
        }
        
        int nextNodeId = node->GetNextNode(context);
        if (nextNodeId == -1) {
            EndConversation();
            return;
        }
        
        currentNodeId = nextNodeId;
        ProcessCurrentNode();
    }
    
    void MakeChoice(int choiceIndex) {
        if (!showChoices || choiceIndex < 0 || choiceIndex >= currentChoiceTargets.size()) return;
        
        if (onChoiceSelected) {
            onChoiceSelected(currentChoices[choiceIndex], choiceIndex);
        }
        
        // Apply choice effects
        auto node = activeGraph->GetNode(currentNodeId);
        if (node && node->GetType() == NodeType::CHOICE) {
            auto choiceNode = std::dynamic_pointer_cast<ChoiceNode>(node);
            int targetId = choiceNode->GetChoiceTarget(choiceIndex);
            if (targetId != -1) {
                currentNodeId = targetId;
                showChoices = false;
                ProcessCurrentNode();
            }
        }
    }
    
    void Update(float dt) {
        if (!conversationActive) return;
        
        // Typewriter effect
        if (!textComplete && !currentText.empty()) {
            textProgress += textSpeed * dt;
            if (textProgress >= currentText.length()) {
                textProgress = currentText.length();
                textComplete = true;
                
                // Show choices if this is a choice node
                auto node = activeGraph->GetNode(currentNodeId);
                if (node && node->GetType() == NodeType::CHOICE) {
                    showChoices = true;
                    selectedChoice = 0;
                }
            }
            displayedText = currentText.substr(0, (int)textProgress);
        }
        
        // Handle input
        if (textComplete && !showChoices) {
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                AdvanceToNextNode();
            }
        } else if (showChoices) {
            // Navigate choices
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                selectedChoice = (selectedChoice - 1 + currentChoices.size()) % currentChoices.size();
            }
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                selectedChoice = (selectedChoice + 1) % currentChoices.size();
            }
            
            // Select choice
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                MakeChoice(selectedChoice);
            }
        }
    }
    
    void EndConversation() {
        conversationActive = false;
        activeGraph = nullptr;
        currentNodeId = -1;
        textComplete = false;
        showChoices = false;
        
        if (onConversationEnd) onConversationEnd();
    }
    
    void Draw() {
        if (!conversationActive || !activeGraph) return;
        
        auto node = activeGraph->GetNode(currentNodeId);
        if (!node) return;
        
        // Draw dialog box
        Rectangle dialogBox = {50, SCREEN_HEIGHT - 200, SCREEN_WIDTH - 100, 150};
        DrawRectangleRec(dialogBox, ColorAlpha(BLACK, 0.8f));
        DrawRectangleLinesEx(dialogBox, 2, GOLD);
        
        // Draw speaker name
        if (node->GetSpeaker()) {
            DrawText(node->GetSpeaker()->name.c_str(), 
                    dialogBox.x + 20, dialogBox.y - 30, 20, node->GetSpeaker()->color);
            DrawText(node->GetSpeaker()->role.c_str(), 
                    dialogBox.x + 20 + MeasureText(node->GetSpeaker()->name.c_str(), 20) + 10, 
                    dialogBox.y - 25, 15, GRAY);
        }
        
        // Draw text with word wrap
        float textX = dialogBox.x + 20;
        float textY = dialogBox.y + 10;
        float maxWidth = dialogBox.width - 40;
        std::string remainingText = displayedText;
        std::istringstream words(remainingText);
        std::string word, line;
        
        while (words >> word) {
            std::string testLine = line.empty() ? word : line + " " + word;
            if (MeasureText(testLine.c_str(), 18) > maxWidth) {
                DrawText(line.c_str(), textX, textY, 18, WHITE);
                textY += 22;
                line = word;
            } else {
                line = testLine;
            }
        }
        if (!line.empty()) {
            DrawText(line.c_str(), textX, textY, 18, WHITE);
        }
        
        // Draw choices
        if (showChoices) {
            float choiceY = dialogBox.y - currentChoices.size() * 30 - 10;
            for (size_t i = 0; i < currentChoices.size(); i++) {
                Color textColor = (i == selectedChoice) ? YELLOW : WHITE;
                Color bgColor = (i == selectedChoice) ? ColorAlpha(BLUE, 0.5f) : ColorAlpha(BLACK, 0.7f);
                
                Rectangle choiceBox = {dialogBox.x, choiceY + i * 30, dialogBox.width, 25};
                DrawRectangleRec(choiceBox, bgColor);
                
                std::string prefix = (i == selectedChoice) ? "> " : "  ";
                DrawText((prefix + currentChoices[i]).c_str(), 
                        choiceBox.x + 10, choiceBox.y + 3, 16, textColor);
            }
        }
        
        // Draw continue indicator
        if (textComplete && !showChoices) {
            DrawText("▼", SCREEN_WIDTH - 60, SCREEN_HEIGHT - 40, 20, 
                    ColorAlpha(WHITE, 0.5f + 0.5f * sin(GetTime() * 4)));
        }
    }
    
    bool IsConversationActive() const { return conversationActive; }
    
    // Set callbacks
    void SetOnConversationStart(std::function<void()> callback) { onConversationStart = callback; }
    void SetOnConversationEnd(std::function<void()> callback) { onConversationEnd = callback; }
    void SetOnLineDisplayed(std::function<void(NPC*, const std::string&)> callback) { onLineDisplayed = callback; }
    void SetOnChoiceSelected(std::function<void(const std::string&, int)> callback) { onChoiceSelected = callback; }
    void SetOnContextChanged(std::function<void(const ConversationContext&)> callback) { onContextChanged = callback; }
    
    // Access context
    ConversationContext& GetContext() { return context; }
};

// Conversation builder helper
class ConversationBuilder {
private:
    ConversationGraph* graph;
    int nextNodeId;
    NPC* defaultSpeaker;
    std::map<std::string, int> namedNodes;
    
public:
    ConversationBuilder(ConversationGraph* g, NPC* speaker = nullptr) 
        : graph(g), nextNodeId(0), defaultSpeaker(speaker) {}
    
    int CreateDialogNode(const std::string& text, NPC* speaker = nullptr, 
                        Emotion emotion = Emotion::NEUTRAL) {
        int id = nextNodeId++;
        auto node = std::make_shared<DialogNode>(id, text, speaker ? speaker : defaultSpeaker, emotion);
        graph->AddNode(node);
        return id;
    }
    
    int CreateChoiceNode(const std::string& prompt, NPC* speaker = nullptr,
                        Emotion emotion = Emotion::THINKING) {
        int id = nextNodeId++;
        auto node = std::make_shared<ChoiceNode>(id, prompt, speaker ? speaker : defaultSpeaker, emotion);
        graph->AddNode(node);
        return id;
    }
    
    int CreateConditionNode(std::function<bool(const ConversationContext&)> condition,
                           int trueTarget, int falseTarget, NPC* speaker = nullptr) {
        int id = nextNodeId++;
        auto node = std::make_shared<ConditionNode>(id, speaker ? speaker : defaultSpeaker);
        node->SetCondition(condition);
        node->SetTargets(trueTarget, falseTarget);
        graph->AddNode(node);
        return id;
    }
    
    int CreateActionNode(std::function<void(ConversationContext&)> action, 
                        int nextTarget, NPC* speaker = nullptr) {
        int id = nextNodeId++;
        auto node = std::make_shared<ActionNode>(id, speaker ? speaker : defaultSpeaker, action);
        node->AddNextNode(nextTarget);
        graph->AddNode(node);
        return id;
    }
    
    int CreateRandomNode(std::vector<std::pair<int, float>> targets, NPC* speaker = nullptr) {
        int id = nextNodeId++;
        auto node = std::make_shared<RandomNode>(id, speaker ? speaker : defaultSpeaker);
        for (const auto& target : targets) {
            node->AddTarget(target.first, target.second);
        }
        graph->AddNode(node);
        return id;
    }
    
    int CreateEndNode(NPC* speaker = nullptr) {
        int id = nextNodeId++;
        auto node = std::make_shared<EndNode>(id, speaker ? speaker : defaultSpeaker);
        graph->AddNode(node);
        return id;
    }
    
    void Connect(int fromId, int toId) {
        graph->ConnectNodes(fromId, toId);
    }
    
    void AddChoice(int choiceNodeId, const std::string& choiceText, int targetNode) {
        auto node = std::dynamic_pointer_cast<ChoiceNode>(graph->GetNode(choiceNodeId));
        if (node) {
            node->AddChoice(choiceText, targetNode);
        }
    }
    
    void SetStartNode(int nodeId) {
        graph->SetStartNode(nodeId);
    }
    
    void NameNode(const std::string& name, int nodeId) {
        namedNodes[name] = nodeId;
    }
    
    int GetNodeByName(const std::string& name) const {
        auto it = namedNodes.find(name);
        return it != namedNodes.end() ? it->second : -1;
    }
};

// Example complex conversation setup
void SetupComplexConversation(DialogManager& dialogManager, NPC* merchant, NPC* guard, NPC* questGiver) {
    ConversationGraph* graph = new ConversationGraph("complex_trade");
    ConversationBuilder builder(graph, merchant);
    
    // Start node - merchant greets
    int start = builder.CreateDialogNode(
        "Welcome, traveler! I have many fine goods for sale.", 
        merchant, Emotion::HAPPY);
    builder.NameNode("start", start);
    
    // Choice: what do you want?
    int mainChoice = builder.CreateChoiceNode(
        "What would you like to do?", 
        merchant, Emotion::THINKING);
    builder.Connect(start, mainChoice);
    
    // Trade option
    int tradeStart = builder.CreateDialogNode(
        "Excellent! Let me show you my wares.", 
        merchant, Emotion::EXCITED);
    builder.AddChoice(mainChoice, "I want to trade.", tradeStart);
    
    // Check if player has enough gold
    int goldCheck = builder.CreateConditionNode(
        [](const ConversationContext& ctx) { return ctx.playerGold >= 50; },
        0, 0); // Will be set below
    builder.Connect(tradeStart, goldCheck);
    
    int richPath = builder.CreateDialogNode(
        "You look wealthy! I have special items for you.", 
        merchant, Emotion::HAPPY);
    builder.NameNode("rich_path", richPath);
    
    int poorPath = builder.CreateDialogNode(
        "Come back when you have more gold, friend.", 
        merchant, Emotion::SAD);
    builder.NameNode("poor_path", poorPath);
    
    // Set condition targets
    auto conditionNode = std::dynamic_pointer_cast<ConditionNode>(graph->GetNode(goldCheck));
    if (conditionNode) {
        conditionNode->SetTargets(richPath, poorPath);
    }
    
    // Rich path continues
    int buyChoice = builder.CreateChoiceNode(
        "What would you like to buy?", 
        merchant, Emotion::HAPPY);
    builder.Connect(richPath, buyChoice);
    
    int buySword = builder.CreateDialogNode(
        "A fine choice! That'll be 100 gold.", 
        merchant, Emotion::HAPPY);
    int buyAction = builder.CreateActionNode(
        [](ConversationContext& ctx) { 
            ctx.playerGold -= 100;
            ctx.SetFlag("has_sword");
        },
        0);
    builder.Connect(buyAction, buySword);
    builder.AddChoice(buyChoice, "Buy sword (100 gold)", buyAction);
    
    int buyPotion = builder.CreateDialogNode(
        "Health potion, coming right up! 25 gold.", 
        merchant, Emotion::HAPPY);
    int potionAction = builder.CreateActionNode(
        [](ConversationContext& ctx) { 
            ctx.playerGold -= 25;
            ctx.intVariables["potions"]++;
        },
        0);
    builder.Connect(potionAction, buyPotion);
    builder.AddChoice(buyChoice, "Buy potion (25 gold)", potionAction);
    
    int leaveShop = builder.CreateEndNode(merchant);
    builder.AddChoice(buyChoice, "Nothing, thanks.", leaveShop);
    
    // Quest option
    int questStart = builder.CreateDialogNode(
        "Ah, you want adventure? Talk to the captain over there.", 
        merchant, Emotion::NEUTRAL);
    builder.Connect(questStart, leaveShop);
    builder.AddChoice(mainChoice, "I'm looking for work.", questStart);
    
    // Gossip option
    int gossipStart = builder.CreateDialogNode(
        "Did you hear about the dragon in the mountains?", 
        merchant, Emotion::SURPRISED);
    builder.Connect(gossipStart, leaveShop);
    builder.AddChoice(mainChoice, "Tell me the latest gossip.", gossipStart);
    
    // Leave option
    int leaveEnd = builder.CreateEndNode(merchant);
    builder.AddChoice(mainChoice, "I must be going.", leaveEnd);
    
    builder.SetStartNode(start);
    
    dialogManager.RegisterConversationGraph("merchant_trade", graph);
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "NPC Conversation Framework");
    SetTargetFPS(60);
    
    // Create NPCs
    NPC merchant(0, "Marcus", ORANGE, {400, 300}, "Merchant");
    NPC guard(1, "Roderick", RED, {600, 400}, "Guard");
    NPC questGiver(2, "Eleanor", PURPLE, {800, 300}, "Quest Giver");
    
    // Create dialog manager
    DialogManager dialogManager;
    dialogManager.RegisterNPC(&merchant);
    dialogManager.RegisterNPC(&guard);
    dialogManager.RegisterNPC(&questGiver);
    
    // Setup conversations
    SetupComplexConversation(dialogManager, &merchant, &guard, &questGiver);
    
    // Player position
    Vector2 playerPos = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};
    
    // Set callbacks
    dialogManager.SetOnConversationEnd([]() {
        std::cout << "Conversation ended" << std::endl;
    });
    
    dialogManager.SetOnContextChanged([](const ConversationContext& ctx) {
        // Update UI or game state based on context
    });
    
    // Game loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Update
        // Player movement
        float speed = 200.0f;
        if (IsKeyDown(KEY_A)) playerPos.x -= speed * dt;
        if (IsKeyDown(KEY_D)) playerPos.x += speed * dt;
        if (IsKeyDown(KEY_W)) playerPos.y -= speed * dt;
        if (IsKeyDown(KEY_S)) playerPos.y += speed * dt;
        
        // Interaction check
        if (!dialogManager.IsConversationActive() && IsKeyPressed(KEY_E)) {
            if (merchant.IsNear(playerPos)) {
                dialogManager.StartConversation("merchant_trade");
            }
        }
        
        dialogManager.Update(dt);
        
        // Draw
        BeginDrawing();
        ClearBackground(Color{30, 30, 50, 255});
        
        // Draw title
        DrawText("Conversation Framework Demo", SCREEN_WIDTH/2 - 150, 30, 30, WHITE);
        
        // Draw NPCs
        merchant.Draw();
        guard.Draw();
        questGiver.Draw();
        
        // Draw player
        DrawCircleV(playerPos, 15, BLUE);
        DrawCircleV(playerPos, 5, WHITE);
        
        // Draw dialog
        dialogManager.Draw();
        
        // Draw instructions
        if (!dialogManager.IsConversationActive()) {
            DrawText("Walk to an NPC and press E to interact", 
                    SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT - 50, 20, WHITE);
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}