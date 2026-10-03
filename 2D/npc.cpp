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

// Message types for NPC communication
enum class MessageType {
    GREETING,
    QUESTION,
    ANSWER,
    STATEMENT,
    GOSSIP,
    WARNING,
    REQUEST,
    RESPONSE,
    FAREWELL,
    EMOTION
};

// Emotion states
enum class Emotion {
    NEUTRAL,
    HAPPY,
    SAD,
    ANGRY,
    SURPRISED,
    THINKING,
    SCARED,
    EXCITED,
    CURIOUS,
    BORED
};

// Message structure
struct Message {
    int senderId;
    int receiverId;
    MessageType type;
    std::string content;
    Emotion emotion;
    float timestamp;
    float priority;
    bool isRead;
    bool isResponse;
    
    Message(int from, int to, MessageType t, const std::string& msg, 
            Emotion e = Emotion::NEUTRAL, float p = 1.0f)
        : senderId(from), receiverId(to), type(t), content(msg), 
          emotion(e), timestamp(GetTime()), priority(p), isRead(false), 
          isResponse(false) {}
};

// NPC personality traits
struct Personality {
    float sociability;     // How likely to start conversations
    float friendliness;    // How positive in interactions
    float curiosity;       // How likely to ask questions
    float talkativeness;   // How long their responses are
    float aggressiveness;  // How likely to argue/fight
    float honesty;         // How truthful they are
    float humor;           // How funny they try to be
    float empathy;         // How well they understand others
    
    Personality() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.3f, 1.0f);
        
        sociability = dist(gen);
        friendliness = dist(gen);
        curiosity = dist(gen);
        talkativeness = dist(gen);
        aggressiveness = dist(gen);
        honesty = dist(gen);
        humor = dist(gen);
        empathy = dist(gen);
    }
};

// Relationship between NPCs
struct Relationship {
    int familiarity;    // How well they know each other (0-100)
    int affection;      // How much they like each other (-100 to 100)
    int trust;          // How much they trust each other (0-100)
    int conversations;  // Number of conversations had
    float lastInteraction;
    
    Relationship() : familiarity(0), affection(0), trust(0), 
                    conversations(0), lastInteraction(0) {}
};

// NPC class
class NPC {
public:
    int id;
    std::string name;
    std::string role;
    Vector2 position;
    Color color;
    Personality personality;
    Emotion currentEmotion;
    
    // Communication
    std::queue<Message> inbox;
    std::vector<Message> sentMessages;
    std::vector<Message> receivedMessages;
    
    // Relationships with other NPCs
    std::map<int, Relationship> relationships;
    
    // Knowledge and memories
    std::vector<std::string> knowledge;
    std::map<std::string, bool> beliefs;
    std::vector<std::string> rumors;
    
    // State
    bool isBusy;
    bool isInConversation;
    int conversationPartner;
    float conversationTimer;
    float idleTimer;
    float moveTimer;
    Vector2 targetPosition;
    
    // Conversation topics
    std::vector<std::string> favoriteTopics;
    std::vector<std::string> currentTopics;
    
    NPC(int npcId, const std::string& npcName, const std::string& npcRole, 
        Vector2 pos, Color npcColor)
        : id(npcId), name(npcName), role(npcRole), position(pos), 
          color(npcColor), currentEmotion(Emotion::NEUTRAL),
          isBusy(false), isInConversation(false), conversationPartner(-1),
          conversationTimer(0), idleTimer(0), moveTimer(0) {
        
        targetPosition = position;
        
        // Initialize favorite topics based on role
        if (role == "Merchant") {
            favoriteTopics = {"trade", "prices", "goods", "wealth"};
            knowledge.push_back("Knows current market prices");
            knowledge.push_back("Has connections with suppliers");
        } else if (role == "Guard") {
            favoriteTopics = {"security", "threats", "patrol", "weapons"};
            knowledge.push_back("Knows town defenses");
            knowledge.push_back("Trained in combat");
        } else if (role == "Scholar") {
            favoriteTopics = {"history", "magic", "science", "books"};
            knowledge.push_back("Extensive historical knowledge");
            knowledge.push_back("Can read ancient texts");
        } else if (role == "Farmer") {
            favoriteTopics = {"crops", "weather", "animals", "harvest"};
            knowledge.push_back("Knows farming techniques");
            knowledge.push_back("Can predict weather");
        } else if (role == "Healer") {
            favoriteTopics = {"medicine", "herbs", "healing", "health"};
            knowledge.push_back("Knows medicinal herbs");
            knowledge.push_back("Can treat various ailments");
        }
    }
    
