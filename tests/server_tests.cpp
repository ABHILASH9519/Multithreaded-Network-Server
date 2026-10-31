#include "server.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
int main() {
    ThreadPool pool(2);
    bool rejected = false;
    try { ThreadPool invalid(0); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    pool.shutdown();
    std::cout << "server core tests passed\n";
}