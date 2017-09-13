#ifndef LRU_MAP_H
#define LRU_MAP_H

#include <iostream>
#include <vector>
#include "CacheMap.h"
using namespace std;
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
    LRUMap(size_t size){
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

    bool Put(K key, D data, K& update_key, D& update_data, bool same = false);
    bool Get(K key, D& data);
    bool Delete(K key);
    
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
    unordered_map<K, Node<K,D>* > cached_map;
    vector<Node<K,D>* > free_entries;
    Node<K,D> * head, *tail;
    Node<K,D> * entries;
};

template<class K , class D>
bool LRUMap<K,D>::Put(K key , D data, K& update_key, D& update_data, bool same){
    update_key = K();
    update_data = D();
    Node<K,D> *node = cached_map[key];
    bool poped;
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
bool LRUMap<K,D>::Get(K key, D& data){
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
bool LRUMap<K,D>::Delete(K key){
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
