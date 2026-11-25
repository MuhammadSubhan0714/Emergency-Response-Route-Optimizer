
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

// Basic text validation
bool validText(string input)
{
    if(input.empty()){
        return false;
    }

    bool hasLetter = false;

    for(int i = 0; i < input.length(); i++){
        if(input[i] >= 'A' && input[i] <= 'Z'){
            hasLetter = true;
        }
        else if(input[i] >= 'a' && input[i] <= 'z'){
            hasLetter = true;
        }
    }

    return hasLetter;
}

// Basic integer input validation
bool validNumber(int &number)
{
    cin >> number;

    if(cin.fail()){
        cin.clear();
        cin.ignore(1000, '\n');
        return false;
    }

    string extra;
    getline(cin, extra);

    if(!extra.empty()){
        for(int i = 0; i < extra.length(); i++){
            if(extra[i] != ' ' && extra[i] != '\t'){
                return false;
            }
        }
    }

    return true;
}

void showRoute(const CityGraph &city)
{
    string start;
    string destination;

    cout << "Starting location: ";
    getline(cin, start);

    if(!validText(start)){
        cout << "Invalid starting location!" << endl;
        return;
    }

    cout << "Destination: ";
    getline(cin, destination);

    if(!validText(destination)){
        cout << "Invalid destination!" << endl;
        return;
    }

    Route route = city.fastestRoute(start, destination);

    if(!route.found){
        cout << "No route found. Check the location names and open roads." << endl;
        return;
    }

    cout << "Fastest route (" << route.travelMinutes << " minutes):" << endl;

    cout << "  ";

    for(int i = 0; i < route.locationCount; i++){
        if(i > 0){
            cout << " -> ";
        }

        cout << city.locationName(route.locationIds[i]);
    }

    cout << endl;
}

void changeRoad(CityGraph &city)
{
    string firstLocation;
    string secondLocation;
    int minutes = 0;
    int status = 0;

    cout << "First location on the road: ";
    getline(cin, firstLocation);

    if(!validText(firstLocation)){
        cout << "Invalid location name!" << endl;
        return;
    }

    cout << "Second location on the road: ";
    getline(cin, secondLocation);

    if(!validText(secondLocation)){
        cout << "Invalid location name!" << endl;
        return;
    }

    if(firstLocation == secondLocation){
        cout << "Both locations cannot be the same!" << endl;
        return;
    }

    cout << "Travel time in minutes: ";

    if(!validNumber(minutes) || minutes <= 0){
        cout << "Enter a valid positive travel time!" << endl;
        return;
    }

    cout << "Road status (1 = open, 2 = closed): ";

    if(!validNumber(status) || status < 1 || status > 2){
        cout << "Choose 1 for open or 2 for closed." << endl;
        return;
    }

    bool roadIsOpen = (status == 1);

    if(!city.updateRoad(firstLocation, secondLocation, minutes, roadIsOpen)){
        cout << "Road not found. Only existing roads can be changed." << endl;
        return;
    }

    string error;

    if(!city.saveRoads(ROADS_FILE, error)){
        cout << error << endl;
    }
    else{
        cout << "Road condition saved successfully." << endl;
    }
}

void dispatchNextIncident(const CityGraph &city, IncidentManager &incidents,
                          UnitManager &units, DispatchHistory &history);

void reportEmergency(const CityGraph &city, IncidentManager &manager,
                     UnitManager &units, DispatchHistory &history)
{
    int type = 0;
    int severity = 0;
    int locationId = 0;
    string location;

    cout << "Emergency type: 1. Medical  2. Fire  3. Accident  4. Security" << endl;
    cout << "Choose type: ";

    if(!validNumber(type) || type < 1 || type > 4){
        cout << "Choose an emergency type from 1 to 4." << endl;
        return;
    }

    cout << "Incident location: ";
    getline(cin, location);

    if(!validText(location)){
        cout << "Invalid location name!" << endl;
        return;
    }

    if(!city.findLocation(location, locationId)){
        cout << "Location not found. Use a name shown in the locations list." << endl;
        return;
    }

    cout << "Severity (1 = low, 5 = critical): ";

    if(!validNumber(severity) || severity < 1 || severity > 5){
        cout << "Severity must be from 1 to 5." << endl;
        return;
    }

    if(manager.report(type, locationId, severity)){
        manager.prioritizeNewReports();

        string error;

        if(manager.save(INCIDENTS_FILE, error)){
            cout << "Emergency prioritized automatically." << endl;
            dispatchNextIncident(city, manager, units, history);
        }
        else{
            cout << error << endl;
        }
    }
    else{
        cout << "The incident list is full." << endl;
    }
}

