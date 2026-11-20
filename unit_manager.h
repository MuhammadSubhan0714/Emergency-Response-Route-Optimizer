#ifndef UNIT_MANAGER_H
#define UNIT_MANAGER_H

#include "city_graph.h"

using namespace std;

const int MAX_UNITS = 100;

struct EmergencyUnit {
    int id;
    int type;
    int locationId;
    bool available;
};

class UnitManager {
public:
    UnitManager();

    bool load(const string& fileName, const CityGraph& city, string& error);
    bool save(const string& fileName, string& error) const;
    bool registerUnit(int type, int locationId);
    bool setAvailable(int unitId, bool available);
    bool completeDispatch(int unitId, int newLocationId);
    bool findFastestUnit(int incidentType,int incidentLocation, const CityGraph& city, EmergencyUnit& chosenUnit, Route& chosenRoute) const;
    void showUnits(const CityGraph& city) const;

private:
    EmergencyUnit units[MAX_UNITS];
    int unitCount;
    int nextUnitId;

    bool unitCanHandle(int unitType, int incidentType) const;
};

#endif
