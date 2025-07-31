# 🌟 Gold Miner ECS

A competitive 2-player game built entirely on **BAGEL**, a custom Entity Component System (ECS) framework developed during our game development course.

Dig, grab, collect, and compete — it's time to become the ultimate gold miner!

---
### 🧍‍♀️ Team Avatars

<table>
  <tr>
    <th>Amal</th>
    <th>Ofek</th>
    <th>Noa</th>
    <th>Elizabeth</th>
  </tr>
  <tr>
    <td><img width="160" src="https://github.com/user-attachments/assets/0bb61431-c0d1-4ad1-a1ef-1eca9f5f78d3" alt="Player_Amal" /></td>
    <td><img width="160" src="https://github.com/user-attachments/assets/972ba432-b3f0-478a-83bd-00bb99f81c27" alt="Player_Ofek" /></td>
    <td><img width="160" src="https://github.com/user-attachments/assets/fd6bfc76-1176-45ca-b2cc-214bd1ca65fc" alt="Player_Noa" /></td>
    <td><img width="160" src="https://github.com/user-attachments/assets/feade4dc-d12d-43f6-a1f7-f1335fd0cc61" alt="Player_Elizabeth" /></td>
  </tr>
</table>

---

## ✨ Gameplay Overview

Gold Miner ECS is a 2D arcade-style game in which two players compete in real-time to collect the most valuable treasures using retractable ropes. Timing, strategy, and a bit of luck will determine the winner.

* ⭐ Two players side-by-side
* ⚖️ Compete to collect gold, diamonds, and treasure chests
* ⏱ Each has a limited timer
* ✨ Each round ends with a clear winner — or a tie!

---

## 🚀 Powered by BAGEL ECS

This project runs on **BAGEL** (Barebones Game ECS Library) — a minimal, fast, and expressive ECS engine built during class sessions.

* Packed / Sparse / Tagged storage
* Custom bitmasking for performance
* No runtime heap allocations for core logic
* Built-in entity lifecycle and masking

> ✨ BAGEL was developed and taught by our instructor **Moshe Sulamy** during the course. Special thanks for this powerful and elegant ECS foundation!

---

## 🎓 Developed Features

* ✏️ Player character selection with custom avatars
* 🌊 5 thematic backgrounds (Desert, Underwater, Japan, Space, Classic)
* ⚛️ Rope physics powered by Box2D
* 🎧 Dynamic sound effects using SFML (stretch, coins, ticking clock)
* ⏰ Real-time countdown timer per player
* 🎁 Smart UI with pixel-art digits and icons
* 🎨 Smooth animations, retractable rope logic
* 🌈 Real-time collision detection with collectible logic
* ⚖️ Score system with dynamic value logic and weight-based rope speed

---

## 🕹️ Controls

| Player | Action    | Key          |
| ------ | --------- | ------------ |
| P1     | Send rope | `SPACE`      |
| P2     | Send rope | `ENTER`      |
| Any    | Select    | `LEFT/RIGHT` |
| Any    | Confirm   | `ENTER`      |
| Any    | Exit Game | `ESC`        |

---

## 🖼️ Screenshots

### 🤠 Desert Cowboy Theme

<img width="1282" height="711" alt="image" src="https://github.com/user-attachments/assets/3afaddb3-5d97-4db1-8777-e7339a59f300" />

### 🌊 Ocean Adventure Theme

<img width="1277" height="718" alt="image" src="https://github.com/user-attachments/assets/19ac1874-7f49-431a-a119-74957b15bbd8" />

### 🌟 Space Explorers Theme

<img width="1284" height="711" alt="image" src="https://github.com/user-attachments/assets/f44c582f-e152-41c2-8ebe-95272c52153a" />

### 🌞 Classic Miner Theme

<img width="1282" height="711" alt="image" src="https://github.com/user-attachments/assets/f73267a2-7ad6-4958-a712-f41a6cd49739" />

### 🌸 Japanese Festival Theme
<img width="1282" height="726" alt="image" src="https://github.com/user-attachments/assets/35504378-a574-415e-91cf-81e850b5dbaa" />
---

## 📹 Gameplay Demo
[![Watch Gameplay Demo](https://img.youtube.com/vi/rqiis5Ma67k/0.jpg)](https://youtu.be/rqiis5Ma67k)
---


## 💼 License

MIT License — feel free to use this project for learning, improvements, or fun!

---

## ✨ Want to Customize?

* Add more player skins in the `res/` folder
* Extend layouts in `gold_miner_ecs.cpp`
* Modify scoring logic or add power-ups!

### Happy mining! ⛏️
