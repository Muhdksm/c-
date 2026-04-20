/*  Dungeon Quest — a mini terminal RPG in C++
 *
 *  Build:  g++ -std=c++17 -O2 -o dungeon game.cpp
 *  Run:    ./dungeon
 */

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

// ── Utilities ────────────────────────────────────────────────────────────────

static void clearScreen() {
#if defined(_WIN32)
    std::system("cls");   // NOLINT
#else
    std::system("clear"); // NOLINT
#endif
    // Return value intentionally ignored — used only for screen clearing
    (void)0;
}

static void pause() {
    std::cout << "\n[Press ENTER to continue]";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

static int rollDice(int sides) {
    return (std::rand() % sides) + 1;
}

static void printBar(const std::string& label, int cur, int max, int width = 20) {
    int filled = (max > 0) ? (cur * width / max) : 0;
    std::cout << label << " [";
    for (int i = 0; i < width; ++i)
        std::cout << (i < filled ? '#' : '.');
    std::cout << "] " << cur << "/" << max;
}

// ── Entity ───────────────────────────────────────────────────────────────────

struct Entity {
    std::string name;
    int hp, maxHp;
    int attack, defense;
    bool alive() const { return hp > 0; }
    void takeDamage(int dmg) { hp = std::max(0, hp - dmg); }
};

// ── Items ────────────────────────────────────────────────────────────────────

struct Item {
    std::string name;
    enum class Type { HEALTH_POTION, WEAPON, ARMOR } type;
    int value; // heal amount / attack boost / defense boost
};

static const std::vector<Item> ITEM_POOL = {
    {"Health Potion",  Item::Type::HEALTH_POTION, 30},
    {"Greater Potion", Item::Type::HEALTH_POTION, 60},
    {"Iron Sword",     Item::Type::WEAPON,          8},
    {"Steel Axe",      Item::Type::WEAPON,         14},
    {"Leather Armor",  Item::Type::ARMOR,            5},
    {"Chain Mail",     Item::Type::ARMOR,            9},
};

// ── Monster templates ────────────────────────────────────────────────────────

struct MonsterTemplate {
    std::string name, ascii;
    int hp, attack, defense;
};

static const std::vector<MonsterTemplate> MONSTERS = {
    {"Slime",    "  (o_o)  ", 18,  4, 0},
    {"Goblin",   "  /\\(^)/\\", 28,  7, 1},
    {"Skeleton", "  &skull; [T_T]", 35,  9, 2},
    {"Orc",      "  [>#<]  ", 50, 12, 3},
    {"Vampire",  "  >=<>   ", 45, 14, 4},
};

static const MonsterTemplate BOSS = {
    "Dragon", "  <({*_*})>", 120, 20, 6
};

// ── Combat ───────────────────────────────────────────────────────────────────

static int calcDamage(const Entity& attacker, const Entity& defender) {
    int base = attacker.attack + rollDice(6) - 1;
    int dmg  = std::max(1, base - defender.defense);
    return dmg;
}

// Returns true if the player survived the fight
static bool doCombat(Entity& player, Entity enemy) {
    std::cout << "\n=== COMBAT: " << player.name << " vs " << enemy.name << " ===\n";

    while (player.alive() && enemy.alive()) {
        // Show status
        std::cout << "\n";
        printBar("  YOU  ", player.hp, player.maxHp);
        std::cout << "\n";
        printBar("  " + enemy.name, enemy.hp, enemy.maxHp);
        std::cout << "\n";

        // Player action
        std::cout << "\nYour turn!\n";
        std::cout << "  1) Attack\n";
        std::cout << "  2) Defend (reduce next hit by half)\n";
        std::cout << "Choice: ";

        int choice = 0;
        std::cin >> choice;
        std::cin.ignore();

        bool defending = (choice == 2);

        int playerDmg = calcDamage(player, enemy);
        enemy.takeDamage(playerDmg);
        std::cout << "  >> You deal " << playerDmg << " damage to " << enemy.name << "!\n";

        if (!enemy.alive()) {
            std::cout << "  >> " << enemy.name << " is defeated!\n";
            break;
        }

        // Enemy turn
        int enemyDmg = calcDamage(enemy, player);
        if (defending) enemyDmg = std::max(1, enemyDmg / 2);
        player.takeDamage(enemyDmg);
        std::cout << "  >> " << enemy.name << " hits you for " << enemyDmg
                  << " damage" << (defending ? " (blocked half)" : "") << "!\n";

        if (!player.alive()) {
            std::cout << "  >> You have been slain...\n";
        }
    }

    return player.alive();
}

// ── Shop / Item find ──────────────────────────────────────────────────────────

static void applyItem(Entity& player, const Item& item) {
    switch (item.type) {
        case Item::Type::HEALTH_POTION:
            player.hp = std::min(player.maxHp, player.hp + item.value);
            std::cout << "  >> HP restored by " << item.value
                      << " (now " << player.hp << "/" << player.maxHp << ")\n";
            break;
        case Item::Type::WEAPON:
            player.attack += item.value;
            std::cout << "  >> Attack increased by " << item.value
                      << " (now " << player.attack << ")\n";
            break;
        case Item::Type::ARMOR:
            player.defense += item.value;
            std::cout << "  >> Defense increased by " << item.value
                      << " (now " << player.defense << ")\n";
            break;
    }
}

static void offerItem(Entity& player) {
    const Item& item = ITEM_POOL[std::rand() % ITEM_POOL.size()];
    std::cout << "\n  * You find a " << item.name << "! Use it? (y/n): ";
    char c;
    std::cin >> c;
    std::cin.ignore();
    if (c == 'y' || c == 'Y') applyItem(player, item);
}

// ── Floor ────────────────────────────────────────────────────────────────────

static bool doFloor(Entity& player, int floor) {
    clearScreen();
    std::cout << "╔══════════════════════════════╗\n";
    std::cout << "║    DUNGEON QUEST — Floor " << floor
              << (floor < 10 ? "  " : " ") << "║\n";
    std::cout << "╚══════════════════════════════╝\n";
    std::cout << "\nYou descend into the darkness...\n";

    // 2–3 monsters per floor
    int numEnemies = 2 + (floor > 3 ? 1 : 0);
    for (int e = 0; e < numEnemies; ++e) {
        int idx = std::min((int)MONSTERS.size() - 1,
                           (floor - 1) + rollDice(2) - 1);
        const MonsterTemplate& tmpl = MONSTERS[idx];

        Entity enemy;
        enemy.name    = tmpl.name;
        enemy.hp      = tmpl.hp + (floor - 1) * 5;
        enemy.maxHp   = enemy.hp;
        enemy.attack  = tmpl.attack + (floor - 1) * 2;
        enemy.defense = tmpl.defense + (floor - 1);

        pause();
        std::cout << "\n" << tmpl.ascii << "\n";
        std::cout << "A wild " << enemy.name << " appears!\n";

        if (!doCombat(player, enemy)) return false;

        // Chance of item drop after each fight
        if (rollDice(3) == 1) {
            std::cout << "\n  * A chest is nearby...\n";
            offerItem(player);
        }
    }

    // Guaranteed item between floors
    std::cout << "\n  * You find a supply cache after clearing the floor!\n";
    offerItem(player);

    return true;
}

// ── Print header ─────────────────────────────────────────────────────────────

static void printTitle() {
    clearScreen();
    std::cout << R"(
  ██████╗ ██╗   ██╗███╗   ██╗ ██████╗ ███████╗ ██████╗ ███╗   ██╗
  ██╔══██╗██║   ██║████╗  ██║██╔════╝ ██╔════╝██╔═══██╗████╗  ██║
  ██║  ██║██║   ██║██╔██╗ ██║██║  ███╗█████╗  ██║   ██║██╔██╗ ██║
  ██║  ██║██║   ██║██║╚██╗██║██║   ██║██╔══╝  ██║   ██║██║╚██╗██║
  ██████╔╝╚██████╔╝██║ ╚████║╚██████╔╝███████╗╚██████╔╝██║ ╚████║
  ╚═════╝  ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝ ╚══════╝ ╚═════╝ ╚═╝  ╚═══╝
                       ██████╗ ██╗   ██╗███████╗███████╗████████╗
                      ██╔═══██╗██║   ██║██╔════╝██╔════╝╚══██╔══╝
                      ██║   ██║██║   ██║█████╗  ███████╗   ██║
                      ██║▄▄ ██║██║   ██║██╔══╝  ╚════██║   ██║
                      ╚██████╔╝╚██████╔╝███████╗███████║   ██║
                       ╚══▀▀═╝  ╚═════╝ ╚══════╝╚══════╝   ╚═╝
)";
    std::cout << "              A terminal dungeon RPG\n\n";
}

