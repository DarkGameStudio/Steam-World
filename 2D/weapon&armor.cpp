#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <functional>
#include <random>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iostream>

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;

// Item rarity
enum class Rarity {
    COMMON,
    UNCOMMON,
    RARE,
    EPIC,
    LEGENDARY,
    MYTHIC
};

// Weapon types
enum class WeaponType {
    SWORD,
    AXE,
    BOW,
    STAFF,
    DAGGER,
    SPEAR,
    HAMMER,
    WAND,
    CROSSBOW,
    SCYTHE
};

// Armor types
enum class ArmorType {
    HELMET,
    CHESTPLATE,
    LEGGINGS,
    BOOTS,
    SHIELD,
    GAUNTLETS,
    CLOAK,
    RING,
    AMULET,
    BELT
};

// Damage types
enum class DamageType {
    PHYSICAL,
    FIRE,
    ICE,
    LIGHTNING,
    POISON,
    HOLY,
    DARK,
    ARCANE
};

// Weapon class
class Weapon {
public:
    std::string name;
    WeaponType type;
    Rarity rarity;
    DamageType damageType;
    float baseDamage;
    float attackSpeed;
    float criticalChance;
    float criticalMultiplier;
    float range;
    int level;
    int durability;
    int maxDurability;
    std::vector<std::string> enchantments;
    std::map<std::string, float> statBonuses;
    Color weaponColor;
    
    Weapon(const std::string& weaponName, WeaponType weaponType, Rarity weaponRarity, int weaponLevel)
        : name(weaponName), type(weaponType), rarity(weaponRarity), 
          damageType(DamageType::PHYSICAL), baseDamage(10), attackSpeed(1.0f),
          criticalChance(0.1f), criticalMultiplier(1.5f), range(50),
          level(weaponLevel), durability(100), maxDurability(100), weaponColor(WHITE) {
        
        // Set base stats based on type
        switch (type) {
            case WeaponType::SWORD:
                baseDamage = 15;
                attackSpeed = 1.2f;
                range = 50;
                break;
            case WeaponType::AXE:
                baseDamage = 20;
                attackSpeed = 0.8f;
                range = 45;
                break;
            case WeaponType::BOW:
                baseDamage = 12;
                attackSpeed = 1.5f;
                range = 300;
                break;
            case WeaponType::STAFF:
                baseDamage = 18;
                attackSpeed = 0.7f;
                range = 150;
                damageType = DamageType::ARCANE;
                break;
            case WeaponType::DAGGER:
                baseDamage = 8;
                attackSpeed = 2.0f;
                range = 30;
                criticalChance = 0.25f;
                break;
            case WeaponType::SPEAR:
                baseDamage = 16;
                attackSpeed = 1.0f;
                range = 80;
                break;
            case WeaponType::HAMMER:
                baseDamage = 25;
                attackSpeed = 0.6f;
                range = 40;
                criticalMultiplier = 2.0f;
                break;
            case WeaponType::WAND:
                baseDamage = 10;
                attackSpeed = 1.8f;
                range = 200;
                damageType = DamageType::ARCANE;
                break;
            case WeaponType::CROSSBOW:
                baseDamage = 22;
                attackSpeed = 0.9f;
                range = 250;
                break;
            case WeaponType::SCYTHE:
                baseDamage = 24;
                attackSpeed = 0.7f;
                range = 70;
                damageType = DamageType::DARK;
                break;
        }
        
        // Apply rarity multipliers
        ApplyRarityModifiers();
        
        // Apply level scaling
        ScaleForLevel();
        
        // Set weapon color based on rarity
        weaponColor = GetRarityColor(rarity);
        
        // Generate enchantments for rare+ weapons
        if (rarity >= Rarity::RARE) {
            GenerateEnchantments();
        }
    }
    
