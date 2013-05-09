/* pid_set.c
 *
 * Implementation of the pid_set.h module. */

#include <stdlib.h>
#include "pid_set.h"

/* Type definition for a tree node. */
typedef struct pid_set_node_struct node_t;

/* Type definition for a node color. */
typedef enum pid_set_node_color color_t;

/* Node color data type. */
enum pid_set_node_color {
	RED = 0, BLACK = 1
};

/* Tree node (set entry) data type. */
struct pid_set_node_struct {
	pid_t pid; /* Key value. */
	size_t cluster;
	size_t class;

	color_t color;
	node_t *parent;
	node_t *left;
	node_t *right;
};

/* Set instance data type. */
struct pid_set_struct {
	size_t size;
	node_t *root;
};

/* Helper functions, see definitions below. */
static inline color_t get_color(node_t *node);
static inline node_t *get_sibling(node_t *node);
static node_t *find_node(pid_set_t *set, pid_t pid);
static node_t *get_minimum_node(node_t *root, node_t *node, size_t cluster);
static node_t *create_node(pid_t pid, size_t cluster, size_t class);
static void free_tree_nodes(node_t *root);
static void rotate_left(pid_set_t *set, node_t *node);
static void rotate_right(pid_set_t *set, node_t *node);
static void insert_fix(pid_set_t *set, node_t *node);
static void remove_fix(pid_set_t *set, node_t *node);

/* Creates a new set instance. */
pid_set_t *pid_set_create(void)
{
	/* Allocate a new set. */
	return calloc(1, sizeof(pid_set_t));
}

/* Deallocates set from memory. */
void pid_set_destroy(pid_set_t *set)
{
	free_tree_nodes(set->root);
	free(set);
}

/* Inserts a new entry to the set. The new node is inserted with a simple
 * traversal of the binary search tree. After insertion, a helper function is
 * called to fix any red-black tree property violations. */
int pid_set_insert(pid_set_t *set, pid_t pid, size_t cluster, size_t class)
{
	node_t *node, *next;
	/* If set is empty. */
	if (set->size == 0) {
		node = create_node(pid, cluster, class);
		if (node != NULL) {
			node->color = BLACK;
			set->root = node;
			set->size++;
			return 0;
		} else {
			return -1;
		}
	}
	/* Traverse tree and insert new node. */
	next = set->root;
	while (1) {
		if (pid < next->pid) {
			if (next->left == NULL) {
				node = create_node(pid, cluster, class);
				next->left = node;
				break;
			} else {
				next = next->left;
			}
		} else if (pid > next->pid) {
			if (next->right == NULL) {
				node = create_node(pid, cluster, class);
				next->right = node;
				break;
			} else {
				next = next->right;
			}
		} else {
			/* Set contains an equal entry. */
			return 1;
		}
	}
	if (node != NULL) {
		node->parent = next;
		insert_fix(set, node);
		set->size++;
		return 0;
	} else {
		return -1;
	}
}

/* Removes the matching entry from the set. The node is removed using a
 * standard binary search tree removal and after removal a helper function is
 * called to restore the red-black tree properties if needed. */
int pid_set_remove(pid_set_t *set, pid_t pid)
{
	node_t *node, *pred, *child;
	/* Find node to remove. */
	node = find_node(set, pid);
	if (node == NULL) {
		return -1;
	}
	/* If node has two children, we find the in-order predecessor (max node of
	 * left subtree) of the node and copy the process ID and the cpu from the
	 * predecessor to the node. */
	if (node->left != NULL && node->right != NULL) {
		pred = node->left;
		while (pred->right != NULL) {
			pred = pred->right;
		}
		node->pid = pred->pid;
		node->cluster = pred->cluster;
		node = pred;
	}
	/* Check if the node has a child (at most one here). */
	if (node->left != NULL) {
		child = node->left;
	} else if (node->right != NULL) {
		child = node->right;
	} else {
		child = NULL;
	}
	/* If node is black we may have to adjust the tree to maintain the
	 * red-black tree properties after removal. */
	if (get_color(node) == BLACK) {
		if (get_color(child) == RED) {
			child->color = BLACK;
		} else {
			remove_fix(set, node);
		}
	}
	/* Unlink node from tree substituting it with the child. */
	if (node->parent == NULL) {
		set->root = child;
	} else {
		if (node == node->parent->left) {
			node->parent->left = child;
		} else {
			node->parent->right = child;
		}
	}
	if (child != NULL) {
		child->parent = node->parent;
	}
	set->size--;
	free(node);
	return 0;
}

