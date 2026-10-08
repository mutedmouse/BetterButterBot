#ifndef CMF_BINARYTREE_H
#define CMF_BINARYTREE_H

#include <set>
#include <queue>
#include <cstddef>
#include <mutex>
#include "Util/stdafx.h"

/**
 * **WARNING** Not in use, not fully optimized when it comes to memory usage
 * @brief A binary tree of key-value pairs. The tree is constructed using the comparison operators of the type of key given.
 * @tparam K The type of key. The key type must have overridden comparison operators.
 * @tparam V The type of value.
 */
template<typename K, typename V>
class BinaryTree {
public:
	/**
	 * @brief A default empty constructor.
	 */
	inline BinaryTree() noexcept = default;

	/**
	 * @brief A copy constructor from another binary tree with same template types.
	 * @param other The binary tree being copied from.
	 */
	inline BinaryTree(const BinaryTree& other) noexcept {
		std::lock_guard guard(accessMutex);
		*this = other;
	}

	/**
	 * @brief A move constructor from another binary tree with the same template types.
	 * @param other The binary tree being moved. This binary tree is empty when the constructor finishes execution.
	 */
	inline BinaryTree(BinaryTree&& other) noexcept {
		std::lock_guard guard(accessMutex);
		std::lock_guard guardOther(other.accessMutex);
		*this = std::move(other);
	}

	/**
	 * @brief
	 */
	inline virtual ~BinaryTree() noexcept {
		destruct();
	}

	/**
	 * @brief Assignment copy operator from another binary tree with the same template types.
	 * @param other The binary tree being copied.
	 * @return The reference to the binary tree called on.
	 */
	inline BinaryTree& operator = (const BinaryTree& other) noexcept {
		std::lock_guard guard(accessMutex);

		if(&other == this){
			return *this;
		}

		std::set<K> keys = other.keys();

		for(const K& key : keys){
			set(key, other[key]);
		}

		return *this;
	}

	/**
	 * @brief Assignment move operator from another binary tree with the same template types.
	 * @param other The binary tree being moved. This tree is empty after the operator finished with execution.
	 * @return The reference to the binary tree called on.
	 */
	inline BinaryTree& operator = (BinaryTree&& other) noexcept {
		if(&other == this){
			return *this;
		}

		destruct();

		std::lock_guard guard(accessMutex);
		std::lock_guard guardOther(other.accessMutex);

		head = other.head;
		start = other.start;
		end = other.end;

		other.head = nullptr;
		other.start = nullptr;
		other.end = nullptr;

		return *this;
	}

	/**
	 * @brief The equals comparison operator that compares two binary trees.
	 * @param other The second binary tree in comparison.
	 * @return True if the head pointer is same in both, false otherwise.
	 */
	inline bool operator == (const BinaryTree& other) const noexcept {
		// TODO consider changing this to equals by data rather than by pointer
		return other.head == head;
	}

	/**
	 * @brief The index operator. If no value is paired to the give key,
	 * a new key-value pair with the default value of type V will be created and returned.
	 * @param key The key of which the value should be returned.
	 * @return The value paired to the given key.
	 */
	inline V& operator [] (const K& key) noexcept {
		V* val = get(key);

		if(val == nullptr){
			set(key);
			val = get(key);
		}

		return *val;
	}

	/**
	 * @brief Set function used to create a new key-value pair from given parameters, or alter an existing one.
	 * @param key The key in the key-value pair.
	 * @param value The value in the key-value pair.
	 */
	inline void set(const K& key, const V& value = V()) noexcept {
		std::lock_guard guard(accessMutex);

		if(head == nullptr){
			head = new Node();
			head->key = key;
			head->value = value;

			start = end = head;
			return;
		}

		bool updateStart = true; // Changes to false as soon as we go right through the tree
		bool updateEnd = true; // Changes to false as soon as we go left through the tree

		Node* newNode = nullptr;

		for(Node* current = head; current != nullptr; ){
			Node** next = nullptr;

			if(key == current->key){
				current->value = value;
				return;
			}else if(key > current->key){
				next = &current->rightNode;
				updateStart = false;
			}else if(key < current->key){
				next = &current->leftNode;
				updateEnd = false;
			}

			if(next == nullptr){
				// This should not be possible
				return;
			}

			if(*next == nullptr){
				*next = newNode = new Node();
				newNode->key = key;
				newNode->value = value;
				newNode->parent = current;
				break;
			}

			current = *next;
		}

		if(newNode == nullptr){
			return;
		}

		if(updateEnd){
			end = newNode;
		}else if(updateStart){
			start = newNode;
		}
	}