    void ApplyRarityModifiers() {
        float multiplier = 1.0f;
        
        switch (rarity) {
            case Rarity::COMMON: multiplier = 1.0f; break;
            case Rarity::UNCOMMON: multiplier = 1.2f; break;
            case Rarity::RARE: multiplier = 1.5f; break;
            case Rarity::EPIC: multiplier = 2.0f; break;
            case Rarity::LEGENDARY: multiplier = 3.0f; break;
            case Rarity::MYTHIC: multiplier = 5.0f; break;
        }
        
        baseDamage *= multiplier;
        criticalChance *= multiplier;
        criticalMultiplier *= (1.0f + (multiplier - 1.0f) * 0.5f);
    }
    
    void ScaleForLevel() {
        float levelMultiplier = 1.0f + (level - 1) * 0.1f;
        baseDamage *= levelMultiplier;
        maxDurability = 100 + (level - 1) * 20;
        durability = maxDurability;
    }
    
    void GenerateEnchantments() {
        std::vector<std::string> possibleEnchantments = {
            "Fire Damage", "Ice Damage", "Lightning Damage", "Poison Damage",
            "Life Steal", "Mana Steal", "Armor Penetration", "Attack Speed",
            "Critical Strike", "Vampiric", "Berserker", "Executioner"
        };
        
        int numEnchantments = static_cast<int>(rarity) - 2; // Rare = 1, Epic = 2, etc.
        
        for (int i = 0; i < numEnchantments && i < possibleEnchantments.size(); i++) {
            enchantments.push_back(possibleEnchantments[GetRandomValue(0, possibleEnchantments.size() - 1)]);
            
            // Add stat bonus based on enchantment
            std::string enchantment = enchantments.back();
            if (enchantment == "Fire Damage") {
                damageType = DamageType::FIRE;
                statBonuses["fire_damage"] += 15;
            } else if (enchantment == "Attack Speed") {
                statBonuses["attack_speed"] += 0.2f;
            } else if (enchantment == "Critical Strike") {
                statBonuses["critical_chance"] += 0.1f;
            }
        }
    }
    
    float CalculateDamage(float characterStrength) {
        float damage = baseDamage + characterStrength * 0.5f;
        
        // Apply stat bonuses
        if (statBonuses.find("fire_damage") != statBonuses.end()) {
            damage += statBonuses["fire_damage"];
        }
        
        return damage;
    }
    
    void Repair(int amount) {
        durability = std::min(maxDurability, durability + amount);
    }
    
    void TakeDamage(int amount) {
        durability = std::max(0, durability - amount);
    }
    
    bool IsBroken() const {
        return durability <= 0;
    }
    
    Color GetRarityColor(Rarity rarity) {
        switch (rarity) {
            case Rarity::COMMON: return GRAY;
            case Rarity::UNCOMMON: return GREEN;
            case Rarity::RARE: return BLUE;
            case Rarity::EPIC: return PURPLE;
            case Rarity::LEGENDARY: return ORANGE;
            case Rarity::MYTHIC: return RED;
            default: return WHITE;
        }
    }
    
    std::string GetRarityName() const {
        switch (rarity) {
            case Rarity::COMMON: return "Common";
            case Rarity::UNCOMMON: return "Uncommon";
            case Rarity::RARE: return "Rare";
            case Rarity::EPIC: return "Epic";
            case Rarity::LEGENDARY: return "Legendary";
            case Rarity::MYTHIC: return "Mythic";
            default: return "Unknown";
        }
    }
    
    void Draw(Vector2 position, float rotation = 0.0f) {
        // Draw weapon based on type
        switch (type) {
            case WeaponType::SWORD:
                DrawLineEx(position, {position.x + 40, position.y}, 5, weaponColor);
                DrawLineEx({position.x + 40, position.y - 10}, {position.x + 40, position.y + 10}, 3, GRAY);
                break;
            case WeaponType::AXE:
                DrawLineEx(position, {position.x + 40, position.y}, 5, BROWN);
                DrawCircleV({position.x + 40, position.y}, 12, weaponColor);
                break;
            case WeaponType::BOW:
                DrawCircleLines(position.x, position.y, 25, BROWN);
                DrawLineEx({position.x, position.y - 25}, {position.x, position.y + 25}, 3, WHITE);
                break;
            case WeaponType::STAFF:
                DrawLineEx(position, {position.x, position.y - 50}, 5, BROWN);
                DrawCircleV({position.x, position.y - 50}, 8, weaponColor);
                break;
            default:
                DrawCircleV(position, 15, weaponColor);
                break;
        }
    }
};

