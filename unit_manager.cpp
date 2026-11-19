#include "unit_manager.h"

#include <fstream>
#include <iostream>

using namespace std;

namespace {
bool textToNumber(const string& text, int maximum, int& value) {
    if (text.length() == 0) {
        return false;
    }

    value = 0;
    int length = text.length();
    for (int i = 0; i < length; i++) {
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
        int digit = text[i] - '0';
        if (digit > maximum || value > (maximum - digit) / 10) {
            return false;
        }
        value = value * 10 + digit;
    }
    return true;
}

int splitLine(const string& line, string fields[], int fieldLimit) {
    int count = 0;
    int start = 0;
    int length = line.length();

    for (int i = 0; i <= length; i++) {
        if (i == length || line[i] == '|') {
            if (count >= fieldLimit) {
                return fieldLimit + 1;
            }
            fields[count] = line.substr(start, i - start);
            count++;
            start = i + 1;
        }
    }
    return count;
}

string unitTypeName(int type) {
    if (type == 1) {
        return "Ambulance";
    }
    if (type == 2) {
        return "Fire engine";
    }
    if (type == 3) {
        return "Rescue unit";
    }
    return "Patrol unit";
}
}

UnitManager::UnitManager() : unitCount(0), nextUnitId(1) {
}

bool UnitManager::load(const string& fileName,
                       const CityGraph& city,
                       string& error) {
    ifstream file(fileName.c_str());
    if (!file) {
        error = "Could not open emergency units file: " + fileName;
        return false;
    }

    unitCount = 0;
    nextUnitId = 1;
    string line;
    int lineNumber = 0;

    while (getline(file, line)) {
        lineNumber++;
        if (line.length() == 0 || line[0] == '#') {
            continue;
        }

        string fields[4];
        if (splitLine(line, fields, 4) != 4 || unitCount == MAX_UNITS) {
            error = "Invalid or excessive unit records near line " +
                    to_string(lineNumber);
            return false;
        }

        EmergencyUnit unit;
        int availableValue = 0;
        if (!textToNumber(fields[0], MAX_UNITS, unit.id) ||
            !textToNumber(fields[1], 4, unit.type) ||
            !textToNumber(fields[2], MAX_LOCATIONS, unit.locationId) ||
            !textToNumber(fields[3], 1, availableValue)) {
            error = "Invalid unit record at line " + to_string(lineNumber);
            return false;
        }

        if (unit.id <= 0 || unit.type < 1 || unit.type > 4 ||
            unit.locationId <= 0 || unit.locationId > MAX_LOCATIONS ||
            city.locationName(unit.locationId) == "Unknown location" ||
            (availableValue != 0 && availableValue != 1)) {
            error = "Invalid unit record at line " + to_string(lineNumber);
            return false;
        }
        for (int i = 0; i < unitCount; i++) {
            if (units[i].id == unit.id) {
                error = "Duplicate emergency unit ID " + to_string(unit.id);
                return false;
            }
        }

        unit.available = (availableValue == 1);
        units[unitCount] = unit;
        unitCount++;
        if (unit.id >= nextUnitId) {
            nextUnitId = unit.id + 1;
        }
    }
    return true;
}

bool UnitManager::save(const string& fileName, string& error) const {
    ofstream file(fileName.c_str());
    if (!file) {
        error = "Could not save emergency units file: " + fileName;
        return false;
    }

    file << "# id|type(1=ambulance,2=fire,3=rescue,4=patrol)|locationId|available(1=yes,0=no)"
         << endl;
    for (int i = 0; i < unitCount; i++) {
        file << units[i].id << '|' << units[i].type << '|' << units[i].locationId
             << '|' << (units[i].available ? 1 : 0) << endl;
    }
    if (!file) {
        error = "A write error occurred while saving emergency units.";
        return false;
    }
    return true;
}

bool UnitManager::registerUnit(int type, int locationId) {
    if (unitCount == MAX_UNITS || type < 1 || type > 4 ||
        locationId <= 0 || locationId > MAX_LOCATIONS) {
        return false;
    }

    EmergencyUnit unit;
    unit.id = nextUnitId;
    unit.type = type;
    unit.locationId = locationId;
    unit.available = true;
    units[unitCount] = unit;
    unitCount++;
    nextUnitId++;
    return true;
}

bool UnitManager::setAvailable(int unitId, bool available) {
    for (int i = 0; i < unitCount; i++) {
        if (units[i].id == unitId) {
            units[i].available = available;
            return true;
        }
    }
    return false;
}

bool UnitManager::completeDispatch(int unitId, int newLocationId) {
    for (int i = 0; i < unitCount; i++) {
        if (units[i].id == unitId) {
            units[i].locationId = newLocationId;
            units[i].available = true;
            return true;
        }
    }
    return false;
}

bool UnitManager::unitCanHandle(int unitType, int incidentType) const {
    if (incidentType == 1) {
        return unitType == 1;
    }
    if (incidentType == 2) {
        return unitType == 2;
    }
    if (incidentType == 3) {
        return unitType == 1 || unitType == 3;
    }
    return unitType == 4;
}

bool UnitManager::findFastestUnit(int incidentType,
                                  int incidentLocation,
                                  const CityGraph& city,
                                  EmergencyUnit& chosenUnit,
                                  Route& chosenRoute) const {
    bool foundUnit = false;

    for (int i = 0; i < unitCount; i++) {
        if (!units[i].available || !unitCanHandle(units[i].type, incidentType)) {
            continue;
        }

        Route route = city.fastestRoute(city.locationName(units[i].locationId),
                                        city.locationName(incidentLocation));
        if (route.found &&
            (!foundUnit || route.travelMinutes < chosenRoute.travelMinutes)) {
            chosenUnit = units[i];
            chosenRoute = route;
            foundUnit = true;
        }
    }
    return foundUnit;
}

void UnitManager::showUnits(const CityGraph& city) const {
    cout << endl << "Emergency units" << endl;
    if (unitCount == 0) {
        cout << "No units are registered." << endl;
        return;
    }

    for (int i = 0; i < unitCount; i++) {
        cout << "ID " << units[i].id << " | " << unitTypeName(units[i].type)
             << " | " << city.locationName(units[i].locationId) << " | "
             << (units[i].available ? "Available" : "Busy") << endl;
    }
}
