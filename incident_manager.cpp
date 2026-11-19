#include "incident_manager.h"

#include <fstream>
#include <iostream>

using namespace std;

namespace {
const int WAITING_FOR_PRIORITY = 0;
const int IN_PRIORITY_HEAP = 1;
const int DISPATCHED = 2;
const int COMPLETED = 3;
const int MAX_WAITING_MINUTES = 1000000;

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
        if (digit > maximum || value > (maximum - digit) / 10) {
            return false;
        }
        value = value * 10 + digit;
    }
    return true;
}

int splitRecord(const string& line, string fields[], int fieldLimit) {
    int fieldCount = 0;
    int start = 0;
    int lineLength = line.length();

    for (int i = 0; i <= lineLength; i++) {
        if (i == lineLength || line[i] == '|') {
            if (fieldCount >= fieldLimit) {
                return fieldLimit + 1;
            }
            fields[fieldCount] = line.substr(start, i - start);
            fieldCount++;
            start = i + 1;
        }
    }
    return fieldCount;
}

string incidentTypeName(int type) {
    if (type == 1) {
        return "Medical";
    }
    if (type == 2) {
        return "Fire";
    }
    if (type == 3) {
        return "Accident";
    }
    return "Security";
}

string incidentStatusName(int status) {
    if (status == WAITING_FOR_PRIORITY) {
        return "New";
    }
    if (status == IN_PRIORITY_HEAP) {
        return "Prioritized";
    }
    if (status == DISPATCHED) {
        return "Dispatched";
    }
    return "Completed";
}

int priorityScore(const Incident& incident) {
    return incident.severity * 100 + incident.waitingMinutes;
}
}

IncidentQueue::IncidentQueue() : front(0), rear(-1), count(0) {
}

bool IncidentQueue::empty() const {
    return count == 0;
}

bool IncidentQueue::add(int incidentId) {
    if (count == MAX_INCIDENTS) {
        return false;
    }

    rear = (rear + 1) % MAX_INCIDENTS;
    incidentIds[rear] = incidentId;
    count++;
    return true;
}

int IncidentQueue::remove() {
    int incidentId = incidentIds[front];
    front = (front + 1) % MAX_INCIDENTS;
    count--;
    return incidentId;
}

IncidentHeap::IncidentHeap() : count(0) {
}

bool IncidentHeap::empty() const {
    return count == 0;
}

void IncidentHeap::clear() {
    count = 0;
}

bool IncidentHeap::hasHigherPriority(const Incident& first,
                                    const Incident& second) const {
    int firstScore = first.severity * 100 + first.waitingMinutes;
    int secondScore = second.severity * 100 + second.waitingMinutes;

    if (firstScore != secondScore) {
        return firstScore > secondScore;
    }
    return first.id < second.id;
}

void IncidentHeap::swapIncidents(int first, int second) {
    Incident temporary = incidents[first];
    incidents[first] = incidents[second];
    incidents[second] = temporary;
}

bool IncidentHeap::add(Incident incident) {
    if (count == MAX_INCIDENTS) {
        return false;
    }

    incidents[count] = incident;
    int child = count;
    count++;

    while (child > 0) {
        int parent = (child - 1) / 2;
        if (!hasHigherPriority(incidents[child], incidents[parent])) {
            break;
        }
        swapIncidents(child, parent);
        child = parent;
    }
    return true;
}

Incident IncidentHeap::removeHighestPriority() {
    Incident highest = incidents[0];
    count--;

    if (count == 0) {
        return highest;
    }

    incidents[0] = incidents[count];
    int parent = 0;

    while (true) {
        int left = parent * 2 + 1;
        int right = left + 1;
        int highestIndex = parent;

        if (left < count && hasHigherPriority(incidents[left], incidents[highestIndex])) {
            highestIndex = left;
        }
        if (right < count && hasHigherPriority(incidents[right], incidents[highestIndex])) {
            highestIndex = right;
        }
        if (highestIndex == parent) {
            break;
        }

        swapIncidents(parent, highestIndex);
        parent = highestIndex;
    }
    return highest;
}

