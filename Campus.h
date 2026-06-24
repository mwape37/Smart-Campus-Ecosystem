#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <limits>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <cstdio>
#include <climits>
#include "Resource.h"
#include "LabHardware.h"
#include "Cafeteria.h"
#include "BookStore.h"
#include "User.h"
#include "Order.h"
#include "Payment.h"
#include "DeliveryMethod.h"
#include "DataManager.h"
using namespace std;

class Campus {
    vector<Resource*> inventory;
    vector<User*>     users;
    DataManager       dataManager;
    int               nextResourceId;

    // recursive_mutex lets the same thread re-lock without deadlocking,
    // which happens when a locking public method calls another locking method
    mutable recursive_mutex inventoryMutex;

    // Prevents console lines from two threads mixing together
    mutable mutex      consoleMutex;

    // Used with shutdownCV to wake sleeping monitor threads immediately on exit
    mutex              cvMutex;
    condition_variable shutdownCV;
    atomic<bool>       monitorsRunning;

    // One thread per monitor
    thread lowStockThread;
    thread expiryThread;
    thread autoSaveThread;

    // Monitor intervals and thresholds — adjust as needed
    static constexpr int LOW_STOCK_THRESHOLD = 5;    // units
    static constexpr int EXPIRY_WARNING_DAYS = 3;    // days
    static constexpr int LOW_STOCK_INTERVAL  = 30;   // seconds
    static constexpr int EXPIRY_INTERVAL     = 60;   // seconds
    static constexpr int AUTOSAVE_INTERVAL   = 60;   // seconds

    // Prints a complete message without interleaving with other threads
    void safePrint(const string& msg) const;

    // Sleeps for 'seconds' but returns early if shutdown is requested.
    // Returns true = keep running, false = shut down
    bool interruptibleSleep(int seconds);

    // Parses "YYYY-MM-DD" and returns days until that date; negative = already expired
    static int daysUntilExpiry(const string& dateStr);

    // Monitor thread bodies
    void lowStockMonitor();
    void expiryMonitor();
    void autoSaveMonitor();

    // Launches all three monitor threads
    void startMonitors();

    // Signals threads to wake and stop, then blocks until all have finished
    void stopMonitors();

    // Input helpers — retry until clean input is received
    int    getIntInput(const string& prompt)    const;
    double getDoubleInput(const string& prompt) const;
    string getStringInput(const string& prompt) const;

    // Formats a double as "x.xx" for transaction log entries
    string formatAmount(double amount) const;

    // Sub-menu handlers
    void menuResources();
    void menuUsers();
    void menuPlaceOrder();
    void menuReports();

    void printHeader() const;

public:
    Campus();
    ~Campus();

    // Adds a resource and keeps nextResourceId in sync
    void addResource(Resource* r);

    // Returns pointer to resource with given ID, or nullptr
    Resource* findResourceById(int id) const;

    // Prints a formatted report for one resource by ID
    void printResourceReport(int id) const;

    // Prints a summary table of the full inventory
    void printAllResources() const;

    // Compares two resources by price and prints the result
    void compareTwoResources(int id1, int id2) const;

    // Registers a user; returns false if campus ID is already taken
    bool  registerUser(User* u);

    // Removes resource by ID; returns false if not found
    bool removeResource(int id);

    // Removes user by campus ID; returns false if not found
    bool removeUser(const string& campusID);

    // Returns pointer to user with given campus ID, or nullptr
    User* findUserById(const string& campusID) const;

    // Loads resources and users from file on startup
    void loadData();

    // Saves resources and users to file
    void saveData() const;

    // Starts the interactive main menu loop
    void run();
};