#ifndef CITY_GRAPH_H
#define CITY_GRAPH_H

#include "location_lookup.h"

#include <string>

using namespace std;

static const int MAX_LOCATIONS = 100;

struct Location {
    int id;
    string name;
};

struct Road {
    int destinationId;
    int travelMinutes;
    bool open;
    Road* next;
};

struct Route {
    bool found;
    int travelMinutes;
    int locationIds[MAX_LOCATIONS];
    int locationCount;

    Route() : found(false), travelMinutes(0), locationCount(0) {}
};

class CityGraph {
public:
    CityGraph();
    ~CityGraph();

    bool load(const string& locationsFile,
              const string& roadsFile,
              string& error);
    bool saveRoads(const string& roadsFile, string& error) const;

    bool findLocation(const string& name, int& id) const;
    const string& locationName(int id) const;
    void printLocations() const;
    void printRoads() const;

    Route fastestRoute(const string& startName,
                       const string& destinationName) const;
    bool updateRoad(const string& firstName,
                    const string& secondName,
                    int travelMinutes,
                    bool open);

private:
    // This class owns linked-list nodes, so copying it would share pointers.
    CityGraph(const CityGraph&);
    CityGraph& operator=(const CityGraph&);

    Location locations[MAX_LOCATIONS + 1];
    bool locationExists[MAX_LOCATIONS + 1];
    Road* roads[MAX_LOCATIONS + 1];
    int locationCount;
    LocationLookup locationTable;

    void clear();
    bool loadLocations(const string& fileName, string& error);
    bool loadRoads(const string& fileName, string& error);
    bool hasLocation(int id) const;
    void addRoad(int firstId, int secondId, int minutes, bool open);
};

#endif