IncidentManager::IncidentManager() : incidentCount(0), nextIncidentId(1) {
}

bool IncidentManager::load(const string& fileName,
                           const CityGraph& city,
                           string& error) {
    ifstream file(fileName.c_str());
    if (!file) {
        error = "Could not open incidents file: " + fileName;
        return false;
    }

    incidentCount = 0;
    nextIncidentId = 1;
    newReports = IncidentQueue();
    priorityHeap.clear();

    string line;
    int lineNumber = 0;
    while (getline(file, line)) {
        lineNumber++;
        if (line.length() == 0 || line[0] == '#') {
            continue;
        }

        string fields[8];
        int fieldCount = splitRecord(line, fields, 8);
        if ((fieldCount != 6 && fieldCount != 8) || incidentCount == MAX_INCIDENTS) {
            error = "Invalid or excessive incident records near line " +
                    to_string(lineNumber);
            return false;
        }

        Incident incident;
        if (!textToNumber(fields[0], MAX_INCIDENTS, incident.id) ||
            !textToNumber(fields[1], 4, incident.type) ||
            !textToNumber(fields[2], MAX_LOCATIONS, incident.locationId) ||
            !textToNumber(fields[3], 5, incident.severity) ||
            !textToNumber(fields[4], MAX_WAITING_MINUTES, incident.waitingMinutes) ||
            !textToNumber(fields[5], 1, incident.status)) {
            error = "Invalid incident record at line " + to_string(lineNumber);
            return false;
        }
        incident.unitId = 0;
        incident.travelMinutes = 0;
        if (fieldCount == 8 &&
            (!textToNumber(fields[6], 100, incident.unitId) ||
             !textToNumber(fields[7], 1000000, incident.travelMinutes))) {
            error = "Invalid incident record at line " + to_string(lineNumber);
            return false;
        }

        if (incident.id <= 0 || incident.type < 1 || incident.type > 4 ||
            incident.locationId <= 0 || incident.locationId > MAX_LOCATIONS ||
            city.locationName(incident.locationId) == "Unknown location" ||
            incident.severity < 1 || incident.severity > 5 ||
            incident.waitingMinutes < 0 || incident.waitingMinutes > MAX_WAITING_MINUTES ||
            (incident.status < WAITING_FOR_PRIORITY || incident.status > COMPLETED) ||
            (incident.status < DISPATCHED &&
             (incident.unitId != 0 || incident.travelMinutes != 0)) ||
            (incident.status >= DISPATCHED &&
             (incident.unitId <= 0 || incident.travelMinutes < 0))) {
            error = "Invalid incident record at line " + to_string(lineNumber);
            return false;
        }

        for (int i = 0; i < incidentCount; i++) {
            if (incidents[i].id == incident.id) {
                error = "Duplicate incident ID " + to_string(incident.id);
                return false;
            }
        }

        incidents[incidentCount] = incident;
        incidentCount++;
        if (incident.id >= nextIncidentId) {
            nextIncidentId = incident.id + 1;
        }

        if (incident.status == WAITING_FOR_PRIORITY) {
            newReports.add(incident.id);
        } else if (incident.status == IN_PRIORITY_HEAP) {
            priorityHeap.add(incident);
        }
    }
    return true;
}

bool IncidentManager::save(const string& fileName, string& error) const {
    ofstream file(fileName.c_str());
    if (!file) {
        error = "Could not save incidents file: " + fileName;
        return false;
    }

    file << "# id|type|locationId|severity|waitMinutes|status(0=new,1=prioritized,2=dispatched,3=completed)|unitId|travelMinutes"
         << endl;
    for (int i = 0; i < incidentCount; i++) {
        file << incidents[i].id << '|' << incidents[i].type << '|'
             << incidents[i].locationId << '|' << incidents[i].severity << '|'
             << incidents[i].waitingMinutes << '|' << incidents[i].status << '|'
             << incidents[i].unitId << '|' << incidents[i].travelMinutes << endl;
    }

    if (!file) {
        error = "A write error occurred while saving incidents.";
        return false;
    }
    return true;
}

