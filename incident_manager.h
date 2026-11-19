#ifndef INCIDENT_MANAGER_H
#define INCIDENT_MANAGER_H

#include "city_graph.h"

using namespace std;

const int MAX_INCIDENTS = 100;

struct Incident {
    int id;
    int type;
    int locationId;
    int severity;
    int waitingMinutes;
    int status;
    int unitId;
    int travelMinutes;
};

class IncidentQueue {
public:
    IncidentQueue();
    bool empty() const;
    bool add(int incidentId);
    int remove();

private:
    int incidentIds[MAX_INCIDENTS];
    int front;
    int rear;
    int count;
};

class IncidentHeap {
public:
    IncidentHeap();
    bool empty() const;
    bool add(Incident incident);
    Incident removeHighestPriority();
    void clear();

private:
    Incident incidents[MAX_INCIDENTS];
    int count;

    bool hasHigherPriority(const Incident& first, const Incident& second) const;
    void swapIncidents(int first, int second);
};

class IncidentManager {
public:
    IncidentManager();

    bool load(const string& fileName, const CityGraph& city, string& error);
    bool save(const string& fileName, string& error) const;
    bool report(int type, int locationId, int severity);
    void prioritizeNewReports();
    bool addWaitingTime(int minutes);
    bool nextForDispatch(Incident& incident) const;
    bool markDispatched(int incidentId, int unitId, int travelMinutes);
    bool completeIncident(int incidentId, int& unitId, int& locationId);
    void showPriorities(const CityGraph& city) const;
    void searchLinear(int incidentId, const CityGraph& city) const;
    void searchBinary(int incidentId, const CityGraph& city) const;
    void searchByLocation(const string& location, const CityGraph& city) const;
    void showSortedByPriority(const CityGraph& city) const;

private:
    Incident incidents[MAX_INCIDENTS];
    int incidentCount;
    int nextIncidentId;
    IncidentQueue newReports;
    IncidentHeap priorityHeap;

    Incident* findIncident(int id);
    void rebuildHeap();
    void printIncident(const Incident& incident, const CityGraph& city) const;
};

#endif
