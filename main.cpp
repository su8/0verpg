/*
 * Copyright 10/13/2026 https://github.com/su8/0verpg
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <random>
#include <fstream>
#include <algorithm>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <cstdio>
#include <termios.h>
#include <unistd.h>
#endif /* _WIN32 */

#include "json.hpp"

#define RESETCOLOUR "\033[0;0m"
#define YELLOW "\033[1;33m"
#define GREEN "\033[1;32m"
#define CYAN "\033[1;36m"
#define RED "\033[1;31m"
#define MAGENTA "\033[1;35m"
#define BLUE "\033[1;34m"

// Armour structure
struct Armour {
  std::string name;
  int defense;
};

// Weapon structure
struct Weapon {
  std::string name;
  int attack;
  int cost;
};

// Potion structure
struct Potion {
  std::string name;
  int healAmount;
  int cost;
};

// Enemy structure
struct Enemy {
  std::string name;
  int attack;
  int health;
  int xpReward;
};

// Spell structure
struct Spell {
  std::string name;
  int attack;
  int manaCost;
};

// Companion structure
struct Companion {
  std::string name;
  int attack;
  int health;
  int maxHP;
};

// Hero structure
struct Hero {
  std::string name;
  int health;
  int baseDefense;
  int baseAttack;
  int mana;
  int maxMana;
  Armour equippedArmour;
  Weapon equippedWeapon;
  Spell equippedSpell;
  Companion companion;
  std::vector<Armour> inventoryArmours;
  std::vector<Weapon> inventoryWeapons;
  std::vector<Potion> inventoryPotions;
  std::vector<Spell> inventorySpells;
  int gold;
  int x, y;
  int xp;
  int level;
  int maxHP;
  int weaponAttackDecreased;
};

// All of the available characters/heros
struct Character {
  std::string name;
  int health;
  int baseDefense;
  int baseAttack;
  int mana;
  int maxMana;
};

// Configurable input game keys that get established when the game starts
int keys[8];
// readKey() codes for the arrow keys, the same on every platform
enum ArrowKey { KEY_UP = 1001, KEY_DOWN, KEY_RIGHT, KEY_LEFT };

// These 3 variables are used to retrieve the configuration file and setup the used language from "argv[1]" - if there is any input to request the game language
// The config in the current directory wins, so the game runs straight from the repository; otherwise use the one in HOME
std::string configHome = std::ifstream(".0verpg.json") ? std::string(".0verpg.json") : (std::getenv("HOME") ? std::string(std::getenv("HOME")) + std::string("/") : std::string("./")) + ".0verpg.json";
std::string userLang = "en";
std::ifstream cfg(configHome);
// Inventory weapons/armours colours for their names
const char *coloursArr[] = { BLUE, MAGENTA, GREEN, RED, YELLOW, CYAN };
const int coloursArrLen = sizeof(coloursArr) / sizeof(coloursArr[0]);

using json = nlohmann::json;
json config;
#define CONF config[userLang]

// Equip armour from inventory
void equipArmour(Hero &hero);
// Equip weapon
void equipWeapon(Hero &hero, std::vector<Weapon> &weapons, int index, int usedFromWebShop);
// Equip spell from inventory
void equipSpell(Hero &hero);
// Battle simulation
bool battle(Hero &hero, Enemy enemy);
// Loot room
void lootRoom(Hero &hero, std::vector<Armour> &armours, std::vector<Weapon> &weapons, std::vector<Potion> &potionsList,  std::vector<Spell> &spells);
// Use potion
void usePotion(Hero &hero);
// Trap room
bool trapRoom(Hero &hero);
// Treasure chest room
void chestRoom(Hero &hero, std::vector<Armour> &armours);
// Explore room
bool exploreRoom(Hero &hero, std::vector<Enemy> &enemies, std::vector<Armour> &armours, std::vector<Weapon> &weapons,char &cell, std::vector<Potion> &potionsList, std::vector<Spell> &spells);
// Shop system
void shop(Hero &hero, std::vector<Armour> &armours, std::vector<Potion> &potionsList);
// Weapon shop system
void weaponShop(Hero &hero, std::vector<Weapon> &weapons);
// Potion shop system
void potionShop(Hero &hero, std::vector<Potion> &potionsList);
// Generate map with enemies, loot, traps, chests, and boss
std::vector<std::vector<char>> generateMap(int size);
// Display map with fog of war
void displayMap(const std::vector<std::vector<char>> &map, const std::vector<std::vector<bool>> &visited, const Hero &hero);
// Gain XP and check for level up
void gainXP(Hero &her, int amount);
// On startup setup the movement and inventories keys
int readKey(void);
void setupKeys(void);
std::string keyName(int key);
std::string trim(std::string str);
// Read a menu number
int readNumber(void);