/* Returns the size of the set. */
size_t pid_set_get_size(pid_set_t *set)
{
	return set->size;
}

/* Returns the cpu set allocation for the process ID. */
int pid_set_get_cluster(pid_set_t *set, pid_t pid)
{
	node_t *node = find_node(set, pid);
	if (node != NULL) {
		return node->cluster;
	} else {
		return -1;
	}
}

/* Sets the cpu set allocation for the process ID. */
int pid_set_set_cluster(pid_set_t *set, pid_t pid, size_t cluster)
{
	node_t *node = find_node(set, pid);
	if (node != NULL) {
		node->cluster = cluster;
	} else {
		return -1;
	}
	return 0;
}

/* Returns the class for the process ID. */
int pid_set_get_class(pid_set_t *set, pid_t pid)
{
	node_t *node = find_node(set, pid);
	if (node != NULL) {
		return node->class;
	} else {
		return -1;
	}
}

/* Sets the class for the process ID. */
int pid_set_set_class(pid_set_t *set, pid_t pid, size_t class)
{
	node_t *node = find_node(set, pid);
	if (node != NULL) {
		node->class = class;
	} else {
		return -1;
	}
	return 0;
}

/* Traverses the entire set looking for the entry with the lowest class value
 * for the cpu set. When found, the process ID is returned, or -1 on failure. */
pid_t pid_set_get_minimum_pid(pid_set_t *set, size_t cluster)
{
	node_t *node;
	if (set->size == 0) {
		return -1;
	} else {
		node = get_minimum_node(set->root, set->root, cluster);
	}
	return node->pid;
}

/* HELPER FUNCTIONS */

/* Returns the color of the node. If the node is NULL, black is returned by
 * convention. */
static inline color_t get_color(node_t *node)
{
	if (node != NULL) {
		return node->color;
	} else {
		return BLACK;
	}
}

/* Returns the sibling of the node. May be NULL if no sibling exist. */
static inline node_t *get_sibling(node_t *node)
{
	if (node == node->parent->left) {
		return node->parent->right;
	} else {
		return node->parent->left;
	}
}

/* Searches the set for an entry matching the process ID. If found, a pointer
 * to the node is returned, otherwise NULL. */
static node_t *find_node(pid_set_t *set, pid_t pid)
{
	/* Traverse tree. Break out if matching entry is found. */
	node_t *node = set->root;
	while (node != NULL) {
		if (pid < node->pid) {
			node = node->left;
		} else if (pid > node->pid) {
			node = node->right;
		} else {
			break;
		}
	}
	return node;
}

/* Recursively traverses all nodes returning the one with the lowest class
 * value (first one found if multiple equal valued nodes exist) for the cpu. */
node_t *get_minimum_node(node_t *root, node_t *node, size_t cluster)
{
	node_t *left, *right;
	if (root == NULL || node->class == 0) {
		return node;
	} else if (root->cluster == cluster && root->class < node->class) {
		left = get_minimum_node(root->left, root, cluster);
		right = get_minimum_node(root->right, root, cluster);
	} else {
		left = get_minimum_node(root->left, node, cluster);
		right = get_minimum_node(root->right, node, cluster);
	}
	return ((left->class) <= (right->class) ? left : right);
}

/* Creates and initializes a new node. */
static node_t *create_node(pid_t pid, size_t cluster, size_t class)
{
	node_t *node = malloc(sizeof(node_t));
	if (node != NULL) {
		node->pid = pid;
		node->cluster = cluster;
		node->class = class;
		node->color = RED; /* New nodes are red by default. */
		node->parent = NULL;
		node->left = NULL;
		node->right = NULL;
	}
	return node;
}

