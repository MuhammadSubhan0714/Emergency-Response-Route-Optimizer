#include "location_lookup.h"

using namespace std;

LocationLookup::LocationLookup() {
    for (int i = 0; i < BUCKET_COUNT; i++) {
        buckets[i] = nullptr;
    }
}

LocationLookup::~LocationLookup() {
    clear();
}

void LocationLookup::clear() {
    for (int i = 0; i < BUCKET_COUNT; i++) {
        LocationEntry* current = buckets[i];
        while (current != nullptr) {
            LocationEntry* next = current->next;
            delete current;
            current = next;
        }
        buckets[i] = nullptr;
    }
}

string LocationLookup::lowerCase(const string& name) const {
    string result = name;
    int nameLength = result.length();
    for (int i = 0; i < nameLength; i++) {
        if (result[i] >= 'A' && result[i] <= 'Z') {
            result[i] = result[i] - 'A' + 'a';
        }
    }
    return result;
}

int LocationLookup::hash(const string& name) const {
    string lowerName = lowerCase(name);
    int value = 0;
    int nameLength = lowerName.length();
    for (int i = 0; i < nameLength; i++) {
        value = (value * 31 + lowerName[i]) % BUCKET_COUNT;
    }
    return value;
}

void LocationLookup::insert(const string& name, int id) {
    string lowerName = lowerCase(name);
    int bucketNumber = hash(lowerName);
    LocationEntry* current = buckets[bucketNumber];

    while (current != nullptr) {
        if (current->name == lowerName) {
            current->id = id;
            return;
        }
        current = current->next;
    }

    LocationEntry* newEntry = new LocationEntry;
    newEntry->name = lowerName;
    newEntry->id = id;
    newEntry->next = buckets[bucketNumber];
    buckets[bucketNumber] = newEntry;
}

bool LocationLookup::find(const string& name, int& id) const {
    string lowerName = lowerCase(name);
    LocationEntry* current = buckets[hash(lowerName)];

    while (current != nullptr) {
        if (current->name == lowerName) {
            id = current->id;
            return true;
        }
        current = current->next;
    }
    return false;
}
