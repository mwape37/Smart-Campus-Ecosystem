#include "DataManager.h"
#include "LabHardware.h"
#include "Cafeteria.h"
#include "BookStore.h"
#include <iomanip>
DataManager::DataManager(const string& resFile, const string& uFile,
                         const string& transFile)
    : resourceFile(resFile), userFile(uFile), transactionFile(transFile) {}

// JSON helpers 
// Returns every top-level { ... } block found inside a JSON array string.
vector<string> DataManager::splitJsonArray(const string& json) {
    vector<string> objects;
    int depth = 0, start = -1;
    for (int i = 0; i < (int)json.size(); i++) {
        if (json[i] == '{') {
            if (depth == 0) start = i;
            depth++;
        } else if (json[i] == '}') {
            depth--;
            if (depth == 0 && start >= 0) {
                objects.push_back(json.substr(start, i - start + 1));
                start = -1;
            }
        }
    }
    return objects;
}

// Extracts a JSON string value: "key": "value"
string DataManager::getJsonString(const string& obj, const string& key) {
    string search = "\"" + key + "\"";
    size_t pos = obj.find(search);
    if (pos == string::npos) return "";
    pos = obj.find(':', pos + search.size());
    if (pos == string::npos) return "";
    pos = obj.find('"', pos);
    if (pos == string::npos) return "";
    pos++;  // skip opening quote
    string result;
    while (pos < obj.size() && obj[pos] != '"') {
        if (obj[pos] == '\\' && pos + 1 < obj.size()) {
            pos++;
            if      (obj[pos] == '"')  result += '"';
            else if (obj[pos] == '\\') result += '\\';
            else if (obj[pos] == 'n')  result += '\n';
            else if (obj[pos] == 'r')  result += '\r';
            else if (obj[pos] == 't')  result += '\t';
            else                       result += obj[pos];
        } else {
            result += obj[pos];
        }
        pos++;
    }
    return result;
}

// Extracts a JSON numeric value (returned as a string): "key": 42
string DataManager::getJsonNumber(const string& obj, const string& key) {
    string search = "\"" + key + "\"";
    size_t pos = obj.find(search);
    if (pos == string::npos) return "0";
    pos = obj.find(':', pos + search.size());
    if (pos == string::npos) return "0";
    pos++;
    while (pos < obj.size() &&
           (obj[pos] == ' ' || obj[pos] == '\t' ||
            obj[pos] == '\n' || obj[pos] == '\r')) pos++;
    string num;
    while (pos < obj.size() &&
           (isdigit((unsigned char)obj[pos]) || obj[pos] == '.' || obj[pos] == '-'))
        num += obj[pos++];
    return num.empty() ? "0" : num;
}

// Escapes a string so it is safe inside JSON double quotes.
string DataManager::jsonEscape(const string& s) {
    string out;
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else                out += c;
    }
    return out;
}

// Resources 

void DataManager::saveResources(const vector<Resource*>& resources) const {
    try {
        ofstream out(resourceFile);
        if (!out.is_open())
            throw runtime_error("Cannot open file: " + resourceFile);

        out << "[\n";
        for (size_t i = 0; i < resources.size(); i++) {
            const Resource* r = resources[i];

            // Pull the subclass-specific "extra" field via dynamic_cast.
            string extra;
            if (auto* lh = dynamic_cast<const LabHardware*>(r))
                extra = lh->getWarrantyDate();
            else if (auto* c = dynamic_cast<const Cafeteria*>(r))
                extra = c->getExpiryDate();
            else if (auto* b = dynamic_cast<const BookStore*>(r))
                extra = b->getAuthor();

            // Write price as an integer when it has no fractional part,
            // matching the format server.js already produces.
            ostringstream priceStr;
            double p = r->getPrice();
            if (p == static_cast<int>(p))
                priceStr << static_cast<int>(p);
            else
                priceStr << fixed << setprecision(2) << p;

            out << "  {\n"
                << "    \"id\": "          << r->getId()                    << ",\n"
                << "    \"name\": \""      << jsonEscape(r->getName())      << "\",\n"
                << "    \"category\": \"" << jsonEscape(r->getCategory())  << "\",\n"
                << "    \"price\": "      << priceStr.str()                 << ",\n"
                << "    \"stock\": "      << r->getStock()                  << ",\n"
                << "    \"extra\": \""    << jsonEscape(extra)              << "\"\n"
                << "  }";
            if (i + 1 < resources.size()) out << ",";
            out << "\n";
        }
        out << "]\n";
        cout << "[DataManager] Saved " << resources.size()
             << " resource(s) to '" << resourceFile << "'.\n";
    } catch (const exception& e) {
        cerr << "[DataManager ERROR] " << e.what() << "\n";
    }
}

