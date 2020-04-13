//
// Created by komdosh on 05.04.2020.
//

#ifndef CIRCULARPRIORITYQUEUE_NODE_HPP
#define CIRCULARPRIORITYQUEUE_NODE_HPP

#include "boost/lockfree/queue.hpp"
#include <mutex>
#include <iostream>
#include <thread>
#include <atomic>

#define CPQ_NULL INT32_MIN

template<typename T>
class Node {
    boost::heap::priority_queue<T> structure;

    inline bool readyToDelete() {
        return !isHead && isEmpty();
    }
public:
    std::mutex usedMutex;
    bool isHead;
    Node<T> *next = nullptr;

    Node<T>(bool isHead) {
        this->isHead = isHead;
        next = this;
    }

    void push(T el) {
        structure.push(el);
    }

    std::mutex popMutex;
    bool pop() {
        popMutex.lock();
        if (isEmpty()) {
            popMutex.unlock();
            return true;
        }
        structure.pop();
        popMutex.unlock();
        return readyToDelete();
    }

    std::mutex topMutex;
    T top() {
        topMutex.lock();
        if (isEmpty()) {
            topMutex.unlock();
            return CPQ_NULL;
        }

        T value = structure.top();
        topMutex.unlock();
        return value;
    }

    Node<T> *createNewNext() {
        Node *node = new Node<T>(false);
        node->next = next;
        next = node;
        return node;
    }


    inline bool isEmpty() {
        return structure.empty();
    }

    int size() {
        return structure.size();
    }

};

#endif //CIRCULARPRIORITYQUEUE_NODE_HPP
