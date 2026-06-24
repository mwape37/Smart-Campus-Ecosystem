#include "Campus.h"

int main() {
    try {
        Campus campus;
        campus.run();
    } catch (const exception& e) {
        cerr << "[FATAL] " << e.what() << "\n";
        return 1;
    }
    return 0;
}