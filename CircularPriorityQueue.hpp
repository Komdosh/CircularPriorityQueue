//
// Created by komdosh on 05.04.2020.
//

#ifndef CIRCULARPRIORITYQUEUE_CIRCULARPRIORITYQUEUE_HPP
#define CIRCULARPRIORITYQUEUE_CIRCULARPRIORITYQUEUE_HPP

#include "boost/heap/priority_queue.hpp"
#include <mutex>
#include <iostream>
#include <thread>
#include <atomic>
#include "Node.hpp"

#define CPQ_NULL INT32_MIN

template<typename T>
class CircularPriorityQueue {
    Node<T> *head = nullptr;

    Node<T> *getPrevPriorNode(bool isPop = false) {
        Node<T> *node = head->next;

        if (node->isHead) {
            return node;
        }

        Node<T> *prevNode = head->next;
        Node<T> *prevPriorNode = prevNode;

        do {
            T priorValue = prevNode->top();
            do {
                T value = node->top();

                if (priorValue == CPQ_NULL || (value != CPQ_NULL && priorValue < value)) {
                    priorValue = value;
                    prevPriorNode = prevNode;
                }
                node = node->next;
                prevNode = node;
            } while (!node->isHead);
        } while (isPop && prevPriorNode->next->usedMutex.try_lock());

        return prevPriorNode;
    }

public:

    CircularPriorityQueue() {
        this->head = new Node<T>(true);
    }

    void push(T el) {
        Node<T> *node = head;

        do {
            if (node->usedMutex.try_lock()) {
                node->push(el);
                return;
            }
            node = node->next;
        } while (!node->isHead);

        node = node->createNewNext();

        node->push(el);
    }

    void pop() {
        Node<T> *prev = getPrevPriorNode(true);
        Node<T> *nodeToPop = prev->next;

        bool needToDelete = nodeToPop->pop();
        nodeToPop->usedMutex.unlock();
        if (needToDelete) {
            prev->next = nodeToPop->next;
            delete nodeToPop;
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

#endif //CIRCULARPRIORITYQUEUE_CIRCULARPRIORITYQUEUE_HPP