// Armor class
class Armor {
public:
    std::string name;
    ArmorType type;
    Rarity rarity;
    float defense;
    float magicResistance;
    float healthBonus;
    float manaBonus;
    float speedBonus;
    int level;
    int durability;
    int maxDurability;
    std::vector<std::string> enchantments;
    std::map<std::string, float> statBonuses;
    Color armorColor;
    
    Armor(const std::string& armorName, ArmorType armorType, Rarity armorRarity, int armorLevel)
        : name(armorName), type(armorType), rarity(armorRarity),
          defense(5), magicResistance(5), healthBonus(0), manaBonus(0),
          speedBonus(0), level(armorLevel), durability(100), maxDurability(100),
          armorColor(WHITE) {
        
        // Set base stats based on type
        switch (type) {
            case ArmorType::HELMET:
                defense = 10;
                magicResistance = 5;
                break;
            case ArmorType::CHESTPLATE:
                defense = 25;
                magicResistance = 10;
                healthBonus = 50;
                break;
            case ArmorType::LEGGINGS:
                defense = 15;
                magicResistance = 5;
                break;
            case ArmorType::BOOTS:
                defense = 8;
                magicResistance = 3;
                speedBonus = 10;
                break;
            case ArmorType::SHIELD:
                defense = 20;
                magicResistance = 15;
                break;
            case ArmorType::GAUNTLETS:
                defense = 8;
                magicResistance = 5;
                break;
            case ArmorType::CLOAK:
                defense = 5;
                magicResistance = 10;
                break;
            case ArmorType::RING:
                defense = 3;
                magicResistance = 3;
                break;
            case ArmorType::AMULET:
                defense = 3;
                magicResistance = 8;
                break;
            case ArmorType::BELT:
                defense = 5;
                magicResistance = 3;
                break;
        }
        
        // Apply rarity multipliers
        ApplyRarityModifiers();
        
        // Apply level scaling
        ScaleForLevel();
        
        // Set armor color based on rarity
        armorColor = GetRarityColor(rarity);
        
        // Generate enchantments for rare+ armor
        if (rarity >= Rarity::RARE) {
            GenerateEnchantments();
        }
    }
    
    void ApplyRarityModifiers() {
        float multiplier = 1.0f;
        
        switch (rarity) {
            case Rarity::COMMON: multiplier = 1.0f; break;
            case Rarity::UNCOMMON: multiplier = 1.2f; break;
            case Rarity::RARE: multiplier = 1.5f; break;
            case Rarity::EPIC: multiplier = 2.0f; break;
            case Rarity::LEGENDARY: multiplier = 3.0f; break;
            case Rarity::MYTHIC: multiplier = 5.0f; break;
        }
        
        defense *= multiplier;
        magicResistance *= multiplier;
        healthBonus *= multiplier;
        manaBonus *= multiplier;
        speedBonus *= multiplier;
    }
    
    void ScaleForLevel() {
        float levelMultiplier = 1.0f + (level - 1) * 0.1f;
        defense *= levelMultiplier;
        magicResistance *= levelMultiplier;
        healthBonus *= levelMultiplier;
        manaBonus *= levelMultiplier;
        maxDurability = 100 + (level - 1) * 20;
        durability = maxDurability;
    }
    