int play(int argc, char *argv[]) {
#ifdef _WIN32
  SetConsoleTitle("0verpg");
#endif /*_WIN32 */
  std::srand(static_cast<unsigned>(std::time(nullptr)));
  if (argc >= 2) { userLang = std::string(argv[1]); }
  unsigned int foundRequestedLanguage = 0U;
  if (userLang == "en" || userLang == "bg" || userLang == "it") { foundRequestedLanguage = 1U; }
  if (foundRequestedLanguage == 0U) { std::cerr << "Error the given language " << userLang << " does not exists. Exiting..." << std::endl; return EXIT_FAILURE; }
  if (!cfg) { std::cerr << "Error: Could not open " << configHome << "\n"; return EXIT_FAILURE; }
  cfg >> config;
  cfg.close();
  setupKeys();
  int classChoice = 0;
  // All of the available character classes
  std::vector<Character> characters = {
    {.name = trim(CONF["warrior"]), .health = 60, .baseDefense = 5,  .baseAttack = 5, .mana = 60, .maxMana = 60},
    {.name = trim(CONF["mage"]),    .health = 90, .baseDefense = 10, .baseAttack = 1, .mana = 90, .maxMana = 90},
    {.name = trim(CONF["rogue"]),   .health = 70, .baseDefense = 7,  .baseAttack = 3, .mana = 70, .maxMana = 70},
    {.name = trim(CONF["paladin"]), .health = 75, .baseDefense = 6,  .baseAttack = 2, .mana = 75, .maxMana = 75}
  };
  while (classChoice < 1 || classChoice > static_cast<int>(characters.size())) {
    std::cout << GREEN << trim(CONF["chooseClass"]) << RESETCOLOUR << ":\n";
    for (unsigned int indexNum = 0U; indexNum < characters.size(); indexNum++) {
      std::cout << YELLOW << indexNum + 1 << ". " << RESETCOLOUR << BLUE << characters[indexNum].name << RESETCOLOUR << "\n";
    }
    classChoice = readNumber();
  }
  classChoice--;
  std::cout << BLUE << trim(CONF["classChosen"]) << " " << characters[classChoice].name << RESETCOLOUR << std::endl;
  Hero hero;
  hero.name = characters[classChoice].name;
  hero.health = characters[classChoice].health;
  hero.maxHP = hero.health;
  hero.mana = characters[classChoice].mana;
  hero.maxMana = characters[classChoice].mana;
  hero.weaponAttackDecreased = 0;
  hero.baseDefense = characters[classChoice].baseDefense;
  hero.baseAttack = characters[classChoice].baseAttack;
  hero.equippedArmour = {.name = trim(CONF["none"]), .defense = 0};
  hero.equippedWeapon = {.name = trim(CONF["fists"]), .attack = 0, .cost = 0};
  hero.equippedSpell = {.name = trim(CONF["basicSpell"]), .attack = 0, .manaCost = 0};
  hero.companion = {.name = trim(CONF["companion"]), .attack = 2, .health = 100, .maxHP = 100};
  hero.gold = 50;
  hero.x = 0;
  hero.y = 0;
  hero.level = 1;
  hero.xp = 0;
  std::vector<Armour> armours;
  std::vector<Weapon> weapons;
  std::vector<Spell> spells;
  armours.reserve(4);
  weapons.reserve(4);
  spells.reserve(4);
  // Pick the gear by the chosen class number, as the class names are translated
  bool isMage = classChoice == 1, isRogue = classChoice == 2;
  if (classChoice == 0 || isMage) {
    // Armours Warrior, Mage
    armours = {
      {.name = isMage ? trim(CONF["mageArmour1"]) : trim(CONF["armour1"]), .defense = 5},
      {.name = isMage ? trim(CONF["mageArmour2"]) : trim(CONF["armour2"]), .defense = 10},
      {.name = isMage ? trim(CONF["mageArmour3"]) : trim(CONF["armour3"]), .defense = 15},
      {.name = isMage ? trim(CONF["mageArmour4"]) : trim(CONF["armour4"]), .defense = 20}
    };
    // Weapons Warrior, Mage
    weapons = {
      {.name = isMage ? trim(CONF["mageWeapon1"]) : trim(CONF["weapon1"]), .attack = 5, .cost = 20},
      {.name = isMage ? trim(CONF["mageWeapon2"]) : trim(CONF["weapon2"]), .attack = 10, .cost = 40},
      {.name = isMage ? trim(CONF["mageWeapon3"]) : trim(CONF["weapon3"]), .attack = 15, .cost = 60},
      {.name = isMage ? trim(CONF["mageWeapon4"]) : trim(CONF["weapon4"]), .attack = 25, .cost = 100}
    };
    // Spells Warrior, Mage
    spells = {
      {.name = isMage ? trim(CONF["mageSpell1"]) : trim(CONF["warriorSpell1"]), .attack = 5, .manaCost = 0},
      {.name = isMage ? trim(CONF["mageSpell2"]) : trim(CONF["warriorSpell2"]), .attack = 10, .manaCost = 10},
      {.name = isMage ? trim(CONF["mageSpell3"]) : trim(CONF["warriorSpell3"]), .attack = 15, .manaCost = 15},
      {.name = isMage ? trim(CONF["mageSpell4"]) : trim(CONF["warriorSpell4"]), .attack = 25, .manaCost = 20}
    };
  }
  else {
      // Armours Rogue, Paladin
    armours = {
      {.name = isRogue ? trim(CONF["rogueArmour1"]) : trim(CONF["paladinArmour1"]), .defense = 5},
      {.name = isRogue ? trim(CONF["rogueArmour2"]) : trim(CONF["paladinArmour2"]), .defense = 10},
      {.name = isRogue ? trim(CONF["rogueArmour3"]) : trim(CONF["paladinArmour3"]), .defense = 15},
      {.name = isRogue ? trim(CONF["rogueArmour4"]) : trim(CONF["paladinArmour4"]), .defense = 20}
    };
    // Weapons Rogue, Paladin
    weapons = {
      {.name = isRogue ? trim(CONF["rogueWeapon1"]) : trim(CONF["paladinWeapon1"]), .attack = 5, .cost = 20},
      {.name = isRogue ? trim(CONF["rogueWeapon2"]) : trim(CONF["paladinWeapon2"]), .attack = 10, .cost = 40},
      {.name = isRogue ? trim(CONF["rogueWeapon3"]) : trim(CONF["paladinWeapon3"]), .attack = 15, .cost = 60},
      {.name = isRogue ? trim(CONF["rogueWeapon4"]) : trim(CONF["paladinWeapon4"]), .attack = 25, .cost = 100}
    };
    // Spells Rogue, Paladin
    spells = {
      {.name = isRogue ? trim(CONF["rogueSpell1"]) : trim(CONF["paladinSpell1"]), .attack = 5, .manaCost = 0},
      {.name = isRogue ? trim(CONF["rogueSpell2"]) : trim(CONF["paladinSpell2"]), .attack = 10, .manaCost = 10},
      {.name = isRogue ? trim(CONF["rogueSpell3"]) : trim(CONF["paladinSpell3"]), .attack = 15, .manaCost = 15},
      {.name = isRogue ? trim(CONF["rogueSpell4"]) : trim(CONF["paladinSpell4"]), .attack = 25, .manaCost = 20}
    };
  }
  // Establish default and starting spell
  hero.inventorySpells.emplace_back(spells[0]);
  // Potions
  std::vector<Potion> potionsList = {
    {.name = trim(CONF["potion1"]), .healAmount = 20, .cost = 15},
    {.name = trim(CONF["potion2"]), .healAmount = 40, .cost = 30},
    {.name = trim(CONF["potion3"]), .healAmount = 60, .cost = 45}
  };
  // per level enemies
  std::vector<Enemy> enemies = {
    {.name = trim(CONF["enemy1"]), .attack = 8, .health = 30, .xpReward = 20},
    {.name = trim(CONF["enemy2"]), .attack = 12, .health = 40, .xpReward = 30},
    {.name = trim(CONF["enemy3"]), .attack = 15, .health = 50, .xpReward = 40}
  };
  std::string wKey = keyName(keys[1]);
  std::string dKey = keyName(keys[0]);
  std::string sKey = keyName(keys[3]);
  std::string aKey = keyName(keys[2]);
  std::string potionKey = keyName(keys[6]);
  std::string armourKey = keyName(keys[4]);
  std::string weaponKey = keyName(keys[5]);
  std::string spellKey = keyName(keys[7]);
  std::cout << "\n" << trim(CONF["chooseStartingArmour"]) << "\n";
  for (size_t i = 0; i < armours.size(); ++i) {
    std::cout << YELLOW << i+1 << ". " << RESETCOLOUR << MAGENTA << armours[i].name << CYAN << " (" << trim(CONF["defense"]) << " +" << RESETCOLOUR << YELLOW << armours[i].defense << RESETCOLOUR << CYAN << ")" << RESETCOLOUR << "\n";
  }
  int choice;
  choice = readNumber();
  if (choice >= 1 && choice <= static_cast<int>(armours.size())) {
    hero.equippedArmour = armours[choice-1];
    hero.inventoryArmours.emplace_back(armours[choice-1]);
  }
  std::cout << CYAN << hero.name << RESETCOLOUR << " " << trim(CONF["equipped"]) << " " << BLUE << hero.equippedArmour.name << " (" << trim(CONF["defense"]) << " +" << hero.equippedArmour.defense << ")" << RESETCOLOUR << "\n";
  int floors = 3;      // Number of dungeon floors
  int mapSize = 5;     // Map size (square grid)
  for (int floor = 1; floor <= floors; ++floor) {
    // Generate map and visited tracker
    std::vector<std::vector<char>> dungeon = generateMap(mapSize);
    std::vector<std::vector<bool>> visited(mapSize, std::vector<bool>(mapSize, false));
    hero.x = 0;
    hero.y = 0;
    visited[0][0] = true;
    bool floorCleared = false;
    weaponShop(hero, weapons);
    potionShop(hero, potionsList);
    std::cout << YELLOW << "\n🏰 " << trim(CONF["floor"]) << " " << floor << " " << trim(CONF["begins"]) << RESETCOLOUR << "\n";
    while (!floorCleared && hero.health > 0) {
      displayMap(dungeon, visited, hero);
      // Emojis from - www.emojishark.com
      std::cout << YELLOW << "\n💰 " << trim(CONF["gold"]) << ": " << hero.gold << RESETCOLOUR << " | " << MAGENTA << "💗 " << trim(CONF["hp"]) << ": " << hero.health << RESETCOLOUR
        << " | " << GREEN << "🧪 " << trim(CONF["potions"]) << ": " << hero.inventoryPotions.size() << RESETCOLOUR << " | "
        << BLUE << "♻️ " << trim(CONF["mana"]) << ": " << hero.mana << RESETCOLOUR <<  " | 💀" << CYAN << " " << trim(CONF["attack"]) << ": " << hero.baseAttack + hero.equippedWeapon.attack + hero.equippedSpell.attack << " " << trim(CONF["defense"]) << ": " << hero.baseDefense + hero.equippedArmour.defense << RESETCOLOUR
        << " | " << "🪄 " << MAGENTA << trim(CONF["spell"]) << ": " << hero.equippedSpell.name << RESETCOLOUR
        << " 👹 " << CYAN << hero.name << RESETCOLOUR << YELLOW << " " << trim(CONF["level"]) << ": " << RESETCOLOUR << MAGENTA << hero.level << RESETCOLOUR << ", " 
        << CYAN << hero.xp << "/" << hero.level * 50 << " " << trim(CONF["xp"]) << RESETCOLOUR << " | 🐶 " << RED << trim(CONF["companion"])
        << " " << std::max(0, hero.companion.health) << trim(CONF["hp"]) << RESETCOLOUR << "\n";
      std::cout << YELLOW << trim(CONF["move"]) <<  " (" << wKey << "/" <<  dKey << "/" <<  aKey << "/" <<  sKey << "), " <<  potionKey << " " << trim(CONF["usePotion"]) << ", "
        <<  armourKey << " " << trim(CONF["toChangeArmour"]) << ", " <<  weaponKey << " " << trim(CONF["toChangeWeapon"])
        << ", " <<  spellKey << " " << trim(CONF["toChangeSpell"]) << ", Q " << trim(CONF["toQuit"]) << ": " << RESETCOLOUR << std::endl;
      //std::cin >> move;
      int move = readKey();
      if (move < 0) { return EXIT_SUCCESS; } // stdin closed
      if (move < 256) { move = std::tolower(move); }
      if (move == 'q') { std::cout << MAGENTA << "🏁 " << trim(CONF["gameEnded"]) << RESETCOLOUR << "\n"; return EXIT_SUCCESS; }
      if (move == keys[6]) { usePotion(hero); continue; }
      if (move == keys[4]) { equipArmour(hero); continue; }
      if (move == keys[5]) { equipWeapon(hero, hero.inventoryWeapons, 1, 0); continue; }
      if (move == keys[7]) { equipSpell(hero); continue; }
      int newX = hero.x, newY = hero.y;
      if (move == keys[1]) newX--;
      else if (move == keys[3]) newX++;
      else if (move == keys[2]) newY--;
      else if (move == keys[0]) newY++;
      else continue; // unbound key: don't explore the current room again
      // Check boundaries
      if (newX < 0 || newX >= mapSize || newY < 0 || newY >= mapSize) { std::cout << RED << "❌ " << trim(CONF["cantMoveOutsideDungeon"]) << RESETCOLOUR << "\n"; continue; }
      hero.x = newX;
      hero.y = newY;
      visited[hero.x][hero.y] = true;
      // Explore the room
      if (!exploreRoom(hero, enemies, armours, weapons, dungeon[hero.x][hero.y], potionsList, spells)) { std::cout << RED << "\n💀 " << trim(CONF["gameOver"]) << RESETCOLOUR << "\n"; return EXIT_SUCCESS; }
      // Check if boss defeated
      bool bossAlive = false;
      for (int i = 0; i < mapSize; ++i) {
        for (int j = 0; j < mapSize; ++j) {
          if (dungeon[i][j] == 'B' || dungeon[i][j] == 'G' || dungeon[i][j] == 'Z') bossAlive = true;
        }
      }
      if (!bossAlive) {
        std::cout << GREEN << "\n✅ " << trim(CONF["floor"]) << " " << floor << " " << trim(CONF["cleared"]) << RESETCOLOUR << "\n";
        floorCleared = true;
        // Heal hero slightly after round
        hero.health += 15;
        if (hero.health > hero.maxHP) hero.health = hero.maxHP;
        std::cout << CYAN << hero.name << RESETCOLOUR << " " << trim(CONF["restsAndRecoversTo"]) << " " << GREEN << hero.health << RESETCOLOUR << " " << trim(CONF["hp"]) << "." << "\n";
      }
    }
    // Shop between floors
    if (floorCleared && floor < floors) { shop(hero, armours, potionsList); }
  }
  std::cout << GREEN << "\n🎉 " << trim(CONF["win"]) << RESETCOLOUR << "\n";
  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  // An old or hand-edited config is missing keys or isn't valid JSON
  try { return play(argc, argv); }
  catch (const json::exception &e) {
    std::cerr << "Error: " << configHome << " is outdated or broken, copy the new .0verpg.json from the repository (" << e.what() << ")\n";
    return EXIT_FAILURE;
  }
}

