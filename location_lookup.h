#ifndef LOCATION_LOOKUP_H
#define LOCATION_LOOKUP_H

#include <string>

using namespace std;

struct LocationEntry {
    string name;
    int id;
    LocationEntry* next;
};

// Finds a location ID by its name using a hash table.
class LocationLookup {
public:
    LocationLookup();
    ~LocationLookup();

    void clear();
    void insert(const string& name, int id);
    bool find(const string& name, int& id) const;

private:
    // Keep the table from being copied accidentally because it owns pointers.
    LocationLookup(const LocationLookup&);
    LocationLookup& operator=(const LocationLookup&);

    static const int BUCKET_COUNT = 31;
    LocationEntry* buckets[BUCKET_COUNT];

    int hash(const string& name) const;
};

#endif