    void GenerateEnchantments() {
        std::vector<std::string> possibleEnchantments = {
            "Fire Resistance", "Ice Resistance", "Lightning Resistance",
            "Health Regeneration", "Mana Regeneration", "Thorns",
            "Magic Shield", "Fortified", "Swiftness", "Vitality"
        };
        
        int numEnchantments = static_cast<int>(rarity) - 2;
        
        for (int i = 0; i < numEnchantments && i < possibleEnchantments.size(); i++) {
            enchantments.push_back(possibleEnchantments[GetRandomValue(0, possibleEnchantments.size() - 1)]);
            
            // Add stat bonus based on enchantment
            std::string enchantment = enchantments.back();
            if (enchantment == "Health Regeneration") {
                statBonuses["health_regen"] += 5;
            } else if (enchantment == "Thorns") {
                statBonuses["thorns_damage"] += 10;
            } else if (enchantment == "Fortified") {
                statBonuses["defense_bonus"] += 10;
            }
        }
    }
    
    Color GetRarityColor(Rarity rarity) {
        switch (rarity) {
            case Rarity::COMMON: return GRAY;
            case Rarity::UNCOMMON: return GREEN;
            case Rarity::RARE: return BLUE;
            case Rarity::EPIC: return PURPLE;
            case Rarity::LEGENDARY: return ORANGE;
            case Rarity::MYTHIC: return RED;
            default: return WHITE;
        }
    }
    
    std::string GetRarityName() const {
        switch (rarity) {
            case Rarity::COMMON: return "Common";
            case Rarity::UNCOMMON: return "Uncommon";
            case Rarity::RARE: return "Rare";
            case Rarity::EPIC: return "Epic";
            case Rarity::LEGENDARY: return "Legendary";
            case Rarity::MYTHIC: return "Mythic";
            default: return "Unknown";
        }
    }
    
    void Repair(int amount) {
        durability = std::min(maxDurability, durability + amount);
    }
    
    void Draw(Vector2 position) {
        // Draw armor piece
        DrawCircleV(position, 15, armorColor);
        DrawCircleV(position, 5, ColorAlpha(WHITE, 0.5f));
    }
};

// Character equipment class
class CharacterEquipment {
public:
    std::map<ArmorType, std::shared_ptr<Armor>> armorSlots;
    std::shared_ptr<Weapon> equippedWeapon;
    std::vector<std::shared_ptr<Weapon>> weaponInventory;
    std::vector<std::shared_ptr<Armor>> armorInventory;
    
    CharacterEquipment() {
        // Initialize empty armor slots
        armorSlots[ArmorType::HELMET] = nullptr;
        armorSlots[ArmorType::CHESTPLATE] = nullptr;
        armorSlots[ArmorType::LEGGINGS] = nullptr;
        armorSlots[ArmorType::BOOTS] = nullptr;
        armorSlots[ArmorType::SHIELD] = nullptr;
        armorSlots[ArmorType::GAUNTLETS] = nullptr;
        armorSlots[ArmorType::CLOAK] = nullptr;
        armorSlots[ArmorType::RING] = nullptr;
        armorSlots[ArmorType::AMULET] = nullptr;
        armorSlots[ArmorType::BELT] = nullptr;
    }
    
    void EquipWeapon(std::shared_ptr<Weapon> weapon) {
        if (equippedWeapon) {
            weaponInventory.push_back(equippedWeapon);
        }
        equippedWeapon = weapon;
    }
    
    void EquipArmor(std::shared_ptr<Armor> armor) {
        if (armorSlots[armor->type]) {
            armorInventory.push_back(armorSlots[armor->type]);
        }
        armorSlots[armor->type] = armor;
    }
    
    float GetTotalDefense() {
        float total = 0;
        for (const auto& [slot, armor] : armorSlots) {
            if (armor) {
                total += armor->defense;
                if (armor->statBonuses.find("defense_bonus") != armor->statBonuses.end()) {
                    total += armor->statBonuses["defense_bonus"];
                }
            }
        }
        return total;
    }
    
    float GetTotalMagicResistance() {
        float total = 0;
        for (const auto& [slot, armor] : armorSlots) {
            if (armor) {
                total += armor->magicResistance;
            }
        }
        return total;
    }
    
