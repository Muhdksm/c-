# Dungeon Quest 🗡️

A mini terminal dungeon-crawler RPG written in C++.

## Features

- Turn-based combat with attack / defend choices
- 5 dungeon floors with scaling enemies
- Random item drops (health potions, weapons, armour)
- Difficulty selection (Easy / Normal / Hard)
- Epic final boss fight

## Requirements

- A C++17 compiler (`g++` or `clang++`)
- `make` (optional — you can compile manually too)

## Build & Run

```bash
# with Make
make
./dungeon

# or compile directly
g++ -std=c++17 -O2 -o dungeon game.cpp
./dungeon
```

## How to Play

1. Enter your hero's name and choose a difficulty.
2. On each floor you will encounter 2–3 monsters. Before each round:
   - `1` — **Attack**: deal full damage to the enemy.
   - `2` — **Defend**: halve incoming damage this round.
3. After battles you may find items — type `y` to use them, `n` to skip.
4. Survive all 5 floors and defeat the **Dragon** to win!

## Clean up

```bash
make clean
```
