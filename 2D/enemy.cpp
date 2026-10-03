#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <memory>
#include <random>

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;

// Status effect types
enum class StatusEffectType {
    NONE,
    POISON,
    BURN,
    FREEZE,
    STUN,
    BLEED,
    SHIELD,
    REGENERATION,
    STRENGTH_BOOST,
    SPEED_BOOST,
    WEAKNESS,
    SLOW
};

// Status effect structure
struct StatusEffect {
    StatusEffectType type;
    float duration;
    float tickTimer;
    float tickInterval;
    float magnitude;
    bool isBuff;
    
    StatusEffect(StatusEffectType t, float dur, float mag, float tickInt = 1.0f)
        : type(t), duration(dur), magnitude(mag), tickTimer(0), 
          tickInterval(tickInt), isBuff(false) {
        // Determine if this is a buff or debuff
        switch (type) {
            case StatusEffectType::SHIELD:
            case StatusEffectType::REGENERATION:
            case StatusEffectType::STRENGTH_BOOST:
            case StatusEffectType::SPEED_BOOST:
                isBuff = true;
                break;
            default:
                isBuff = false;
        }
    }
};

// Character stats
struct CharacterStats {
    float maxHealth;
    float currentHealth;
    float maxMana;
    float currentMana;
    float maxStamina;
    float currentStamina;
    float attackPower;
    float defense;
    float speed;
    float critChance;
    float critMultiplier;
    float healthRegen;
    float manaRegen;
    float staminaRegen;
    int level;
    int experience;
    int experienceToNextLevel;
    int gold;
    
    CharacterStats() {
        maxHealth = 100;
        currentHealth = 100;
        maxMana = 50;
        currentMana = 50;
        maxStamina = 100;
        currentStamina = 100;
        attackPower = 10;
        defense = 5;
        speed = 200;
        critChance = 0.1f;
        critMultiplier = 1.5f;
        healthRegen = 2;
        manaRegen = 5;
        staminaRegen = 10;
        level = 1;
        experience = 0;
        experienceToNextLevel = 100;
        gold = 0;
    }
};

// Enemy class
class Enemy {
public:
    Vector2 position;
    CharacterStats stats;
    std::vector<StatusEffect> statusEffects;
    Color color;
    float width;
    float height;
    bool isAlive;
    std::string name;
    
    // Combat
    float attackRange;
    float attackCooldown;
    float currentCooldown;
    float detectionRange;
    bool isAggro;
    
    // Animation
    float hitFlashTimer;
    float deathTimer;
    
    Enemy(Vector2 pos, const std::string& enemyName, Color enemyColor) 
        : position(pos), color(enemyColor), width(30), height(40), 
          isAlive(true), name(enemyName), attackRange(50), attackCooldown(1.5f),
          currentCooldown(0), detectionRange(200), isAggro(false),
          hitFlashTimer(0), deathTimer(0) {
        stats.maxHealth = 50;
        stats.currentHealth = 50;
        stats.attackPower = 8;
        stats.defense = 3;
        stats.speed = 100;
    }
    
    void TakeDamage(float damage) {
        float actualDamage = std::max(0.0f, damage - stats.defense);
        stats.currentHealth -= actualDamage;
        hitFlashTimer = 0.2f;
        
        if (stats.currentHealth <= 0) {
            stats.currentHealth = 0;
            isAlive = false;
            deathTimer = 0;
        }
    }
    
    void Update(float dt) {
        if (!isAlive) {
            deathTimer += dt;
            return;
        }
        
        // Update status effects
        UpdateStatusEffects(dt);
        
        // Update cooldown
        if (currentCooldown > 0) {
            currentCooldown -= dt;
        }
        
        // Update hit flash
        if (hitFlashTimer > 0) {
            hitFlashTimer -= dt;
        }
        
        // Regenerate health
        stats.currentHealth += stats.healthRegen * dt;
        stats.currentHealth = std::min(stats.currentHealth, stats.maxHealth);
    }
    
