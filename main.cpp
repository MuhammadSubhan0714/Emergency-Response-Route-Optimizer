#include "city_graph.h"
#include "dispatch_history.h"
#include "incident_manager.h"
#include "unit_manager.h"

#include <iostream>
#include <string>

using namespace std;

const string LOCATIONS_FILE = "data/locations.txt";
const string ROADS_FILE = "data/roads.txt";
const string INCIDENTS_FILE = "data/incidents.txt";
const string UNITS_FILE = "data/units.txt";
const string HISTORY_FILE = "data/dispatch_history.txt";

void showRoute(const CityGraph& city) {
    string start;
    string destination;

    cout << "Starting location: ";
    getline(cin, start);
    cout << "Destination: ";
    getline(cin, destination);

    Route route = city.fastestRoute(start, destination);

    if (!route.found) {
        cout << "No route found. Check the location names and open roads." << endl;
        return;
    }

    cout << "Fastest route (" << route.travelMinutes << " minutes):" << endl;
    cout << "  ";
    for (int i = 0; i < route.locationCount; i++) {
        if (i > 0) {
            cout << " -> ";
        }
        cout << city.locationName(route.locationIds[i]);
    }
    cout << endl;
}

void changeRoad(CityGraph& city) {
    string firstLocation;
    string secondLocation;
    string restOfLine;
    int minutes = 0;
    int status = 0;

    cout << "First location on the road: ";
    getline(cin, firstLocation);
    cout << "Second location on the road: ";
    getline(cin, secondLocation);

    cout << "Travel time in minutes: ";
    cin >> minutes;
    getline(cin, restOfLine);

    cout << "Road status (1 = open, 2 = closed): ";
    cin >> status;
    getline(cin, restOfLine);

    if (status != 1 && status != 2) {
        cout << "Choose 1 for open or 2 for closed." << endl;
        return;
    }

    bool roadIsOpen = (status == 1);
    if (!city.updateRoad(firstLocation, secondLocation, minutes, roadIsOpen)) {
        cout << "Road not found. Only roads already on the map can be changed."
             << endl;
        return;
    }

    string error;
    if (!city.saveRoads(ROADS_FILE, error)) {
        cout << error << endl;
    } else {
        cout << "Road condition saved. The next route search will use the change."
             << endl;
    }
}

void reportEmergency(const CityGraph& city, IncidentManager& manager) {
    int type = 0;
    int severity = 0;
    int locationId = 0;
    string location;
    string restOfLine;

    cout << "Emergency type: 1. Medical  2. Fire  3. Accident  4. Security"
         << endl;
    cout << "Choose type: ";
    cin >> type;
    getline(cin, restOfLine);
    if (type < 1 || type > 4) {
        cout << "Choose an emergency type from 1 to 4." << endl;
        return;
    }

    cout << "Incident location: ";
    getline(cin, location);
    if (!city.findLocation(location, locationId)) {
        cout << "Location not found. Use a name shown in the locations list." << endl;
        return;
    }

    cout << "Severity (1 = low, 5 = critical): ";
    cin >> severity;
    getline(cin, restOfLine);
    if (severity < 1 || severity > 5) {
        cout << "Severity must be from 1 to 5." << endl;
        return;
    }

    if (manager.report(type, locationId, severity)) {
        string error;
        if (manager.save(INCIDENTS_FILE, error)) {
            cout << "Emergency report added to the incoming queue." << endl;
        } else {
            cout << error << endl;
        }
    } else {
        cout << "The incident list is full." << endl;
    }
}

void advanceTime(IncidentManager& manager) {
    int minutes = 0;
    string restOfLine;

    cout << "Minutes to advance: ";
    cin >> minutes;
    getline(cin, restOfLine);

    if (!manager.addWaitingTime(minutes)) {
        cout << "Enter a positive number of minutes within the simulation limit." << endl;
        return;
    }

    string error;
    if (manager.save(INCIDENTS_FILE, error)) {
        cout << "Waiting time updated. The priority heap has been rebuilt." << endl;
    } else {
        cout << error << endl;
    }
}

