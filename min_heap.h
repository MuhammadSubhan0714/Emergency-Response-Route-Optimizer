#ifndef MIN_HEAP_H
#define MIN_HEAP_H

using namespace std;

struct RouteCandidate {
    int minutes;
    int locationId;
};

// A binary min-heap used by Dijkstra's algorithm.
class MinHeap {
public:
    static const int MAX_ITEMS = 10000;

    MinHeap() : size(0) {
    }

    bool empty() const {
        return size == 0;
    }

    bool push(RouteCandidate candidate) {
        if (size >= MAX_ITEMS) {
            return false;
        }

        items[size] = candidate;
        int child = size;
        size++;

        while (child > 0) {
            int parent = (child - 1) / 2;
            if (items[child].minutes >= items[parent].minutes) {
                break;
            }
            swapItems(child, parent);
            child = parent;
        }
        return true;
    }

    RouteCandidate pop() {
        RouteCandidate smallest = items[0];
        size--;

        if (size == 0) {
            return smallest;
        }

        items[0] = items[size];
        int parent = 0;

        while (true) {
            int left = parent * 2 + 1;
            int right = left + 1;
            int smallestIndex = parent;

            if (left < size && less(items[left], items[smallestIndex])) {       // less compares the minutes
                smallestIndex = left;
            }
            if (right < size && less(items[right], items[smallestIndex])) {
                smallestIndex = right;
            }
            if (smallestIndex == parent) {
                break;
            }

            swapItems(parent, smallestIndex);
            parent = smallestIndex;
        }
        return smallest;
    }

private:
    RouteCandidate items[MAX_ITEMS];
    int size;

    static bool less(const RouteCandidate& first, const RouteCandidate& second) {
        return first.minutes < second.minutes;
    }

    void swapItems(int first, int second) {
        RouteCandidate temp = items[first];
        items[first] = items[second];
        items[second] = temp;
    }
};

#endif
