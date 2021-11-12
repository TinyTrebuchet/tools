#include <unordered_set>

class RBNode {
public:
	int key;
	int weight {1};
	int sum {0};
	bool red;
	RBNode* left{nullptr};
	RBNode* right{nullptr};
	RBNode* parent{nullptr};

	RBNode(int k, bool r, RBNode* n) :key(k), red(r), sum(k), left(n), right(n), parent(n) {}
	RBNode(int k, bool r) :key(k), red(r), weight(0) {}
};

class RBTree {
private:
	RBNode* root;
	RBNode nn {0, false};
	RBNode* null {&nn};

public:
	RBTree() { root = null; }

	void insert(int key)
	{
		int rankVal = rank(key);
		if (rankVal)
			return;

		RBNode* node = new RBNode(key, true, null);

		insert_node(node);
	}

	void remove(int key)
	{
		RBNode* node;

		node = root;
		while (node->key != key && node != null) {
			if (key < node->key)
				node = node->left;
			else
				node = node->right;
		}

		if (node != null)
			remove_node(node);

		delete node;
	}

	int rank(int key) 
	{
		int rank;
		RBNode* node;

		rank = 1;
		node = root;

		while (node != null) {
			if (key == node->key) {
				rank += node->left->weight;
				return rank;
			}
			else if (key > node->key) {
				rank += node->left->weight + 1;
				node = node->right;
			}
			else 
				node = node->left;
		}

		return 0;
	}

	int select(int rank)
	{
		RBNode* node;

		if (rank < 1 || rank > root->weight)
			return -1;

		node = select_node(rank);
		return node->key;

	}

	int range_sum(int i, int j) 
	{
		int sum, nodeRank, subRank;
		RBNode* node;
		RBNode* minNode;
		RBNode* maxNode;
		RBNode* superNode;
		unordered_set<int> visited;

		if (i < 1)
			i = 1;
		if (j > root->weight)
			j = root->weight;
		if (i == j) {
			node = select_node(i);
			return node->key;
		}

		node = root;
		subRank = 0;
		nodeRank = (node->weight - node->right->weight);
		visited.insert(root->key);
		while (nodeRank != i) {
			if (i < nodeRank) 
				node = node->left;
			else {
				subRank = nodeRank;
				node = node->right;
			}
			nodeRank = subRank + (node->weight - node->right->weight);
			visited.insert(node->key);
		}
		minNode = node;

		node = root;
		subRank = 0;
		nodeRank = (node->weight - node->right->weight);
		superNode = root;
		while (nodeRank != j) {
			if (j < nodeRank) 
				node = node->left;
			else {
				subRank = nodeRank;
				node = node->right;
			}
			nodeRank = subRank + (node->weight - node->right->weight);
			if (visited.count(node->key) > 0) 
				superNode = node;
		}
		maxNode = node;

		sum = 0;

		node = minNode;
		if (node != superNode) {
			sum += node->key + node->right->sum;
			while (node->parent != superNode) {
				if (node == node->parent->left)
					sum += node->parent->key + node->parent->right->sum;
				node = node->parent;
			}
		}

		node = maxNode;
		if (node != superNode) {
			sum += node->key + node->left->sum;
			while (node->parent != superNode) {
				if (node == node->parent->right)
					sum += node->parent->key + node->parent->left->sum;
				node = node->parent;
			}
		}

		sum += superNode->key;

		return sum;
	}

private:
	RBNode* select_node(int rank)
	{
		int nodeRank, subRank;
		RBNode* node;

		node = root;
		subRank = 0;
		nodeRank = (node->weight - node->right->weight);
		while (nodeRank != rank) {
			if (rank < nodeRank) 
				node = node->left;
			else {
				subRank = nodeRank;
				node = node->right;
			}
			nodeRank = subRank + (node->weight - node->right->weight);
		}

		return node;
	}

	void insert_node(RBNode* node)
	{
		RBNode* parentNode;
		RBNode* currentNode;

		parentNode = null;
		currentNode = root;
		while (currentNode != null) {
			parentNode = currentNode;
			if (node->key < currentNode->key)
				currentNode = currentNode->left;
			else
				currentNode = currentNode->right;
		}
		node->parent = parentNode;

		if (parentNode == null)
			root = node;
		else if (node->key < parentNode->key)
			parentNode->left = node;
		else
			parentNode->right = node;

		while (parentNode != null) {
			parentNode->weight += 1;
			parentNode->sum += node->key;
			parentNode = parentNode->parent;
		} // done before fixing so that weights are updated correctly during rotations

		insert_fix_up(node);

	}

	void insert_fix_up(RBNode* node)
	{
		RBNode* parentNode;
		RBNode* uncleNode;
		RBNode* grandParent;
		
		parentNode = node->parent;
		grandParent = parentNode->parent;

		while (parentNode->red) {
			if (parentNode == grandParent->left) {
				uncleNode = grandParent->right;
				if (uncleNode->red) {
					parentNode->red = false;
					uncleNode->red = false;
					grandParent->red = true;
					node = grandParent;
					parentNode = node->parent;
					grandParent = parentNode->parent;
				}
				else {
					if (node == parentNode->right) {
						RBNode* tmp = node;
						node = parentNode;
						parentNode = tmp;
						left_rotate(node);
					}
					parentNode->red = false;
					grandParent->red = true;
					right_rotate(grandParent);
				}
			}
			else {
				uncleNode = grandParent->left;
				if (uncleNode->red) {
					parentNode->red = false;
					uncleNode->red = false;
					grandParent->red = true;
					node = grandParent;
					parentNode = node->parent;
					grandParent = parentNode->parent;
				}
				else {
					if (node == parentNode->left) {
						RBNode* tmp = node;
						node = parentNode;
						parentNode = tmp;
						right_rotate(node);
					}
					parentNode->red = false;
					grandParent->red = true;
					left_rotate(grandParent);
				}
			}
		}
		root->red = false;
	}

