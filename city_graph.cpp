#include "city_graph.h"
#include "min_heap.h"

#include <fstream>
#include <iostream>

using namespace std;

namespace {
bool textToNumber(const string& text, int maximum, int& value) {
    if (text.length() == 0) {
        return false;
    }

    value = 0;
    int textLength = text.length();
    for (int i = 0; i < textLength; i++) {
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }

        int digit = text[i] - '0';
        if (digit > maximum) {
            return false;
        }
        if (value > (maximum - digit) / 10) {
            return false;
        }
        value = value * 10 + digit;
    }
    return true;
}

int splitLine(const string& line, string fields[], int maxFields) {
    int fieldCount = 0;
    int fieldStart = 0;
    int lineLength = line.length();

    for (int i = 0; i <= lineLength; i++) {
        if (i == lineLength || line[i] == '|') {
            if (fieldCount >= maxFields) {
                return maxFields + 1;
            }
            fields[fieldCount] = line.substr(fieldStart, i - fieldStart);
            fieldCount++;
            fieldStart = i + 1;
        }
    }
    return fieldCount;
}
}

CityGraph::CityGraph() : locationCount(0) {
    for (int id = 0; id <= MAX_LOCATIONS; id++) {
        locationExists[id] = false;
        roads[id] = nullptr;
        locations[id].id = id;
    }
}

CityGraph::~CityGraph() {
    clear();
}

void CityGraph::clear() {
    for (int id = 0; id <= MAX_LOCATIONS; id++) {
        Road* current = roads[id];
        while (current != nullptr) {
            Road* nextRoad = current->next;
            delete current;
            current = nextRoad;
        }

        roads[id] = nullptr;
        locationExists[id] = false;
        locations[id].name.clear();
    }
    locationCount = 0;
    locationTable.clear();
}

bool CityGraph::load(const string& locationsFile,
                     const string& roadsFile,
                     string& error) {
    clear();

    if (!loadLocations(locationsFile, error)) {
        return false;
    }
    if (!loadRoads(roadsFile, error)) {
        clear();
        return false;
    }
    return true;
}

bool CityGraph::loadLocations(const string& fileName, string& error) {
    ifstream file(fileName.c_str());
    if (!file) {
        error = "Could not open locations file: " + fileName;
        return false;
    }

    string line;
    int lineNumber = 0;
    while (getline(file, line)) {
        lineNumber++;
        if (line.length() == 0 || line[0] == '#') {
            continue;
        }

        string fields[2];
        int fieldCount = splitLine(line, fields, 2);
        int id = 0;
        if (fieldCount != 2 || !textToNumber(fields[0], MAX_LOCATIONS, id) || id <= 0 ||
            id > MAX_LOCATIONS || fields[1].length() == 0) {
            error = "Invalid location record at line " + to_string(lineNumber);
            return false;
        }
        if (locationExists[id]) {
            error = "Duplicate location ID " + to_string(id);
            return false;
        }

        int oldId = 0;
        if (locationTable.find(fields[1], oldId)) {
            error = "Duplicate location name: " + fields[1];
            return false;
        }

        locations[id].id = id;
        locations[id].name = fields[1];
        locationExists[id] = true;
        locationCount++;
        locationTable.insert(fields[1], id);
    }

    if (locationCount == 0) {
        error = "The locations file contains no locations.";
        return false;
    }
    return true;
}

void CityGraph::addRoad(int firstId, int secondId, int minutes, bool open) {
    Road* roadFromFirst = new Road;
    Road* roadFromSecond = new Road;

    roadFromFirst->destinationId = secondId;
    roadFromFirst->travelMinutes = minutes;
    roadFromFirst->open = open;
    roadFromFirst->next = roads[firstId];
    roads[firstId] = roadFromFirst;

    roadFromSecond->destinationId = firstId;
    roadFromSecond->travelMinutes = minutes;
    roadFromSecond->open = open;
    roadFromSecond->next = roads[secondId];
    roads[secondId] = roadFromSecond;
}

bool CityGraph::loadRoads(const string& fileName, string& error) {
    ifstream file(fileName.c_str());
    if (!file) {
        error = "Could not open roads file: " + fileName;
        return false;
    }

    string line;
    int lineNumber = 0;
    while (getline(file, line)) {
        lineNumber++;
        if (line.length() == 0 || line[0] == '#') {
            continue;
        }

        string fields[4];
        int fieldCount = splitLine(line, fields, 4);
        int firstId = 0;
        int secondId = 0;
        int minutes = 0;
        int openValue = 0;

        if (fieldCount != 4 || !textToNumber(fields[0], MAX_LOCATIONS, firstId) ||
            !textToNumber(fields[1], MAX_LOCATIONS, secondId) ||
            !textToNumber(fields[2], 10000, minutes) ||
            !textToNumber(fields[3], 1, openValue) || minutes <= 0 ||
            (openValue != 0 && openValue != 1) || !hasLocation(firstId) ||
            !hasLocation(secondId) || firstId == secondId) {
            error = "Invalid road record at line " + to_string(lineNumber);
            return false;
        }

        int smallerId = firstId;
        int largerId = secondId;
        if (smallerId > largerId) {
            int temporary = smallerId;
            smallerId = largerId;
            largerId = temporary;
        }

        Road* road = roads[smallerId];
        while (road != nullptr) {
            if (road->destinationId == largerId) {
                error = "Duplicate road between location IDs " +
                        to_string(smallerId) + " and " + to_string(largerId);
                return false;
            }
            road = road->next;
        }

        addRoad(firstId, secondId, minutes, openValue == 1);
    }
    return true;
}

