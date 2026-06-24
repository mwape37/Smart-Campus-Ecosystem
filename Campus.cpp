#include "Campus.h"

Campus::Campus() : nextResourceId(1), monitorsRunning(false) {
    loadData();
    startMonitors();
}

// stopMonitors() first — threads must finish before memory is freed
Campus::~Campus() {
    stopMonitors();
    saveData();
    for (auto* r : inventory) delete r;
    for (auto* u : users)     delete u;
}

// Locks consoleMutex so the full message prints as one atomic block
void Campus::safePrint(const string& msg) const {
    lock_guard<mutex> lock(consoleMutex);
    cout << msg << flush;
}

// Sleeps for 'seconds' but the condition_variable wakes it immediately
// when stopMonitors() calls shutdownCV.notify_all()
bool Campus::interruptibleSleep(int seconds) {
    unique_lock<mutex> lock(cvMutex);
    shutdownCV.wait_for(lock, chrono::seconds(seconds),
                        [this] { return !monitorsRunning.load(); });
    return monitorsRunning.load();
}

// Parses "YYYY-MM-DD"; returns INT_MAX for unparseable dates so they are ignored
int Campus::daysUntilExpiry(const string& dateStr) {
    int year = 0, month = 0, day = 0;
    if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) != 3)
        return INT_MAX;

    tm expiry      = {};
    expiry.tm_year = year - 1900;
    expiry.tm_mon  = month - 1;
    expiry.tm_mday = day;
    expiry.tm_hour = 12;   // noon avoids DST boundary issues
    time_t expiryTime = mktime(&expiry);
    if (expiryTime == -1) return INT_MAX;

    double diff = difftime(expiryTime, time(nullptr));
    return static_cast<int>(diff / 86400.0);
}

// Checks every LOW_STOCK_INTERVAL seconds for resources at or below threshold
void Campus::lowStockMonitor() {
    while (interruptibleSleep(LOW_STOCK_INTERVAL)) {
        // Snapshot while holding the lock — release before printing
        vector<pair<string, int>> lowItems;
        {
            lock_guard<recursive_mutex> lock(inventoryMutex);
            for (const auto* r : inventory)
                if (r->getStock() <= LOW_STOCK_THRESHOLD)
                    lowItems.push_back({r->getName(), r->getStock()});
        }

        if (!lowItems.empty()) {
            ostringstream oss;
            oss << "\n+--------------------------------------+\n";
            oss << "|         [LOW STOCK ALERT]            |\n";
            oss << "+--------------------------------------+\n";
            for (const auto& item : lowItems)
                oss << "  ! " << item.first
                    << " — only " << item.second << " unit(s) left\n";
            oss << "\n";
            safePrint(oss.str());
        }
    }
}

// Checks every EXPIRY_INTERVAL seconds for Cafeteria items expiring soon
void Campus::expiryMonitor() {
    while (interruptibleSleep(EXPIRY_INTERVAL)) {
        vector<pair<string, int>> warnings;
        {
            lock_guard<recursive_mutex> lock(inventoryMutex);
            for (const auto* r : inventory) {
                // dynamic_cast returns nullptr for non-Cafeteria resources
                const Cafeteria* c = dynamic_cast<const Cafeteria*>(r);
                if (!c) continue;
                int days = daysUntilExpiry(c->getExpiryDate());
                if (days <= EXPIRY_WARNING_DAYS)
                    warnings.push_back({c->getName(), days});
            }
        }

        if (!warnings.empty()) {
            ostringstream oss;
            oss << "\n+--------------------------------------+\n";
            oss << "|           [EXPIRY ALERT]             |\n";
            oss << "+--------------------------------------+\n";
            for (const auto& w : warnings) {
                oss << "  ! " << w.first << ": ";
                if (w.second < 0)
                    oss << "EXPIRED " << abs(w.second) << " day(s) ago!\n";
                else if (w.second == 0)
                    oss << "EXPIRES TODAY!\n";
                else
                    oss << "expires in " << w.second << " day(s)\n";
            }
            oss << "\n";
            safePrint(oss.str());
        }
    }
}

// Saves all data every AUTOSAVE_INTERVAL seconds as a crash safety net
void Campus::autoSaveMonitor() {
    while (interruptibleSleep(AUTOSAVE_INTERVAL)) {
        {
            lock_guard<recursive_mutex> lock(inventoryMutex);
            dataManager.saveResources(inventory);
            dataManager.saveUsers(users);
        }
        safePrint("[AutoSave] Data saved automatically.\n");
    }
}