	void left_rotate(RBNode* node)
	{
		RBNode* childNode;

		childNode = node->right;
		node->weight -= childNode->right->weight + 1;
		childNode->weight += node->left->weight + 1;

		node->right = childNode->left;
		if (childNode->left != null)
			childNode->left->parent = node;
		childNode->parent = node->parent;
		if (node->parent == null) 
			root = childNode;
		else if (node == node->parent->left)
			node->parent->left = childNode;
		else
			node->parent->right = childNode;
		childNode->left = node;
		node->parent = childNode;
	}

	void right_rotate(RBNode* node)
	{
		RBNode* childNode;

		childNode = node->left;
		node->weight -= childNode->left->weight + 1;
		childNode->weight += node->right->weight + 1;

		node->left = childNode->right;
		if (childNode->right != null)
			childNode->right->parent = node;
		childNode->parent = node->parent;
		if (node->parent == null) 
			root = childNode;
		else if (node == node->parent->left)
			node->parent->left = childNode;
		else
			node->parent->right = childNode;
		childNode->right = node;
		node->parent = childNode;
	}

	void remove_node(RBNode* node)
	{
		RBNode* moveNode;
		RBNode* subNode;
		RBNode* parentNode;
		bool moveRed;

		moveNode = node;				// node either removed (if and elseif) or moved within subtree (else)
		moveRed = moveNode->red;
		if (node->left == null) {
			subNode = node->right;
			transplant(node, node->right);
		}
		else if (node->right == null) {
			subNode = node->left;
			transplant(node, node->left);
		}
		else {
			moveNode = tree_minimum(node->right);
			parentNode = moveNode->parent;
			moveRed = moveNode->red;
			subNode = moveNode->right;

			while (parentNode != node) {
				parentNode->weight -= 1;
				parentNode->sum -= moveNode->key;
				parentNode = parentNode->parent;
			}

			if (moveNode->parent == node)
				subNode->parent = moveNode;
			else {
				transplant(moveNode, moveNode->right);
				moveNode->right = node->right;
				moveNode->right->parent = moveNode;
			}
			transplant(node, moveNode);
			moveNode->left = node->left;
			moveNode->left->parent = moveNode;
			moveNode->red = node->red;
		}
		if (!moveRed)
			remove_fix_up(subNode);

		parentNode = node->parent;
		while (parentNode != null) {
			parentNode->weight -= 1;
			parentNode->sum -= node->key;
			parentNode = parentNode->parent;
		}

		if (moveNode != null) {
			moveNode->weight = node->weight - 1;
			moveNode->sum = node->sum - node->key;
		}
	}

	void remove_fix_up(RBNode* node)
	{

		RBNode* siblingNode;

		while (node != root && !node->red) {
			if (node == node->parent->left) {
				siblingNode = node->parent->right;
				if (siblingNode->red) {
					siblingNode->red = false;
					node->parent->red = true;
					left_rotate(node->parent);
					siblingNode = node->parent->right;
				}
				if (!siblingNode->left->red && !siblingNode->right->red) {
					siblingNode->red = true;
					node = node->parent;
				}
				else {
					if (!siblingNode->right->red) {
						siblingNode->left->red = false;
						siblingNode->red = true;
						right_rotate(siblingNode);
						siblingNode = node->parent->right;
					}
					siblingNode->red = node->parent->red;
					node->parent->red = false;
					siblingNode->right->red = false;
					left_rotate(node->parent);
					node = root;
				}
			}

			else {
				siblingNode = node->parent->left;
				if (siblingNode->red) {
					siblingNode->red = false;
					node->parent->red = true;
					right_rotate(node->parent);
					siblingNode = node->parent->left;
				}
				if (!siblingNode->left->red && !siblingNode->right->red) {
					siblingNode->red = true;
					node = node->parent;
				}
				else {
					if (!siblingNode->left->red) {
						siblingNode->right->red = false;
						siblingNode->red = false;
						left_rotate(siblingNode);
						siblingNode = node->parent->left;
					}
					siblingNode->red = node->parent->red;
					node->parent->red = false;
					siblingNode->left->red = false;
					right_rotate(node->parent);
					node = root;
				}
			}
			
		}
		node->red = false;
	}

	void transplant(RBNode* destNode, RBNode* srcNode)
	{
		if (destNode->parent == null) 
			root = srcNode;
		else if (destNode == destNode->parent->left) 
			destNode->parent->left = srcNode;
		else 
			destNode->parent->right = srcNode;
		srcNode->parent = destNode->parent;
	}

	RBNode* tree_minimum(RBNode* node)
	{
		while (node->left != null)
			node = node->left;
		return node;
	}
};
