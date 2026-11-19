#ifndef DISPATCH_HISTORY_H
#define DISPATCH_HISTORY_H

#include <string>

using namespace std;

struct DispatchRecord {
    int incidentId;
    int unitId;
    int travelMinutes;
    bool completed;
    DispatchRecord* next;
};

class DispatchHistory {
public:
    DispatchHistory();
    ~DispatchHistory();

    bool load(const string& fileName, string& error);
    bool save(const string& fileName, string& error) const;
    bool add(int incidentId, int unitId, int travelMinutes);
    bool markCompleted(int incidentId);
    void show() const;
    void searchByIncident(int incidentId) const;
    void showSortedByTravelTime() const;

private:
    DispatchRecord* head;
    DispatchRecord* tail;

    DispatchHistory(const DispatchHistory&);
    DispatchHistory& operator=(const DispatchHistory&);

    void clear();
    DispatchRecord* findRecord(int incidentId) const;
};

#endif