bool CityGraph::saveRoads(const string& roadsFile, string& error) const {
    ofstream file(roadsFile.c_str());
    if (!file) {
        error = "Could not save roads file: " + roadsFile;
        return false;
    }

    file << "# firstLocationId|secondLocationId|travelMinutes|open(1=yes,0=no)"
         << endl;
    for (int fromId = 1; fromId <= MAX_LOCATIONS; fromId++) {
        const Road* road = roads[fromId];
        while (road != nullptr) {
            if (fromId < road->destinationId) {
                file << fromId << '|' << road->destinationId << '|'
                     << road->travelMinutes << '|' << (road->open ? 1 : 0)
                     << endl;
            }
            road = road->next;
        }
    }

    if (!file) {
        error = "A write error occurred while saving roads.";
        return false;
    }
    return true;
}

bool CityGraph::findLocation(const string& name, int& id) const {
    return locationTable.find(name, id);
}

bool CityGraph::hasLocation(int id) const {
    return id > 0 && id <= MAX_LOCATIONS && locationExists[id];
}

const string& CityGraph::locationName(int id) const {
    if (hasLocation(id)) {
        return locations[id].name;
    }

    static const string unknown = "Unknown location";
    return unknown;
}

void CityGraph::printLocations() const {
    cout << endl << "City locations" << endl;
    for (int id = 1; id <= MAX_LOCATIONS; id++) {
        if (locationExists[id]) {
            cout << "  " << id << ". " << locations[id].name << endl;
        }
    }
}

void CityGraph::printRoads() const {
    cout << endl << "Roads (travel time in minutes)" << endl;
    for (int fromId = 1; fromId <= MAX_LOCATIONS; fromId++) {
        const Road* road = roads[fromId];
        while (road != nullptr) {
            if (fromId < road->destinationId) {
                cout << "  " << locationName(fromId) << " <-> "
                     << locationName(road->destinationId) << " | "
                     << road->travelMinutes << " min | "
                     << (road->open ? "Open" : "Closed") << endl;
            }
            road = road->next;
        }
    }
}

Route CityGraph::fastestRoute(const string& startName,
                              const string& destinationName) const {
    Route route;
    int startId = 0;
    int destinationId = 0;

    if (!locationTable.find(startName, startId) ||
        !locationTable.find(destinationName, destinationId)) {
        return route;
    }

    const int infinity = 1000000000;
    int distance[MAX_LOCATIONS + 1];
    int previous[MAX_LOCATIONS + 1];
    for (int id = 0; id <= MAX_LOCATIONS; id++) {
        distance[id] = infinity;
        previous[id] = -1;
    }

    MinHeap nextLocations;
    distance[startId] = 0;
    nextLocations.push({0, startId});

    while (!nextLocations.empty()) {
        RouteCandidate current = nextLocations.pop();
        if (current.minutes != distance[current.locationId]) {
            continue;
        }
        if (current.locationId == destinationId) {
            break;
        }

        const Road* road = roads[current.locationId];
        while (road != nullptr) {
            if (road->open) {
                int newTime = current.minutes + road->travelMinutes;
                if (newTime < distance[road->destinationId]) {
                    distance[road->destinationId] = newTime;
                    previous[road->destinationId] = current.locationId;
                    if (!nextLocations.push({newTime, road->destinationId})) {
                        return route;
                    }
                }
            }
            road = road->next;
        }
    }

    if (distance[destinationId] == infinity) {
        return route;
    }

    int reversedPath[MAX_LOCATIONS];
    int pathLength = 0;
    int currentId = destinationId;
    while (currentId != -1 && pathLength < MAX_LOCATIONS) {
        reversedPath[pathLength] = currentId;
        pathLength++;
        if (currentId == startId) {
            break;
        }
        currentId = previous[currentId];
    }

    if (pathLength == 0 || reversedPath[pathLength - 1] != startId) {
        return route;
    }

    route.found = true;
    route.travelMinutes = distance[destinationId];
    route.locationCount = pathLength;
    for (int i = 0; i < pathLength; i++) {
        route.locationIds[i] = reversedPath[pathLength - 1 - i];
    }
    return route;
}

bool CityGraph::updateRoad(const string& firstName,
                           const string& secondName,
                           int travelMinutes,
                           bool open) {
    int firstId = 0;
    int secondId = 0;

    if (travelMinutes <= 0 || !locationTable.find(firstName, firstId) ||
        !locationTable.find(secondName, secondId)) {
        return false;
    }

    Road* firstRoad = roads[firstId];
    while (firstRoad != nullptr && firstRoad->destinationId != secondId) {
        firstRoad = firstRoad->next;
    }
    if (firstRoad == nullptr) {
        return false;
    }

    Road* secondRoad = roads[secondId];
    while (secondRoad != nullptr && secondRoad->destinationId != firstId) {
        secondRoad = secondRoad->next;
    }
    if (secondRoad == nullptr) {
        return false;
    }

    firstRoad->travelMinutes = travelMinutes;
    firstRoad->open = open;
    secondRoad->travelMinutes = travelMinutes;
    secondRoad->open = open;
    return true;
}