    void Update(float dt) {
        // Update timers
        if (isInConversation) {
            conversationTimer -= dt;
            if (conversationTimer <= 0) {
                EndConversation();
            }
        } else {
            idleTimer += dt;
            
            // Random movement
            moveTimer -= dt;
            if (moveTimer <= 0) {
                moveTimer = GetRandomValue(2, 5);
                if (GetRandomValue(0, 100) < 30) { // 30% chance to move
                    targetPosition = {
                        position.x + GetRandomValue(-100, 100),
                        position.y + GetRandomValue(-100, 100)
                    };
                }
            }
            
            // Move towards target
            if (Vector2Distance(position, targetPosition) > 5) {
                Vector2 direction = Vector2Subtract(targetPosition, position);
                direction = Vector2Normalize(direction);
                float speed = 50.0f;
                position = Vector2Add(position, Vector2Scale(direction, speed * dt));
            }
        }
        
        // Process messages
        ProcessInbox();
    }
    
    void ProcessInbox() {
        if (inbox.empty() || isBusy) return;
        
        Message msg = inbox.front();
        inbox.pop();
        msg.isRead = true;
        receivedMessages.push_back(msg);
        
        // Update relationship based on message
        UpdateRelationship(msg.senderId, msg);
        
        // Handle message based on type
        switch (msg.type) {
            case MessageType::GREETING:
                HandleGreeting(msg);
                break;
            case MessageType::QUESTION:
                HandleQuestion(msg);
                break;
            case MessageType::STATEMENT:
                HandleStatement(msg);
                break;
            case MessageType::GOSSIP:
                HandleGossip(msg);
                break;
            case MessageType::WARNING:
                HandleWarning(msg);
                break;
            case MessageType::REQUEST:
                HandleRequest(msg);
                break;
            case MessageType::FAREWELL:
                HandleFarewell(msg);
                break;
            default:
                break;
        }
        
        // Update emotion based on message
        UpdateEmotion(msg);
    }
    
    void HandleGreeting(const Message& msg) {
        if (!isInConversation) {
            // Start a conversation
            isInConversation = true;
            conversationPartner = msg.senderId;
            conversationTimer = GetRandomValue(3, 8);
            
            // Respond with greeting
            std::string response = GenerateGreeting();
            SendMessage(msg.senderId, MessageType::GREETING, response, 
                       personality.friendliness > 0.6f ? Emotion::HAPPY : Emotion::NEUTRAL);
        }
    }
    
    void HandleQuestion(const Message& msg) {
        // Generate answer based on knowledge
        std::string answer = GenerateAnswer(msg.content);
        SendMessage(msg.senderId, MessageType::ANSWER, answer, Emotion::THINKING);
        
        // Maybe ask a question back
        if (personality.curiosity > 0.5f && GetRandomValue(0, 100) < 40) {
            std::string question = GenerateQuestion();
            SendMessage(msg.senderId, MessageType::QUESTION, question, Emotion::CURIOUS);
        }
    }
    
    void HandleStatement(const Message& msg) {
        // React to statement
        if (msg.emotion == Emotion::ANGRY && personality.aggressiveness > 0.6f) {
            // Argue back
            SendMessage(msg.senderId, MessageType::STATEMENT, 
                       "I disagree with you!", Emotion::ANGRY);
        } else {
            // Acknowledge
            if (personality.friendliness > 0.5f) {
                SendMessage(msg.senderId, MessageType::RESPONSE, 
                           "Interesting point.", Emotion::NEUTRAL);
            }
        }
    }
    
    void HandleGossip(const Message& msg) {
        // Store rumor
        if (std::find(rumors.begin(), rumors.end(), msg.content) == rumors.end()) {
            rumors.push_back(msg.content);
        }
        
        // Spread gossip if sociable
        if (personality.sociability > 0.6f && GetRandomValue(0, 100) < 30) {
            // Will spread to others later
            currentTopics.push_back(msg.content);
        }
        
        // React to gossip
        if (personality.curiosity > 0.5f) {
            SendMessage(msg.senderId, MessageType::QUESTION, 
                       "Really? Tell me more!", Emotion::SURPRISED);
        }
    }
    
