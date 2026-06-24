Smart Campus Ecosystem
A dual-interface campus resource management system built with C++ (CLI) and Node.js + HTML (Web).

Overview
The Smart Campus Ecosystem allows campus staff and students to manage resources (Lab Hardware, Cafeteria items, Bookstore books), place orders, track users, and monitor inventory — all through either a terminal-based CLI or a browser-based web dashboard.

Features
- Inventory Management — Add, remove, and browse campus resources across three categories: Lab Hardware, Cafeteria, and Bookstore
- User Management — Register and manage students and staff by campus ID
- Order Placement — Place orders with multiple delivery/payment methods
- Reports — Compare resources and generate inventory summaries
- Background Monitors (C++ CLI) — Auto-running threads for low-stock alerts, expiry warnings, and periodic auto-save
- Web Dashboard — Zero-dependency Node.js server with a full single-page frontend (`index.html`)
- Persistent Storage — Data saved to JSON files; transactions logged automatically

Getting Started

Option 1 – Web Interface (Node.js)
Requirements: Node.js v14+

bash
node server.js

Then open your browser at: [http://localhost:3000](http://localhost:3000)

No npm install needed — the server uses only built-in Node.js modules.


Option 2 – CLI (C++)
Requirements: A C++17-compatible compiler (g++, clang++)

Compile:
bash
g++ -std=c++17 -o CampusSystem "Source Files/Main.cpp" "Source Files/Campus.cpp" \
    "Source Files/User.cpp" "Source Files/Resource.cpp" "Source Files/Order.cpp" \
    "Source Files/Payment.cpp" "Source Files/DeliveryMethod.cpp" \
    "Source Files/DataManager.cpp" "Source Files/Cafeteria.cpp" \
    "Source Files/BookStore.cpp" "Source Files/LabHardware.cpp" \
    -I "Header files" -pthread

Run:
bash
./CampusSystem


On Windows you can also run the pre-built `CampusSystem.exe` directly.

Tech Stack

CLI Application: C++17, pthreads 
Web Server: Node.js (no dependencies) 
Web Frontend: Vanilla HTML/CSS/JS 
Data Storage: JSON files 
