//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef _KV_DB_LINKEDLIST_H_
#define _KV_DB_LINKEDLIST_H_

#include <string.h>
#include <unistd.h>
#include <vector>

using namespace std;
namespace kvdb {

template<typename T>
class Node {
public:
    T data;
    Node<T>* next;
    Node<T>* prev;

    Node(T value, Node<T>* nd1,Node<T>* nd2) :
        data(value), next(nd1),prev(nd2) {
    }
    ;
};

template<typename T>
class LinkedList {
public:
    LinkedList() :
        head_(NULL), size_(0) {
    }
    LinkedList(const LinkedList<T> &);
    ~LinkedList();
    LinkedList& operator=(const LinkedList<T> &);
    void iterator();

    bool search(T& toBeSearched);
    //if put T is existed, return false
    bool put(T& toBePuted);
    bool remove(T& toBeRemoved);

    T* getRef(T toBeGeted);
    vector<T> get();
    int get_size() {
        return size_;
    }

    Node<T>* GetHead() {
        return head_;
    }
    Node<T>* GetLast();
    void SetLast();
    Node<T>* GetNext();
    Node<T>* GetPrev();
private:
    Node<T>* head_;
    Node<T>* cur_;
    int size_;

    void copyHelper(const LinkedList &);
    void removeAll();

};

template<typename T>
LinkedList<T>::LinkedList(const LinkedList<T> &toBeCopied) {
    copyHelper(toBeCopied);
}

template<typename T>
LinkedList<T>::~LinkedList() {
    removeAll();
}

template<typename T>
LinkedList<T> &LinkedList<T>::operator=(const LinkedList<T> &toBeCopied) {
    if (this != &toBeCopied) {
        removeAll();
        copyHelper(toBeCopied);
    }
    return *this;
}

template<typename T>
void LinkedList<T>::copyHelper(const LinkedList<T> &toBeCopied) {
    if (toBeCopied.head_ == NULL) {
        head_ = NULL;
        size_ = 0;
    } else {
        size_ = toBeCopied.size_;
        Node<T>* copyNode = new Node<T> (toBeCopied.head_->data, NULL,NULL);
        head_ = copyNode;

        Node<T>* ptr = toBeCopied.head_;
        ptr = ptr->next;
        while (ptr != NULL) {
            copyNode->next = new Node<T> (ptr->data, NULL,copyNode);
            copyNode = copyNode->next;
            ptr = ptr->next;
        }
    }

}

template<typename T>
void LinkedList<T>::removeAll() {
    Node<T>* tempNode = head_;
    while (tempNode != NULL) {
        tempNode = head_->next;
        delete head_;
        head_ = tempNode;
    }
    size_ = 0;
}

template<typename T>
bool LinkedList<T>::search(T& toBeSearched) {
    Node<T>* curNode = head_;

    while (curNode != NULL) {
        if (curNode->data == toBeSearched) {
            return true;
        }
        curNode = curNode->next;
    }
    return false;
}

template<typename T>
void LinkedList<T>::iterator() {
    cur_ = NULL;
}

template<typename T>
Node<T>* LinkedList<T>::GetNext() {
    if (NULL == cur_) {
        cur_ = head_;
    } else {
        if (NULL != cur_->next) {
            cur_ = cur_->next;
        } else {
            return NULL;
        }
    }
    return cur_;
}

template<typename T>
Node<T>* LinkedList<T>::GetPrev() {
    if (NULL == cur_ ) {
        cur_ = GetLast();
    } else {
        if (NULL != cur_->prev) {
            cur_ = cur_->prev;
        } else {
            return NULL;
        }
    }
    return cur_;
}
template<typename T>
Node<T>* LinkedList<T>::GetLast() {

        Node<T>* curNode = head_;
        Node<T>* preNode;
        while (curNode != NULL) {
            preNode=curNode;
            curNode = curNode->next;
        }

        return preNode;
}

/*
template<typename T>
void LinkedList<T>::SetLast(){
    cur_=GetLast();
}
*/

template<typename T>
bool LinkedList<T>::put(T& toBePuted) {
    bool is_new = true;

    Node<T>* newNode = new Node<T> (toBePuted, NULL,NULL);
    if (head_ == NULL) {
        head_ = newNode;
        size_++;
    } else {
        Node<T>* curNode = head_;
        Node<T>* preNode;
        while (curNode != NULL) {
            if (curNode->data == toBePuted) {
                curNode->data = toBePuted;
                is_new = false;
                delete newNode;
                break;
            }
            preNode = curNode;
            curNode = curNode->next;
        }
        if (is_new) {
            curNode = newNode;
            preNode->next = curNode;
            curNode->prev = preNode;
            size_++;
        }
    }
    return is_new;
}

template<typename T>
bool LinkedList<T>::remove(T& toBeRemoved) {
    bool flag = false;
    if (head_ == NULL) {
        return flag;
    }

    Node<T>* preNode = head_;
    Node<T>* curNode = preNode->next;

    if (head_->data == toBeRemoved) {
        head_ = curNode;
        delete preNode;
        size_--;
        flag = true;
    } else {
        while (curNode != NULL) {
            if (curNode->data == toBeRemoved) {
                preNode->next = curNode->next;
                curNode->next->prev = preNode;
                delete curNode;
                size_--;
                flag = true;
                break;
            }
            preNode = curNode;
            curNode = curNode->next;
        }
    }
    return flag;
}

template<typename T>
T* LinkedList<T>::getRef(T toBeGeted) {

    Node<T>* tempNode = head_;

    while (tempNode != NULL) {
        if (tempNode->data == toBeGeted) {
            return &tempNode->data;
        }
        tempNode = tempNode->next;

    }
    return NULL;

}

template<typename T>
vector<T> LinkedList<T>::get() {
    vector<T> tempVector;
    if (head_ != NULL) {
        Node<T>* tempNode = head_;
        while (tempNode != NULL) {
            tempVector.push_back(tempNode->data);
            tempNode = tempNode->next;
        }
    }
    return tempVector;
}

}

#endif