void advanceTime(IncidentManager &manager)
{
    int minutes = 0;

    cout << "Minutes to advance: ";

    if(!validNumber(minutes) || minutes <= 0){
        cout << "Enter a positive number of minutes." << endl;
        return;
    }

    if(!manager.addWaitingTime(minutes)){
        cout << "Enter a positive number within the simulation limit." << endl;
        return;
    }

    string error;

    if(manager.save(INCIDENTS_FILE, error)){
        cout << "Waiting time updated successfully." << endl;
    }
    else{
        cout << error << endl;
    }
}

void registerUnit(const CityGraph &city, UnitManager &units)
{
    int type = 0;
    int locationId = 0;
    string location;

    cout << "Unit type: 1. Ambulance  2. Fire engine  3. Rescue  4. Patrol" << endl;
    cout << "Choose type: ";

    if(!validNumber(type) || type < 1 || type > 4){
        cout << "Choose a unit type from 1 to 4." << endl;
        return;
    }

    cout << "Unit starting location: ";
    getline(cin, location);

    if(!validText(location)){
        cout << "Invalid location name!" << endl;
        return;
    }

    if(!city.findLocation(location, locationId)){
        cout << "Location not found. Use a name shown in the locations list." << endl;
        return;
    }

    if(!units.registerUnit(type, locationId)){
        cout << "The unit list is full." << endl;
        return;
    }

    string error;

    if(units.save(UNITS_FILE, error)){
        cout << "Emergency unit registered and saved." << endl;
    }
    else{
        cout << error << endl;
    }
}

void dispatchNextIncident(const CityGraph &city, IncidentManager &incidents,
                          UnitManager &units, DispatchHistory &history)
{
    Incident incident;

    if(!incidents.nextForDispatch(incident)){
        cout << "There are no prioritized incidents to dispatch." << endl;
        return;
    }

    EmergencyUnit unit;
    Route route;

    if(!units.findFastestUnit(incident.type, incident.locationId, city, unit, route)){
        cout << "No available unit can reach this incident right now." << endl;
        return;
    }

    if(!incidents.markDispatched(incident.id, unit.id, route.travelMinutes)){
        cout << "Could not update the incident status." << endl;
        return;
    }

    units.setAvailable(unit.id, false);
    history.add(incident.id, unit.id, route.travelMinutes);

    cout << "Dispatched unit " << unit.id
         << " with an estimated route time of "
         << route.travelMinutes << " minutes." << endl;

    cout << "Route: ";

    for(int i = 0; i < route.locationCount; i++){
        if(i > 0){
            cout << " -> ";
        }

        cout << city.locationName(route.locationIds[i]);
    }

    cout << endl;

    string error;

    if(!incidents.save(INCIDENTS_FILE, error)){
        cout << error << endl;
    }

    if(!units.save(UNITS_FILE, error)){
        cout << error << endl;
    }

    if(!history.save(HISTORY_FILE, error)){
        cout << error << endl;
    }
}

void completeIncident(const CityGraph &city, IncidentManager &incidents,
                      UnitManager &units, DispatchHistory &history)
{
    int incidentId = 0;
    int unitId = 0;
    int incidentLocation = 0;
    int choice = 0;

    incidents.showDispatched(city);

    cout << "Choose the incident number to complete: ";

    if(!validNumber(choice) || choice <= 0){
        cout << "Enter a valid incident number!" << endl;
        return;
    }

    if(!incidents.completeDispatchedAt(choice, incidentId, unitId,
                                       incidentLocation)){
        cout << "Choose a number from the dispatched incident list." << endl;
        return;
    }

    units.completeDispatch(unitId, incidentLocation);
    history.markCompleted(incidentId);

    string error;

    if(!incidents.save(INCIDENTS_FILE, error)){
        cout << error << endl;
    }

    if(!units.save(UNITS_FILE, error)){
        cout << error << endl;
    }

    if(!history.save(HISTORY_FILE, error)){
        cout << error << endl;
    }

    cout << "Incident completed. Unit " << unitId
         << " is available again." << endl;
}