bool IncidentManager::report(int type, int locationId, int severity) {
    if (incidentCount == MAX_INCIDENTS || type < 1 || type > 4 ||
        locationId <= 0 || locationId > MAX_LOCATIONS || severity < 1 || severity > 5) {
        return false;
    }

    Incident incident;
    incident.id = nextIncidentId;
    incident.type = type;
    incident.locationId = locationId;
    incident.severity = severity;
    incident.waitingMinutes = 0;
    incident.status = WAITING_FOR_PRIORITY;
    incident.unitId = 0;
    incident.travelMinutes = 0;

    incidents[incidentCount] = incident;
    incidentCount++;
    nextIncidentId++;
    newReports.add(incident.id);
    return true;
}

Incident* IncidentManager::findIncident(int id) {
    for (int i = 0; i < incidentCount; i++) {
        if (incidents[i].id == id) {
            return &incidents[i];
        }
    }
    return nullptr;
}

void IncidentManager::prioritizeNewReports() {
    while (!newReports.empty()) {
        int id = newReports.remove();
        Incident* incident = findIncident(id);
        if (incident != nullptr) {
            incident->status = IN_PRIORITY_HEAP;
            priorityHeap.add(*incident);
        }
    }
}

bool IncidentManager::addWaitingTime(int minutes) {
    if (minutes <= 0) {
        return false;
    }

    for (int i = 0; i < incidentCount; i++) {
        if (incidents[i].status <= IN_PRIORITY_HEAP &&
            incidents[i].waitingMinutes > MAX_WAITING_MINUTES - minutes) {
            return false;
        }
    }
    for (int i = 0; i < incidentCount; i++) {
        if (incidents[i].status <= IN_PRIORITY_HEAP) {
            incidents[i].waitingMinutes += minutes;
        }
    }

    rebuildHeap();
    return true;
}

bool IncidentManager::nextForDispatch(Incident& incident) const {
    if (priorityHeap.empty()) {
        return false;
    }
    IncidentHeap heapCopy = priorityHeap;
    incident = heapCopy.removeHighestPriority();
    return true;
}

bool IncidentManager::markDispatched(int incidentId, int unitId, int travelMinutes) {
    if (priorityHeap.empty() || unitId <= 0 || travelMinutes < 0) {
        return false;
    }

    IncidentHeap heapCopy = priorityHeap;
    Incident highest = heapCopy.removeHighestPriority();
    if (highest.id != incidentId) {
        return false;
    }

    Incident* incident = findIncident(incidentId);
    if (incident == nullptr || incident->status != IN_PRIORITY_HEAP) {
        return false;
    }

    priorityHeap.removeHighestPriority();
    incident->status = DISPATCHED;
    incident->unitId = unitId;
    incident->travelMinutes = travelMinutes;
    return true;
}

bool IncidentManager::completeIncident(int incidentId,
                                       int& unitId,
                                       int& locationId) {
    Incident* incident = findIncident(incidentId);
    if (incident == nullptr || incident->status != DISPATCHED) {
        return false;
    }

    incident->status = COMPLETED;
    unitId = incident->unitId;
    locationId = incident->locationId;
    return true;
}

void IncidentManager::rebuildHeap() {
    priorityHeap.clear();
    for (int i = 0; i < incidentCount; i++) {
        if (incidents[i].status == IN_PRIORITY_HEAP) {
            priorityHeap.add(incidents[i]);
        }
    }
}

void IncidentManager::printIncident(const Incident& incident,
                                    const CityGraph& city) const {
    cout << "ID " << incident.id << " | " << incidentTypeName(incident.type)
         << " | " << city.locationName(incident.locationId)
         << " | severity " << incident.severity
         << " | waiting " << incident.waitingMinutes << " min"
         << " | priority " << priorityScore(incident)
         << " | " << incidentStatusName(incident.status) << endl;
}