/* Performs a post-order depth-first traversal to recursively deallocate memory
 * for all nodes in the tree rooted at the specified root. */
static void free_tree_nodes(node_t *root)
{
	if (root == NULL) {
		return;
	} else {
		free_tree_nodes(root->left);
		free_tree_nodes(root->right);
		free(root);
	}
}

/* Performs a left tree rotation around the specified node. */
static void rotate_left(pid_set_t *set, node_t *node)
{
	/* Rotate by redirecting appropriate pointers. */
	node_t *new_root = node->right;
	new_root->parent = node->parent;
	if (node->parent == NULL) {
		set->root = new_root;
	} else if (node == node->parent->left) {
		node->parent->left = new_root;
	} else {
		node->parent->right = new_root;
	}
	node->parent = new_root;
	node->right = new_root->left;
	if (new_root->left != NULL) {
		new_root->left->parent = node;
	}
	new_root->left = node;
}

/* Performs a right tree rotation around the specified node. */
static void rotate_right(pid_set_t *set, node_t *node)
{
	/* Rotate by redirecting appropriate pointers. */
	node_t *new_root = node->left;
	new_root->parent = node->parent;
	if (node->parent == NULL) {
		set->root = new_root;
	} else if (node == node->parent->left) {
		node->parent->left = new_root;
	} else {
		node->parent->right = new_root;
	}
	node->parent = new_root;
	node->left = new_root->right;
	if (new_root->right != NULL) {
		new_root->right->parent = node;
	}
	new_root->right = node;
}

/* Fixes any violated red-black tree properties after insertion. */
static void insert_fix(pid_set_t *set, node_t *node)
{
	node_t *uncle, *grandp;
	while (1) {
		/* If inserted node is tree root, just color it black. */
		if (node->parent == NULL) {
			node->color = BLACK;
			return;
		}
		/* If parent is black no tree property is violated by the insertion. */
		if (get_color(node->parent) == BLACK) {
			return;
		}
		grandp = node->parent->parent;
		if (node->parent == grandp->left) {
			uncle = grandp->right;
		} else {
			uncle = grandp->left;
		}
		/* If node has a red uncle, color the uncle and parent black. Then
		 * color the grandparent red and restart the fix operation starting
		 * from grandparent. */
		if (get_color(uncle) == RED) {
			node->parent->color = BLACK;
			uncle->color = BLACK;
			grandp->color = RED;
			node = grandp;
		} else {
			break;
		}
	}
	/* If no uncle exist or if the uncle is black the (sub)tree must be
	 * rotated to maintain the red-black tree properties. The rotation is
	 * performed in two steps where the first step may not be needed
	 * depending on the position of the node.
	 *
	 * First step (may not be needed): */
	if ((node == node->parent->right) && (node->parent == grandp->left)) {
		rotate_left(set, node->parent);
		node = node->left;
	} else if ((node == node->parent->left) && (node->parent == grandp->right)) {
		rotate_right(set, node->parent);
		node = node->right;
	}
	/* Second step (always executed): */
	node->parent->color = BLACK;
	grandp->color = RED;
	if (node == node->parent->left) {
		rotate_right(set, grandp);
	} else {
		rotate_left(set, grandp);
	}
}

/* Fixes any violated red-black tree properties after removal. Don't try this
 * at home... */