// Equip armour from inventory
void equipArmour(Hero &hero) {
  std::cout << MAGENTA << trim(CONF["curEquippedArmour"]) << ": " << RESETCOLOUR << GREEN << hero.equippedArmour.name << " (" << trim(CONF["defense"]) << " +" << hero.equippedArmour.defense << ")" << RESETCOLOUR << "\n";
  std::cout << trim(CONF["inventoryArmours"]) << ":\n";
  for (size_t i = 0; i < hero.inventoryArmours.size(); ++i) {
    std::cout << YELLOW << i + 1 << ". " << RESETCOLOUR << coloursArr[std::rand() % coloursArrLen] << hero.inventoryArmours[i].name << RESETCOLOUR
      << CYAN << " (" << trim(CONF["defense"]) << " +" << RESETCOLOUR << YELLOW << hero.inventoryArmours[i].defense << RESETCOLOUR << CYAN << ")" << RESETCOLOUR
      << BLUE << " (" << (hero.inventoryArmours[i].name == hero.equippedArmour.name ? trim(CONF["currentlyEquippedArmour"]) : trim(CONF["goodArmour"])) << ")" << RESETCOLOUR << "\n";
  }
  std::cout << trim(CONF["chooseArmourNumber"]) << ": ";
  int index;
  index = readNumber();
  index--;
  if (index < 0 || index >= static_cast<int>(hero.inventoryArmours.size())) { std::cout << trim(CONF["invalidArmourIndex"]) << ".\n"; return; }
  hero.equippedArmour = hero.inventoryArmours[index];
  std::cout << CYAN << hero.name << RESETCOLOUR << " " << trim(CONF["equipped"]) << " " << BLUE << hero.equippedArmour.name << " (" << trim(CONF["defense"]) << " +" << hero.equippedArmour.defense << ")" << RESETCOLOUR << "\n";
}