vector<Resource*> DataManager::loadResources() const {
    vector<Resource*> resources;
    try {
        ifstream in(resourceFile);
        if (!in.is_open())
            throw runtime_error("File not found: " + resourceFile);

        string json((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        for (const auto& obj : splitJsonArray(json)) {
            Resource* r = deserializeResource(obj);
            if (r) resources.push_back(r);
        }
        cout << "[DataManager] Loaded " << resources.size()
             << " resource(s) from '" << resourceFile << "'.\n";
    } catch (const exception& e) {
        cerr << "[DataManager] " << e.what()
             << " — starting with empty inventory.\n";
    }
    return resources;
}

Resource* DataManager::deserializeResource(const string& obj) const {
    try {
        int    id    = stoi(getJsonNumber(obj, "id"));
        string name  = getJsonString(obj, "name");
        string cat   = getJsonString(obj, "category");
        double price = stod(getJsonNumber(obj, "price"));
        int    stock = stoi(getJsonNumber(obj, "stock"));
        string extra = getJsonString(obj, "extra");

        if (cat == "Lab Hardware")
            return new LabHardware(id, name, price, stock, extra);
        if (cat == "Cafeteria Perishables")
            return new Cafeteria(id, name, price, stock, extra);
        if (cat == "Bookstore Media")
            return new BookStore(id, name, price, stock, extra);

        return new Resource(id, name, cat, price, stock);
    } catch (const exception& e) {
        cerr << "[DataManager] Skipping corrupt resource: " << e.what() << "\n";
        return nullptr;
    }
}

// Users 

void DataManager::saveUsers(const vector<User*>& users) const {
    try {
        ofstream out(userFile);
        if (!out.is_open())
            throw runtime_error("Cannot open file: " + userFile);

        out << "[\n";
        for (size_t i = 0; i < users.size(); i++) {
            const User* u = users[i];
            out << "  {\n"
                << "    \"campusID\": \"" << jsonEscape(u->getCampusID()) << "\",\n"
                << "    \"name\": \""     << jsonEscape(u->getName())     << "\",\n"
                << "    \"role\": \""     << jsonEscape(u->getRole())     << "\"\n"
                << "  }";
            if (i + 1 < users.size()) out << ",";
            out << "\n";
        }
        out << "]\n";
        cout << "[DataManager] Saved " << users.size()
             << " user(s) to '" << userFile << "'.\n";
    } catch (const exception& e) {
        cerr << "[DataManager ERROR] " << e.what() << "\n";
    }
}

vector<User*> DataManager::loadUsers() const {
    vector<User*> users;
    try {
        ifstream in(userFile);
        if (!in.is_open())
            throw runtime_error("File not found: " + userFile);

        string json((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        for (const auto& obj : splitJsonArray(json)) {
            User* u = deserializeUser(obj);
            if (u) users.push_back(u);
        }
        cout << "[DataManager] Loaded " << users.size()
             << " user(s) from '" << userFile << "'.\n";
    } catch (const exception& e) {
        cerr << "[DataManager] " << e.what()
             << " — starting with no users.\n";
    }
    return users;
}

User* DataManager::deserializeUser(const string& obj) const {
    try {
        string campusID = getJsonString(obj, "campusID");
        string name     = getJsonString(obj, "name");
        string role     = getJsonString(obj, "role");

        if (role == "Student") return new Student(name, campusID);
        if (role == "Staff")   return new Staff(name, campusID);

        cerr << "[DataManager] Unknown role '" << role << "' — skipping.\n";
        return nullptr;
    } catch (const exception& e) {
        cerr << "[DataManager] Skipping corrupt user: " << e.what() << "\n";
        return nullptr;
    }
}

// Transactions 

void DataManager::logTransaction(const string& entry) const {
    try {
        ofstream out(transactionFile, ios::app);
        if (!out.is_open())
            throw runtime_error("Cannot open file: " + transactionFile);
        out << entry << "\n";
    } catch (const exception& e) {
        cerr << "[DataManager ERROR] " << e.what() << "\n";
    }
}
