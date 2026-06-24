#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>
#include "Resource.h"
#include "User.h"
using namespace std;

// Handles all file I/O — saves/loads resources, users, and transaction logs.
// Data is stored as JSON so the C++ CLI and the Node.js web GUI share the
// exact same files under the data/ directory.
class DataManager {
    string resourceFile;
    string userFile;
    string transactionFile;

    static vector<string> splitJsonArray(const string& json);
    static string getJsonString(const string& obj, const string& key);
    static string getJsonNumber(const string& obj, const string& key);
    static string jsonEscape(const string& s);

    Resource* deserializeResource(const string& obj) const;
    User*     deserializeUser(const string& obj) const;

public:
    // Defaults match the exact paths used by server.js — shared source of truth.
    explicit DataManager(
        const string& resFile   = "data/campus_resources.json",
        const string& uFile     = "data/campus_users.json",
        const string& transFile = "data/campus_transactions.log");

    void saveResources(const vector<Resource*>& resources) const;
    vector<Resource*> loadResources() const;

    void saveUsers(const vector<User*>& users) const;
    vector<User*> loadUsers() const;

    void logTransaction(const string& entry) const;
};