int main()
{
    CityGraph city;
    IncidentManager incidentManager;
    UnitManager unitManager;
    DispatchHistory dispatchHistory;
    string error;

    if(!city.load(LOCATIONS_FILE, ROADS_FILE, error)){
        cout << "Could not load the city map. " << error << endl;
        return 1;
    }

    if(!incidentManager.load(INCIDENTS_FILE, city, error)){
        cout << "Could not load incident records. " << error << endl;
        return 1;
    }

    if(!unitManager.load(UNITS_FILE, city, error)){
        cout << "Could not load emergency units. " << error << endl;
        return 1;
    }

    if(!dispatchHistory.load(HISTORY_FILE, error)){
        cout << "Could not load dispatch history. " << error << endl;
        return 1;
    }

    incidentManager.prioritizeNewReports();

    if(!incidentManager.save(INCIDENTS_FILE, error)){
        cout << error << endl;
    }

    cout << "============================================================" << endl;
    cout << "       SMART EMERGENCY RESPONSE & ROUTE OPTIMIZER" << endl;
    cout << "              Emergency Dispatch Simulation" << endl;
    cout << "============================================================" << endl;

    while(true)
    {
        cout << endl << "1. Show locations" << endl;
        cout << "2. Show roads" << endl;
        cout << "3. Find fastest route" << endl;
        cout << "4. Change traffic or road status" << endl;
        cout << "5. Report an emergency" << endl;
        cout << "6. Search incidents or view all by priority" << endl;
        cout << "7. Advance waiting time" << endl;
        cout << "8. Show emergency units" << endl;
        cout << "9. Register an emergency unit" << endl;
        cout << "10. Dispatch the next incident" << endl;
        cout << "11. Complete a dispatched incident" << endl;
        cout << "12. Show dispatch history" << endl;
        cout << "13. Sort incidents by priority" << endl;
        cout << "14. Sort dispatches by travel time" << endl;
        cout << "15. Find incidents at a location" << endl;
        cout << "16. Save and exit" << endl;

        int choice = -1;

        cout << "Choose: ";

        if(!validNumber(choice)){
            cout << "Invalid input! Please enter numbers only." << endl;
            continue;
        }

        if(choice < 1 || choice > 16){
            cout << "Choose a menu option from 1 to 16." << endl;
            continue;
        }

        switch(choice)
        {
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
                reportEmergency(city, incidentManager, unitManager,
                                dispatchHistory);
                break;

            case 6:
            {
                string keyword;

                cout << "Search by type, location, or status (Enter for all, ranked by priority): ";
                getline(cin, keyword);

                incidentManager.searchByKeyword(keyword, city);
                break;
            }

            case 7:
                advanceTime(incidentManager);
                break;

            case 8:
                unitManager.showUnits(city);
                break;

            case 9:
                registerUnit(city, unitManager);
                break;

            case 10:
                dispatchNextIncident(city, incidentManager, unitManager,
                                     dispatchHistory);
                break;

            case 11:
                completeIncident(city, incidentManager, unitManager,
                                 dispatchHistory);
                break;

            case 12:
                dispatchHistory.show();
                break;

            case 13:
                incidentManager.showSortedByPriority(city);
                break;

            case 14:
                dispatchHistory.showSortedByTravelTime();
                break;

            case 15:
            {
                string location;

                cout << "Location name: ";
                getline(cin, location);

                if(!validText(location)){
                    cout << "Invalid location name!" << endl;
                    break;
                }

                incidentManager.searchByLocation(location, city);
                break;
            }

            case 16:
                if(!city.saveRoads(ROADS_FILE, error)){
                    cout << error << endl;
                }

                if(!incidentManager.save(INCIDENTS_FILE, error)){
                    cout << error << endl;
                }

                if(!unitManager.save(UNITS_FILE, error)){
                    cout << error << endl;
                }

                if(!dispatchHistory.save(HISTORY_FILE, error)){
                    cout << error << endl;
                }

                cout << "Project data saved. Goodbye." << endl;
                return 0;
        }
    }
}