void registerUnit(const CityGraph& city, UnitManager& units) {
    int type = 0;
    int locationId = 0;
    string location;
    string restOfLine;

    cout << "Unit type: 1. Ambulance  2. Fire engine  3. Rescue  4. Patrol"
         << endl;
    cout << "Choose type: ";
    cin >> type;
    getline(cin, restOfLine);
    if (type < 1 || type > 4) {
        cout << "Choose a unit type from 1 to 4." << endl;
        return;
    }

    cout << "Unit starting location: ";
    getline(cin, location);
    if (!city.findLocation(location, locationId)) {
        cout << "Location not found. Use a name shown in the locations list." << endl;
        return;
    }

    if (!units.registerUnit(type, locationId)) {
        cout << "The unit list is full." << endl;
        return;
    }

    string error;
    if (units.save(UNITS_FILE, error)) {
        cout << "Emergency unit registered and saved." << endl;
    } else {
        cout << error << endl;
    }
}

void dispatchNextIncident(const CityGraph& city,
                          IncidentManager& incidents,
                          UnitManager& units,
                          DispatchHistory& history) {
    Incident incident;
    if (!incidents.nextForDispatch(incident)) {
        cout << "There are no prioritized incidents to dispatch." << endl;
        return;
    }

    EmergencyUnit unit;
    Route route;
    if (!units.findFastestUnit(incident.type, incident.locationId, city, unit, route)) {
        cout << "No available unit can reach this incident right now." << endl;
        return;
    }

    if (!incidents.markDispatched(incident.id, unit.id, route.travelMinutes)) {
        cout << "Could not update the incident status." << endl;
        return;
    }
    units.setAvailable(unit.id, false);
    history.add(incident.id, unit.id, route.travelMinutes);

    cout << "Dispatched unit " << unit.id << " with an estimated route time of "
         << route.travelMinutes << " minutes." << endl;
    cout << "Route: ";
    for (int i = 0; i < route.locationCount; i++) {
        if (i > 0) {
            cout << " -> ";
        }
        cout << city.locationName(route.locationIds[i]);
    }
    cout << endl;

    string error;
    if (!incidents.save(INCIDENTS_FILE, error)) {
        cout << error << endl;
    }
    if (!units.save(UNITS_FILE, error)) {
        cout << error << endl;
    }
    if (!history.save(HISTORY_FILE, error)) {
        cout << error << endl;
    }
}

void completeIncident(IncidentManager& incidents,
                      UnitManager& units,
                      DispatchHistory& history) {
    int incidentId = 0;
    int unitId = 0;
    int incidentLocation = 0;
    string restOfLine;

    cout << "Incident ID to complete: ";
    cin >> incidentId;
    getline(cin, restOfLine);

    if (!incidents.completeIncident(incidentId, unitId, incidentLocation)) {
        cout << "That incident is not currently dispatched." << endl;
        return;
    }

    units.completeDispatch(unitId, incidentLocation);
    history.markCompleted(incidentId);

    string error;
    if (!incidents.save(INCIDENTS_FILE, error)) {
        cout << error << endl;
    }
    if (!units.save(UNITS_FILE, error)) {
        cout << error << endl;
    }
    if (!history.save(HISTORY_FILE, error)) {
        cout << error << endl;
    }
    cout << "Incident completed. Unit " << unitId << " is available again." << endl;
}