// Equip weapon from inventory
void equipWeapon(Hero &hero, std::vector<Weapon> &weapons, int index, int usedFromWebShop) {
  if (usedFromWebShop == 0) {
    std::cout << MAGENTA << trim(CONF["curEquippedWeapon"]) << ": " << RESETCOLOUR << GREEN << hero.equippedWeapon.name << " (" << trim(CONF["attack"]) << " +" << hero.equippedWeapon.attack << ")" << RESETCOLOUR << "\n";
    std::cout << trim(CONF["inventoryWeapons"]) << ":\n";
    for (size_t i = 0; i < weapons.size(); ++i) {
      std::cout << YELLOW << i + 1 << ". " << RESETCOLOUR << coloursArr[std::rand() % coloursArrLen] << weapons[i].name << RESETCOLOUR
        << CYAN << " (" << trim(CONF["attack"]) << " +" << RESETCOLOUR << YELLOW << weapons[i].attack << RESETCOLOUR << CYAN << ")" << RESETCOLOUR
        << BLUE << " (" << (weapons[i].name == hero.equippedWeapon.name ? trim(CONF["currentlyEquippedWeapon"]) : trim(CONF["goodWeapon"])) << ")" << RESETCOLOUR << "\n";
    }
    std::cout << trim(CONF["chooseWeaponNumber"]) << ": ";
    index = readNumber();
    index--;
  }
  if (index < 0 || index >= static_cast<int>(weapons.size())) { std::cout << trim(CONF["invalidWeaponIndex"]) << ".\n"; return; }
  hero.equippedWeapon = weapons[index];
  std::cout << CYAN << hero.name << RESETCOLOUR << " " <<  trim(CONF["equipped"]) << " " << BLUE << hero.equippedWeapon.name << " (" <<  trim(CONF["attack"]) << " +" << hero.equippedWeapon.attack << ")" << RESETCOLOUR << "\n";
}

// Equip spell from inventory
void equipSpell(Hero &hero) {
  std::cout << MAGENTA << trim(CONF["curEquippedSpell"]) << ": " << RESETCOLOUR << GREEN << hero.equippedSpell.name << " (+" << hero.equippedSpell.attack << " " << trim(CONF["attack"]) << " " << trim(CONF["manaCost"]) << " -" << hero.equippedSpell.manaCost << ")" << RESETCOLOUR << "\n";
  std::cout << trim(CONF["inventorySpells"]) << ":\n";
  for (size_t i = 0; i < hero.inventorySpells.size(); ++i) {
    std::cout << YELLOW << i+1 << ". " << RESETCOLOUR << coloursArr[std::rand() % coloursArrLen] << hero.inventorySpells[i].name << RESETCOLOUR
      << CYAN << " (" << trim(CONF["attack"]) << " +" << RESETCOLOUR << YELLOW << hero.inventorySpells[i].attack << RESETCOLOUR << " -"
      << YELLOW << hero.inventorySpells[i].manaCost << " " << trim(CONF["manaCost"]) << RESETCOLOUR  << CYAN << ")"
      << RESETCOLOUR << BLUE << " (" << (hero.inventorySpells[i].name == hero.equippedSpell.name ? trim(CONF["currentlyEquippedSpell"]) : trim(CONF["goodSpell"])) << ")" << RESETCOLOUR "\n";
  }
  std::cout << trim(CONF["chooseSpellNumber"]) << ": ";
  int index;
  index = readNumber();
  index--;
  if (index < 0 || index >= static_cast<int>(hero.inventorySpells.size())) { std::cout << trim(CONF["invalidSpellIndex"]) << ".\n"; return; }
  hero.equippedSpell = hero.inventorySpells[index];
  std::cout << CYAN << hero.name << RESETCOLOUR << " " << trim(CONF["equipped"]) << " " << BLUE << hero.equippedSpell.name
    << " (+" << trim(CONF["attack"]) << " " << hero.inventorySpells[index].attack << " " << trim(CONF["manaCost"]) << " -" << hero.equippedSpell.manaCost << ")" << RESETCOLOUR << "\n";
}

