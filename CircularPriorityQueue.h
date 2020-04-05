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
        if (!node->isUsed.compare_exchange_weak(expected, true, std::memory_order_seq_cst)) {
            if (!node->next->isHead) {
                do {
                    node = node->next;
                } while (!node->isHead &&
                         !node->isUsed.compare_exchange_weak(expected, true, std::memory_order_seq_cst));
            }
            if (node->isHead) {
                Node<T> *next = node->createNewNext();
                head->next = next;
                node = next;
            }
        }

        node->push(el);
        node->isUsed = false;
    }

    void pop() {
        Node<T> *prev = getPrevPriorNode();
        Node<T> *nodeToPop = prev->next;

        nodeToPop->pop();
        if (nodeToPop->readyToDelete()) {
            prev->next = nodeToPop->next;
        }
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


};

#endif //CIRCULARPRIORITYQUEUE_CIRCULARPRIORITYQUEUE_H