    float GetTotalHealthBonus() {
        float total = 0;
        for (const auto& [slot, armor] : armorSlots) {
            if (armor) {
                total += armor->healthBonus;
            }
        }
        return total;
    }
    
    float GetTotalManaBonus() {
        float total = 0;
        for (const auto& [slot, armor] : armorSlots) {
            if (armor) {
                total += armor->manaBonus;
            }
        }
        return total;
    }
    
    float GetTotalSpeedBonus() {
        float total = 0;
        for (const auto& [slot, armor] : armorSlots) {
            if (armor) {
                total += armor->speedBonus;
            }
        }
        return total;
    }
};

// Game character class with equipment
class GameCharacter {
public:
    std::string name;
    Vector2 position;
    Color color;
    float health;
    float maxHealth;
    float mana;
    float maxMana;
    float baseStrength;
    float baseDefense;
    int level;
    CharacterEquipment equipment;
    bool isAlive;
    
    GameCharacter(const std::string& characterName, Vector2 pos, Color characterColor, int characterLevel)
        : name(characterName), position(pos), color(characterColor),
          health(100), maxHealth(100), mana(50), maxMana(50),
          baseStrength(10), baseDefense(5), level(characterLevel), isAlive(true) {
        
        // Scale stats with level
        maxHealth = 100 + (level - 1) * 20;
        health = maxHealth;
        maxMana = 50 + (level - 1) * 10;
        mana = maxMana;
        baseStrength = 10 + (level - 1) * 2;
        baseDefense = 5 + (level - 1);
    }
    
    void EquipWeapon(std::shared_ptr<Weapon> weapon) {
        equipment.EquipWeapon(weapon);
    }
    
    void EquipArmor(std::shared_ptr<Armor> armor) {
        equipment.EquipArmor(armor);
        
        // Update max health with armor bonus
        maxHealth = 100 + (level - 1) * 20 + equipment.GetTotalHealthBonus();
        health = std::min(health, maxHealth);
    }
    
    float GetAttackDamage() {
        if (equipment.equippedWeapon) {
            return equipment.equippedWeapon->CalculateDamage(baseStrength);
        }
        return baseStrength;
    }
    
    float GetDefense() {
        return baseDefense + equipment.GetTotalDefense();
    }
    
    void TakeDamage(float damage) {
        float actualDamage = std::max(0.0f, damage - GetDefense() * 0.5f);
        health -= actualDamage;
        
        if (health <= 0) {
            health = 0;
            isAlive = false;
        }
    }
    
    void Attack(GameCharacter* target) {
        if (!isAlive || !target || !target->isAlive) return;
        
        float damage = GetAttackDamage();
        target->TakeDamage(damage);
        
        // Reduce weapon durability
        if (equipment.equippedWeapon) {
            equipment.equippedWeapon->TakeDamage(1);
            if (equipment.equippedWeapon->IsBroken()) {
                equipment.equippedWeapon = nullptr;
            }
        }
    }
    
    void Draw() {
        if (!isAlive) return;
        
        // Draw character
        DrawCircleV(position, 20, color);
        DrawCircleV(position, 8, WHITE);
        
        // Draw name
        DrawText(name.c_str(), position.x - 30, position.y - 50, 12, WHITE);
        
        // Draw health bar
        DrawRectangle(position.x - 25, position.y - 40, 50, 8, DARKGRAY);
        DrawRectangle(position.x - 25, position.y - 40, 50 * (health / maxHealth), 8, RED);
        
        // Draw mana bar
        DrawRectangle(position.x - 25, position.y - 30, 50, 8, DARKGRAY);
        DrawRectangle(position.x - 25, position.y - 30, 50 * (mana / maxMana), 8, BLUE);
        
        // Draw equipped weapon
        if (equipment.equippedWeapon) {
            equipment.equippedWeapon->Draw({position.x + 25, position.y});
        }
        
        // Draw equipment indicator
        int equippedCount = 0;
        for (const auto& [slot, armor] : equipment.armorSlots) {
            if (armor) equippedCount++;
        }
        if (equippedCount > 0) {
            DrawText(TextFormat("+%d", equippedCount), position.x + 20, position.y - 20, 10, GREEN);
        }
    }
    