// Battle simulation
bool battle(Hero &hero, Enemy enemy) {
  std::cout << "\n⚔ " << trim(CONF["battle"]) << " " << CYAN << hero.name << RESETCOLOUR << " vs " << RED << enemy.name << RESETCOLOUR << "\n";
  while (hero.health > 0 && enemy.health > 0) {
    // Hero attacks
    int heroAttack = hero.baseAttack + hero.equippedWeapon.attack + hero.equippedSpell.attack;
    if(std::rand() % 100 < 20) { heroAttack *= 2; std::cout << YELLOW << trim(CONF["criticalHit"]) << "!" << RESETCOLOUR << "\n"; }
    enemy.health -= heroAttack;
    if (hero.companion.health > 0) { enemy.health -= hero.companion.attack; }
    int manaCost = 10 + hero.equippedSpell.manaCost;
    if (hero.mana - manaCost <= 0) { hero.mana = 0; }
    else {
      hero.mana -= manaCost;
      if (hero.weaponAttackDecreased == 1) { hero.equippedWeapon.attack += 3; hero.weaponAttackDecreased = 0; } // mana came back mid-battle
    }
    if (hero.mana == 0 && hero.weaponAttackDecreased == 0) { hero.equippedWeapon.attack -= 3; hero.weaponAttackDecreased = 1; }
    std::cout << CYAN << hero.name << RESETCOLOUR << " " << trim(CONF["attacksFor"]) << " " << YELLOW << heroAttack << RESETCOLOUR << " " << trim(CONF["damage"]) << "! "
      << BLUE << trim(CONF["companionAttacksFor"]) << " " << (hero.companion.health > 0 ? hero.companion.attack : 0) << " " << trim(CONF["damage"]) << " " << RESETCOLOUR
      << GREEN << (hero.mana > 0 ? trim(CONF["manaUsed"]) : trim(CONF["noManaUsed"])) << RESETCOLOUR << " | " << enemy.name << " " << trim(CONF["hp"])
      << ": " << RED << std::max(0, enemy.health) << RESETCOLOUR << "\n";
    if (enemy.health <= 0) {
      std::cout << MAGENTA << enemy.name << RESETCOLOUR << GREEN << " " << trim(CONF["isDefeated"]) << "!" << RESETCOLOUR << "\n";
      hero.gold += 30;
      std::cout << CYAN << hero.name << RESETCOLOUR << " " << trim(CONF["earns"]) << " " << YELLOW << "30" << RESETCOLOUR << " " << trim(CONF["goldTotalGold"]) << ": " << YELLOW << hero.gold << RESETCOLOUR << "\n";
      gainXP(hero, enemy.xpReward);
      if (hero.mana + 10 > hero.maxMana) { hero.mana = hero.maxMana; } else { hero.mana += 10; }
      if (hero.weaponAttackDecreased == 1) { hero.equippedWeapon.attack += 3; hero.weaponAttackDecreased = 0; }
      std::cout << BLUE << hero.name << RESETCOLOUR << " " << MAGENTA << trim(CONF["restoresMana"]) << RESETCOLOUR << "\n";
      return true;
    }
    // Enemy attacks
    int totalDefense = hero.baseDefense + hero.equippedArmour.defense;
    int damage = enemy.attack - totalDefense;
    if (damage < 0) damage = 0;
    if(std::rand() % 100 < 15) { std::cout << MAGENTA << hero.name << RESETCOLOUR << GREEN << " " << trim(CONF["dodged"]) << "!" << RESETCOLOUR << "\n"; damage = 0; }
    hero.health -= damage;
    bool companionWasAlive = hero.companion.health > 0;
    hero.companion.health = std::max(0, hero.companion.health - damage);
    std::cout << RED << enemy.name << RESETCOLOUR << " " << trim(CONF["attacksFor"]) << " " << RED << enemy.attack << RESETCOLOUR
      << " (" << trim(CONF["blocked"]) << " " << MAGENTA << totalDefense << RESETCOLOUR << ") " << trim(CONF["damageTaken"]) << ": " << YELLOW << damage
      << RESETCOLOUR << RED << " | " << trim(CONF["companion"]) << RESETCOLOUR << ": " << damage << YELLOW << " " << hero.name << RESETCOLOUR
      << " " << trim(CONF["hp"]) << ": " << GREEN << std::max(0, hero.health) << RESETCOLOUR << " | " << RED << trim(CONF["companion"]) << " "
      << std::max(0, hero.companion.health) << trim(CONF["hp"]) << RESETCOLOUR << "\n";
    // Auto-use potion if low HP
    if (hero.health <= 30 && hero.inventoryPotions.size() > 0) { std::cout << CYAN << "⚠ " << trim(CONF["autoUsingPotion"]) << "..." << RESETCOLOUR << "\n"; usePotion(hero); }
    if (companionWasAlive && hero.companion.health == 0) { std::cout << RED <<trim(CONF["companion"]) << " " << trim(CONF["hasFallen"]) << "..." << RESETCOLOUR << "\n"; }
    if (hero.health <= 0) { std::cout << RED << hero.name << " " << trim(CONF["hasFallen"]) << "..." << RESETCOLOUR << "\n"; return false; }
  }
  return false;
}