int main() {
    CityGraph city;
    IncidentManager incidentManager;
    UnitManager unitManager;
    DispatchHistory dispatchHistory;
    string error;

    if (!city.load(LOCATIONS_FILE, ROADS_FILE, error)) {
        cout << "Could not load the city map. " << error << endl;
        return 1;
    }
    if (!incidentManager.load(INCIDENTS_FILE, city, error)) {
        cout << "Could not load incident records. " << error << endl;
        return 1;
    }
    if (!unitManager.load(UNITS_FILE, city, error)) {
        cout << "Could not load emergency units. " << error << endl;
        return 1;
    }
    if (!dispatchHistory.load(HISTORY_FILE, error)) {
        cout << "Could not load dispatch history. " << error << endl;
        return 1;
    }

    cout << "Smart Emergency Response and Route Optimizer" << endl;
    cout << "City map, route finder, and incident prioritization" << endl;

    while (true) {
        cout << endl << "1. Show locations" << endl;
        cout << "2. Show roads" << endl;
        cout << "3. Find fastest route" << endl;
        cout << "4. Change traffic or road status" << endl;
        cout << "5. Report an emergency" << endl;
        cout << "6. Prioritize new reports" << endl;
        cout << "7. Show incident priorities" << endl;
        cout << "8. Advance waiting time" << endl;
        cout << "9. Show emergency units" << endl;
        cout << "10. Register an emergency unit" << endl;
        cout << "11. Dispatch the next incident" << endl;
        cout << "12. Complete a dispatched incident" << endl;
        cout << "13. Show dispatch history" << endl;
        cout << "14. Search incidents (linear)" << endl;
        cout << "15. Search incidents (binary)" << endl;
        cout << "16. Sort incidents by priority" << endl;
        cout << "17. Sort dispatches by travel time" << endl;
        cout << "18. Search dispatch history" << endl;
        cout << "19. Find incidents at a location" << endl;
        cout << "0. Save and exit" << endl;

        int choice = -1;
        cout << "Choose: ";
        cin >> choice;
        string restOfLine;
        getline(cin, restOfLine);

        switch (choice) {
            case 0:
                if (!city.saveRoads(ROADS_FILE, error)) {
                    cout << error << endl;
                }
                if (!incidentManager.save(INCIDENTS_FILE, error)) {
                    cout << error << endl;
                }
                if (!unitManager.save(UNITS_FILE, error)) {
                    cout << error << endl;
                }
                if (!dispatchHistory.save(HISTORY_FILE, error)) {
                    cout << error << endl;
                }
                cout << "Project data saved. Goodbye." << endl;
                return 0;
            case 1:
                city.printLocations();
                break;
            case 2:
                city.printRoads();
                break;
            case 3:
                showRoute(city);
                break;
            case 4:
                changeRoad(city);
                break;
            case 5:
                reportEmergency(city, incidentManager);
                break;
            case 6:
                incidentManager.prioritizeNewReports();
                if (incidentManager.save(INCIDENTS_FILE, error)) {
                    cout << "Incoming reports moved into the priority heap." << endl;
                } else {
                    cout << error << endl;
                }
                break;
            case 7:
                incidentManager.showPriorities(city);
                break;
            case 8:
                advanceTime(incidentManager);
                break;
            case 9:
                unitManager.showUnits(city);
                break;
            case 10:
                registerUnit(city, unitManager);
                break;
            case 11:
                dispatchNextIncident(city, incidentManager, unitManager,
                                     dispatchHistory);
                break;
            case 12:
                completeIncident(incidentManager, unitManager, dispatchHistory);
                break;
            case 13:
                dispatchHistory.show();
                break;
            case 14: {
                int incidentId = 0;
                cout << "Incident ID to find: ";
                cin >> incidentId;
                getline(cin, restOfLine);
                incidentManager.searchLinear(incidentId, city);
                break;
            }
            case 15: {
                int incidentId = 0;
                cout << "Incident ID to find: ";
                cin >> incidentId;
                getline(cin, restOfLine);
                incidentManager.searchBinary(incidentId, city);
                break;
            }
            case 16:
                incidentManager.showSortedByPriority(city);
                break;
            case 17:
                dispatchHistory.showSortedByTravelTime();
                break;
            case 18: {
                int incidentId = 0;
                cout << "Incident ID to find in dispatch history: ";
                cin >> incidentId;
                getline(cin, restOfLine);
                dispatchHistory.searchByIncident(incidentId);
                break;
            }
            case 19: {
                string location;
                cout << "Location name: ";
                getline(cin, location);
                incidentManager.searchByLocation(location, city);
                break;
            }
            default:
                cout << "Choose a menu option from 0 to 19." << endl;
                break;
        }
    }
}
