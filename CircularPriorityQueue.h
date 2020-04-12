//
// Created by komdosh on 05.04.2020.
//

#ifndef CIRCULARPRIORITYQUEUE_CIRCULARPRIORITYQUEUE_H
#define CIRCULARPRIORITYQUEUE_CIRCULARPRIORITYQUEUE_H

#include "boost/heap/priority_queue.hpp"
#include <mutex>
#include <iostream>
#include <thread>
#include <atomic>
#include "Node.h"

#define CPQ_NULL INT32_MIN

template<typename T>
class CircularPriorityQueue {
    Node<T> *head = nullptr;

    Node<T> *getPrevPriorNode() {
        Node<T> *node = head->next;
        Node<T> *prevNode = head->next;

        if (node->isHead) {
            return node;
        }

        T priorValue = prevNode->top();
        Node<T> *prevPriorNode = prevNode;

        do {
            T value = node->top();

            if (priorValue == CPQ_NULL || (value != CPQ_NULL && priorValue < value)) {
                priorValue = value;
                prevPriorNode = prevNode;
            }
            node = node->next;
            prevNode = node;
        } while (!node->isHead);

        return prevPriorNode;
    }

public:

    CircularPriorityQueue() {
        this->head = new Node<T>(true);
    }

    void push(T el) {
        Node<T> *node = head;
        bool expected = false;
        if (!node->isUsed.compare_exchange_weak(expected, true)) {
            if (!node->next->isHead) {
                do {
                    node = node->next;
                } while (!node->isHead &&
                         !node->isUsed.compare_exchange_weak(expected, true));
            }
            if (node->isHead) {
                node = node->createNewNext();
                head->next = node->createNewNext();
            }
        }

        node->push(el);
        node->isUsed = false;
    }

    std::mutex m2;

    void pop() {
        Node<T> *prev;
        Node<T> *nodeToPop;

        do {
            prev = getPrevPriorNode();
            nodeToPop = prev->next;
        } while (!m2.try_lock());
        nodeToPop->pop();
        if (nodeToPop->readyToDelete()) {
            prev->next = nodeToPop->next;
        }
        m2.unlock();
    }

    T top() {
        Node<T> *prev = getPrevPriorNode();
        return prev->next->top();
    }

    bool isEmpty() {
        Node<T> *node = head;
        do {
            if (!node->isEmpty()) {
                return false;
            }
            node = node->next;
        } while (node != head);
        return true;
    }

    int size() {
        int size = 0;
        Node<T> *node = head;
        do {
            size += node->size();
            node = node->next;
        } while (node != head);
        return size;
    }

    int nodes() {
        int size = 0;
        Node<T> *node = head;
        do {
            ++size;
            node = node->next;
        } while (node != head);
        return size;
    }
};

#endif //CIRCULARPRIORITYQUEUE_CIRCULARPRIORITYQUEUE_H
