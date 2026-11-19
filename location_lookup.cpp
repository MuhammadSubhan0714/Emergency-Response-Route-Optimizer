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

int LocationLookup::hash(const string& name) const {
    int value = 0;
    int nameLength = name.length();
    for (int i = 0; i < nameLength; i++) {
        value = (value * 31 + name[i]) % BUCKET_COUNT;
    }
    return value;
}

void LocationLookup::insert(const string& name, int id) {
    int bucketNumber = hash(name);
    LocationEntry* current = buckets[bucketNumber];

    while (current != nullptr) {
        if (current->name == name) {
            current->id = id;
            return;
        }
        current = current->next;
    }

    LocationEntry* newEntry = new LocationEntry;
    newEntry->name = name;
    newEntry->id = id;
    newEntry->next = buckets[bucketNumber];
    buckets[bucketNumber] = newEntry;
}

bool LocationLookup::find(const string& name, int& id) const {
    LocationEntry* current = buckets[hash(name)];

    while (current != nullptr) {
        if (current->name == name) {
            id = current->id;
            return true;
        }
        current = current->next;
    }
    return false;
}
