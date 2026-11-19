# Smart Emergency Response and Route Optimizer

## Current stage: Emergency dispatch

The program loads a sample city, emergency reports, and response units. New
reports enter a FIFO queue. When they are prioritized, a heap orders them by
`severity * 100 + waiting minutes`. For the highest-priority report, the
program checks available units that can handle that incident type, calculates
each unit's route with Dijkstra's algorithm, and dispatches the one with the
lowest travel time. Completing the incident makes the unit available at the
incident location. Road updates, incidents, units, and dispatch history are
saved between runs.

Reports include linear and binary incident searches, bubble sort by incident
priority, and insertion sort by dispatch travel time. These views work on
copies so they do not change the live queue, heap, or linked-list history.
You can also search for all incidents at a location.

The sample data contains 20 locations, 42 roads, 12 emergency units, and 6
incoming incidents so route selection and prioritization have several options.

### Data structures in this stage

- **Graph:** locations are vertices and roads are weighted, undirected edges.
- **Linked adjacency lists:** each location stores its connected roads as nodes.
- **Dijkstra's algorithm:** calculates the route with the least total travel
  time while ignoring closed roads.
- **Binary min-heap:** selects the next closest location in Dijkstra's search.
- **Hash table with linked chaining:** looks up exact location names without
  scanning the entire location list.
- **FIFO queue:** keeps incoming emergency reports in arrival order.
- **Priority heap:** chooses which incident to handle first.
- **Linked list:** stores the dispatch and completion history.
- **Searching:** linear and binary searches by incident ID.
- **Hash lookup:** find incident records by a location name.
- **Sorting:** bubble sort by priority and insertion sort by route time.
- **Fixed-size arrays:** keep graph locations, route costs, and heap entries;
  this version supports up to 100 locations and 100 incidents.
- **File handling:** `data/locations.txt` stores locations and
  `data/roads.txt` stores travel times and road availability. Incident records
  are in `data/incidents.txt`, units are in `data/units.txt`, and history is in
  `data/dispatch_history.txt`.

### Build and run

From the project folder, with a C++17 compiler installed:

```text
g++ -std=c++17 -Wall -Wextra -pedantic src/main.cpp src/city_graph.cpp src/location_lookup.cpp src/incident_manager.cpp src/unit_manager.cpp src/dispatch_history.cpp -o emergency_route
./emergency_route
```

On Windows, run `emergency_route.exe` after compiling. Start the program from
the project folder so it can find the `data` files.

When entering a route, use the location names exactly as they appear in the
menu. To simulate traffic, set a road's travel time to a higher number. To
simulate a closure, mark the road closed; Dijkstra will then search for another
open route.

### Example walkthrough

1. Report a medical emergency in Clifton with severity 5.
2. Prioritize new reports, then view the heap order.
3. Dispatch the next incident and compare the available ambulances' routes.
4. Complete the incident and check that the assigned unit is available at Clifton.

### Still to add

- More algorithm comparisons using larger sample data sets.