    void DrawInventory(int x, int y) {
        DrawRectangle(x, y, 400, 300, ColorAlpha(BLACK, 0.8f));
        DrawRectangleLines(x, y, 400, 300, GOLD);
        
        DrawText("INVENTORY", x + 10, y + 10, 16, YELLOW);
        
        // Draw equipped weapon
        DrawText("Weapon:", x + 10, y + 40, 14, WHITE);
        if (equipment.equippedWeapon) {
            DrawText(equipment.equippedWeapon->name.c_str(), x + 80, y + 40, 14, 
                    equipment.equippedWeapon->weaponColor);
            DrawText(TextFormat("DMG: %.0f", equipment.equippedWeapon->baseDamage), 
                    x + 80, y + 60, 12, RED);
            DrawText(TextFormat("Durability: %d/%d", 
                    equipment.equippedWeapon->durability, 
                    equipment.equippedWeapon->maxDurability), 
                    x + 80, y + 80, 12, WHITE);
        } else {
            DrawText("None", x + 80, y + 40, 14, GRAY);
        }
        
        // Draw armor slots
        int armorY = y + 110;
        DrawText("Armor:", x + 10, armorY, 14, WHITE);
        
        for (const auto& [slot, armor] : equipment.armorSlots) {
            armorY += 20;
            std::string slotName;
            switch (slot) {
                case ArmorType::HELMET: slotName = "Helmet"; break;
                case ArmorType::CHESTPLATE: slotName = "Chest"; break;
                case ArmorType::LEGGINGS: slotName = "Legs"; break;
                case ArmorType::BOOTS: slotName = "Boots"; break;
                case ArmorType::SHIELD: slotName = "Shield"; break;
                default: continue;
            }
            
            if (armor) {
                DrawText(TextFormat("%s: %s (DEF: %.0f)", slotName.c_str(), 
                        armor->name.c_str(), armor->defense), 
                        x + 10, armorY, 12, armor->armorColor);
            } else {
                DrawText(TextFormat("%s: Empty", slotName.c_str()), 
                        x + 10, armorY, 12, GRAY);
            }
        }
        
        // Draw stats summary
        DrawText("STATS:", x + 250, y + 40, 14, WHITE);
        DrawText(TextFormat("Damage: %.0f", GetAttackDamage()), x + 250, y + 60, 12, RED);
        DrawText(TextFormat("Defense: %.0f", GetDefense()), x + 250, y + 80, 12, BLUE);
        DrawText(TextFormat("Magic Res: %.0f", equipment.GetTotalMagicResistance()), 
                x + 250, y + 100, 12, PURPLE);
    }
};

// Loot generator
class LootGenerator {
private:
    std::mt19937 rng;
    
public:
    LootGenerator() : rng(std::random_device{}()) {}
    
    std::shared_ptr<Weapon> GenerateWeapon(int level) {
        std::uniform_int_distribution<int> typeDist(0, 9);
        std::uniform_int_distribution<int> rarityDist(0, 100);
        
        WeaponType type = static_cast<WeaponType>(typeDist(rng));
        Rarity rarity;
        
        int rarityRoll = rarityDist(rng);
        if (rarityRoll < 40) rarity = Rarity::COMMON;
        else if (rarityRoll < 70) rarity = Rarity::UNCOMMON;
        else if (rarityRoll < 85) rarity = Rarity::RARE;
        else if (rarityRoll < 95) rarity = Rarity::EPIC;
        else if (rarityRoll < 99) rarity = Rarity::LEGENDARY;
        else rarity = Rarity::MYTHIC;
        
        std::string weaponName = GenerateWeaponName(type, rarity);
        return std::make_shared<Weapon>(weaponName, type, rarity, level);
    }
    