void Campus::startMonitors() {
    monitorsRunning = true;
    lowStockThread = thread(&Campus::lowStockMonitor, this);
    expiryThread   = thread(&Campus::expiryMonitor,   this);
    autoSaveThread = thread(&Campus::autoSaveMonitor,  this);
    cout << "[Monitor] Low-stock, expiry, and auto-save monitors are running.\n";
}

// Sets the flag, wakes all threads via notify_all, then waits for each to exit
void Campus::stopMonitors() {
    if (!monitorsRunning) return;
    monitorsRunning = false;
    shutdownCV.notify_all();
    if (lowStockThread.joinable()) lowStockThread.join();
    if (expiryThread.joinable())   expiryThread.join();
    if (autoSaveThread.joinable()) autoSaveThread.join();
    cout << "[Monitor] All monitors stopped cleanly.\n";
}

void Campus::addResource(Resource* r) {
    lock_guard<recursive_mutex> lock(inventoryMutex);
    if (r->getId() >= nextResourceId)
        nextResourceId = r->getId() + 1;
    inventory.push_back(r);
}

Resource* Campus::findResourceById(int id) const {
    lock_guard<recursive_mutex> lock(inventoryMutex);
    for (auto* r : inventory)
        if (r->getId() == id) return r;
    return nullptr;
}

void Campus::printResourceReport(int id) const {
    // Search inline rather than calling findResourceById to avoid double-lock
    lock_guard<recursive_mutex> lock(inventoryMutex);
    Resource* found = nullptr;
    for (auto* r : inventory)
        if (r->getId() == id) { found = r; break; }

    if (!found) { cout << "[Not Found] No resource with ID " << id << ".\n"; return; }
    cout << "\n========== Resource Report ==========\n";
    found->display();
    cout << "=====================================\n";
}

void Campus::printAllResources() const {
    lock_guard<recursive_mutex> lock(inventoryMutex);
    if (inventory.empty()) { cout << "  Inventory is empty.\n"; return; }
    cout << "\n" << left
         << setw(6)  << "ID"
         << setw(24) << "Name"
         << setw(24) << "Category"
         << setw(14) << "Price (EGP)"
         << "Stock\n"
         << string(72, '-') << "\n";
    for (const auto* r : inventory)
        cout << left
             << setw(6)  << r->getId()
             << setw(24) << r->getName()
             << setw(24) << r->getCategory()
             << setw(14) << fixed << setprecision(2) << r->getPrice()
             << r->getStock() << "\n";
    cout << string(72, '-') << "\n";
}

void Campus::compareTwoResources(int id1, int id2) const {
    // One lock, both searches — avoids taking the lock twice
    lock_guard<recursive_mutex> lock(inventoryMutex);
    Resource* a = nullptr;
    Resource* b = nullptr;
    for (auto* r : inventory) {
        if (r->getId() == id1) a = r;
        if (r->getId() == id2) b = r;
    }
    if (!a) { cout << "[Not Found] No resource with ID " << id1 << ".\n"; return; }
    if (!b) { cout << "[Not Found] No resource with ID " << id2 << ".\n"; return; }
    compareCost(*a, *b);
}

bool Campus::registerUser(User* u) {
    lock_guard<recursive_mutex> lock(inventoryMutex);
    for (const auto* existing : users)
        if (existing->getCampusID() == u->getCampusID())
            return false;
    users.push_back(u);
    return true;
}

User* Campus::findUserById(const string& campusID) const {
    lock_guard<recursive_mutex> lock(inventoryMutex);
    for (auto* u : users)
        if (u->getCampusID() == campusID) return u;
    return nullptr;
}

void Campus::loadData() {
    // Called before threads start, but locked for consistency
    lock_guard<recursive_mutex> lock(inventoryMutex);
    for (auto* r : dataManager.loadResources()) {
        if (r->getId() >= nextResourceId)
            nextResourceId = r->getId() + 1;
        inventory.push_back(r);
    }
    for (auto* u : dataManager.loadUsers())
        users.push_back(u);
}

void Campus::saveData() const {
    lock_guard<recursive_mutex> lock(inventoryMutex);
    dataManager.saveResources(inventory);
    dataManager.saveUsers(users);
}

bool Campus::removeResource(int id) {
    lock_guard<recursive_mutex> lock(inventoryMutex);
    for (auto it = inventory.begin(); it != inventory.end(); ++it) {
        if ((*it)->getId() == id) {
            delete *it;
            inventory.erase(it);
            saveData();
            return true;
        }
    }
    return false;
}

bool Campus::removeUser(const string& campusID) {
    lock_guard<recursive_mutex> lock(inventoryMutex);
    for (auto it = users.begin(); it != users.end(); ++it) {
        if ((*it)->getCampusID() == campusID) {
            delete *it;
            users.erase(it);
            saveData();
            return true;
        }
    }
    return false;
}