    void HandleWarning(const Message& msg) {
        // Take warning seriously based on trust
        Relationship& rel = relationships[msg.senderId];
        if (rel.trust > 50) {
            currentEmotion = Emotion::SCARED;
            SendMessage(msg.senderId, MessageType::RESPONSE, 
                       "Thank you for warning me!", Emotion::SCARED);
        } else {
            SendMessage(msg.senderId, MessageType::RESPONSE, 
                       "I'll be careful.", Emotion::NEUTRAL);
        }
    }
    
    void HandleRequest(const Message& msg) {
        // Decide whether to help based on relationship
        Relationship& rel = relationships[msg.senderId];
        if (rel.affection > 20 || personality.friendliness > 0.7f) {
            SendMessage(msg.senderId, MessageType::RESPONSE, 
                       "Of course, I'll help you!", Emotion::HAPPY);
            isBusy = true;
        } else {
            SendMessage(msg.senderId, MessageType::RESPONSE, 
                       "Sorry, I'm busy right now.", Emotion::NEUTRAL);
        }
    }
    
    void HandleFarewell(const Message& msg) {
        if (isInConversation && conversationPartner == msg.senderId) {
            EndConversation();
            SendMessage(msg.senderId, MessageType::FAREWELL, 
                       "Goodbye!", Emotion::NEUTRAL);
        }
    }
    
    void UpdateRelationship(int otherId, const Message& msg) {
        Relationship& rel = relationships[otherId];
        
        rel.conversations++;
        rel.lastInteraction = GetTime();
        rel.familiarity = std::min(100, rel.familiarity + 5);
        
        // Update affection based on message content and emotion
        switch (msg.emotion) {
            case Emotion::HAPPY:
                rel.affection = std::max(-100, std::min(100, rel.affection + 3));
                break;
            case Emotion::ANGRY:
                rel.affection = std::max(-100, std::min(100, rel.affection - 5));
                break;
            case Emotion::SAD:
                rel.affection = std::max(-100, std::min(100, rel.affection + 1));
                break;
            case Emotion::SCARED:
                rel.trust = std::max(0, std::min(100, rel.trust - 2));
                break;
            case Emotion::EXCITED:
                rel.affection = std::max(-100, std::min(100, rel.affection + 4));
                break;
            default:
                break;
        }
        
        // Update trust
        if (personality.honesty > 0.7f) {
            rel.trust = std::max(0, std::min(100, rel.trust + 2));
        }
    }
    
    void UpdateEmotion(const Message& msg) {
        // Update current emotion based on received message
        if (msg.priority > 0.8f) {
            currentEmotion = msg.emotion;
        } else {
            // Gradual return to neutral
            if (GetRandomValue(0, 100) < 5) {
                currentEmotion = Emotion::NEUTRAL;
            }
        }
    }
    
    void SendMessage(int receiverId, MessageType type, const std::string& content, 
                    Emotion emotion = Emotion::NEUTRAL) {
        Message msg(id, receiverId, type, content, emotion);
        sentMessages.push_back(msg);
        
        // The actual delivery will be handled by the communication manager
        // For now, we'll just store it
    }
    
    std::string GenerateGreeting() {
        std::vector<std::string> greetings = {
            "Hello there!",
            "Good day to you!",
            "Nice to see you!",
            "How are you today?",
            "Greetings!"
        };
        
        if (personality.friendliness > 0.7f) {
            greetings.push_back("Wonderful to see you, friend!");
            greetings.push_back("What a pleasant surprise!");
        }
        
        if (personality.humor > 0.7f) {
            greetings.push_back("Well, look who it is!");
            greetings.push_back("Did you miss me?");
        }
        
        return greetings[GetRandomValue(0, greetings.size() - 1)];
    }
    
    std::string GenerateQuestion() {
        std::vector<std::string> questions = {
            "What do you think about that?",
            "Have you heard any news?",
            "How has your day been?",
            "What are you working on?",
            "Did you see anything interesting?"
        };
        
        // Add topic-specific questions
        for (const auto& topic : favoriteTopics) {
            questions.push_back("What do you know about " + topic + "?");
        }
        
        return questions[GetRandomValue(0, questions.size() - 1)];
    }
    
