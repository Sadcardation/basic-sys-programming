#include "potato.hpp"

Potato::Potato(int num_hops) : hops(num_hops), trace(num_hops) {}

Potato::~Potato() {}

Potato::Potato(const Potato &p) : hops(p.hops), trace(p.trace) {}

Potato &Potato::operator=(const Potato &p) {
    if (this != &p) {
        hops = p.hops;
        trace = p.trace;
    }
    return *this;
}

int Potato::get_hops() const {
    return hops;
}

void Potato::add_trace(int id) {
    trace.push_back(id);
}

vector<int> Potato::get_trace() const {
    return trace;
}