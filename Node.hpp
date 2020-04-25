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
    boost::heap::priority_queue<T> *structure = new boost::heap::priority_queue<T>;

    inline bool readyToDelete() {
        return !isHead && isEmpty();
    }

public:
    std::mutex usedMutex;
    bool isHead = false;
    Node<T> *next = nullptr;

    Node<T>(bool isHead) {
        this->isHead = isHead;
        next = this;
        if (!isHead) {
            usedMutex.lock();
        }
    }

    inline void push(T el) {
        structure->push(el);
    }

    void pushAndUnlock(T el) {
        push(el);
        usedMutex.unlock();
    }

    bool pop() {
        if (isEmpty()) {
            return true;
        }
        structure->pop();
        return readyToDelete();
    }

    T top() {
        if (isEmpty()) {
            return CPQ_NULL;
        }

        return structure->top();
    }

    Node<T> *createNewNext() {
        Node *node = new Node<T>(false);
        node->next = next;
        next = node;

        return node;
    }

    inline bool isEmpty() {
        return structure->empty();
    }

    int size() {
        return structure->size();
    }

    ~Node() {
    }
};

#endif //CIRCULARPRIORITYQUEUE_NODE_HPP