	/**
	 * @brief Removes a key-value pair from the tree.
	 * @param key The key in the key-value pair being removed.
	 */
	inline void remove(const K& key) noexcept {
		std::lock_guard guard(accessMutex);

		Node** foundNode = nullptr;
		Node** replacementNode = nullptr;

		for(Node** current = &head; *current != nullptr; ){
			if(key == (*current)->key){
				foundNode = current;
				break;
			}else if(key > (*current)->key){
				current = &(*current)->rightNode;
			}else if(key < (*current)->key){
				current = &(*current)->leftNode;
			}
		}

		if(foundNode == nullptr){
			return;
		}

		if(*foundNode == nullptr){
			return;
		}

		if((*foundNode)->rightNode != nullptr){
			for(replacementNode = &(*foundNode)->rightNode; (*replacementNode)->leftNode != nullptr; replacementNode = &(*replacementNode)->leftNode);
		}else if((*foundNode)->leftNode != nullptr){
			for(replacementNode = &(*foundNode)->leftNode; (*replacementNode)->rightNode != nullptr; replacementNode = &(*replacementNode)->rightNode);
		}else{
			if(start == *foundNode){
				start = (*foundNode)->parent;
			}

			if(end == *foundNode){
				end = (*foundNode)->parent;
			}

			if(head == *foundNode){
				head = nullptr;
			}

			delete *foundNode;
			*foundNode = nullptr;
			return;
		}

		if(replacementNode == nullptr){
			return;
		}

		if(*replacementNode == nullptr){
			return;
		}

		std::swap((*foundNode)->key, (*replacementNode)->key);
		std::swap((*foundNode)->value, (*replacementNode)->value);

		delete *replacementNode;
		*replacementNode = nullptr;
	}

	/**
	 * @brief Checker function for weather the key-value pair exists in the tree or not.
	 * @param key The key in the key-value pair.
	 * @return True if key-value pair with the given key exists. False otherwise.
	 */
	inline bool contains(const K& key) const noexcept {
		std::set<K> treeKeys = keys();
		if(treeKeys.empty()){
			return false;
		}

		return treeKeys.contains(key);
	}

	/**
	 * @return The number of key-value pairs in the tree.
	 */
	inline size_t size() const noexcept {
		return keys().size();
	}

	/**
	 * @return Returns a set of all keys paired with values in the tree.
	 */
	inline std::set<K> keys() noexcept {
		std::lock_guard guard(accessMutex);

		std::set<K> keys;

		if(head == nullptr){
			return keys;
		}

		std::queue<Node*> nodeQueue;
		nodeQueue.push(head);

		for(const Node* current = nodeQueue.front(); true; ){
			nodeQueue.pop();

			if(current == nullptr){
				continue;
			}

			if(current->leftNode != nullptr){
				nodeQueue.push(current->leftNode);
			}

			if(current->rightNode != nullptr){
				nodeQueue.push(current->rightNode);
			}

			keys.insert(current->key);

			if(nodeQueue.empty()){
				break;
			}

			current = nodeQueue.front();
		}

		return keys;
	}

	/**
	 * @return Returns a set of all keys paired with values in the tree.
	 */
	inline std::set<K> keys() const noexcept {
		std::set<K> keys;

		if(head == nullptr){
			return keys;
		}

		std::queue<Node*> nodeQueue;
		nodeQueue.push(head);

		for(const Node* current = nodeQueue.front(); true; ){
			nodeQueue.pop();

			if(current == nullptr){
				continue;
			}

			if(current->leftNode != nullptr){
				nodeQueue.push(current->leftNode);
			}

			if(current->rightNode != nullptr){
				nodeQueue.push(current->rightNode);
			}

			keys.insert(current->key);

			if(nodeQueue.empty()){
				break;
			}

			current = nodeQueue.front();
		}

		return keys;
	}

private:
	/**
	 * @brief The internal node used for the structure of the tree.
	 */
	struct Node {
		K key = K();
		V value = V();
		Node* parent = nullptr;
		Node* leftNode = nullptr;
		Node* rightNode = nullptr;
	};

private:
	Node* head = nullptr;

	// These two will serve well for iterating through the tree, it will always be sorted while iterating,
	// and provides faster start of iteration in either direction
	Node* start = nullptr;
	Node* end = nullptr;

	std::mutex accessMutex;

private:
	/**
	 * @brief Getter for a pointer of a value paired to the given key.
	 * @param key The key in the key-value pair.
	 * @return The pointer to the value paired to the given key.
	 */
	inline V* get(const K& key) noexcept {
		std::lock_guard guard(accessMutex);

		if(head == nullptr){
			return nullptr;
		}

		for(Node* current = head; current != nullptr; ){
			if(key == current->key){
				return &current->value;
			}else if(key > current->key){
				current = current->rightNode;
			}else if(key < current->key){
				current = current->leftNode;
			}
		}

		return nullptr;
	}

	/**
	 * @brief Destroys the tree and all nodes in it, deallocates memory.
	 */
	inline void destruct() noexcept {
		std::lock_guard guard(accessMutex);

		std::queue<Node*> nodeQueue;
		nodeQueue.push(head);

		for(const Node* current = nodeQueue.front(); true; ){
			nodeQueue.pop();

			if(current == nullptr){
				continue;
			}

			if(current->leftNode != nullptr){
				nodeQueue.push(current->leftNode);
			}

			if(current->rightNode != nullptr){
				nodeQueue.push(current->rightNode);
			}

			delete current;

			if(nodeQueue.empty()){
				break;
			}

			current = nodeQueue.front();
		}

		head = nullptr;
		start = nullptr;
		end = nullptr;
	}
};

#endif //CMF_BINARYTREE_H