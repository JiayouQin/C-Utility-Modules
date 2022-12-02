#pragma once
#include  <assert.h>
template<typename T> class LinkedList;


template<typename T>
class Node {
	T* value = 0;
public:
	Node* prev = 0;
	Node* next = 0;
	
	Node() { value = 0; }

	Node(T& arg) {
		value = new T(arg);
	}
	~Node() {
		delete value;
	}

	T getVal() {
		if (!value) { return 0; }
		return *(value);
	}

	void insert(T arg) {
		Node* node = new Node(arg);
		node->prev = prev;
		node->next = this;
		prev = node;
	}

	void append(T arg) {
		Node* node = new Node(arg);
		node->next = next;
		node->prev = this;
		next = node;
	}

	void pop() {
		if (prev) {
			prev->next = next;
			if (next) { next->prev = prev; }
			delete this;
		}
		else {
			
		}
		
	}


};

template<typename T>
class LinkedList {
public:
	Node<T>* head;

	LinkedList() {
		if (!head) {
			head = new Node<T>();
		}
	};
	
	~LinkedList() {
		Node<T>* ptr = head;
		if (!ptr) { return; }
		while (ptr->next) {
			Node<T>* sent = ptr;
			ptr = ptr->next;
			delete sent;
		}
	}

	void append(T arg) {
		Node<T>* ptr = head->next;
		if (!ptr) {
			head->next = new Node<T>(arg);
			return;
		}
		while (ptr->next){
			ptr = ptr->next;
		}
		ptr->append(arg);
	}

	Node<T>*& select (int index) {
		int i = -1;
		Node<T>* ptr = head;
		while (i < index) {
			assert(head->next);
			ptr = ptr->next;
			i++;
		}
		return ptr;
	}


};