    void UpdateStatusEffects(float dt) {
        for (auto it = statusEffects.begin(); it != statusEffects.end();) {
            it->duration -= dt;
            it->tickTimer -= dt;
            
            // Apply tick effects
            if (it->tickTimer <= 0) {
                it->tickTimer = it->tickInterval;
                
                switch (it->type) {
                    case StatusEffectType::POISON:
                        TakeDamage(it->magnitude);
                        break;
                    case StatusEffectType::BURN:
                        TakeDamage(it->magnitude * 1.5f);
                        break;
                    case StatusEffectType::BLEED:
                        TakeDamage(it->magnitude * 0.8f);
                        break;
                    case StatusEffectType::REGENERATION:
                        stats.currentHealth += it->magnitude;
                        stats.currentHealth = std::min(stats.currentHealth, stats.maxHealth);
                        break;
                    default:
                        break;
                }
            }
            
            if (it->duration <= 0) {
                it = statusEffects.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void Draw() {
        if (!isAlive && deathTimer > 1.0f) return;
        
        Color drawColor = color;
        if (hitFlashTimer > 0) {
            drawColor = WHITE;
        }
        
        if (!isAlive) {
            drawColor = ColorAlpha(drawColor, 1.0f - deathTimer);
        }
        
        // Draw enemy body
        DrawRectangleV({position.x - width/2, position.y - height/2}, 
                      {width, height}, drawColor);
        
        // Draw enemy eyes (if alive)
        if (isAlive) {
            DrawCircleV({position.x - 5, position.y - 5}, 3, RED);
            DrawCircleV({position.x + 5, position.y - 5}, 3, RED);
        }
        
        // Draw health bar above enemy
        DrawEnemyHealthBar();
        
        // Draw status effect icons
        DrawStatusEffectIcons();
    }
    
    void DrawEnemyHealthBar() {
        float barWidth = 40;
        float barHeight = 5;
        float barX = position.x - barWidth/2;
        float barY = position.y - height/2 - 15;
        
        // Background
        DrawRectangle(barX, barY, barWidth, barHeight, DARKGRAY);
        
        // Health
        float healthPercent = stats.currentHealth / stats.maxHealth;
        Color healthColor = GREEN;
        if (healthPercent < 0.3f) healthColor = RED;
        else if (healthPercent < 0.6f) healthColor = ORANGE;
        
        DrawRectangle(barX, barY, barWidth * healthPercent, barHeight, healthColor);
        
        // Border
        DrawRectangleLines(barX, barY, barWidth, barHeight, BLACK);
        
        // Draw name
        DrawText(name.c_str(), barX - 10, barY - 15, 10, WHITE);
    }
    
    void DrawStatusEffectIcons() {
        float iconX = position.x - (statusEffects.size() * 12) / 2;
        float iconY = position.y - height/2 - 30;
        
        for (size_t i = 0; i < statusEffects.size(); i++) {
            Color iconColor = GetStatusEffectColor(statusEffects[i].type);
            DrawRectangle(iconX + i * 12, iconY, 10, 10, iconColor);
            DrawRectangleLines(iconX + i * 12, iconY, 10, 10, BLACK);
        }
    }
    
    Color GetStatusEffectColor(StatusEffectType type) {
        switch (type) {
            case StatusEffectType::POISON: return GREEN;
            case StatusEffectType::BURN: return ORANGE;
            case StatusEffectType::FREEZE: return SKYBLUE;
            case StatusEffectType::STUN: return YELLOW;
            case StatusEffectType::BLEED: return RED;
            case StatusEffectType::SHIELD: return BLUE;
            case StatusEffectType::REGENERATION: return PINK;
            case StatusEffectType::STRENGTH_BOOST: return RED;
            case StatusEffectType::SPEED_BOOST: return CYAN;
            case StatusEffectType::WEAKNESS: return PURPLE;
            case StatusEffectType::SLOW: return DARKBLUE;
            default: return WHITE;
        }
    }
};

// Player character class
class PlayerCharacter {
public:
    Vector2 position;
    CharacterStats stats;
    std::vector<StatusEffect> statusEffects;
    Color color;
    float width;
    float height;
    bool isAlive;
    bool isAttacking;
    float attackTimer;
    float attackDuration;
    float attackRange;
    float attackCooldown;
    float currentCooldown;
    
    // Animation
    float hitFlashTimer;
    float damageNumberTimer;
    float lastDamageTaken;
    
    // Combat combo
    int comboCount;
    float comboTimer;
    
    PlayerCharacter(Vector2 pos) 
        : position(pos), color(BLUE), width(25), height(35), 
          isAlive(true), isAttacking(false), attackTimer(0), attackDuration(0.3f),
          attackRange(60), attackCooldown(0.5f), currentCooldown(0),
          hitFlashTimer(0), damageNumberTimer(0), lastDamageTaken(0),
          comboCount(0), comboTimer(0) {
        stats.maxHealth = 150;
        stats.currentHealth = 150;
        stats.maxMana = 75;
        stats.currentMana = 75;
        stats.maxStamina = 120;
        stats.currentStamina = 120;
        stats.attackPower = 15;
        stats.defense = 8;
        stats.speed = 250;
        stats.critChance = 0.15f;
        stats.critMultiplier = 1.8f;
        stats.healthRegen = 5;
        stats.manaRegen = 8;
        stats.staminaRegen = 15;
    }
    
    void Update(float dt) {
        if (!isAlive) return;
        
        // Update status effects
        UpdateStatusEffects(dt);
        
        // Update timers
        if (currentCooldown > 0) currentCooldown -= dt;
        if (hitFlashTimer > 0) hitFlashTimer -= dt;
        if (damageNumberTimer > 0) damageNumberTimer -= dt;
        if (comboTimer > 0) {
            comboTimer -= dt;
            if (comboTimer <= 0) comboCount = 0;
        }
        
        // Update attack animation
        if (isAttacking) {
            attackTimer -= dt;
            if (attackTimer <= 0) {
                isAttacking = false;
            }
        }
        
        // Regenerate resources
        stats.currentHealth += stats.healthRegen * dt;
        stats.currentMana += stats.manaRegen * dt;
        stats.currentStamina += stats.staminaRegen * dt;
        
        // Clamp values
        stats.currentHealth = std::min(stats.currentHealth, stats.maxHealth);
        stats.currentMana = std::min(stats.currentMana, stats.maxMana);
        stats.currentStamina = std::min(stats.currentStamina, stats.maxStamina);
    }
    
    void UpdateStatusEffects(float dt) {
        for (auto it = statusEffects.begin(); it != statusEffects.end();) {
            it->duration -= dt;
            it->tickTimer -= dt;
            
            if (it->tickTimer <= 0) {
                it->tickTimer = it->tickInterval;
                
                switch (it->type) {
                    case StatusEffectType::POISON:
                        TakeDamage(it->magnitude);
                        break;
                    case StatusEffectType::BURN:
                        TakeDamage(it->magnitude * 1.5f);
                        break;
                    case StatusEffectType::BLEED:
                        TakeDamage(it->magnitude * 0.8f);
                        break;
                    case StatusEffectType::REGENERATION:
                        stats.currentHealth += it->magnitude;
                        stats.currentHealth = std::min(stats.currentHealth, stats.maxHealth);
                        break;
                    default:
                        break;
                }
            }
            
            if (it->duration <= 0) {
                it = statusEffects.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void TakeDamage(float damage) {
        float actualDamage = std::max(0.0f, damage - stats.defense);
        
        // Check for shield
        for (auto& effect : statusEffects) {
            if (effect.type == StatusEffectType::SHIELD) {
                float absorbed = std::min(effect.magnitude, actualDamage);
                effect.magnitude -= absorbed;
                actualDamage -= absorbed;
                if (effect.magnitude <= 0) {
                    effect.duration = 0;
                }
            }
        }
        
        stats.currentHealth -= actualDamage;
        hitFlashTimer = 0.2f;
        lastDamageTaken = actualDamage;
        damageNumberTimer = 1.0f;
        
        if (stats.currentHealth <= 0) {
            stats.currentHealth = 0;
            isAlive = false;
        }
    }
    
    void Attack(Enemy* enemy) {
        if (!isAlive || !enemy || !enemy->isAlive || currentCooldown > 0) return;
        
        // Check distance
        float dist = Vector2Distance(position, enemy->position);
        if (dist > attackRange) return;
        
        // Start attack animation
        isAttacking = true;
        attackTimer = attackDuration;
        currentCooldown = attackCooldown;
        
        // Check stamina
        if (stats.currentStamina < 10) return;
        stats.currentStamina -= 10;
        
        // Calculate damage
        float damage = stats.attackPower;
        
        // Check critical hit
        bool isCrit = (GetRandomValue(0, 100) / 100.0f) < stats.critChance;
        if (isCrit) {
            damage *= stats.critMultiplier;
        }
        
        // Apply strength boost
        for (const auto& effect : statusEffects) {
            if (effect.type == StatusEffectType::STRENGTH_BOOST) {
                damage *= 1.0f + effect.magnitude;
            }
        }
        
        // Apply weakness
        for (const auto& effect : statusEffects) {
            if (effect.type == StatusEffectType::WEAKNESS) {
                damage *= 0.7f;
            }
        }
        
        // Deal damage to enemy
        enemy->TakeDamage(damage);
        
        // Update combo
        comboCount++;
        comboTimer = 2.0f;
        
        // Apply combo multiplier
        if (comboCount > 1) {
            float comboMultiplier = 1.0f + (comboCount - 1) * 0.1f;
            // Additional damage already applied, just for show
        }
        
        // Gain experience if enemy dies
        if (!enemy->isAlive) {
            GainExperience(20);
        }
    }
    
    void GainExperience(int amount) {
        stats.experience += amount;
        
        // Level up check
        while (stats.experience >= stats.experienceToNextLevel) {
            stats.experience -= stats.experienceToNextLevel;
            LevelUp();
        }
    }
    
    void LevelUp() {
        stats.level++;
        stats.experienceToNextLevel = (int)(stats.experienceToNextLevel * 1.5f);
        
        // Increase stats
        stats.maxHealth += 20;
        stats.currentHealth = stats.maxHealth;
        stats.maxMana += 10;
        stats.currentMana = stats.maxMana;
        stats.attackPower += 3;
        stats.defense += 2;
    }
    
    void Draw() {
        if (!isAlive) return;
        
        Color drawColor = color;
        if (hitFlashTimer > 0) {
            drawColor = RED;
        }
        
        // Draw player body
        DrawRectangleV({position.x - width/2, position.y - height/2}, 
                      {width, height}, drawColor);
        
        // Draw eyes
        DrawCircleV({position.x - 5, position.y - 5}, 2, WHITE);
        DrawCircleV({position.x + 5, position.y - 5}, 2, WHITE);
        
        // Draw attack animation
        if (isAttacking) {
            DrawCircleV({position.x + attackRange * 0.7f, position.y}, 10, 
                       ColorAlpha(YELLOW, 0.5f));
        }
        
        // Draw damage number
        if (damageNumberTimer > 0 && lastDamageTaken > 0) {
            DrawText(TextFormat("-%.0f", lastDamageTaken), 
                    position.x - 15, position.y - height, 16, RED);
        }
        
        // Draw combo counter
        if (comboCount > 1 && comboTimer > 0) {
            DrawText(TextFormat("%d COMBO!", comboCount), 
                    position.x - 30, position.y - height - 20, 16, YELLOW);
        }
    }
    
    Color GetStatusEffectColor(StatusEffectType type) {
        switch (type) {
            case StatusEffectType::POISON: return GREEN;
            case StatusEffectType::BURN: return ORANGE;
            case StatusEffectType::FREEZE: return SKYBLUE;
            case StatusEffectType::STUN: return YELLOW;
            case StatusEffectType::BLEED: return RED;
            case StatusEffectType::SHIELD: return BLUE;
            case StatusEffectType::REGENERATION: return PINK;
            case StatusEffectType::STRENGTH_BOOST: return RED;
            case StatusEffectType::SPEED_BOOST: return CYAN;
            case StatusEffectType::WEAKNESS: return PURPLE;
            case StatusEffectType::SLOW: return DARKBLUE;
            default: return WHITE;
        }
    }
};

// Status bar UI class
class StatusBarUI {
private:
    // Player status bars
    Rectangle healthBar;
    Rectangle manaBar;
    Rectangle staminaBar;
    Rectangle experienceBar;
    
    // Enemy status bars
    std::vector<Rectangle> enemyHealthBars;
    
    // Animation
    float healthBarAnimation;
    float manaBarAnimation;
    float staminaBarAnimation;
    float experienceBarAnimation;
    
    // Colors
    Color healthColor;
    Color manaColor;
    Color staminaColor;
    Color experienceColor;
    
public:
    StatusBarUI() {
        // Initialize player status bars
        healthBar = {50, 50, 300, 25};
        manaBar = {50, 85, 300, 20};
        staminaBar = {50, 115, 300, 20};
        experienceBar = {50, 145, 300, 15};
        
        // Initialize animations
        healthBarAnimation = 1.0f;
        manaBarAnimation = 1.0f;
        staminaBarAnimation = 1.0f;
        experienceBarAnimation = 1.0f;
        
        // Initialize colors
        healthColor = RED;
        manaColor = BLUE;
        staminaColor = GREEN;
        experienceColor = PURPLE;
    }
    
    void Update(const PlayerCharacter& player, float dt) {
        // Smooth bar animations
        float healthPercent = player.stats.currentHealth / player.stats.maxHealth;
        float manaPercent = player.stats.currentMana / player.stats.maxMana;
        float staminaPercent = player.stats.currentStamina / player.stats.maxStamina;
        float expPercent = (float)player.stats.experience / player.stats.experienceToNextLevel;
        
        healthBarAnimation = Lerp(healthBarAnimation, healthPercent, dt * 5);
        manaBarAnimation = Lerp(manaBarAnimation, manaPercent, dt * 5);
        staminaBarAnimation = Lerp(staminaBarAnimation, staminaPercent, dt * 5);
        experienceBarAnimation = Lerp(experienceBarAnimation, expPercent, dt * 5);
    }
    
    float Lerp(float a, float b, float t) {
        return a + (b - a) * std::min(1.0f, t);
    }
    
    void Draw(const PlayerCharacter& player) {
        // Draw player status bars
        DrawPlayerStatusBars(player);
        
        // Draw player level and info
        DrawPlayerInfo(player);
        
        // Draw status effects
        DrawPlayerStatusEffects(player);
    }
    
    void DrawPlayerStatusBars(const PlayerCharacter& player) {
        // Health bar
        DrawRectangleRec(healthBar, DARKGRAY);
        DrawRectangleRec({healthBar.x, healthBar.y, healthBar.width * healthBarAnimation, healthBar.height}, 
                        healthColor);
        DrawRectangleLinesEx(healthBar, 2, BLACK);
        
        // Health bar text
        DrawText(TextFormat("HP: %.0f / %.0f", player.stats.currentHealth, player.stats.maxHealth),
                healthBar.x + 5, healthBar.y + 3, 15, WHITE);
        
        // Mana bar
        DrawRectangleRec(manaBar, DARKGRAY);
        DrawRectangleRec({manaBar.x, manaBar.y, manaBar.width * manaBarAnimation, manaBar.height}, 
                        manaColor);
        DrawRectangleLinesEx(manaBar, 2, BLACK);
        
        // Mana bar text
        DrawText(TextFormat("MP: %.0f / %.0f", player.stats.currentMana, player.stats.maxMana),
                manaBar.x + 5, manaBar.y + 2, 15, WHITE);
        
        // Stamina bar
        DrawRectangleRec(staminaBar, DARKGRAY);
        DrawRectangleRec({staminaBar.x, staminaBar.y, staminaBar.width * staminaBarAnimation, staminaBar.height}, 
                        staminaColor);
        DrawRectangleLinesEx(staminaBar, 2, BLACK);
        
        // Stamina bar text
        DrawText(TextFormat("SP: %.0f / %.0f", player.stats.currentStamina, player.stats.maxStamina),
                staminaBar.x + 5, staminaBar.y + 2, 15, WHITE);
        
        // Experience bar
        DrawRectangleRec(experienceBar, DARKGRAY);
        DrawRectangleRec({experienceBar.x, experienceBar.y, experienceBar.width * experienceBarAnimation, experienceBar.height}, 
                        experienceColor);
        DrawRectangleLinesEx(experienceBar, 2, BLACK);
        
        // Experience bar text
        DrawText(TextFormat("EXP: %d / %d", player.stats.experience, player.stats.experienceToNextLevel),
                experienceBar.x + 5, experienceBar.y, 12, WHITE);
    }
    
    void DrawPlayerInfo(const PlayerCharacter& player) {
        // Player level
        DrawText(TextFormat("Level %d", player.stats.level), 
                healthBar.x, healthBar.y - 25, 20, YELLOW);
        
        // Player gold
        DrawText(TextFormat("Gold: %d", player.stats.gold), 
                healthBar.x + 200, healthBar.y - 25, 15, GOLD);
        
        // Combat stats
        DrawText(TextFormat("ATK: %.0f  DEF: %.0f", player.stats.attackPower, player.stats.defense),
                healthBar.x, experienceBar.y + experienceBar.height + 5, 12, WHITE);
    }
    
    void DrawPlayerStatusEffects(const PlayerCharacter& player) {
        float iconX = healthBar.x;
        float iconY = experienceBar.y + experienceBar.height + 25;
        
        int buffCount = 0;
        int debuffCount = 0;
        
        // Count buffs and debuffs
        for (const auto& effect : player.statusEffects) {
            if (effect.isBuff) buffCount++;
            else debuffCount++;
        }
        
        // Draw buff icons
        DrawText("Buffs:", iconX, iconY, 12, GREEN);
        int buffIndex = 0;
        for (const auto& effect : player.statusEffects) {
            if (effect.isBuff) {
                Color effectColor = GetStatusEffectColor(effect.type);
                DrawRectangle(iconX + 50 + buffIndex * 25, iconY, 20, 20, effectColor);
                DrawRectangleLines(iconX + 50 + buffIndex * 25, iconY, 20, 20, BLACK);
                
                // Draw duration
                DrawText(TextFormat("%.0f", effect.duration), 
                        iconX + 50 + buffIndex * 25 + 5, iconY + 22, 10, WHITE);
                buffIndex++;
            }
        }
        
        // Draw debuff icons
        DrawText("Debuffs:", iconX, iconY + 30, 12, RED);
        int debuffIndex = 0;
        for (const auto& effect : player.statusEffects) {
            if (!effect.isBuff) {
                Color effectColor = GetStatusEffectColor(effect.type);
                DrawRectangle(iconX + 70 + debuffIndex * 25, iconY + 30, 20, 20, effectColor);
                DrawRectangleLines(iconX + 70 + debuffIndex * 25, iconY + 30, 20, 20, BLACK);
                
                // Draw duration
                DrawText(TextFormat("%.0f", effect.duration), 
                        iconX + 70 + debuffIndex * 25 + 5, iconY + 52, 10, WHITE);
                debuffIndex++;
            }
        }
    }
    
    Color GetStatusEffectColor(StatusEffectType type) {
        switch (type) {
            case StatusEffectType::POISON: return GREEN;
            case StatusEffectType::BURN: return ORANGE;
            case StatusEffectType::FREEZE: return SKYBLUE;
            case StatusEffectType::STUN: return YELLOW;
            case StatusEffectType::BLEED: return RED;
            case StatusEffectType::SHIELD: return BLUE;
            case StatusEffectType::REGENERATION: return PINK;
            case StatusEffectType::STRENGTH_BOOST: return RED;
            case StatusEffectType::SPEED_BOOST: return CYAN;
            case StatusEffectType::WEAKNESS: return PURPLE;
            case StatusEffectType::SLOW: return DARKBLUE;
            default: return WHITE;
        }
    }
};

// Combat system class
class CombatSystem {
private:
    std::vector<Enemy*> enemies;
    PlayerCharacter* player;
    StatusBarUI statusBar;
    
    // Combat effects
    std::vector<std::pair<Vector2, std::string>> damageNumbers;
    float shakeTimer;
    float shakeIntensity;
    
public:
    CombatSystem(PlayerCharacter* p) : player(p), shakeTimer(0), shakeIntensity(0) {}
    
    void AddEnemy(Enemy* enemy) {
        enemies.push_back(enemy);
    }
    
    void Update(float dt) {
        if (!player->isAlive) return;
        
        // Update player
        player->Update(dt);
        
        // Update status bar
        statusBar.Update(*player, dt);
        
        // Update enemies
        for (auto enemy : enemies) {
            if (enemy) {
                enemy->Update(dt);
                
                // Enemy AI
                UpdateEnemyAI(enemy, dt);
                
                // Check collision with player attacks
                if (player->isAttacking) {
                    float dist = Vector2Distance(player->position, enemy->position);
                    if (dist < player->attackRange) {
                        player->Attack(enemy);
                    }
                }
            }
        }
        
        // Clean up dead enemies
        enemies.erase(
            std::remove_if(enemies.begin(), enemies.end(),
                [](Enemy* e) { return !e->isAlive && e->deathTimer > 2.0f; }),
            enemies.end()
        );
        
        // Update screen shake
        if (shakeTimer > 0) {
            shakeTimer -= dt;
        }
    }
    
    void UpdateEnemyAI(Enemy* enemy, float dt) {
        if (!enemy->isAlive) return;
        
        float distToPlayer = Vector2Distance(enemy->position, player->position);
        
        // Check detection
        if (distToPlayer < enemy->detectionRange) {
            enemy->isAggro = true;
        }
        
        if (enemy->isAggro) {
            // Move towards player
            if (distToPlayer > enemy->attackRange) {
                Vector2 direction = Vector2Subtract(player->position, enemy->position);
                direction = Vector2Normalize(direction);
                enemy->position = Vector2Add(enemy->position, 
                    Vector2Scale(direction, enemy->stats.speed * dt));
            }
            
            // Attack player
            if (distToPlayer <= enemy->attackRange && enemy->currentCooldown <= 0) {
                player->TakeDamage(enemy->stats.attackPower);
                enemy->currentCooldown = enemy->attackCooldown;
                shakeTimer = 0.3f;
                shakeIntensity = 5.0f;
            }
        }
    }
    
    void Draw() {
        // Apply screen shake
        float shakeX = 0, shakeY = 0;
        if (shakeTimer > 0) {
            shakeX = GetRandomValue(-shakeIntensity, shakeIntensity) * shakeTimer;
            shakeY = GetRandomValue(-shakeIntensity, shakeIntensity) * shakeTimer;
        }
        
        // Draw player
        player->Draw();
        
        // Draw enemies
        for (auto enemy : enemies) {
            if (enemy) {
                enemy->Draw();
            }
        }
        
        // Draw status bars
        statusBar.Draw(*player);
        
        // Draw combat info
        DrawCombatInfo();
    }
    
    void DrawCombatInfo() {
        // Draw enemy count
        DrawText(TextFormat("Enemies: %zu", enemies.size()), 
                SCREEN_WIDTH - 150, 50, 20, RED);
        
        // Draw combat tips
        DrawText("Press SPACE to attack", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT - 50, 20, WHITE);
        
        // Draw player position
        DrawText(TextFormat("Position: %.0f, %.0f", player->position.x, player->position.y),
                SCREEN_WIDTH - 200, 80, 15, GRAY);
    }
    
    void ApplyStatusEffectToPlayer(StatusEffectType type, float duration, float magnitude) {
        if (!player) return;
        
        // Check if effect already exists
        for (auto& effect : player->statusEffects) {
            if (effect.type == type) {
                effect.duration = std::max(effect.duration, duration);
                effect.magnitude = std::max(effect.magnitude, magnitude);
                return;
            }
        }
        
        // Add new effect
        player->statusEffects.push_back(StatusEffect(type, duration, magnitude));
    }
    
    void ApplyStatusEffectToEnemy(Enemy* enemy, StatusEffectType type, float duration, float magnitude) {
        if (!enemy) return;
        
        // Check if effect already exists
        for (auto& effect : enemy->statusEffects) {
            if (effect.type == type) {
                effect.duration = std::max(effect.duration, duration);
                effect.magnitude = std::max(effect.magnitude, magnitude);
                return;
            }
        }
        
        // Add new effect
        enemy->statusEffects.push_back(StatusEffect(type, duration, magnitude));
    }
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Combat Status Bar System");
    SetTargetFPS(60);
    
    // Create player
    PlayerCharacter player({SCREEN_WIDTH/2, SCREEN_HEIGHT/2});
    
    // Create combat system
    CombatSystem combatSystem(&player);
    
    // Create enemies
    std::vector<Enemy*> enemies;
    enemies.push_back(new Enemy({400, 300}, "Goblin", GREEN));
    enemies.push_back(new Enemy({800, 500}, "Orc", DARKGREEN));
    enemies.push_back(new Enemy({600, 200}, "Skeleton", LIGHTGRAY));
    
    // Add enemies to combat system
    for (auto enemy : enemies) {
        combatSystem.AddEnemy(enemy);
    }
    
    // Apply some status effects for demonstration
    combatSystem.ApplyStatusEffectToPlayer(StatusEffectType::REGENERATION, 10.0f, 5.0f);
    combatSystem.ApplyStatusEffectToPlayer(StatusEffectType::STRENGTH_BOOST, 15.0f, 0.3f);
    combatSystem.ApplyStatusEffectToEnemy(enemies[0], StatusEffectType::POISON, 5.0f, 3.0f);
    
    // Game loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Update
        // Player movement
        float moveSpeed = player.stats.speed;
        
        // Check for speed effects
        for (const auto& effect : player.statusEffects) {
            if (effect.type == StatusEffectType::SPEED_BOOST) {
                moveSpeed *= 1.0f + effect.magnitude;
            }
            if (effect.type == StatusEffectType::SLOW) {
                moveSpeed *= 0.5f;
            }
            if (effect.type == StatusEffectType::STUN) {
                moveSpeed = 0;
            }
            if (effect.type == StatusEffectType::FREEZE) {
                moveSpeed = 0;
            }
        }
        
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) player.position.x -= moveSpeed * dt;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) player.position.x += moveSpeed * dt;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) player.position.y -= moveSpeed * dt;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) player.position.y += moveSpeed * dt;
        
        // Attack
        if (IsKeyPressed(KEY_SPACE)) {
            // Find nearest enemy in range
            Enemy* nearestEnemy = nullptr;
            float nearestDist = player.attackRange;
            
            for (auto enemy : enemies) {
                if (enemy && enemy->isAlive) {
                    float dist = Vector2Distance(player.position, enemy->position);
                    if (dist < nearestDist) {
                        nearestDist = dist;
                        nearestEnemy = enemy;
                    }
                }
            }
            
            if (nearestEnemy) {
                player.Attack(nearestEnemy);
            }
        }
        
        combatSystem.Update(dt);
        
        // Draw
        BeginDrawing();
        ClearBackground(Color{30, 30, 40, 255});
        
        // Draw ground grid
        for (int x = 0; x < SCREEN_WIDTH; x += 50) {
            DrawLine(x, 0, x, SCREEN_HEIGHT, ColorAlpha(DARKGRAY, 0.3f));
        }
        for (int y = 0; y < SCREEN_HEIGHT; y += 50) {
            DrawLine(0, y, SCREEN_WIDTH, y, ColorAlpha(DARKGRAY, 0.3f));
        }
        
        // Draw combat scene
        combatSystem.Draw();
        
        // Draw title
        DrawText("Combat System Demo", SCREEN_WIDTH/2 - 100, 20, 30, WHITE);
        
        // Draw controls
        DrawText("WASD: Move | SPACE: Attack | ESC: Exit", 
                SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT - 80, 18, GRAY);
        
        EndDrawing();
    }
    
    // Clean up
    for (auto enemy : enemies) {
        delete enemy;
    }
    
    CloseWindow();
    return 0;
}