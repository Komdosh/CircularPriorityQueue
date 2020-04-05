//
// Created by komdosh on 05.04.2020.
//

#ifndef CIRCULARPRIORITYQUEUE_NODE_H
#define CIRCULARPRIORITYQUEUE_NODE_H

#include "boost/heap/priority_queue.hpp"
#include "boost/lockfree/queue.hpp"
#include <mutex>
#include <iostream>
#include <thread>
#include <atomic>

#define CPQ_NULL INT32_MIN

template<typename T>
class Node {
    boost::heap::priority_queue<T> structure;
public:
    std::atomic<bool> isUsed;
    std::atomic<bool> isHead;
    Node<T> *next = nullptr;

    Node<T>(bool isHead) {
        this->isHead = isHead;
        next = this;
    }

    void push(T el) {
        structure.push(el);
    }

    void pop() {
        if (isEmpty()) {
            return;
        }
        structure.pop();
    }

    T top() {
        if (isEmpty()) {
            return CPQ_NULL;
        }
        return structure.top();
    }

    Node<T> *createNewNext() {
        Node *node = new Node<T>(false);
        node->next = next;
        return node;
    }

    bool readyToDelete() {
        return !isHead && isEmpty();
    }

    inline bool isEmpty() {
        return structure.empty();
    }

    int size() {
        return structure.size();
    }

};

#endif //CIRCULARPRIORITYQUEUE_NODE_H
