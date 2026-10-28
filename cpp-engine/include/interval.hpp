#pragma once

struct Interval {
    int start = 0;
    int end = 0;

    bool isValid() const {
        return start < end;
    }
};

inline bool overlaps(const Interval& a, const Interval& b) {
    return a.start < b.end && b.start < a.end;
}
