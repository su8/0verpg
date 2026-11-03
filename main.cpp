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
// Build: g++ -std=c++20 -O2 -Wall -Wextra 0verpg.cpp -o 0verpg   (needs json.hpp next to it)
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cctype>
#include <random>
#include <fstream>
#include <algorithm>
#include <cstdio>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
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

// Enemy status effect
enum StatusEffect { NONE, BURN, FREEZE, BLEED, POISON };

struct Armour { std::string name; int defense; };
struct Weapon { std::string name; int attack; int cost; };
struct Potion { std::string name; int healAmount; int cost; };
struct Spell  { std::string name; int attack; int manaCost; };

struct Enemy {
  std::string name;
  int attack;
  int health;
  int xpReward;
  int goldReward;
  StatusEffect status = NONE;
  int statusTurns = 0;
};

struct Companion { std::string name; int attack; int health; int maxHP; };
struct ClassSkill { std::string name; int damage; int manaCost; int unlockLevel; };

struct Hero {
  std::string name;
  int classChoice = 0;
  int health = 0;
  int baseDefense = 0;
  int baseAttack = 0;
  int mana = 0;
  int maxMana = 0;
  int critChance = 0;
  Armour equippedArmour;
  Weapon equippedWeapon;
  Spell equippedSpell;
  Companion companion;
  ClassSkill activeSkill;
  std::vector<Armour> inventoryArmours;
  std::vector<Weapon> inventoryWeapons;
  std::vector<Potion> inventoryPotions;
  std::vector<Spell> inventorySpells;
  std::vector<ClassSkill> skills;
  int gold = 0;
  int x = 0;
  int y = 0;
  int xp = 0;
  int level = 1;
  int maxHP = 0;
  int kills = 0;
};

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

// The config in the current directory wins, so the game runs straight from the repository; otherwise use the one in HOME
std::string configHome = std::ifstream(".0verpg.json") ? std::string(".0verpg.json") : (std::getenv("HOME") ? std::string(std::getenv("HOME")) + std::string("/") : std::string("./")) + ".0verpg.json";
std::string userLang = "en";
std::ifstream cfg(configHome);
const std::string SAVE_FILE = std::ifstream(".0verpg.save") ? std::string(".0verpg.save") : (std::getenv("HOME") ? std::string(std::getenv("HOME")) + std::string("/") : std::string("./")) + ".0verpg.save";
// Inventory weapons/armours colours for their names
const char *coloursArr[] = { BLUE, MAGENTA, GREEN, RED, YELLOW, CYAN };
const int coloursArrLen = sizeof(coloursArr) / sizeof(coloursArr[0]);

using json = nlohmann::json;
json config;
#define CONF config[userLang]

// Random numbers in [0, n) - replaces std::rand()
static std::mt19937 rng{std::random_device{}()};
static int rnd(int n) { return std::uniform_int_distribution<int>(0, n - 1)(rng); }

void equipArmour(Hero &hero);
void equipWeapon(Hero &hero, std::vector<Weapon> &weapons, int index, int usedFromWebShop);
void equipSpell(Hero &hero);
bool battle(Hero &hero, Enemy enemy);
void lootRoom(Hero &hero, std::vector<Armour> &armours, std::vector<Weapon> &weapons, std::vector<Potion> &potionsList, std::vector<Spell> &spells);
void usePotion(Hero &hero);
bool trapRoom(Hero &hero);
void chestRoom(Hero &hero, std::vector<Armour> &armours);
bool exploreRoom(Hero &hero, std::vector<Enemy> &enemies, std::vector<Armour> &armours, std::vector<Weapon> &weapons, char &cell, std::vector<Potion> &potionsList, std::vector<Spell> &spells, int floor);
void shop(Hero &hero, std::vector<Armour> &armours, std::vector<Potion> &potionsList);
void weaponShop(Hero &hero, std::vector<Weapon> &weapons);
void potionShop(Hero &hero, std::vector<Potion> &potionsList);
std::vector<std::vector<char>> generateMap(int size);
void displayMap(const std::vector<std::vector<char>> &map, const std::vector<std::vector<bool>> &visited, const Hero &hero);
void gainXP(Hero &hero, int amount);
int readKey(void);
void setupKeys(void);
std::string keyName(int key);
std::string trim(std::string str);
int readNumber(void);
bool processStatus(Enemy &enemy);
Enemy scaled(Enemy e, int floor);
bool saveGame(const Hero &h, int floor);
void applySave(Hero &hero, const json &j);

// Translation lookup that never throws: uses the config text when it exists, otherwise the English fallback.
// New features use this, so the game works even with an older .0verpg.json
std::string T(const char *key, const char *fallback) {
  json &lang = config[userLang];
  if (lang.is_object() && lang.contains(key) && lang[key].is_string()) { return trim(lang[key].get<std::string>()); }
  return fallback;
}

