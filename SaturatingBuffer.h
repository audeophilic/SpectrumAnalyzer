#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#elif defined(ARDUINO) && ARDUINO < 100
#include "WProgram.h"
#endif

#define DEFAULT_MAX_BUFFER_ITEMS 100

template <typename T> class SaturatingBuffer {
    class Node {
    public:
        T item;
        Node* next;

        Node() { next = nullptr; }

        ~Node() { next = nullptr; }
    };

    Node* _head;
    Node* _tail;
    Node* _max;
    bool _write_enabled;
    unsigned int _max_items;
    unsigned int _items_cnt;

    Node* findMax() {

        //Case where the queue is empty: return nullptr;
        if (!_head) return nullptr;

        Node* maxptr = _head;
        Node* ptr = _head->next;
        while (ptr)
        {
            //If the item at pointer is greater than the max, it becomes new max
            if (ptr->item > maxptr->item) maxptr = ptr;
            ptr = ptr->next;
        }
        return maxptr;
    }

public:
    SaturatingBuffer() :
        SaturatingBuffer(DEFAULT_MAX_BUFFER_ITEMS) {}

    SaturatingBuffer(unsigned int max_items) :
        _head(nullptr), _tail(nullptr), _max(nullptr),
        _max_items(max_items), _write_enabled(true), _items_cnt(0) {}

    ~SaturatingBuffer() {
        clear();
    }

    /*
            If the queue is full, pop
            the head before pushing.
    */
    bool push(T item) {

        Node* node = new Node;

        //If the creation of a new node failed
        if (node == nullptr) return false;

        if (!_write_enabled) return false;
        //If the queue is full, pop the head
        if (_items_cnt == _max_items) pop();
        node->item = item;

        //If the queue is empty, set head max and tail, increment count
        if (_head == nullptr) {
            _head = node;
            _tail = node;
            _max = node;
            _items_cnt++;
            return true;
        }

        //If the queue had elements in it
        _tail->next = node;
        _tail = node;
        if (node->item > _max->item) _max = node;
        _items_cnt++;
        return true;
    }

    /*
            Pop the front of the queue.
            Because exceptions are not
            usually implemented for
            microcontrollers, if queue
            is empty, a dummy item is
            returned.
    */
    T pop() {
        //Return a default T if the queue was empty
        if ((_items_cnt == 0) || (_head == nullptr)) {
            return T();
        }

        //If the queue was not empty
        Node* node = _head;
        _head = node->next;
        T item = node->item;

        //If we just popped the max, find a new max
        if (_max == node) _max = findMax();

        //Free the memory for the node
        delete node;
        node = nullptr;

        //If we just popped the last entry, reset _head and _tail
        if (_head == nullptr) {
            _tail = nullptr;
        }

        _items_cnt--;
        return item;
    }

    /*
            Returns true if the queue
            is empty, false otherwise.
    */
    bool isEmpty() { return _head == nullptr; }

    /*
            Returns true if the queue
            is full, false otherwise.
    */
    bool isFull() { return _items_cnt == _max_items; }

    /*
            Returns the number of items
            currently in the queue.
    */
    unsigned int item_count() { return _items_cnt; }

    /*
            Returns the size of the queue
            (maximum number of items)
    */
    unsigned int max_queue_size() { return _max_items; }

    /*
            Returns the size of the queue
            (maximum size in bytes)
    */
    unsigned int max_memory_size() { return sizeof(T) * _max_items; }

    /*
            DEACTIVATED FEATURE
            Returns the ith element of the queue for indexing.
            May hog a lot of resources if done in a loop, so for better performance,
            use the toArray function.
    *//*
    T operator[](const int i) {
        if (isEmpty() || i >= max_queue_size()) return pop();
        int counter = 0;
        Node* ptr = _head;
        while (ptr)
        {
            if (counter == i) return ptr->item;
            ++counter;
            ptr = ptr->next;
        }
    }*/

    /*
            Copies each item in the queue over one-by-one into an array of the same # elements
            Requires that passed in array MUST BE EQUAL TO THE NUMBER OF ITEMS IN THE QUEUE.
            If this is not true, the function will return false without completing.
            RETURNS TRUE IF COMPLETED SUCCESSFULLY, ELSE FALSE
    */
    bool toArray(T arr[], const int size)
    {
        if (size != _items_cnt) return false;
        //disable write so there are no issues
        disableWrite();
        int i = 0;
        for (Node* node = _head; node; node = node->next) {
            arr[i] = node->item;
            ++i;
        }
        enableWrite();
        return true;
    }

    /*
            Saturates the queue with val, clearing if needed
    */
    void saturate(T val)
    {
        if (_write_enabled)
        {

            if (!isEmpty()) clear();
            while (!isFull()) push(val);
        }
    }

    /*
            Cleares every item from the queue
    */
    void clear()
    {
        for (Node* node = _head; node != nullptr; node = _head) {
            _head = node->next;
            delete node;
        }
    }

    void enableWrite()
    {
        _write_enabled = true;
    }

    void disableWrite()
    {
        _write_enabled = false;
    }

    bool writeIsEnabled()
    {
        return _write_enabled;
    }

    T max() {
        return _max->item;
    }
};