// ── Main ─────────────────────────────────────────────────────────────────────

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    printTitle();

    // Character creation
    std::cout << "Enter your hero's name: ";
    std::string name;
    std::getline(std::cin, name);
    if (name.empty()) name = "Hero";

    std::cout << "\nChoose difficulty:\n";
    std::cout << "  1) Easy   (HP: 120, Atk: 14, Def: 4)\n";
    std::cout << "  2) Normal (HP: 100, Atk: 12, Def: 3)\n";
    std::cout << "  3) Hard   (HP:  80, Atk: 10, Def: 2)\n";
    std::cout << "Choice [1-3]: ";

    int diff = 2;
    std::cin >> diff;
    std::cin.ignore();

    Entity player;
    player.name = name;
    switch (diff) {
        case 1: player.maxHp = 120; player.attack = 14; player.defense = 4; break;
        case 3: player.maxHp =  80; player.attack = 10; player.defense = 2; break;
        default: player.maxHp = 100; player.attack = 12; player.defense = 3; break;
    }
    player.hp = player.maxHp;

    std::cout << "\nWelcome, " << player.name << "! Your quest: reach floor 5 and slay the Dragon.\n";
    pause();

    // 5 floors of monsters
    const int TOTAL_FLOORS = 5;
    bool survived = true;
    for (int f = 1; f <= TOTAL_FLOORS && survived; ++f) {
        survived = doFloor(player, f);
        if (survived && f < TOTAL_FLOORS) {
            std::cout << "\n  You cleared floor " << f << "! Descending deeper...\n";
        }
    }

    if (!survived) {
        std::cout << "\n╔══════════════════════╗\n";
        std::cout << "║    GAME  OVER  :(    ║\n";
        std::cout << "╚══════════════════════╝\n";
        std::cout << "\n" << player.name << " fell in the dungeon.\n";
        std::cout << "Better luck next time!\n\n";
        return 0;
    }

    // Boss fight
    clearScreen();
    std::cout << "\n╔══════════════════════════════╗\n";
    std::cout << "║    THE FINAL BOSS AWAITS!    ║\n";
    std::cout << "╚══════════════════════════════╝\n";
    std::cout << "\n" << BOSS.ascii << "\n";
    std::cout << "The " << BOSS.name << " blocks your path!\n";
    pause();

    Entity boss;
    boss.name    = BOSS.name;
    boss.hp      = BOSS.hp;
    boss.maxHp   = BOSS.hp;
    boss.attack  = BOSS.attack;
    boss.defense = BOSS.defense;

    survived = doCombat(player, boss);

    std::cout << "\n";
    if (survived) {
        std::cout << "╔══════════════════════════════╗\n";
        std::cout << "║   *** YOU WIN!  \\o/  ***     ║\n";
        std::cout << "╚══════════════════════════════╝\n";
        std::cout << "\n" << player.name << " slew the Dragon and escaped the dungeon!\n";
        std::cout << "Remaining HP: " << player.hp << "/" << player.maxHp << "\n";
    } else {
        std::cout << "╔══════════════════════╗\n";
        std::cout << "║    GAME  OVER  :(    ║\n";
        std::cout << "╚══════════════════════╝\n";
        std::cout << "\n" << player.name << " was no match for the Dragon...\n";
    }

    std::cout << "\nThanks for playing Dungeon Quest!\n\n";
    return 0;
}