// Loot room
void lootRoom(Hero &hero, std::vector<Armour> &armours, std::vector<Weapon> &weapons, std::vector<Potion> &potionsList,  std::vector<Spell> &spells) {
  int lootType = std::rand() % 4;
  if (lootType == 0) {
    Potion lootPotion = potionsList[std::rand() % potionsList.size()];
    hero.inventoryPotions.emplace_back(lootPotion);
    std::cout << GREEN << "🧪 " << trim(CONF["foundHealthPotion"]) << ": " << hero.inventoryPotions.size() << ", " << lootPotion.healAmount << trim(CONF["hp"]) << RESETCOLOUR << "\n";
  }
  else if (lootType == 1) {
    Weapon newWeapon = weapons[std::rand() % weapons.size()];
    hero.inventoryWeapons.emplace_back(newWeapon);
    std::cout << GREEN << "⚔ " << trim(CONF["foundNewWeapon"]) << ": " << newWeapon.name << " (" << trim(CONF["attack"]) << " +" << newWeapon.attack << ")" << RESETCOLOUR << "\n";
  }
  else if (lootType == 2) {
    Spell newSpell = spells[std::rand() % spells.size()];
    hero.inventorySpells.emplace_back(newSpell);
    std::cout << GREEN << "⚔ " << trim(CONF["foundNewSpell"]) << ": " << newSpell.name << " (" << trim(CONF["attack"]) << " +" << newSpell.attack << "-" << trim(CONF["manaCost"]) << " " << newSpell.manaCost << ")" << RESETCOLOUR << "\n";
  }
  else {
    Armour newArmour = armours[std::rand() % armours.size()];
    hero.inventoryArmours.emplace_back(newArmour);
    std::cout << GREEN << "🛡 " << trim(CONF["foundNewArmour"]) << ": " << newArmour.name << " (" << trim(CONF["defense"]) << " +" << newArmour.defense << ")" << RESETCOLOUR << "\n";
  }
}

// Use potion
void usePotion(Hero &hero) {
  if (hero.inventoryPotions.size() > 0) {
    if (hero.companion.health <= 0) { hero.companion.health = 0; std::cout << GREEN << trim(CONF["resurrectCompanion"]) << RESETCOLOUR << "\n"; }
    Potion p = hero.inventoryPotions.back();
    hero.mana = std::min(hero.maxMana, hero.mana + 20);
    hero.health = std::min(hero.maxHP, hero.health + p.healAmount);
    hero.companion.health = std::min(hero.companion.maxHP, hero.companion.health + p.healAmount);
    std::cout << GREEN << trim(CONF["potionUsed"]) << "!" << RESETCOLOUR << "  +" << p.healAmount << " " << trim(CONF["hpCurrentHP"]) << ": " << MAGENTA << hero.health
      << RESETCOLOUR << " " << CYAN << trim(CONF["companionHeal"]) << " " << hero.companion.health << RESETCOLOUR
      << " " << trim(CONF["addedMana"]) << ": " << BLUE << hero.mana << " " << trim(CONF["mana"]) << RESETCOLOUR << "\n";
    hero.inventoryPotions.pop_back();
  } else {
    std::cout << RED << trim(CONF["noPotionsAvailable"]) << "!" << RESETCOLOUR << "\n";
  }
}

// Trap room
bool trapRoom(Hero &hero) {
  int damage = 5 + std::rand() % 16; // 5-20 damage
  hero.health -= damage;
  std::cout << RED << "💥 " << trim(CONF["trapTriggered"]) << " " << damage << " " << trim(CONF["damageHP"]) <<  ": " << hero.health << RESETCOLOUR << "\n";
  return hero.health > 0;
}

// Treasure chest room
void chestRoom(Hero &hero, std::vector<Armour> &armours) {
  int rewardType = std::rand() % 2;
  if (rewardType == 0) {
    int goldFound = 10 + std::rand() % 41; // 10-50 gold
    hero.gold += goldFound;
    std::cout << GREEN << "💰 " << trim(CONF["foundTreasureChestWith"]) << " " << goldFound << " " << trim(CONF["goldTotalGold"]) << ": " << hero.gold << RESETCOLOUR << "\n";
  } else {
    Armour rareArmour = armours[std::rand() % armours.size()];
    hero.inventoryArmours.emplace_back(rareArmour);
    std::cout << GREEN << "✨ " << trim(CONF["foundRareArmour"]) << ": " << rareArmour.name << " (" << trim(CONF["defense"]) << " +" << rareArmour.defense << RESETCOLOUR << ")\n";
  }
}

// Explore room
bool exploreRoom(Hero &hero, std::vector<Enemy> &enemies, std::vector<Armour> &armours, std::vector<Weapon> &weapon, char &cell, std::vector<Potion> &potionsList, std::vector<Spell> &spells) {
  if (cell == 'E') {
    Enemy enemy = enemies[std::rand() % enemies.size()];
    cell = '.';
    return battle(hero, enemy);
  } else if (cell == 'L') {
    lootRoom(hero, armours, weapon, potionsList, spells);
    cell = '.';
    return true;
  } else if (cell == 'B') {
    Enemy boss = {.name = trim(CONF["dungeonBoss"]), .attack = 25, .health = 80, .xpReward = 50};
    cell = '.';
    return battle(hero, boss);
  } else if (cell == 'G') {
    Enemy boss = {.name = trim(CONF["arenaBoss"]), .attack = 26, .health = 85, .xpReward = 60};
    cell = '.';
    return battle(hero, boss);
  } else if (cell == 'Z') {
    Enemy boss = {.name = trim(CONF["tarkanBoss"]), .attack = 27, .health = 90, .xpReward = 70};
    cell = '.';
    return battle(hero, boss);
  } else if (cell == 'T') {
    cell = '.';
    return trapRoom(hero);
  } else if (cell == 'C') {
    chestRoom(hero, armours);
    cell = '.';
    return true;
  } else if (cell == 'F') {
    std::cout << GREEN << trim(CONF["healingFountain"]) << " " << hero.name << " " << hero.maxHP << " " << trim(CONF["hp"]) << ", " << trim(CONF["companion"]) << " " << hero.companion.maxHP << trim(CONF["hp"]) << RESETCOLOUR << std::endl;
    hero.health = hero.maxHP;
    hero.companion.health = hero.companion.maxHP;
    cell = '.';
    return true;
  } else if (cell == 'S') {
    std::cout << GREEN << trim(CONF["manaShrine"]) << " " << hero.maxMana << " " << trim(CONF["mana"]) << RESETCOLOUR << std::endl;
    hero.mana = hero.maxMana;
    cell = '.';
    return true;
  } else {
    std::cout << "🚪 " << trim(CONF["emptyRoom"]) << ".\n";
    return true;
  }
}

