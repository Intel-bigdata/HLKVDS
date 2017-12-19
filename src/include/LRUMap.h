#ifndef LRU_MAP_H
#define LRU_MAP_H

#include <iostream>
#include <vector>
#include <mutex>
#include "CacheMap.h"

namespace dslab{
template <class K, class D>
struct Node{
    K key;
    D data;
    Node *prev, *next;
};

template <class K, class D>
class LRUMap: public CacheMap<K, D>{
public:
    LRUMap(size_t size = 1024){
        if(size <= 0)
            size = 1024;

        entries = new Node<K,D>[size];
        for(size_t i = 0; i < size; ++i)
            free_entries.push_back(entries + i);

        head = new Node<K,D>;
        tail = new Node<K,D>;
        head->prev = NULL;
        head->next = tail;
        tail->prev = head;
        tail->next = NULL;

    }

    virtual ~LRUMap(){
        delete head;
        delete tail;
        delete[] entries;
    }

    bool Put(const K& key, const D& data, K& update_key, D& update_data, bool same = false);
    bool Get(const K& key, D& data);
    bool Delete(const K& key);
    
private:
    void detach(Node<K,D>* node){
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    void attach(Node<K,D>* node){
        node->prev = head;
        node->next = head->next;
        head->next = node;
        node->next->prev = node;
    }

private:
    std::unordered_map<K, Node<K,D>* > cached_map;
    std::vector<Node<K,D>* > free_entries;
    Node<K,D> * head, *tail;
    Node<K,D> * entries;
    std::mutex mtx_;
};

template<class K , class D>
bool LRUMap<K,D>::Put(const K& key , const D& data, K& update_key, D& update_data, bool same){
    update_key = K();
    update_data = D();
    bool poped;
    std::lock_guard<std::mutex> l(mtx_);
    Node<K,D> *node = cached_map[key];
    if(node){//key exist
        detach(node);
	if(!same){
	        node->data = data;
	}
        attach(node);
	poped = false;//no data poped out
    }
    else{
        if(free_entries.empty()){
            node = tail->prev;
	    update_key = node->key;
	    update_data = node->data;
            detach(node);
            cached_map.erase(node->key);
	    poped = true;//data poped out
        }
        else{
            node = free_entries.back();
            free_entries.pop_back();
	    poped = false;//no data poped out
        }
        node->key = key;
        node->data = data;
        cached_map[key] = node;
        attach(node);
    }
    return poped;
}

template<class K , class D>
bool LRUMap<K,D>::Get(const K& key, D& data){
    std::lock_guard<std::mutex> l(mtx_);
    Node<K,D> *node = cached_map[key];
    if(node){
        detach(node);
        attach(node);
        data = node->data;
	return true;
    }
    else{
        data = D();
	return false;
    }
}

template<class K , class D>
bool LRUMap<K,D>::Delete(const K& key){
    std::lock_guard<std::mutex> l(mtx_);
    Node<K,D> *node = cached_map[key];
    if(node){
        detach(node);
	free_entries.push_back(node);
	cached_map.erase(node->key);
	return true;
    }
    else{
	return false;
    }
}
}
#endif