    std::string GenerateAnswer(const std::string& question) {
        // Simple keyword matching
        std::string lowercaseQuestion = question;
        std::transform(lowercaseQuestion.begin(), lowercaseQuestion.end(), 
                      lowercaseQuestion.begin(), ::tolower);
        
        for (const auto& topic : favoriteTopics) {
            if (lowercaseQuestion.find(topic) != std::string::npos) {
                // Generate topic-specific answer
                if (topic == "trade" || topic == "prices" || topic == "goods") {
                    return "The market is doing well. Prices are stable.";
                } else if (topic == "security" || topic == "threats") {
                    return "The town is safe. I've been patrolling regularly.";
                } else if (topic == "history" || topic == "books") {
                    return "Ah, I've studied that extensively. Fascinating subject!";
                } else if (topic == "crops" || topic == "harvest") {
                    return "The crops are growing well this season.";
                } else if (topic == "medicine" || topic == "healing") {
                    return "I have several remedies for that.";
                }
            }
        }
        
        // Generic answers
        std::vector<std::string> genericAnswers = {
            "That's an interesting question.",
            "I'm not entirely sure about that.",
            "Let me think about it...",
            "I don't know much about that.",
            "That's a good point."
        };
        
        return genericAnswers[GetRandomValue(0, genericAnswers.size() - 1)];
    }
    
    void EndConversation() {
        isInConversation = false;
        conversationPartner = -1;
        conversationTimer = 0;
        currentEmotion = Emotion::NEUTRAL;
    }
    
    void Draw() {
        // Draw NPC body
        DrawCircleV(position, 20, color);
        DrawCircleV(position, 5, WHITE);
        
        // Draw name
        DrawText(name.c_str(), position.x - 20, position.y - 40, 12, WHITE);
        
        // Draw role
        DrawText(role.c_str(), position.x - 20, position.y - 25, 10, GRAY);
        
        // Draw emotion indicator
        Color emotionColor = GetEmotionColor(currentEmotion);
        DrawCircleV({position.x + 20, position.y - 10}, 5, emotionColor);
        
        // Draw conversation indicator
        if (isInConversation) {
            DrawCircleLines(position.x, position.y, 25, YELLOW);
            DrawText("💬", position.x + 25, position.y - 20, 20, YELLOW);
        }
        
        // Draw busy indicator
        if (isBusy) {
            DrawText("⏳", position.x - 35, position.y - 10, 16, ORANGE);
        }
    }
    
    Color GetEmotionColor(Emotion emotion) {
        switch (emotion) {
            case Emotion::HAPPY: return YELLOW;
            case Emotion::SAD: return BLUE;
            case Emotion::ANGRY: return RED;
            case Emotion::SURPRISED: return ORANGE;
            case Emotion::THINKING: return PURPLE;
            case Emotion::SCARED: return DARKPURPLE;
            case Emotion::EXCITED: return PINK;
            case Emotion::CURIOUS: return CYAN;
            case Emotion::BORED: return GRAY;
            default: return WHITE;
        }
    }
};

// NPC Communication Manager
class NPCCommunicationManager {
private:
    std::vector<NPC*> npcs;
    std::vector<Message> messageLog;
    std::queue<std::pair<int, Message>> messageQueue;
    
    // Conversation display
    std::vector<std::pair<int, std::string>> conversationDisplay;
    float displayTimer;
    int maxDisplayLines;
    
    // Communication settings
    float communicationRange;
    float messageDelay;
    bool showConversations;
    
public:
    NPCCommunicationManager() : communicationRange(150), messageDelay(0.5f),
                               showConversations(true), displayTimer(0),
                               maxDisplayLines(5) {}
    
    void RegisterNPC(NPC* npc) {
        npcs.push_back(npc);
    }
    
    void Update(float dt) {
        // Update all NPCs
        for (auto npc : npcs) {
            npc->Update(dt);
        }
        
        // Check for NPCs near each other to start conversations
        CheckProximityConversations();
        
        // Process message queue
        ProcessMessageQueue(dt);
        
        // Update display timer
        if (displayTimer > 0) {
            displayTimer -= dt;
            if (displayTimer <= 0) {
                conversationDisplay.clear();
            }
        }
    }
    