// Shop system
void shop(Hero &hero, std::vector<Armour> &armours, std::vector<Potion> &potionsList) {
  std::cout << CYAN << "\n🏪 " << trim(CONF["welcomeYouHave"]) << " " << RESETCOLOUR << YELLOW << hero.gold << " " << trim(CONF["gold"]) << "." << RESETCOLOUR << "\n";
  std::cout << YELLOW << "1." << RESETCOLOUR << BLUE << " " << trim(CONF["buyPotion"]) << " " << RESETCOLOUR << YELLOW << "(20 " << trim(CONF["gold"]) << ")" << RESETCOLOUR << "\n";
  std::cout << YELLOW << "2." << RESETCOLOUR << BLUE << " " << trim(CONF["buyArmourUpgrade"]) << " " << RESETCOLOUR << YELLOW << "(50 " << trim(CONF["gold"]) << ")" << RESETCOLOUR << "\n";
  std::cout << YELLOW << "3." << RESETCOLOUR << " " << trim(CONF["skip"]) << "\n";
  int choice;
  choice = readNumber();
  if (choice == 1) {
    if (hero.gold >= 20) {
      hero.gold -= 20;
      Potion selected = potionsList[0];
      hero.inventoryPotions.emplace_back(selected);
      std::cout << GREEN << trim(CONF["boughtPotion"]) << ": " << hero.inventoryPotions.size() << RESETCOLOUR << "\n";
    } else {
      std::cout << RED << trim(CONF["notEnoughGold"]) << "!" << RESETCOLOUR << "\n";
    }
  } else if (choice == 2) {
    if (hero.gold >= 50) {
      hero.gold -= 50;
      Armour newArmour = armours[std::rand() % armours.size()];
      hero.inventoryArmours.emplace_back(newArmour);
      std::cout << BLUE << trim(CONF["boughtArmour"]) << ": " << newArmour.name << " (" << trim(CONF["defense"]) << " +" << newArmour.defense << ")" << RESETCOLOUR << "\n";
    } else { std::cout << RED << trim(CONF["notEnoughGold"]) << "!" << RESETCOLOUR << "\n"; }
  } else { std::cout << YELLOW << trim(CONF["leavingShop"]) << "..." << RESETCOLOUR << "\n"; }
}

// Weapon shop system
void weaponShop(Hero &hero, std::vector<Weapon> &weapons) {
  std::cout << CYAN << "\n⚔ " << trim(CONF["weaponShopYouHave"]) << " " << RESETCOLOUR << YELLOW << hero.gold << " " << trim(CONF["gold"]) << "." << RESETCOLOUR << "\n";
  std::cout << MAGENTA << trim(CONF["curEquippedWeapon"]) << ": " << RESETCOLOUR << CYAN << hero.equippedWeapon.name << " (" << trim(CONF["attack"]) << " +" << hero.equippedWeapon.attack << ")" << RESETCOLOUR << "\n";
  for (size_t i = 0; i < weapons.size(); ++i) { std::cout << YELLOW << i+1 << ". " << RESETCOLOUR << CYAN << weapons[i].name << " (" << trim(CONF["attack"]) << " +" << RESETCOLOUR
    << YELLOW << weapons[i].attack  << RESETCOLOUR << "," << YELLOW << " " << trim(CONF["cost"]) << ": " << weapons[i].cost << " " << trim(CONF["gold"]) << RESETCOLOUR << CYAN << ")" << RESETCOLOUR << "\n"; }
  std::cout << "0. " << trim(CONF["skip"]) << "\n";
  int choice;
  std::cout << BLUE << trim(CONF["chooseWeaponNumber"]) << ": " << RESETCOLOUR;
  if (!(std::cin >> choice) || choice < 0 || choice > static_cast<int>(weapons.size())) {
    std::cin.clear(); std::cin.ignore(1000, '\n');
    std::cout << RED << trim(CONF["invalidChoiceSkipping"]) << "." << RESETCOLOUR << "\n";
    return;
  }
  if (choice == 0) return;
  Weapon selected = weapons[choice-1];
  if (hero.gold >= selected.cost) {
    hero.gold -= selected.cost;
    hero.inventoryWeapons.emplace_back(selected);
    equipWeapon(hero, weapons, choice - 1, 1);
  } else { std::cout << RED << trim(CONF["notEnoughGold"]) << "!" << RESETCOLOUR << "\n"; }
}

// Potion shop
void potionShop(Hero &hero, std::vector<Potion> &potionsList) {
  std::cout << CYAN << "\n🍵 " << trim(CONF["potionShopYouHave"]) << " " << RESETCOLOUR << YELLOW << hero.gold << " " << trim(CONF["gold"]) << "." << RESETCOLOUR << "\n";
  for (size_t i = 0; i < potionsList.size(); i++) { std::cout << YELLOW << i+1 << ". " << RESETCOLOUR << CYAN << potionsList[i].name << " (" << trim(CONF["heal"]) << " +" << RESETCOLOUR
    << YELLOW << potionsList[i].healAmount << RESETCOLOUR << "," << YELLOW << " " << trim(CONF["cost"]) << ": " << potionsList[i].cost << " " << trim(CONF["gold"]) << RESETCOLOUR << CYAN << ")" << RESETCOLOUR << "\n"; }
  std::cout << "0. " << trim(CONF["skip"]) << "\n";
  int choice;
  std::cout << BLUE << trim(CONF["choosePotion"]) << ": " << RESETCOLOUR;
  if (!(std::cin >> choice) || choice < 0 || choice > static_cast<int>(potionsList.size())) { std::cin.clear(); std::cin.ignore(1000, '\n'); std::cout << trim(CONF["invalidChoiceSkipping"]) << ".\n"; return; }
  if (choice == 0) return;
  Potion selected = potionsList[choice-1];
  if (hero.gold >= selected.cost) {
    hero.gold -= selected.cost;
    hero.inventoryPotions.emplace_back(selected);
    std::cout << GREEN << trim(CONF["bought"]) << " " << selected.name << ". " << trim(CONF["youNowHave"]) << " " << hero.inventoryPotions.size() << " " << trim(CONF["potions"]) << "." << RESETCOLOUR << "\n";
  } else { std::cout << RED << trim(CONF["notEnoughGold"]) << "!" << RESETCOLOUR << "\n"; }
}

// Generate map with enemies, loot, traps, chests, and boss
std::vector<std::vector<char>> generateMap(int size) {
  std::vector<std::vector<char>> map(size, std::vector<char>(size, '.'));
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) {
      int r = std::rand() % 8; // More variety
      if (r == 0) map[i][j] = 'E'; // Enemy
      else if (r == 1) map[i][j] = 'L'; // Loot
      else if (r == 2) map[i][j] = 'T'; // Trap
      else if (r == 3) map[i][j] = 'C'; // Chest
      else if (r == 4) map[i][j] = 'F'; // healing fountain
      else if (r == 5) map[i][j] = 'S'; // mana shrine
    }
  }
  // Place bosses in random locations
  bool bossSpawned = false;
  int bx = std::rand() % size;
  int by = std::rand() % size;
  map[bx][by] = 'B';
  int bx2 = std::rand() % size;
  int by2 = std::rand() % size;
  int bx3 = std::rand() % size;
  int by3 = std::rand() % size;
  if (by != by2 && bx != bx2) { map[bx2][by2] = 'G'; bossSpawned = true; }
  if (!bossSpawned && (by3 != by && bx3 != bx)) { map[bx3][by3] = 'Z'; }
  return map;
}