    std::shared_ptr<Armor> GenerateArmor(int level) {
        std::uniform_int_distribution<int> typeDist(0, 9);
        std::uniform_int_distribution<int> rarityDist(0, 100);
        
        ArmorType type = static_cast<ArmorType>(typeDist(rng));
        Rarity rarity;
        
        int rarityRoll = rarityDist(rng);
        if (rarityRoll < 40) rarity = Rarity::COMMON;
        else if (rarityRoll < 70) rarity = Rarity::UNCOMMON;
        else if (rarityRoll < 85) rarity = Rarity::RARE;
        else if (rarityRoll < 95) rarity = Rarity::EPIC;
        else if (rarityRoll < 99) rarity = Rarity::LEGENDARY;
        else rarity = Rarity::MYTHIC;
        
        std::string armorName = GenerateArmorName(type, rarity);
        return std::make_shared<Armor>(armorName, type, rarity, level);
    }
    
    std::string GenerateWeaponName(WeaponType type, Rarity rarity) {
        std::vector<std::string> prefixes = {
            "Iron", "Steel", "Ancient", "Dragon", "Shadow", "Holy", "Cursed",
            "Frost", "Flame", "Thunder", "Venom", "Celestial", "Demonic", "Mystic"
        };
        
        std::vector<std::string> suffixes = {
            "of Power", "of Destruction", "of the Ancients", "of Shadows",
            "of Light", "of Fury", "of Wisdom", "of the Dragon"
        };
        
        std::string baseName;
        switch (type) {
            case WeaponType::SWORD: baseName = "Sword"; break;
            case WeaponType::AXE: baseName = "Axe"; break;
            case WeaponType::BOW: baseName = "Bow"; break;
            case WeaponType::STAFF: baseName = "Staff"; break;
            case WeaponType::DAGGER: baseName = "Dagger"; break;
            case WeaponType::SPEAR: baseName = "Spear"; break;
            case WeaponType::HAMMER: baseName = "Hammer"; break;
            case WeaponType::WAND: baseName = "Wand"; break;
            case WeaponType::CROSSBOW: baseName = "Crossbow"; break;
            case WeaponType::SCYTHE: baseName = "Scythe"; break;
        }
        
        std::uniform_int_distribution<int> prefixDist(0, prefixes.size() - 1);
        std::uniform_int_distribution<int> suffixDist(0, suffixes.size() - 1);
        
        return prefixes[prefixDist(rng)] + " " + baseName + " " + suffixes[suffixDist(rng)];
    }
    
    std::string GenerateArmorName(ArmorType type, Rarity rarity) {
        std::vector<std::string> prefixes = {
            "Iron", "Steel", "Ancient", "Dragon", "Shadow", "Holy", "Cursed",
            "Frost", "Flame", "Thunder", "Venom", "Celestial", "Demonic", "Mystic"
        };
        
        std::string baseName;
        switch (type) {
            case ArmorType::HELMET: baseName = "Helmet"; break;
            case ArmorType::CHESTPLATE: baseName = "Chestplate"; break;
            case ArmorType::LEGGINGS: baseName = "Leggings"; break;
            case ArmorType::BOOTS: baseName = "Boots"; break;
            case ArmorType::SHIELD: baseName = "Shield"; break;
            case ArmorType::GAUNTLETS: baseName = "Gauntlets"; break;
            case ArmorType::CLOAK: baseName = "Cloak"; break;
            case ArmorType::RING: baseName = "Ring"; break;
            case ArmorType::AMULET: baseName = "Amulet"; break;
            case ArmorType::BELT: baseName = "Belt"; break;
        }
        
        std::uniform_int_distribution<int> prefixDist(0, prefixes.size() - 1);
        return prefixes[prefixDist(rng)] + " " + baseName;
    }
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Weapon and Armor System");
    SetTargetFPS(60);
    
    // Create loot generator
    LootGenerator lootGen;
    
    // Create player character
    GameCharacter player("Hero", {SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2}, BLUE, 1);
    