    void CheckProximityConversations() {
        for (size_t i = 0; i < npcs.size(); i++) {
            for (size_t j = i + 1; j < npcs.size(); j++) {
                NPC* npc1 = npcs[i];
                NPC* npc2 = npcs[j];
                
                float distance = Vector2Distance(npc1->position, npc2->position);
                
                if (distance < communicationRange && 
                    !npc1->isInConversation && !npc2->isInConversation &&
                    !npc1->isBusy && !npc2->isBusy) {
                    
                    // Check if they should start a conversation
                    float conversationChance = 
                        npc1->personality.sociability * 
                        npc2->personality.sociability * 
                        0.001f;
                    
                    if (GetRandomValue(0, 1000) < conversationChance * 1000) {
                        StartConversation(npc1, npc2);
                    }
                }
            }
        }
    }
    
    void StartConversation(NPC* npc1, NPC* npc2) {
        npc1->isInConversation = true;
        npc1->conversationPartner = npc2->id;
        npc1->conversationTimer = GetRandomValue(5, 15);
        
        npc2->isInConversation = true;
        npc2->conversationPartner = npc1->id;
        npc2->conversationTimer = GetRandomValue(5, 15);
        
        // Initial greeting
        std::string greeting = npc1->GenerateGreeting();
        Message msg(npc1->id, npc2->id, MessageType::GREETING, greeting, 
                   npc1->personality.friendliness > 0.6f ? Emotion::HAPPY : Emotion::NEUTRAL);
        
        QueueMessage(msg);
        AddToDisplay(npc1, npc2, greeting, Emotion::NEUTRAL);
    }
    
    void QueueMessage(const Message& msg) {
        messageQueue.push({0, msg}); // 0 delay for now
    }
    
    void ProcessMessageQueue(float dt) {
        if (messageQueue.empty()) return;
        
        auto& [delay, msg] = messageQueue.front();
        delay -= dt;
        
        if (delay <= 0) {
            // Deliver message
            for (auto npc : npcs) {
                if (npc->id == msg.receiverId) {
                    npc->inbox.push(msg);
                    break;
                }
            }
            
            messageLog.push_back(msg);
            messageQueue.pop();
        }
    }
    
    void AddToDisplay(NPC* speaker, NPC* listener, const std::string& text, Emotion emotion) {
        std::string displayText = speaker->name + ": " + text;
        conversationDisplay.push_back({speaker->id, displayText});
        
        // Keep only last N messages
        if (conversationDisplay.size() > maxDisplayLines) {
            conversationDisplay.erase(conversationDisplay.begin());
        }
        
        displayTimer = 5.0f; // Show for 5 seconds
    }
    
    void Draw() {
        // Draw NPCs
        for (auto npc : npcs) {
            npc->Draw();
        }
        
        // Draw conversation lines
        if (showConversations && !conversationDisplay.empty()) {
            DrawConversationDisplay();
        }
        
        // Draw communication range circles (debug)
        if (IsKeyDown(KEY_C)) {
            for (auto npc : npcs) {
                DrawCircleLines(npc->position.x, npc->position.y, 
                              communicationRange, ColorAlpha(GREEN, 0.3f));
            }
        }
    }
    
    void DrawConversationDisplay() {
        int startY = SCREEN_HEIGHT - conversationDisplay.size() * 25 - 20;
        int panelHeight = conversationDisplay.size() * 25 + 40;
        
        // Draw background
        DrawRectangle(10, startY - 10, 400, panelHeight, ColorAlpha(BLACK, 0.7f));
        DrawRectangleLines(10, startY - 10, 400, panelHeight, GOLD);
        
        DrawText("NPC Conversations", 20, startY, 16, YELLOW);
        
        int y = startY + 20;
        for (const auto& [speakerId, text] : conversationDisplay) {
            // Find speaker color
            Color speakerColor = WHITE;
            for (auto npc : npcs) {
                if (npc->id == speakerId) {
                    speakerColor = npc->color;
                    break;
                }
            }
            
            DrawText(text.c_str(), 20, y, 14, speakerColor);
            y += 20;
        }
    }
    