int Campus::getIntInput(const string& prompt) const {
    int val;
    while (true) {
        cout << prompt;
        if (cin >> val) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return val;
        }
        cout << "  Invalid input — please enter a whole number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

double Campus::getDoubleInput(const string& prompt) const {
    double val;
    while (true) {
        cout << prompt;
        if (cin >> val) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return val;
        }
        cout << "  Invalid input — please enter a number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

string Campus::getStringInput(const string& prompt) const {
    string val;
    cout << prompt;
    getline(cin, val);
    return val;
}

string Campus::formatAmount(double amount) const {
    ostringstream oss;
    oss << fixed << setprecision(2) << amount;
    return oss.str();
}

void Campus::printHeader() const {
    cout << "\n==========================================\n";
    cout << "        SMART CAMPUS ECOSYSTEM\n";
    cout << "==========================================\n";
}

void Campus::menuResources() {
    while (true) {
        cout << "\n--- Resource Management ---\n"
             << "1. View all resources\n"
             << "2. Search resource by ID\n"
             << "3. Add new resource\n"
             << "4. Restock a resource\n"
             << "5. Compare two resources by cost\n"
             << "6. Remove a resource\n"
             << "7. Back\n";
        int choice = getIntInput("Choice: ");

        if (choice == 1) {
            printAllResources();

        } else if (choice == 2) {
            int id = getIntInput("Resource ID: ");
            printResourceReport(id);

        } else if (choice == 3) {
            cout << "\nSelect type:\n"
                 << "1. Lab Hardware\n"
                 << "2. Cafeteria Item\n"
                 << "3. Bookstore Media\n";
            int type = getIntInput("Type: ");
            if (type < 1 || type > 3) { cout << "Invalid type.\n"; continue; }

            string name  = getStringInput("Name: ");
            double price = getDoubleInput("Price (EGP): ");
            int    qty   = getIntInput("Initial stock: ");

            // Read the next ID while holding the lock, then release before user input
            int id;
            { lock_guard<recursive_mutex> lock(inventoryMutex); id = nextResourceId; }

            try {
                Resource* r = nullptr;
                if (type == 1) {
                    string w = getStringInput("Warranty date (YYYY-MM-DD): ");
                    r = new LabHardware(id, name, price, qty, w);
                } else if (type == 2) {
                    string e = getStringInput("Expiry date (YYYY-MM-DD): ");
                    r = new Cafeteria(id, name, price, qty, e);
                } else {
                    string a = getStringInput("Author: ");
                    r = new BookStore(id, name, price, qty, a);
                }
                addResource(r);
                cout << "[Added] " << name << " assigned ID " << id << ".\n";
            } catch (const exception& e) {
                cout << "[Error] " << e.what() << "\n";
            }

        } else if (choice == 4) {
            int id  = getIntInput("Resource ID to restock: ");
            Resource* r = findResourceById(id);
            if (!r) { cout << "[Not Found]\n"; continue; }
            int qty = getIntInput("Quantity to add: ");
            try {
                // Lock covers the stock write so monitors read consistent values
                lock_guard<recursive_mutex> lock(inventoryMutex);
                r->restock(qty);
            } catch (const exception& e) {
                cout << "[Error] " << e.what() << "\n";
            }

        } else if (choice == 5) {
            int id1 = getIntInput("First resource ID: ");
            int id2 = getIntInput("Second resource ID: ");
            compareTwoResources(id1, id2);

        } else if (choice == 6) {
            int id = getIntInput("Resource ID to remove: ");
            if (removeResource(id))
                cout << "[Removed] Resource " << id << " deleted.\n";
            else
                cout << "[Not Found] No resource with ID " << id << ".\n";

        } else if (choice == 7) {
            break;
        } else {
            cout << "Invalid choice.\n";
        }
    }
}

void Campus::menuUsers() {
    while (true) {
        cout << "\n--- User Management ---\n"
             << "1. Register student\n"
             << "2. Register staff\n"
             << "3. View user profile\n"
             << "4. Remove a user\n"
             << "5. Back\n";
        int choice = getIntInput("Choice: ");

        if (choice == 1 || choice == 2) {
            string name     = getStringInput("Full name: ");
            string campusID = getStringInput("Campus ID: ");
            User* u = (choice == 1)
                      ? (User*) new Student(name, campusID)
                      : (User*) new Staff(name, campusID);
            if (!registerUser(u)) {
                cout << "[Error] Campus ID '" << campusID
                     << "' is already registered.\n";
                delete u;
            } else {
                cout << "[Registered] ";
                u->printProfile();
            }

        } else if (choice == 3) {
            string id = getStringInput("Campus ID: ");
            User*  u  = findUserById(id);
            if (!u) cout << "[Not Found] No user with ID '" << id << "'.\n";
            else    u->printProfile();

        } else if (choice == 4) {
            string cid = getStringInput("Campus ID to remove: ");
            if (removeUser(cid))
                cout << "[Removed] User '" << cid << "' deleted.\n";
            else
                cout << "[Not Found] No user with ID '" << cid << "'.\n";

        } else if (choice == 5) {
            break;
        } else {
            cout << "Invalid choice.\n";
        }
    }
}

void Campus::menuPlaceOrder() {
    string campusID = getStringInput("Enter your campus ID: ");
    User*  user     = findUserById(campusID);
    if (!user) { cout << "[Error] User not found.\n"; return; }
    cout << "Welcome, "; user->printProfile();

    cout << "\nDelivery method:\n"
         << "1. Pickup (free)\n"
         << "2. Dorm Delivery (+EGP 5.00)\n";
    int dc = getIntInput("Choice: ");
    if (dc != 1 && dc != 2) { cout << "Invalid choice.\n"; return; }

    Pickup       pickup;
    DormDelivery dorm;
    DeliveryMethod* delivery = (dc == 1)
                               ? (DeliveryMethod*)&pickup
                               : (DeliveryMethod*)&dorm;

    Order order(user, delivery);
    cout << "\nAvailable resources:\n";
    printAllResources();
    cout << "Enter 0 as ID when done.\n";

    while (true) {
        int id = getIntInput("Resource ID: ");
        if (id == 0) break;
        Resource* r = findResourceById(id);
        if (!r) { cout << "[Not Found] Try again.\n"; continue; }
        int qty = getIntInput("Quantity: ");
        try {
            order.addItem(r, qty);
            cout << "  Added: " << r->getName() << " x" << qty << "\n";
        } catch (const exception& e) {
            cout << "[Error] " << e.what() << "\n";
        }
    }

    cout << "\nPayment method:\n"
         << "1. Cash\n"
         << "2. Card\n";
    int pc = getIntInput("Choice: ");
    if (pc != 1 && pc != 2) { cout << "Invalid choice.\n"; return; }

    try {
        if (pc == 1) {
            CashPayment cash;
            cash.validate();
            // Lock covers all stock writes inside checkout()
            { lock_guard<recursive_mutex> lock(inventoryMutex); order.checkout(); }
            cash.processPayment(order.finalTotal());
            dataManager.logTransaction(
                "Order#" + to_string(order.getOrderId()) +
                " | " + user->getName() + " (" + user->getCampusID() + ")" +
                " | EGP " + formatAmount(order.finalTotal()) + " | Cash");

        } else {
            string cardNum = getStringInput("16-digit card number: ");
            CardPayment card(cardNum);
            // Validate before touching stock — invalid card aborts here
            card.validate();
            { lock_guard<recursive_mutex> lock(inventoryMutex); order.checkout(); }
            card.processPayment(order.finalTotal());
            dataManager.logTransaction(
                "Order#" + to_string(order.getOrderId()) +
                " | " + user->getName() + " (" + user->getCampusID() + ")" +
                " | EGP " + formatAmount(order.finalTotal()) + " | Card");
        }
    } catch (const exception& e) {
        cout << "[Error] " << e.what() << "\n";
        cout << "Order was not completed. No stock was deducted.\n";
    }
}

void Campus::menuReports() {
    while (true) {
        cout << "\n--- Reports & Analytics ---\n"
             << "1. Full inventory listing\n"
             << "2. Search resource by ID\n"
             << "3. Compare two resources by cost\n"
             << "4. Back\n";
        int choice = getIntInput("Choice: ");

        if      (choice == 1) { printAllResources(); }
        else if (choice == 2) { printResourceReport(getIntInput("Resource ID: ")); }
        else if (choice == 3) { compareTwoResources(getIntInput("First ID: "),
                                                    getIntInput("Second ID: ")); }
        else if (choice == 4) { break; }
        else { cout << "Invalid choice.\n"; }
    }
}

void Campus::run() {
    printHeader();
    cout << "System ready.\n";
    while (true) {
        printHeader();
        cout << "1. Resource Management\n"
             << "2. User Management\n"
             << "3. Place Order\n"
             << "4. Reports & Analytics\n"
             << "5. Save & Exit\n";
        int choice = getIntInput("Choice: ");

        if      (choice == 1) menuResources();
        else if (choice == 2) menuUsers();
        else if (choice == 3) menuPlaceOrder();
        else if (choice == 4) menuReports();
        else if (choice == 5) {
            saveData();
            cout << "All data saved. Goodbye!\n";
            break;
        } else {
            cout << "Invalid choice.\n";
        }
    }
}