    // Create NPC characters
    GameCharacter npc1("Warrior", {SCREEN_WIDTH/2 + 100, SCREEN_HEIGHT/2 - 100}, RED, 3);
    GameCharacter npc2("Mage", {SCREEN_WIDTH/2 + 100, SCREEN_HEIGHT/2 + 100}, PURPLE, 3);
    GameCharacter npc3("Rogue", {SCREEN_WIDTH/2 + 300, SCREEN_HEIGHT/2}, GREEN, 3);
    
    // Equip NPCs with random gear
    for (int i = 0; i < 5; i++) {
        npc1.EquipWeapon(lootGen.GenerateWeapon(3));
        npc2.EquipWeapon(lootGen.GenerateWeapon(3));
        npc3.EquipWeapon(lootGen.GenerateWeapon(3));
        
        npc1.EquipArmor(lootGen.GenerateArmor(3));
        npc2.EquipArmor(lootGen.GenerateArmor(3));
        npc3.EquipArmor(lootGen.GenerateArmor(3));
    }
    
    // Game loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Player movement
        float speed = 200.0f;
        if (IsKeyDown(KEY_A)) player.position.x -= speed * dt;
        if (IsKeyDown(KEY_D)) player.position.x += speed * dt;
        if (IsKeyDown(KEY_W)) player.position.y -= speed * dt;
        if (IsKeyDown(KEY_S)) player.position.y += speed * dt;
        
        // Attack
        if (IsKeyPressed(KEY_SPACE)) {
            // Attack nearest NPC
            GameCharacter* nearest = nullptr;
            float minDist = 100;
            
            for (auto* npc : {&npc1, &npc2, &npc3}) {
                float dist = Vector2Distance(player.position, npc->position);
                if (dist < minDist && npc->isAlive) {
                    minDist = dist;
                    nearest = npc;
                }
            }
            
            if (nearest) {
                player.Attack(nearest);
            }
        }
        
        // Generate loot
        if (IsKeyPressed(KEY_L)) {
            auto weapon = lootGen.GenerateWeapon(player.level);
            player.EquipWeapon(weapon);
            
            auto armor = lootGen.GenerateArmor(player.level);
            player.EquipArmor(armor);
        }
        
        // Draw
        BeginDrawing();
        ClearBackground(Color{30, 30, 50, 255});
        
        // Draw title
        DrawText("Weapon and Armor System", SCREEN_WIDTH/2 - 150, 20, 30, WHITE);
        
        // Draw characters
        player.Draw();
        npc1.Draw();
        npc2.Draw();
        npc3.Draw();
        
        // Draw inventory
        player.DrawInventory(10, SCREEN_HEIGHT - 320);
        
        // Draw controls
        DrawText("Controls:", SCREEN_WIDTH - 200, 20, 16, WHITE);
        DrawText("WASD: Move", SCREEN_WIDTH - 200, 45, 14, GRAY);
        DrawText("SPACE: Attack", SCREEN_WIDTH - 200, 65, 14, GRAY);
        DrawText("L: Generate Loot", SCREEN_WIDTH - 200, 85, 14, GRAY);
        
        // Draw NPC information
        DrawText("NPC Equipment:", 10, 10, 16, YELLOW);
        DrawText(TextFormat("%s: %s", npc1.name.c_str(), 
                npc1.equipment.equippedWeapon ? npc1.equipment.equippedWeapon->name.c_str() : "None"), 
                10, 35, 12, WHITE);
        DrawText(TextFormat("%s: %s", npc2.name.c_str(), 
                npc2.equipment.equippedWeapon ? npc2.equipment.equippedWeapon->name.c_str() : "None"), 
                10, 55, 12, WHITE);
        DrawText(TextFormat("%s: %s", npc3.name.c_str(), 
                npc3.equipment.equippedWeapon ? npc3.equipment.equippedWeapon->name.c_str() : "None"), 
                10, 75, 12, WHITE);
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}