static void remove_fix(pid_set_t *set, node_t *node)
{
	node_t *sibling;
	while (1) {
		/* Return if node is the tree root. */
		if (node->parent == NULL) {
			return;
		}
		sibling = get_sibling(node);
		/* If node has a red sibling, inverse colors of sibling and parent,
		 * then rotate around parent. */
		if (get_color(sibling) == RED) {
			sibling->color = BLACK;
			node->parent->color = RED;
			if (node == node->parent->left) {
				rotate_left(set, node->parent);
			} else {
				rotate_right(set, node->parent);
			}
			sibling = get_sibling(node);
		}
		/* If parent, sibling, and siblings children are all black, we repaint
		 * the sibling red and restart operation from parent. */
		if (get_color(node->parent) == BLACK && get_color(sibling) == BLACK
				&& get_color(sibling->left) == BLACK && get_color(
				sibling->right) == BLACK) {
			sibling->color = RED;
			node = node->parent;
		} else {
			break;
		}
	}
	/* If parent is red but sibling and children of sibling are black, repaint
	 * parent black and sibling red. */
	if (get_color(node->parent) == RED && get_color(sibling) == BLACK
			&& get_color(sibling->left) == BLACK && get_color(sibling->right)
			== BLACK) {
		sibling->color = RED;
		node->parent->color = BLACK;
		return;
	}
	/* If get_sibling is black but the color of the sibling children differs,
	 * repaint the sibling red, repaint all sibling children black and then
	 * rotate either right or left around the sibling depending on the initial
	 * colors of the sibling children. */
	if (node == node->parent->left && get_color(sibling->left) == RED
			&& get_color(sibling->right) == BLACK) {
		sibling->color = RED;
		sibling->left->color = BLACK;
		rotate_right(set, sibling);
		sibling = get_sibling(node);
	} else if (node == node->parent->right && get_color(sibling->left) == BLACK
			&& get_color(sibling->right) == RED) {
		sibling->color = RED;
		sibling->right->color = BLACK;
		rotate_left(set, sibling);
		sibling = get_sibling(node);
	}
	/* Finally, the sibling is colored as its parent and the parent is
	 * colored black. Then if node is a left child the right child of the
	 * sibling is colored black and the tree is rotated left around the node
	 * parent. The same action is performed in the mirrored case. */
	sibling->color = get_color(node->parent);
	node->parent->color = BLACK;
	if (node == node->parent->left) {
		sibling->right->color = BLACK;
		rotate_left(set, node->parent);
	} else {
		sibling->left->color = BLACK;
		rotate_right(set, node->parent);
	}
}

/* FOR DEBUG:
 * This code can safely be deleted. */

/* Returns the smallest value of the two integer parameters. */
static inline int min_value(int first, int second)
{
	return (first <= second ? first : second);
}

/* Returns the number of black nodes on the shortest path from the specified
 * root to a descendant node with at most one child. */
static int minumum_black_nodes(node_t *root, int count)
{
	/* Increment count if node is black. */
	if (get_color(root) == BLACK) {
		count++;
	}
	/* Stop recursion if a leaf is found. */
	if (root->left == NULL || root->right == NULL) {
		return count;
	} else {
		return min_value(minumum_black_nodes(root->left, count),
				minumum_black_nodes(root->right, count));
	}
}

/* Performs a recursive pre-order depth-first traversal of the set asserting
 * that nodes on the path do not violate any red-black tree properties. */
static int assert_tree(node_t *root, int black_limit, int black_count)
{
	int assertion;
	/* Reached end of path, return. */
	if (root == NULL) {
		return 0;
	}
	/* Assert node, returning immediately if a violation is found. */
	if (get_color(root) == RED && (get_color(root->left) == RED || get_color(
			root->right) == RED)) {
		return 2;
	} else if (get_color(root) == BLACK) {
		black_count++;
		if (black_count > black_limit) {
			return 3;
		}
	}
	/* No violation found, continue assertion. */
	assertion = assert_tree(root->left, black_limit, black_count);
	if (assertion != 0) {
		return assertion;
	} else {
		return assert_tree(root->right, black_limit, black_count);
	}
}

/* Asserts that the set adheres to all red-black tree properties and returns 0
 * if successful. If a violation is found, the number of the violated property
 * is returned. Asserted properties and their values are:
 * 1: The root is black.
 * 2: Red nodes have only black children.
 * 3: Every path from a node to a descendant leaf contains the same number
 *    of black nodes. */
int pid_set_assert_set(pid_set_t *set)
{
	int black_limit;
	/* Empty tree? */
	if (set->root == NULL) {
		return 0;
	}
	/* Assert property 1. */
	if (get_color(set->root) != BLACK) {
		return 1;
	}
	black_limit = minumum_black_nodes(set->root, 0);
	return assert_tree(set->root, black_limit, 0);
}