    void DrawRelationshipPanel() {
        // Draw relationship visualization
        int panelX = SCREEN_WIDTH - 350;
        int panelY = 50;
        int panelWidth = 300;
        int panelHeight = 200;
        
        DrawRectangle(panelX, panelY, panelWidth, panelHeight, ColorAlpha(BLACK, 0.7f));
        DrawRectangleLines(panelX, panelY, panelWidth, panelHeight, WHITE);
        
        DrawText("NPC RELATIONSHIPS", panelX + 10, panelY + 10, 14, YELLOW);
        
        int y = panelY + 40;
        for (auto npc1 : npcs) {
            for (auto npc2 : npcs) {
                if (npc1->id < npc2->id) {
                    auto relIt = npc1->relationships.find(npc2->id);
                    if (relIt != npc1->relationships.end()) {
                        Relationship& rel = relIt->second;
                        
                        DrawText(TextFormat("%s - %s", npc1->name.c_str(), npc2->name.c_str()),
                                panelX + 10, y, 12, WHITE);
                        
                        // Draw relationship bars
                        DrawText(TextFormat("Familiarity: %d", rel.familiarity),
                                panelX + 10, y + 15, 10, GRAY);
                        DrawText(TextFormat("Affection: %d", rel.affection),
                                panelX + 10, y + 30, 10, GRAY);
                        DrawText(TextFormat("Trust: %d", rel.trust),
                                panelX + 10, y + 45, 10, GRAY);
                        
                        y += 55;
                        
                        if (y > panelY + panelHeight - 30) {
                            return; // Don't overflow panel
                        }
                    }
                }
            }
        }
    }
    
    void SetCommunicationRange(float range) { communicationRange = range; }
    void SetShowConversations(bool show) { showConversations = show; }
    
    size_t GetNPCCount() const { return npcs.size(); }
    const std::vector<Message>& GetMessageLog() const { return messageLog; }
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "NPC Communication System");
    SetTargetFPS(60);
    
    // Create NPC Communication Manager
    NPCCommunicationManager commManager;
    
    // Create NPCs
    NPC* merchant = new NPC(0, "Marcus", "Merchant", 
                           {SCREEN_WIDTH/2 - 300, SCREEN_HEIGHT/2}, ORANGE);
    NPC* guard = new NPC(1, "Roderick", "Guard", 
                        {SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 100}, RED);
    NPC* scholar = new NPC(2, "Eleanor", "Scholar", 
                          {SCREEN_WIDTH/2 + 100, SCREEN_HEIGHT/2 - 50}, PURPLE);
    NPC* farmer = new NPC(3, "Thomas", "Farmer", 
                         {SCREEN_WIDTH/2 + 300, SCREEN_HEIGHT/2 + 50}, GREEN);
    NPC* healer = new NPC(4, "Lily", "Healer", 
                         {SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 150}, PINK);
    NPC* blacksmith = new NPC(5, "Gorn", "Blacksmith", 
                             {SCREEN_WIDTH/2 + 50, SCREEN_HEIGHT/2 + 150}, BROWN);
    
    // Register NPCs with communication manager
    commManager.RegisterNPC(merchant);
    commManager.RegisterNPC(guard);
    commManager.RegisterNPC(scholar);
    commManager.RegisterNPC(farmer);
    commManager.RegisterNPC(healer);
    commManager.RegisterNPC(blacksmith);
    
    // Set communication range
    commManager.SetCommunicationRange(200);
    
    // Game loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Update
        commManager.Update(dt);
        
        // Draw
        BeginDrawing();
        ClearBackground(Color{30, 30, 50, 255});
        
        // Draw ground
        DrawRectangle(0, SCREEN_HEIGHT - 100, SCREEN_WIDTH, 100, DARKGREEN);
        
        // Draw NPCs and conversations
        commManager.Draw();
        
        // Draw relationship panel
        commManager.DrawRelationshipPanel();
        
        // Draw UI
        DrawText("NPC Communication System", SCREEN_WIDTH/2 - 150, 10, 25, WHITE);
        DrawText("Watch NPCs interact with each other!", SCREEN_WIDTH/2 - 150, 40, 16, GRAY);
        DrawText("Hold C to show communication range", 10, SCREEN_HEIGHT - 30, 14, GRAY);
        DrawText("Number of NPCs: " + std::to_string(commManager.GetNPCCount()), 
                SCREEN_WIDTH - 200, SCREEN_HEIGHT - 30, 14, WHITE);
        
        // Draw message log statistics
        const auto& messageLog = commManager.GetMessageLog();
        DrawText(TextFormat("Total messages: %zu", messageLog.size()), 
                SCREEN_WIDTH - 200, 20, 14, GRAY);
        
        EndDrawing();
    }
    
    // Clean up
    delete merchant;
    delete guard;
    delete scholar;
    delete farmer;
    delete healer;
    delete blacksmith;
    
    CloseWindow();
    return 0;
}