int play(int argc, char *argv[]) {
#ifdef _WIN32
  SetConsoleTitle("0verpg");
  SetConsoleOutputCP(CP_UTF8); // emojis
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  if (GetConsoleMode(hOut, &mode)) { SetConsoleMode(hOut, mode | 0x0004); } // ENABLE_VIRTUAL_TERMINAL_PROCESSING for the colours
#endif /*_WIN32 */
  if (argc >= 2) { userLang = std::string(argv[1]); }
  if (userLang != "en" && userLang != "bg" && userLang != "it") { std::cerr << "Error the given language " << userLang << " does not exists. Exiting..." << std::endl; return EXIT_FAILURE; }
  if (!cfg) { std::cerr << "Error: Could not open " << configHome << "\n"; return EXIT_FAILURE; }
  cfg >> config;
  cfg.close();
  setupKeys();

  // Saved game (written at the start of every floor)
  json save;
  bool loaded = false;
  int classChoice = 0;
  {
    std::ifstream sf(SAVE_FILE);
    if (sf) {
      try { sf >> save; loaded = save.contains("class") && save.contains("floor"); }
      catch (const json::exception &) { loaded = false; }
    }
    if (loaded) {
      std::cout << GREEN << T("continueSave", "Saved game found. 1. Continue  2. New game") << RESETCOLOUR << "\n";
      if (readNumber() == 1) { classChoice = save["class"].get<int>() + 1; }
      else { loaded = false; }
    }
  }

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
  hero.classChoice = classChoice;
  hero.health = characters[classChoice].health;
  hero.maxHP = hero.health;
  hero.mana = characters[classChoice].mana;
  hero.maxMana = characters[classChoice].mana;
  hero.baseDefense = characters[classChoice].baseDefense;
  hero.baseAttack = characters[classChoice].baseAttack;
  hero.equippedArmour = {.name = trim(CONF["none"]), .defense = 0};
  hero.equippedWeapon = {.name = trim(CONF["fists"]), .attack = 0, .cost = 0};
  hero.equippedSpell = {.name = trim(CONF["basicSpell"]), .attack = 0, .manaCost = 0};
  hero.companion = {.name = trim(CONF["companion"]), .attack = 2, .health = 100, .maxHP = 100};
  hero.gold = 50;

  std::vector<Armour> armours;
  std::vector<Weapon> weapons;
  std::vector<Spell> spells;
  // Pick the gear by the chosen class number, as the class names are translated
  bool isMage = classChoice == 1, isRogue = classChoice == 2;
  if (classChoice == 0 || isMage) {
    armours = {
      {.name = isMage ? trim(CONF["mageArmour1"]) : trim(CONF["armour1"]), .defense = 5},
      {.name = isMage ? trim(CONF["mageArmour2"]) : trim(CONF["armour2"]), .defense = 10},
      {.name = isMage ? trim(CONF["mageArmour3"]) : trim(CONF["armour3"]), .defense = 15},
      {.name = isMage ? trim(CONF["mageArmour4"]) : trim(CONF["armour4"]), .defense = 20}
    };
    weapons = {
      {.name = isMage ? trim(CONF["mageWeapon1"]) : trim(CONF["weapon1"]), .attack = 5, .cost = 20},
      {.name = isMage ? trim(CONF["mageWeapon2"]) : trim(CONF["weapon2"]), .attack = 10, .cost = 40},
      {.name = isMage ? trim(CONF["mageWeapon3"]) : trim(CONF["weapon3"]), .attack = 15, .cost = 60},
      {.name = isMage ? trim(CONF["mageWeapon4"]) : trim(CONF["weapon4"]), .attack = 25, .cost = 100}
    };
    spells = {
      {.name = isMage ? trim(CONF["mageSpell1"]) : trim(CONF["warriorSpell1"]), .attack = 5, .manaCost = 0},
      {.name = isMage ? trim(CONF["mageSpell2"]) : trim(CONF["warriorSpell2"]), .attack = 10, .manaCost = 10},
      {.name = isMage ? trim(CONF["mageSpell3"]) : trim(CONF["warriorSpell3"]), .attack = 15, .manaCost = 15},
      {.name = isMage ? trim(CONF["mageSpell4"]) : trim(CONF["warriorSpell4"]), .attack = 25, .manaCost = 20}
    };
  }
  else {
    armours = {
      {.name = isRogue ? trim(CONF["rogueArmour1"]) : trim(CONF["paladinArmour1"]), .defense = 5},
      {.name = isRogue ? trim(CONF["rogueArmour2"]) : trim(CONF["paladinArmour2"]), .defense = 10},
      {.name = isRogue ? trim(CONF["rogueArmour3"]) : trim(CONF["paladinArmour3"]), .defense = 15},
      {.name = isRogue ? trim(CONF["rogueArmour4"]) : trim(CONF["paladinArmour4"]), .defense = 20}
    };
    weapons = {
      {.name = isRogue ? trim(CONF["rogueWeapon1"]) : trim(CONF["paladinWeapon1"]), .attack = 5, .cost = 20},
      {.name = isRogue ? trim(CONF["rogueWeapon2"]) : trim(CONF["paladinWeapon2"]), .attack = 10, .cost = 40},
      {.name = isRogue ? trim(CONF["rogueWeapon3"]) : trim(CONF["paladinWeapon3"]), .attack = 15, .cost = 60},
      {.name = isRogue ? trim(CONF["rogueWeapon4"]) : trim(CONF["paladinWeapon4"]), .attack = 25, .cost = 100}
    };
    spells = {
      {.name = isRogue ? trim(CONF["rogueSpell1"]) : trim(CONF["paladinSpell1"]), .attack = 5, .manaCost = 0},
      {.name = isRogue ? trim(CONF["rogueSpell2"]) : trim(CONF["paladinSpell2"]), .attack = 10, .manaCost = 10},
      {.name = isRogue ? trim(CONF["rogueSpell3"]) : trim(CONF["paladinSpell3"]), .attack = 15, .manaCost = 15},
      {.name = isRogue ? trim(CONF["rogueSpell4"]) : trim(CONF["paladinSpell4"]), .attack = 25, .manaCost = 20}
    };
  }
  hero.inventorySpells.emplace_back(spells[0]);

  std::vector<Potion> potionsList = {
    {.name = trim(CONF["potion1"]), .healAmount = 20, .cost = 15},
    {.name = trim(CONF["potion2"]), .healAmount = 40, .cost = 30},
    {.name = trim(CONF["potion3"]), .healAmount = 60, .cost = 45}
  };
  std::vector<Enemy> enemies = {
    {.name = trim(CONF["enemy1"]), .attack = 8, .health = 30, .xpReward = 20, .goldReward = 20},
    {.name = trim(CONF["enemy2"]), .attack = 12, .health = 40, .xpReward = 30, .goldReward = 30},
    {.name = trim(CONF["enemy3"]), .attack = 15, .health = 50, .xpReward = 40, .goldReward = 40}
  };
  if (classChoice == 0) {
    hero.critChance = 15;
    hero.skills = {
      {.name = trim(CONF["cleave"]), .damage = 2, .manaCost = 0, .unlockLevel = 1},
      {.name = trim(CONF["shieldBash"]), .damage = 4, .manaCost = 10, .unlockLevel = 2},
      {.name = trim(CONF["whirlwind"]), .damage = 6, .manaCost = 20, .unlockLevel = 3}
    };
  }
  else if (classChoice == 1) {
    hero.critChance = 10;
    hero.skills = {
      {.name = trim(CONF["fireball"]), .damage = 2, .manaCost = 5, .unlockLevel = 1},
      {.name = trim(CONF["iceSpike"]), .damage = 4, .manaCost = 10, .unlockLevel = 2},
      {.name = trim(CONF["lightningBolt"]), .damage = 7, .manaCost = 20, .unlockLevel = 3}
    };
  }
  else if (classChoice == 2) {
    hero.critChance = 35;
    hero.skills = {
      {.name = trim(CONF["backstab"]), .damage = 2, .manaCost = 5, .unlockLevel = 1},
      {.name = trim(CONF["poisonStrike"]), .damage = 4, .manaCost = 10, .unlockLevel = 2},
      {.name = trim(CONF["shadowDance"]), .damage = 5, .manaCost = 20, .unlockLevel = 3}
    };
  }
  else {
    hero.critChance = 20;
    hero.skills = {
      {.name = trim(CONF["smite"]), .damage = 2, .manaCost = 5, .unlockLevel = 1},
      {.name = trim(CONF["holyStrike"]), .damage = 4, .manaCost = 10, .unlockLevel = 2},
      {.name = trim(CONF["divineWrath"]), .damage = 8, .manaCost = 20, .unlockLevel = 3}
    };
  }
  hero.activeSkill = hero.skills[0];

  std::string wKey = keyName(keys[1]);
  std::string dKey = keyName(keys[0]);
  std::string sKey = keyName(keys[3]);
  std::string aKey = keyName(keys[2]);
  std::string potionKey = keyName(keys[6]);
  std::string armourKey = keyName(keys[4]);
  std::string weaponKey = keyName(keys[5]);
  std::string spellKey = keyName(keys[7]);

  int startFloor = 1;
  if (loaded) {
    try { applySave(hero, save); startFloor = save["floor"].get<int>(); }
    catch (const json::exception &e) { std::cerr << "Error: the save file is broken (" << e.what() << "), delete " << SAVE_FILE << "\n"; return EXIT_FAILURE; }
    std::cout << GREEN << T("saveLoaded", "Game loaded.") << RESETCOLOUR << "\n";
  }
  else {
    std::cout << "\n" << trim(CONF["chooseStartingArmour"]) << "\n";
    for (size_t i = 0; i < armours.size(); ++i) {
      std::cout << YELLOW << i+1 << ". " << RESETCOLOUR << MAGENTA << armours[i].name << CYAN << " (" << trim(CONF["defense"]) << " +" << RESETCOLOUR << YELLOW << armours[i].defense << RESETCOLOUR << CYAN << ")" << RESETCOLOUR << "\n";
    }
    int choice = 0;
    // Ask again on bad input instead of silently starting naked
    while (choice < 1 || choice > static_cast<int>(armours.size())) { choice = readNumber(); }
    hero.equippedArmour = armours[choice-1];
    hero.inventoryArmours.emplace_back(armours[choice-1]);
    std::cout << CYAN << hero.name << RESETCOLOUR << " " << trim(CONF["equipped"]) << " " << BLUE << hero.equippedArmour.name << " (" << trim(CONF["defense"]) << " +" << hero.equippedArmour.defense << ")" << RESETCOLOUR << "\n";
  }

  const int floors = 3;      // Number of dungeon floors
  const int mapSize = 5;     // Map size (square grid)
  startFloor = std::clamp(startFloor, 1, floors);
  for (int floor = startFloor; floor <= floors; ++floor) {
    std::vector<std::vector<char>> dungeon = generateMap(mapSize);
    std::vector<std::vector<bool>> visited(mapSize, std::vector<bool>(mapSize, false));
    hero.x = 0;
    hero.y = 0;
    visited[0][0] = true;
    bool floorCleared = false;
    if (saveGame(hero, floor)) { std::cout << CYAN << "💾 " << T("gameSaved", "Game saved") << RESETCOLOUR << "\n"; }
    weaponShop(hero, weapons);
    potionShop(hero, potionsList);
    std::cout << YELLOW << "\n🏰 " << trim(CONF["floor"]) << " " << floor << " " << trim(CONF["begins"]) << RESETCOLOUR << "\n";
    while (!floorCleared && hero.health > 0) {
      displayMap(dungeon, visited, hero);
      std::cout << YELLOW << "\n💰 " << trim(CONF["gold"]) << ": " << hero.gold << RESETCOLOUR << " | " << MAGENTA << "💗 " << trim(CONF["hp"]) << ": " << hero.health << "/" << hero.maxHP << RESETCOLOUR
        << " | " << GREEN << "🧪 " << trim(CONF["potions"]) << ": " << hero.inventoryPotions.size() << RESETCOLOUR << " | "
        << BLUE << "♻️ " << trim(CONF["mana"]) << ": " << hero.mana << RESETCOLOUR <<  " | 💀" << CYAN << " " << trim(CONF["attack"]) << ": " << hero.baseAttack + hero.equippedWeapon.attack + hero.equippedSpell.attack << " " << trim(CONF["defense"]) << ": " << hero.baseDefense + hero.equippedArmour.defense << RESETCOLOUR
        << " | " << "🪄 " << MAGENTA << trim(CONF["spell"]) << ": " << hero.equippedSpell.name << RESETCOLOUR
        << " 🔥 " << BLUE << trim(CONF["skill"]) << ": " << hero.activeSkill.name << RESETCOLOUR
        << " 👹 " << CYAN << hero.name << RESETCOLOUR << YELLOW << " " << trim(CONF["level"]) << ": " << RESETCOLOUR << MAGENTA << hero.level << RESETCOLOUR << ", "
        << CYAN << hero.xp << "/" << hero.level * 50 << " " << trim(CONF["xp"]) << RESETCOLOUR << " | 🐶 " << RED << trim(CONF["companion"])
        << " " << std::max(0, hero.companion.health) << trim(CONF["hp"]) << RESETCOLOUR << "\n";
      std::cout << YELLOW << trim(CONF["move"]) <<  " (" << wKey << "/" <<  dKey << "/" <<  aKey << "/" <<  sKey << "), " <<  potionKey << " " << trim(CONF["usePotion"]) << ", "
        <<  armourKey << " " << trim(CONF["toChangeArmour"]) << ", " <<  weaponKey << " " << trim(CONF["toChangeWeapon"])
        << ", " <<  spellKey << " " << trim(CONF["toChangeSpell"]) << ", Q " << trim(CONF["toQuit"]) << ": " << RESETCOLOUR << std::endl;
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
      if (newX < 0 || newX >= mapSize || newY < 0 || newY >= mapSize) { std::cout << RED << "❌ " << trim(CONF["cantMoveOutsideDungeon"]) << RESETCOLOUR << "\n"; continue; }
      hero.x = newX;
      hero.y = newY;
      visited[hero.x][hero.y] = true;
      if (!exploreRoom(hero, enemies, armours, weapons, dungeon[hero.x][hero.y], potionsList, spells, floor)) {
        std::cout << RED << "\n💀 " << trim(CONF["gameOver"]) << RESETCOLOUR << "\n";
        std::cout << CYAN << T("kills", "Enemies defeated") << ": " << hero.kills << " | " << T("level", "Level") << ": " << hero.level << RESETCOLOUR << "\n";
        std::remove(SAVE_FILE.c_str()); // permadeath
        return EXIT_SUCCESS;
      }
      bool bossAlive = false;
      for (int i = 0; i < mapSize; ++i) {
        for (int j = 0; j < mapSize; ++j) {
          if (dungeon[i][j] == 'B' || dungeon[i][j] == 'G' || dungeon[i][j] == 'Z') bossAlive = true;
        }
      }
      if (!bossAlive) {
        std::cout << GREEN << "\n✅ " << trim(CONF["floor"]) << " " << floor << " " << trim(CONF["cleared"]) << RESETCOLOUR << "\n";
        floorCleared = true;
        hero.health = std::min(hero.maxHP, hero.health + 15);
        std::cout << CYAN << hero.name << RESETCOLOUR << " " << trim(CONF["restsAndRecoversTo"]) << " " << GREEN << hero.health << RESETCOLOUR << " " << trim(CONF["hp"]) << "." << "\n";
      }
    }
    if (floorCleared && floor < floors) { shop(hero, armours, potionsList); }
  }
  std::cout << GREEN << "\n🎉 " << trim(CONF["win"]) << RESETCOLOUR << "\n";
  std::cout << CYAN << T("kills", "Enemies defeated") << ": " << hero.kills << " | " << T("gold", "Gold") << ": " << hero.gold << " | " << T("level", "Level") << ": " << hero.level << RESETCOLOUR << "\n";
  std::remove(SAVE_FILE.c_str());
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

// ---------------------------------------------------------------- save / load
static json toJson(const Armour &a) { return {{"n", a.name}, {"d", a.defense}}; }
static json toJson(const Weapon &w) { return {{"n", w.name}, {"a", w.attack}, {"c", w.cost}}; }
static json toJson(const Potion &p) { return {{"n", p.name}, {"h", p.healAmount}, {"c", p.cost}}; }
static json toJson(const Spell &s) { return {{"n", s.name}, {"a", s.attack}, {"m", s.manaCost}}; }
template <class V> static json listJson(const V &v) {
  json arr = json::array();
  for (const auto &item : v) { arr.push_back(toJson(item)); }
  return arr;
}
static Armour armourFrom(const json &j) { return {j.at("n").get<std::string>(), j.at("d").get<int>()}; }
static Weapon weaponFrom(const json &j) { return {j.at("n").get<std::string>(), j.at("a").get<int>(), j.at("c").get<int>()}; }
static Potion potionFrom(const json &j) { return {j.at("n").get<std::string>(), j.at("h").get<int>(), j.at("c").get<int>()}; }
static Spell spellFrom(const json &j) { return {j.at("n").get<std::string>(), j.at("a").get<int>(), j.at("m").get<int>()}; }

// Save the hero; "floor" is the floor to resume at
bool saveGame(const Hero &h, int floor) {
  json j;
  j["floor"] = floor; j["class"] = h.classChoice;
  j["health"] = h.health; j["maxHP"] = h.maxHP; j["mana"] = h.mana; j["maxMana"] = h.maxMana;
  j["baseDefense"] = h.baseDefense; j["baseAttack"] = h.baseAttack;
  j["gold"] = h.gold; j["xp"] = h.xp; j["level"] = h.level; j["kills"] = h.kills;
  j["compHP"] = h.companion.health; j["compMax"] = h.companion.maxHP;
  j["armour"] = toJson(h.equippedArmour); j["weapon"] = toJson(h.equippedWeapon); j["spell"] = toJson(h.equippedSpell);
  j["armours"] = listJson(h.inventoryArmours); j["weapons"] = listJson(h.inventoryWeapons);
  j["potions"] = listJson(h.inventoryPotions); j["spells"] = listJson(h.inventorySpells);
  std::ofstream f(SAVE_FILE);
  if (!f) { return false; }
  f << j.dump(2);
  return static_cast<bool>(f);
}

// Copy a loaded save onto a freshly created hero of the same class. Throws json::exception on a broken save
void applySave(Hero &hero, const json &j) {
  hero.health = j.at("health").get<int>(); hero.maxHP = j.at("maxHP").get<int>();
  hero.mana = j.at("mana").get<int>(); hero.maxMana = j.at("maxMana").get<int>();
  hero.baseDefense = j.at("baseDefense").get<int>(); hero.baseAttack = j.at("baseAttack").get<int>();
  hero.gold = j.at("gold").get<int>(); hero.xp = j.at("xp").get<int>();
  hero.level = j.at("level").get<int>(); hero.kills = j.at("kills").get<int>();
  hero.companion.health = j.at("compHP").get<int>(); hero.companion.maxHP = j.at("compMax").get<int>();
  hero.equippedArmour = armourFrom(j.at("armour")); hero.equippedWeapon = weaponFrom(j.at("weapon")); hero.equippedSpell = spellFrom(j.at("spell"));
  hero.inventoryArmours.clear(); hero.inventoryWeapons.clear(); hero.inventoryPotions.clear(); hero.inventorySpells.clear();
  for (const auto &x : j.at("armours")) hero.inventoryArmours.emplace_back(armourFrom(x));
  for (const auto &x : j.at("weapons")) hero.inventoryWeapons.emplace_back(weaponFrom(x));
  for (const auto &x : j.at("potions")) hero.inventoryPotions.emplace_back(potionFrom(x));
  for (const auto &x : j.at("spells")) hero.inventorySpells.emplace_back(spellFrom(x));
  for (const auto &s : hero.skills) { if (s.unlockLevel <= hero.level) { hero.activeSkill = s; } }
}

// ---------------------------------------------------------------- items
void equipArmour(Hero &hero) {
  if (hero.inventoryArmours.empty()) { std::cout << RED << T("noItems", "Nothing to equip") << "!" << RESETCOLOUR << "\n"; return; }
  std::cout << MAGENTA << trim(CONF["curEquippedArmour"]) << ": " << RESETCOLOUR << GREEN << hero.equippedArmour.name << " (" << trim(CONF["defense"]) << " +" << hero.equippedArmour.defense << ")" << RESETCOLOUR << "\n";
  std::cout << trim(CONF["inventoryArmours"]) << ":\n";
  for (size_t i = 0; i < hero.inventoryArmours.size(); ++i) {
    std::cout << YELLOW << i + 1 << ". " << RESETCOLOUR << coloursArr[rnd(coloursArrLen)] << hero.inventoryArmours[i].name << RESETCOLOUR
      << CYAN << " (" << trim(CONF["defense"]) << " +" << RESETCOLOUR << YELLOW << hero.inventoryArmours[i].defense << RESETCOLOUR << CYAN << ")" << RESETCOLOUR
      << BLUE << " (" << (hero.inventoryArmours[i].name == hero.equippedArmour.name ? trim(CONF["currentlyEquippedArmour"]) : trim(CONF["goodArmour"])) << ")" << RESETCOLOUR << "\n";
  }
  std::cout << trim(CONF["chooseArmourNumber"]) << ": ";
  int index = readNumber() - 1;
  if (index < 0 || index >= static_cast<int>(hero.inventoryArmours.size())) { std::cout << trim(CONF["invalidArmourIndex"]) << ".\n"; return; }
  hero.equippedArmour = hero.inventoryArmours[index];
  std::cout << CYAN << hero.name << RESETCOLOUR << " " << trim(CONF["equipped"]) << " " << BLUE << hero.equippedArmour.name << " (" << trim(CONF["defense"]) << " +" << hero.equippedArmour.defense << ")" << RESETCOLOUR << "\n";
}

void equipWeapon(Hero &hero, std::vector<Weapon> &weapons, int index, int usedFromWebShop) {
  if (usedFromWebShop == 0) {
    if (weapons.empty()) { std::cout << RED << T("noItems", "Nothing to equip") << "!" << RESETCOLOUR << "\n"; return; }
    std::cout << MAGENTA << trim(CONF["curEquippedWeapon"]) << ": " << RESETCOLOUR << GREEN << hero.equippedWeapon.name << " (" << trim(CONF["attack"]) << " +" << hero.equippedWeapon.attack << ")" << RESETCOLOUR << "\n";
    std::cout << trim(CONF["inventoryWeapons"]) << ":\n";
    for (size_t i = 0; i < weapons.size(); ++i) {
      std::cout << YELLOW << i + 1 << ". " << RESETCOLOUR << coloursArr[rnd(coloursArrLen)] << weapons[i].name << RESETCOLOUR
        << CYAN << " (" << trim(CONF["attack"]) << " +" << RESETCOLOUR << YELLOW << weapons[i].attack << RESETCOLOUR << CYAN << ")" << RESETCOLOUR
        << BLUE << " (" << (weapons[i].name == hero.equippedWeapon.name ? trim(CONF["currentlyEquippedWeapon"]) : trim(CONF["goodWeapon"])) << ")" << RESETCOLOUR << "\n";
    }
    std::cout << trim(CONF["chooseWeaponNumber"]) << ": ";
    index = readNumber() - 1;
  }
  if (index < 0 || index >= static_cast<int>(weapons.size())) { std::cout << trim(CONF["invalidWeaponIndex"]) << ".\n"; return; }
  hero.equippedWeapon = weapons[index];
  std::cout << CYAN << hero.name << RESETCOLOUR << " " <<  trim(CONF["equipped"]) << " " << BLUE << hero.equippedWeapon.name << " (" <<  trim(CONF["attack"]) << " +" << hero.equippedWeapon.attack << ")" << RESETCOLOUR << "\n";
}

void equipSpell(Hero &hero) {
  std::cout << MAGENTA << trim(CONF["curEquippedSpell"]) << ": " << RESETCOLOUR << GREEN << hero.equippedSpell.name << " (+" << hero.equippedSpell.attack << " " << trim(CONF["attack"]) << " " << trim(CONF["manaCost"]) << " -" << hero.equippedSpell.manaCost << ")" << RESETCOLOUR << "\n";
  std::cout << trim(CONF["inventorySpells"]) << ":\n";
  for (size_t i = 0; i < hero.inventorySpells.size(); ++i) {
    std::cout << YELLOW << i+1 << ". " << RESETCOLOUR << coloursArr[rnd(coloursArrLen)] << hero.inventorySpells[i].name << RESETCOLOUR
      << CYAN << " (" << trim(CONF["attack"]) << " +" << RESETCOLOUR << YELLOW << hero.inventorySpells[i].attack << RESETCOLOUR << " -"
      << YELLOW << hero.inventorySpells[i].manaCost << " " << trim(CONF["manaCost"]) << RESETCOLOUR  << CYAN << ")"
      << RESETCOLOUR << BLUE << " (" << (hero.inventorySpells[i].name == hero.equippedSpell.name ? trim(CONF["currentlyEquippedSpell"]) : trim(CONF["goodSpell"])) << ")" << RESETCOLOUR << "\n";
  }
  std::cout << trim(CONF["chooseSpellNumber"]) << ": ";
  int index = readNumber() - 1;
  if (index < 0 || index >= static_cast<int>(hero.inventorySpells.size())) { std::cout << trim(CONF["invalidSpellIndex"]) << ".\n"; return; }
  hero.equippedSpell = hero.inventorySpells[index];
  std::cout << CYAN << hero.name << RESETCOLOUR << " " << trim(CONF["equipped"]) << " " << BLUE << hero.equippedSpell.name
    << " (+" << trim(CONF["attack"]) << " " << hero.equippedSpell.attack << " " << trim(CONF["manaCost"]) << " -" << hero.equippedSpell.manaCost << ")" << RESETCOLOUR << "\n";
}

// ---------------------------------------------------------------- battle
// Enemies get tougher on every floor
Enemy scaled(Enemy e, int floor) {
  int f = floor - 1;
  e.attack += f * 3;
  e.health += f * 15;
  e.xpReward += f * 10;
  e.goldReward += f * 10;
  return e;
}

bool battle(Hero &hero, Enemy enemy) {
  std::cout << "\n⚔ " << trim(CONF["battle"]) << " " << CYAN << hero.name << RESETCOLOUR << " vs " << RED << enemy.name << RESETCOLOUR << "\n";
  while (hero.health > 0 && enemy.health > 0) {
    // Out of mana at the start of the turn: the weapon is weaker (computed, not stored, so swapping weapons can't corrupt stats)
    bool exhausted = hero.mana == 0;
    int heroAttack = hero.baseAttack + std::max(0, hero.equippedWeapon.attack - (exhausted ? 3 : 0));
    // Class skill, only when we can pay for it (the old code compared against the skill damage instead of its cost)
    if (hero.mana >= hero.activeSkill.manaCost) {
      hero.mana -= hero.activeSkill.manaCost;
      heroAttack += hero.activeSkill.damage;
      std::cout << MAGENTA << hero.name << " " << trim(CONF["uses"]) << " " << hero.activeSkill.name << "!" << RESETCOLOUR << "\n";
    }
    // Spell: the bonus only applies when its mana is paid
    int upkeep = 10 + hero.equippedSpell.manaCost;
    bool manaPaid = hero.mana >= upkeep;
    if (manaPaid) { hero.mana -= upkeep; heroAttack += hero.equippedSpell.attack; }
    else { hero.mana = 0; }
    if (rnd(100) < hero.critChance) { heroAttack *= 2; std::cout << YELLOW << trim(CONF["criticalHit"]) << "!" << RESETCOLOUR << "\n"; }
    // One status roll per turn, and only if the enemy has none yet (it used to re-roll 4 times and reset every turn)
    if (enemy.status == NONE && rnd(100) < 30) {
      switch (rnd(4)) {
        case 0: enemy.status = FREEZE; enemy.statusTurns = 1; std::cout << trim(CONF["enemyFrozen2"]) << "!\n"; break;
        case 1: enemy.status = POISON; enemy.statusTurns = 3; std::cout << trim(CONF["enemyPoisoned2"]) << "!\n"; break;
        case 2: enemy.status = BLEED;  enemy.statusTurns = 2; std::cout << trim(CONF["enemyBleeding"]) << "!\n"; break;
        default: enemy.status = BURN;  enemy.statusTurns = 2; std::cout << trim(CONF["enemyBurning"]) << "!\n"; break;
      }
    }
    bool skipTurn = processStatus(enemy);
    enemy.health -= heroAttack;
    if (hero.companion.health > 0) { enemy.health -= hero.companion.attack; }
    std::cout << CYAN << hero.name << RESETCOLOUR << " " << trim(CONF["attacksFor"]) << " " << YELLOW << heroAttack << RESETCOLOUR << " " << trim(CONF["damage"]) << "! "
      << BLUE << trim(CONF["companionAttacksFor"]) << " " << (hero.companion.health > 0 ? hero.companion.attack : 0) << " " << trim(CONF["damage"]) << " " << RESETCOLOUR
      << GREEN << (manaPaid ? trim(CONF["manaUsed"]) : trim(CONF["noManaUsed"])) << RESETCOLOUR << " | " << enemy.name << " " << trim(CONF["hp"])
      << ": " << RED << std::max(0, enemy.health) << RESETCOLOUR << "\n";
    if (enemy.health <= 0) {
      std::cout << MAGENTA << enemy.name << RESETCOLOUR << GREEN << " " << trim(CONF["isDefeated"]) << "!" << RESETCOLOUR << "\n";
      hero.kills++;
      hero.gold += enemy.goldReward;
      std::cout << CYAN << hero.name << RESETCOLOUR << " " << trim(CONF["earns"]) << " " << YELLOW << enemy.goldReward << RESETCOLOUR << " " << trim(CONF["goldTotalGold"]) << ": " << YELLOW << hero.gold << RESETCOLOUR << "\n";
      gainXP(hero, enemy.xpReward);
      hero.mana = std::min(hero.maxMana, hero.mana + 10);
      std::cout << BLUE << hero.name << RESETCOLOUR << " " << MAGENTA << trim(CONF["restoresMana"]) << RESETCOLOUR << "\n";
      return true;
    }
    // Enemy turn. A frozen enemy really skips it now (processStatus used to clear FREEZE before this check)
    if (skipTurn) { std::cout << enemy.name << GREEN << " " << trim(CONF["cantMove"]) << " !" << RESETCOLOUR << "\n"; continue; }
    int totalDefense = hero.baseDefense + hero.equippedArmour.defense;
    int damage = std::max(0, enemy.attack - totalDefense);
    if (rnd(100) < 15) { std::cout << MAGENTA << hero.name << RESETCOLOUR << GREEN << " " << trim(CONF["dodged"]) << "!" << RESETCOLOUR << "\n"; damage = 0; }
    hero.health = std::max(0, hero.health - damage);
    bool companionWasAlive = hero.companion.health > 0;
    hero.companion.health = std::max(0, hero.companion.health - damage);
    std::cout << RED << enemy.name << RESETCOLOUR << " " << trim(CONF["attacksFor"]) << " " << RED << enemy.attack << RESETCOLOUR
      << " (" << trim(CONF["blocked"]) << " " << MAGENTA << totalDefense << RESETCOLOUR << ") " << trim(CONF["damageTaken"]) << ": " << YELLOW << damage
      << RESETCOLOUR << RED << " | " << trim(CONF["companion"]) << RESETCOLOUR << ": " << damage << YELLOW << " " << hero.name << RESETCOLOUR
      << " " << trim(CONF["hp"]) << ": " << GREEN << std::max(0, hero.health) << RESETCOLOUR << " | " << RED << trim(CONF["companion"]) << " "
      << std::max(0, hero.companion.health) << trim(CONF["hp"]) << RESETCOLOUR << "\n";
    // Auto-use a potion when below a third of the max HP (was a fixed 30, which scaled badly)
    if (hero.health > 0 && hero.health <= hero.maxHP / 3 && !hero.inventoryPotions.empty()) { std::cout << CYAN << "⚠ " << trim(CONF["autoUsingPotion"]) << "..." << RESETCOLOUR << "\n"; usePotion(hero); }
    if (companionWasAlive && hero.companion.health == 0) { std::cout << RED << trim(CONF["companion"]) << " " << trim(CONF["hasFallen"]) << "..." << RESETCOLOUR << "\n"; }
    if (hero.health <= 0) { std::cout << RED << hero.name << " " << trim(CONF["hasFallen"]) << "..." << RESETCOLOUR << "\n"; return false; }
  }
  return false;
}

void lootRoom(Hero &hero, std::vector<Armour> &armours, std::vector<Weapon> &weapons, std::vector<Potion> &potionsList, std::vector<Spell> &spells) {
  int lootType = rnd(4);
  if (lootType == 0) {
    Potion lootPotion = potionsList[rnd(static_cast<int>(potionsList.size()))];
    hero.inventoryPotions.emplace_back(lootPotion);
    std::cout << GREEN << "🧪 " << trim(CONF["foundHealthPotion"]) << ": " << hero.inventoryPotions.size() << ", " << lootPotion.healAmount << trim(CONF["hp"]) << RESETCOLOUR << "\n";
  }
  else if (lootType == 1) {
    Weapon newWeapon = weapons[rnd(static_cast<int>(weapons.size()))];
    hero.inventoryWeapons.emplace_back(newWeapon);
    std::cout << GREEN << "⚔ " << trim(CONF["foundNewWeapon"]) << ": " << newWeapon.name << " (" << trim(CONF["attack"]) << " +" << newWeapon.attack << ")" << RESETCOLOUR << "\n";
  }
  else if (lootType == 2) {
    Spell newSpell = spells[rnd(static_cast<int>(spells.size()))];
    hero.inventorySpells.emplace_back(newSpell);
    std::cout << GREEN << "🪄 " << trim(CONF["foundNewSpell"]) << ": " << newSpell.name << " (" << trim(CONF["attack"]) << " +" << newSpell.attack << "-" << trim(CONF["manaCost"]) << " " << newSpell.manaCost << ")" << RESETCOLOUR << "\n";
  }
  else {
    Armour newArmour = armours[rnd(static_cast<int>(armours.size()))];
    hero.inventoryArmours.emplace_back(newArmour);
    std::cout << GREEN << "🛡 " << trim(CONF["foundNewArmour"]) << ": " << newArmour.name << " (" << trim(CONF["defense"]) << " +" << newArmour.defense << ")" << RESETCOLOUR << "\n";
  }
}

void usePotion(Hero &hero) {
  if (!hero.inventoryPotions.empty()) {
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

bool trapRoom(Hero &hero) {
  int damage = 5 + rnd(16); // 5-20 damage
  hero.health = std::max(0, hero.health - damage);
  std::cout << RED << "💥 " << trim(CONF["trapTriggered"]) << " " << damage << " " << trim(CONF["damageHP"]) <<  ": " << hero.health << RESETCOLOUR << "\n";
  return hero.health > 0;
}

void chestRoom(Hero &hero, std::vector<Armour> &armours) {
  if (rnd(2) == 0) {
    int goldFound = 10 + rnd(41); // 10-50 gold
    hero.gold += goldFound;
    std::cout << GREEN << "💰 " << trim(CONF["foundTreasureChestWith"]) << " " << goldFound << " " << trim(CONF["goldTotalGold"]) << ": " << hero.gold << RESETCOLOUR << "\n";
  } else {
    Armour rareArmour = armours[rnd(static_cast<int>(armours.size()))];
    hero.inventoryArmours.emplace_back(rareArmour);
    std::cout << GREEN << "✨ " << trim(CONF["foundRareArmour"]) << ": " << rareArmour.name << " (" << trim(CONF["defense"]) << " +" << rareArmour.defense << RESETCOLOUR << ")\n";
  }
}

bool exploreRoom(Hero &hero, std::vector<Enemy> &enemies, std::vector<Armour> &armours, std::vector<Weapon> &weapon, char &cell, std::vector<Potion> &potionsList, std::vector<Spell> &spells, int floor) {
  if (cell == 'E') {
    Enemy enemy = scaled(enemies[rnd(static_cast<int>(enemies.size()))], floor);
    // 1 in 10 enemies is an elite: stronger, but pays more
    if (rnd(10) == 0) {
      enemy.name = T("elite", "Elite") + " " + enemy.name;
      enemy.attack = enemy.attack * 3 / 2; enemy.health = enemy.health * 3 / 2;
      enemy.xpReward *= 2; enemy.goldReward *= 2;
    }
    cell = '.';
    return battle(hero, enemy);
  } else if (cell == 'L') {
    lootRoom(hero, armours, weapon, potionsList, spells);
    cell = '.';
    return true;
  } else if (cell == 'B') {
    Enemy boss = scaled({.name = trim(CONF["dungeonBoss"]), .attack = 25, .health = 80, .xpReward = 50, .goldReward = 50}, floor);
    cell = '.';
    return battle(hero, boss);
  } else if (cell == 'G') {
    Enemy boss = scaled({.name = trim(CONF["arenaBoss"]), .attack = 26, .health = 85, .xpReward = 60, .goldReward = 60}, floor);
    cell = '.';
    return battle(hero, boss);
  } else if (cell == 'Z') {
    Enemy boss = scaled({.name = trim(CONF["tarkanBoss"]), .attack = 27, .health = 90, .xpReward = 70, .goldReward = 70}, floor);
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
  } else if (cell == 'M') {
    // NEW: wandering merchant, reuses the shop between floors
    std::cout << CYAN << "🧙 " << T("merchantRoom", "A wandering merchant offers his wares") << RESETCOLOUR << "\n";
    shop(hero, armours, potionsList);
    cell = '.';
    return true;
  } else {
    std::cout << "🚪 " << trim(CONF["emptyRoom"]) << ".\n";
    return true;
  }
}

// ---------------------------------------------------------------- shops
void shop(Hero &hero, std::vector<Armour> &armours, std::vector<Potion> &potionsList) {
  const int potionCost = potionsList[0].cost; // the menu said 20 but the potion is worth 15; use one number everywhere
  std::cout << CYAN << "\n🏪 " << trim(CONF["welcomeYouHave"]) << " " << RESETCOLOUR << YELLOW << hero.gold << " " << trim(CONF["gold"]) << "." << RESETCOLOUR << "\n";
  std::cout << YELLOW << "1." << RESETCOLOUR << BLUE << " " << trim(CONF["buyPotion"]) << " " << RESETCOLOUR << YELLOW << "(" << potionCost << " " << trim(CONF["gold"]) << ")" << RESETCOLOUR << "\n";
  std::cout << YELLOW << "2." << RESETCOLOUR << BLUE << " " << trim(CONF["buyArmourUpgrade"]) << " " << RESETCOLOUR << YELLOW << "(50 " << trim(CONF["gold"]) << ")" << RESETCOLOUR << "\n";
  std::cout << YELLOW << "3." << RESETCOLOUR << " " << trim(CONF["skip"]) << "\n";
  int choice = readNumber();
  if (choice == 1) {
    if (hero.gold >= potionCost) {
      hero.gold -= potionCost;
      hero.inventoryPotions.emplace_back(potionsList[0]);
      std::cout << GREEN << trim(CONF["boughtPotion"]) << ": " << hero.inventoryPotions.size() << RESETCOLOUR << "\n";
    } else {
      std::cout << RED << trim(CONF["notEnoughGold"]) << "!" << RESETCOLOUR << "\n";
    }
  } else if (choice == 2) {
    if (hero.gold >= 50) {
      hero.gold -= 50;
      Armour newArmour = armours[rnd(static_cast<int>(armours.size()))];
      hero.inventoryArmours.emplace_back(newArmour);
      std::cout << BLUE << trim(CONF["boughtArmour"]) << ": " << newArmour.name << " (" << trim(CONF["defense"]) << " +" << newArmour.defense << ")" << RESETCOLOUR << "\n";
    } else { std::cout << RED << trim(CONF["notEnoughGold"]) << "!" << RESETCOLOUR << "\n"; }
  } else { std::cout << YELLOW << trim(CONF["leavingShop"]) << "..." << RESETCOLOUR << "\n"; }
}

void weaponShop(Hero &hero, std::vector<Weapon> &weapons) {
  std::cout << CYAN << "\n⚔ " << trim(CONF["weaponShopYouHave"]) << " " << RESETCOLOUR << YELLOW << hero.gold << " " << trim(CONF["gold"]) << "." << RESETCOLOUR << "\n";
  std::cout << MAGENTA << trim(CONF["curEquippedWeapon"]) << ": " << RESETCOLOUR << CYAN << hero.equippedWeapon.name << " (" << trim(CONF["attack"]) << " +" << hero.equippedWeapon.attack << ")" << RESETCOLOUR << "\n";
  for (size_t i = 0; i < weapons.size(); ++i) { std::cout << YELLOW << i+1 << ". " << RESETCOLOUR << CYAN << weapons[i].name << " (" << trim(CONF["attack"]) << " +" << RESETCOLOUR
    << YELLOW << weapons[i].attack  << RESETCOLOUR << "," << YELLOW << " " << trim(CONF["cost"]) << ": " << weapons[i].cost << " " << trim(CONF["gold"]) << RESETCOLOUR << CYAN << ")" << RESETCOLOUR << "\n"; }
  std::cout << "0. " << trim(CONF["skip"]) << "\n";
  std::cout << BLUE << trim(CONF["chooseWeaponNumber"]) << ": " << RESETCOLOUR;
  int choice = readNumber(); // readNumber() also handles a closed stdin, unlike the raw std::cin
  if (choice < 0 || choice > static_cast<int>(weapons.size())) { std::cout << RED << trim(CONF["invalidChoiceSkipping"]) << "." << RESETCOLOUR << "\n"; return; }
  if (choice == 0) return;
  Weapon selected = weapons[choice-1];
  if (hero.gold >= selected.cost) {
    hero.gold -= selected.cost;
    hero.inventoryWeapons.emplace_back(selected);
    equipWeapon(hero, weapons, choice - 1, 1);
  } else { std::cout << RED << trim(CONF["notEnoughGold"]) << "!" << RESETCOLOUR << "\n"; }
}

void potionShop(Hero &hero, std::vector<Potion> &potionsList) {
  std::cout << CYAN << "\n🍵 " << trim(CONF["potionShopYouHave"]) << " " << RESETCOLOUR << YELLOW << hero.gold << " " << trim(CONF["gold"]) << "." << RESETCOLOUR << "\n";
  for (size_t i = 0; i < potionsList.size(); i++) { std::cout << YELLOW << i+1 << ". " << RESETCOLOUR << CYAN << potionsList[i].name << " (" << trim(CONF["heal"]) << " +" << RESETCOLOUR
    << YELLOW << potionsList[i].healAmount << RESETCOLOUR << "," << YELLOW << " " << trim(CONF["cost"]) << ": " << potionsList[i].cost << " " << trim(CONF["gold"]) << RESETCOLOUR << CYAN << ")" << RESETCOLOUR << "\n"; }
  std::cout << "0. " << trim(CONF["skip"]) << "\n";
  std::cout << BLUE << trim(CONF["choosePotion"]) << ": " << RESETCOLOUR;
  int choice = readNumber();
  if (choice < 0 || choice > static_cast<int>(potionsList.size())) { std::cout << trim(CONF["invalidChoiceSkipping"]) << ".\n"; return; }
  if (choice == 0) return;
  Potion selected = potionsList[choice-1];
  if (hero.gold >= selected.cost) {
    hero.gold -= selected.cost;
    hero.inventoryPotions.emplace_back(selected);
    std::cout << GREEN << trim(CONF["bought"]) << " " << selected.name << ". " << trim(CONF["youNowHave"]) << " " << hero.inventoryPotions.size() << " " << trim(CONF["potions"]) << "." << RESETCOLOUR << "\n";
  } else { std::cout << RED << trim(CONF["notEnoughGold"]) << "!" << RESETCOLOUR << "\n"; }
}

// ---------------------------------------------------------------- map
std::vector<std::vector<char>> generateMap(int size) {
  std::vector<std::vector<char>> map(size, std::vector<char>(size, '.'));
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) {
      switch (rnd(8)) {
        case 0: map[i][j] = 'E'; break; // Enemy
        case 1: map[i][j] = 'L'; break; // Loot
        case 2: map[i][j] = 'T'; break; // Trap
        case 3: map[i][j] = 'C'; break; // Chest
        case 4: map[i][j] = 'F'; break; // Healing fountain
        case 5: map[i][j] = 'S'; break; // Mana shrine
        case 6: map[i][j] = 'M'; break; // Merchant (new)
        default: break;                 // Empty
      }
    }
  }
  map[0][0] = '.'; // the start is always safe (it used to hold a random room)
  auto pick = [&](char c) {
    for (int tries = 0; tries < 100; ++tries) {
      int x = rnd(size);
      int y = rnd(size);
      if (x == 0 && y == 0) continue;
      if (map[x][y] == 'B' || map[x][y] == 'G' || map[x][y] == 'Z') continue;
      map[x][y] = c;
      return;
    }
  };
  pick('B');
  pick('G');
  pick('Z');
  return map;
}

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

// Gain XP; a big reward can now give several levels at once
void gainXP(Hero &hero, int amount) {
  hero.xp += amount;
  std::cout << GREEN << hero.name << RESETCOLOUR << YELLOW << " " << trim(CONF["gained"]) << " " << RESETCOLOUR << CYAN << amount << " " << trim(CONF["xp"]) << "!" << RESETCOLOUR << "\n";
  while (hero.xp >= hero.level * 50) {
    hero.xp -= hero.level * 50;
    hero.level++;
    hero.baseAttack += 5;
    hero.maxHP += 5;
    hero.maxMana += 5;
    hero.baseDefense += 2;
    hero.companion.maxHP += 5;
    hero.health = std::min(hero.maxHP, hero.health + 20);
    hero.mana = std::min(hero.maxMana, hero.mana + 20);
    hero.companion.health = std::min(hero.companion.maxHP, hero.companion.health + 20);
    for (const auto &skill : hero.skills) {
      if (skill.unlockLevel == hero.level) {
        hero.activeSkill = skill;
        std::cout << GREEN << trim(CONF["newSkillUnlocked"]) << ": " << skill.name << RESETCOLOUR << "\n";
        break;
      }
    }
    std::cout << GREEN << "*** " << trim(CONF["levelUP"]) << "! *** " << RESETCOLOUR << MAGENTA << hero.name << RESETCOLOUR << YELLOW << " " << trim(CONF["isNowLevel"]) << " " << hero.level << RESETCOLOUR << "!\n";
  }
}

// ---------------------------------------------------------------- input
#ifdef _WIN32
int readKey(void) {
  int c = _getch();
  if (c == 0 || c == 224) { // arrow keys send two values, the first is 0 or 224
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
  raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) < 0) { perror("tcsetattr ICANON"); }
  int result = read(STDIN_FILENO, &c, 1) == 1 ? c : -1;
  // Read the whole escape sequence while still raw
  if (result == 27) {
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1; // wait up to 0.1 s per byte
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    unsigned char intro = 0;
    if (read(STDIN_FILENO, &intro, 1) == 1 && (intro == '[' || intro == 'O')) {
      for (int len = 0; len < 16 && read(STDIN_FILENO, &c, 1) == 1; len++) {
        if (intro == 'O' || (c >= '@' && c <= '~')) {
          if (len == 0 && c >= 'A' && c <= 'D') { result = KEY_UP + (c - 'A'); } // up, down, right, left
          break;
        }
      }
    }
  }
  if (tcsetattr(STDIN_FILENO, TCSADRAIN, &old) < 0) { perror("tcsetattr ~ICANON"); }
  return result;
}
#endif /* _WIN32 */

