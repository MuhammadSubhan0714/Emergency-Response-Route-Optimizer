#include "dispatch_history.h"

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
}

DispatchHistory::DispatchHistory() : head(nullptr), tail(nullptr) {
}

DispatchHistory::~DispatchHistory() {
    clear();
}

void DispatchHistory::clear() {
    DispatchRecord* current = head;
    while (current != nullptr) {
        DispatchRecord* next = current->next;
        delete current;
        current = next;
    }
    head = nullptr;
    tail = nullptr;
}

DispatchRecord* DispatchHistory::findRecord(int incidentId) const {
    DispatchRecord* current = head;
    while (current != nullptr) {
        if (current->incidentId == incidentId) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

bool DispatchHistory::load(const string& fileName, string& error) {
    ifstream file(fileName.c_str());
    if (!file) {
        error = "Could not open dispatch history file: " + fileName;
        return false;
    }

    clear();
    string line;
    int lineNumber = 0;
    while (getline(file, line)) {
        lineNumber++;
        if (line.length() == 0 || line[0] == '#') {
            continue;
        }

        string fields[4];
        if (splitLine(line, fields, 4) != 4) {
            error = "Invalid dispatch record at line " + to_string(lineNumber);
            return false;
        }

        int incidentId = 0;
        int unitId = 0;
        int travelMinutes = 0;
        int completedValue = 0;
        if (!textToNumber(fields[0], 100, incidentId) ||
            !textToNumber(fields[1], 100, unitId) ||
            !textToNumber(fields[2], 1000000, travelMinutes) ||
            !textToNumber(fields[3], 1, completedValue) ||
            incidentId == 0 || unitId == 0 ||
            findRecord(incidentId) != nullptr) {
            error = "Invalid dispatch record at line " + to_string(lineNumber);
            return false;
        }

        if (!add(incidentId, unitId, travelMinutes)) {
            error = "Could not add dispatch record at line " + to_string(lineNumber);
            return false;
        }
        if (completedValue == 1) {
            markCompleted(incidentId);
        }
    }
    return true;
}

bool DispatchHistory::save(const string& fileName, string& error) const {
    ofstream file(fileName.c_str());
    if (!file) {
        error = "Could not save dispatch history file: " + fileName;
        return false;
    }

    file << "# incidentId|unitId|travelMinutes|completed(1=yes,0=no)" << endl;
    DispatchRecord* current = head;
    while (current != nullptr) {
        file << current->incidentId << '|' << current->unitId << '|'
             << current->travelMinutes << '|' << (current->completed ? 1 : 0)
             << endl;
        current = current->next;
    }
    if (!file) {
        error = "A write error occurred while saving dispatch history.";
        return false;
    }
    return true;
}

bool DispatchHistory::add(int incidentId, int unitId, int travelMinutes) {
    if (incidentId <= 0 || unitId <= 0 || travelMinutes < 0 ||
        findRecord(incidentId) != nullptr) {
        return false;
    }

    DispatchRecord* record = new DispatchRecord;
    record->incidentId = incidentId;
    record->unitId = unitId;
    record->travelMinutes = travelMinutes;
    record->completed = false;
    record->next = nullptr;

    if (head == nullptr) {
        head = record;
        tail = record;
    } else {
        tail->next = record;
        tail = record;
    }
    return true;
}

bool DispatchHistory::markCompleted(int incidentId) {
    DispatchRecord* record = findRecord(incidentId);
    if (record == nullptr) {
        return false;
    }
    record->completed = true;
    return true;
}

void DispatchHistory::show() const {
    cout << endl << "Dispatch history" << endl;
    if (head == nullptr) {
        cout << "No units have been dispatched yet." << endl;
        return;
    }

    DispatchRecord* current = head;
    while (current != nullptr) {
        cout << "Incident " << current->incidentId
             << " | unit " << current->unitId
             << " | route " << current->travelMinutes << " min | "
             << (current->completed ? "Completed" : "In progress") << endl;
        current = current->next;
    }
}

void DispatchHistory::searchByIncident(int incidentId) const {
    int comparisons = 0;
    DispatchRecord* current = head;

    while (current != nullptr) {
        comparisons++;
        if (current->incidentId == incidentId) {
            cout << "Dispatch record found:" << endl;
            cout << "Incident " << current->incidentId
                 << " | unit " << current->unitId
                 << " | route " << current->travelMinutes << " min | "
                 << (current->completed ? "Completed" : "In progress") << endl;
            cout << "Linked-list comparisons: " << comparisons << endl;
            return;
        }
        current = current->next;
    }

    cout << "No dispatch record for that incident. Comparisons: "
         << comparisons << endl;
}

void DispatchHistory::showSortedByTravelTime() const {
    const int MAX_HISTORY = 100;
    DispatchRecord sorted[MAX_HISTORY];
    int recordCount = 0;
    DispatchRecord* current = head;

    while (current != nullptr && recordCount < MAX_HISTORY) {
        sorted[recordCount] = *current;
        sorted[recordCount].next = nullptr;
        recordCount++;
        current = current->next;
    }

    int comparisons = 0;
    for (int i = 1; i < recordCount; i++) {
        DispatchRecord record = sorted[i];
        int place = i - 1;

        while (place >= 0) {
            comparisons++;
            if (sorted[place].travelMinutes <= record.travelMinutes) {
                break;
            }
            sorted[place + 1] = sorted[place];
            place--;
        }
        sorted[place + 1] = record;
    }

    cout << endl << "Dispatches sorted by travel time" << endl;
    if (recordCount == 0) {
        cout << "No units have been dispatched yet." << endl;
    }
    for (int i = 0; i < recordCount; i++) {
        cout << "Incident " << sorted[i].incidentId
             << " | unit " << sorted[i].unitId
             << " | route " << sorted[i].travelMinutes << " min | "
             << (sorted[i].completed ? "Completed" : "In progress") << endl;
    }
    cout << "Insertion sort comparisons: " << comparisons << endl;
}