void IncidentManager::showPriorities(const CityGraph& city) const {
    bool foundNewReports = false;
    for (int i = 0; i < incidentCount; i++) {
        if (incidents[i].status == WAITING_FOR_PRIORITY) {
            if (!foundNewReports) {
                cout << endl << "New reports (waiting in arrival order)" << endl;
                foundNewReports = true;
            }
            printIncident(incidents[i], city);
        }
    }
    if (!foundNewReports) {
        cout << endl << "No new reports are waiting in the queue." << endl;
    }

    IncidentHeap heapCopy = priorityHeap;
    if (heapCopy.empty()) {
        cout << endl << "No incidents are in the priority heap yet." << endl;
        return;
    }

    cout << endl << "Prioritized incidents (highest priority first)" << endl;
    while (!heapCopy.empty()) {
        Incident incident = heapCopy.removeHighestPriority();
        printIncident(incident, city);
    }
}

void IncidentManager::searchLinear(int incidentId, const CityGraph& city) const {
    int comparisons = 0;
    for (int i = 0; i < incidentCount; i++) {
        comparisons++;
        if (incidents[i].id == incidentId) {
            cout << "Incident found with linear search:" << endl;
            printIncident(incidents[i], city);
            cout << "Comparisons: " << comparisons << endl;
            return;
        }
    }
    cout << "Incident not found. Comparisons: " << comparisons << endl;
}

void IncidentManager::searchBinary(int incidentId, const CityGraph& city) const {
    Incident sorted[MAX_INCIDENTS];
    for (int i = 0; i < incidentCount; i++) {
        sorted[i] = incidents[i];
    }

    // Binary search needs records in ID order, so sort this report copy first.
    for (int i = 1; i < incidentCount; i++) {
        Incident current = sorted[i];
        int place = i - 1;
        while (place >= 0 && sorted[place].id > current.id) {
            sorted[place + 1] = sorted[place];
            place--;
        }
        sorted[place + 1] = current;
    }

    int left = 0;
    int right = incidentCount - 1;
    int comparisons = 0;
    while (left <= right) {
        int middle = (left + right) / 2;
        comparisons++;
        if (sorted[middle].id == incidentId) {
            cout << "Incident found with binary search:" << endl;
            printIncident(sorted[middle], city);
            cout << "Search comparisons: " << comparisons << endl;
            return;
        }
        if (sorted[middle].id < incidentId) {
            left = middle + 1;
        } else {
            right = middle - 1;
        }
    }
    cout << "Incident not found. Search comparisons: " << comparisons << endl;
}

void IncidentManager::searchByLocation(const string& location,
                                       const CityGraph& city) const {
    int locationId = 0;
    if (!city.findLocation(location, locationId)) {
        cout << "Location not found. Use a name shown in the locations list." << endl;
        return;
    }

    int matches = 0;
    cout << endl << "Incidents at " << city.locationName(locationId) << endl;
    for (int i = 0; i < incidentCount; i++) {
        if (incidents[i].locationId == locationId) {
            printIncident(incidents[i], city);
            matches++;
        }
    }
    if (matches == 0) {
        cout << "No incidents have been reported at this location." << endl;
    }
}

void IncidentManager::showSortedByPriority(const CityGraph& city) const {
    Incident sorted[MAX_INCIDENTS];
    for (int i = 0; i < incidentCount; i++) {
        sorted[i] = incidents[i];
    }

    int comparisons = 0;
    for (int end = incidentCount - 1; end > 0; end--) {
        for (int i = 0; i < end; i++) {
            comparisons++;
            if (priorityScore(sorted[i]) < priorityScore(sorted[i + 1])) {
                Incident temporary = sorted[i];
                sorted[i] = sorted[i + 1];
                sorted[i + 1] = temporary;
            }
        }
    }

    cout << endl << "Incidents sorted by priority" << endl;
    for (int i = 0; i < incidentCount; i++) {
        printIncident(sorted[i], city);
    }
    cout << "Bubble sort comparisons: " << comparisons << endl;
}