// Display map with fog of war
void displayMap(const std::vector<std::vector<char>> &map, const std::vector<std::vector<bool>> &visited, const Hero &hero) {
  std::cout << "\n" << GREEN << trim(CONF["dungeonMap"]) << ":" << RESETCOLOUR << "\n";
  for (int i = 0; i < static_cast<int>(map.size()); ++i) {
    for (int j = 0; j < static_cast<int>(map[i].size()); ++j) {
      if (i == hero.x && j == hero.y) std::cout << GREEN << 'H' << RESETCOLOUR << ' ';
      else if (visited[i][j]) std::cout << map[i][j] << ' ';
      else std::cout << YELLOW << '?' << RESETCOLOUR << ' ';
    }
    std::cout << "\n";
  }
}

// Gain XP and check for level up
void gainXP(Hero &hero, int amount) {
  hero.xp += amount;
  std::cout << GREEN << hero.name << RESETCOLOUR << YELLOW << " " << trim(CONF["gained"]) << " " << RESETCOLOUR << CYAN << amount << " " << trim(CONF["xp"]) << "!" << RESETCOLOUR << "\n";
  if (hero.xp >= hero.level * 50) { // Level up threshold
    hero.xp -= hero.level * 50; // keep the XP above the threshold
    hero.level++;
    hero.baseAttack += 5;
    hero.maxHP += 5;
    hero.maxMana += 5;
    hero.baseDefense += 2;
    hero.companion.maxHP += 5;
    hero.health = std::min(hero.maxHP, hero.health + 20);
    hero.mana = std::min(hero.maxMana, hero.mana + 20);
    hero.companion.health = std::min(hero.companion.maxHP, hero.companion.health + 20);
    std::cout << GREEN << "*** " << trim(CONF["levelUP"]) << "! *** " << RESETCOLOUR << MAGENTA << hero.name << RESETCOLOUR << YELLOW << " " << trim(CONF["isNowLevel"]) << " " << hero.level << RESETCOLOUR << "!\n";
  }
}

#ifdef _WIN32
int readKey(void) {
  int c = _getch();
  if (c == 0 || c == 224) { // in some cases on windows(?), pressing arrow keys returns two of these values, where the first is 0 or 224.
    c = _getch();
    switch (c) {
      case 72: return KEY_UP;
      case 80: return KEY_DOWN;
      case 77: return KEY_RIGHT;
      case 75: return KEY_LEFT;
      default: return 27; // other special keys are ignored like Esc
    }
  }
  return c;
}
#else
int readKey(void) {
  struct termios old = {}, raw;
  unsigned char c = 0;
  fflush(stdout);
  // Not a terminal (piped input): take the next byte from std::cin, which readNumber() may have already buffered
  if (tcgetattr(STDIN_FILENO, &old) < 0) { int ch = std::cin.get(); return ch == EOF ? -1 : ch; }
  raw = old;
  raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO)); // Non canonical mode, no echo
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) < 0) { perror("tcsetattr ICANON"); }
  int result = read(STDIN_FILENO, &c, 1) == 1 ? c : -1;
  // Read the whole escape sequence while still raw: going back to canonical mode first makes macOS echo "[C",
  // and leftover bytes of keys like Home or F1 would be read as more key presses
  if (result == 27) {
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1; // wait up to 0.1 s per byte
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    unsigned char intro = 0;
    if (read(STDIN_FILENO, &intro, 1) == 1 && (intro == '[' || intro == 'O')) {
      // "ESC O x" is one byte long, "ESC [ ..." ends at a byte from '@' to '~'
      for (int len = 0; len < 16 && read(STDIN_FILENO, &c, 1) == 1; len++) {
        if (intro == 'O' || (c >= '@' && c <= '~')) {
          if (len == 0 && c >= 'A' && c <= 'D') { result = KEY_UP + (c - 'A'); } // up, down, right, left
          break;
        }
      }
    }
    // Anything else (Esc alone, Alt+key, Home, F1...) stays 27 and is ignored
  }
  if (tcsetattr(STDIN_FILENO, TCSADRAIN, &old) < 0) { perror("tcsetattr ~ICANON"); }
  return result;
}
#endif /* _WIN32 */

// Configure the movement/inventories/potion consumption
void setupKeys(void) {
  int x = 0;
  std::cout << trim(CONF["pressKeyFor"]) << " ";
  for (auto text : { "right", "up", "left", "down", "armourInventory",
      "weaponInventory", "consumeApotion", "spellInventory" }) {
    std::cout << trim(CONF[text]);
    int key = readKey();
    if (key < 0) { std::exit(EXIT_SUCCESS); } // stdin closed
    keys[x++] = (key < 256) ? std::tolower(key) : key;
  }
  std::cout << BLUE << "\n" << trim(CONF["keysEstablished"]) << "\n" << RESETCOLOUR << std::endl;
}

// Printable name of a key for the menus
std::string keyName(int key) {
  switch (key) {
    case KEY_UP:    return "↑";
    case KEY_DOWN:  return "↓";
    case KEY_RIGHT: return "→";
    case KEY_LEFT:  return "←";
    case 27:        return "Esc";
    case ' ':       return "Space";
    case '\t':      return "Tab";
    case '\n':
    case '\r':      return "Enter";
    case 127:
    case 8:         return "Backspace";
  }
  if (key > 32 && key < 127) { return std::string(1, static_cast<char>(key)); }
  return "?";
}

// Trim excess quotes
std::string trim(std::string str) {
  std::string buf;
  for (auto &x : str) {
    if (x == '"') { continue; }
    buf.push_back(x);
  }
  return buf;
}

// Read a menu number; -1 on bad input, exits if stdin is closed
int readNumber(void) {
  int n;
  if (std::cin >> n) { return n; }
  if (std::cin.eof()) { std::exit(EXIT_SUCCESS); }
  std::cin.clear();
  std::cin.ignore(1000, '\n');
  return -1;
}