// Configure the movement/inventories/potion consumption. Duplicate keys and Q (quit) are refused
void setupKeys(void) {
  int x = 0;
  std::cout << trim(CONF["pressKeyFor"]) << " ";
  for (auto text : { "right", "up", "left", "down", "armourInventory",
      "weaponInventory", "consumeApotion", "spellInventory" }) {
    std::cout << trim(CONF[text]);
    while (true) {
      int key = readKey();
      if (key < 0) { std::exit(EXIT_SUCCESS); } // stdin closed
      if (key < 256) { key = std::tolower(key); }
      bool ok = key != 'q';
      for (int i = 0; i < x; ++i) { if (keys[i] == key) { ok = false; } }
      if (ok) { keys[x++] = key; break; }
      std::cout << RED << "\n" << T("keyTaken", "That key is already used or reserved, press another one") << RESETCOLOUR << ": " << trim(CONF[text]);
    }
  }
  std::cout << BLUE << "\n" << trim(CONF["keysEstablished"]) << "\n" << RESETCOLOUR << std::endl;
}

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

// Applies the status damage of this turn. Returns true when the enemy loses its turn (frozen)
bool processStatus(Enemy &enemy) {
  bool skipTurn = false;
  switch (enemy.status) {
    case BURN:
      std::cout << enemy.name << " " << trim(CONF["enemySuffersBurn"]) << "!\n";
      enemy.health -= 5;
      break;
    case FREEZE:
      std::cout << enemy.name << " " << trim(CONF["enemyFrozen"]) << " !\n";
      skipTurn = true;
      break;
    case BLEED:
      std::cout << enemy.name << " " << trim(CONF["enemyBleeds"]) << " !\n";
      enemy.health -= 4;
      break;
    case POISON:
      std::cout << enemy.name << " " << trim(CONF["enemyPoisoned"]) << "!\n";
      enemy.health -= 3;
      break;
    default:
      break;
  }
  if (enemy.status != NONE) {
    enemy.statusTurns--;
    if (enemy.statusTurns <= 0) { enemy.status = NONE; std::cout << enemy.name << " " << trim(CONF["enemyRecovered"]) << ".\n"; }
  }
  return skipTurn;
}