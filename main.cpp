#include <iostream>
#include <x86intrin.h>
#include "cpu_helper.h"
#include "CircularPriorityQueue.h"
#include "boost/heap/priority_queue.hpp"


int main() {
    CircularPriorityQueue<int> cpq;
    cpq.push(1);
    cpq.push(2);
    cpq.push(3);
    cpq.push(4);
    cpq.pop();
    std::cout << cpq.top() << std::endl;

    std::cout << "Hello, World!" << std::endl;
    return 0;
}
