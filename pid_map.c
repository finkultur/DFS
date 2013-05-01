/* pid_map.c
 *
 * Implementation of the pid_map.h module. */

#include "pid_map.h"
#include <stdlib.h>

/* Nasty hack to fix the eclipse-cdt code formatter since it won't handle NULLs
 * properly otherwise. */
#undef NULL
#define NULL 0

/* Typedef for a tree node. */
typedef struct pmap_node_struct node_t;

/* Typedef for the node colour. */
typedef enum pmap_node_colour colour_t;

/* Type to represent the colour of a node. */
enum pmap_node_colour {
	RED = 0, BLACK = 1
};

/* Type of a tree node (map entry). */
struct pmap_node_struct {
	pid_t pid; /* key. */
	int cpu; /* value. */
	colour_t colour;
	node_t *parent;
	node_t *left;
	node_t *right;
};

/* Type of a map instance. */
struct pmap_map_struct {
	size_t size;
	struct pmap_node_struct *root;
};

/* Helper functions, see definitions below. */
static inline colour_t get_colour(node_t *node);
static inline node_t *get_sibling(node_t *node);
static node_t *find_node(pid_map_t *map, pid_t pid);
static void rotate_left(pid_map_t *map, node_t *node);
static void rotate_right(pid_map_t *map, node_t *node);
static void insert_fix(pid_map_t *map, node_t *node);
static void remove_fix(pid_map_t *map, node_t *node);
static void free_tree(node_t *root);

/* Creates a new map instance. */
pid_map_t *pmap_create() {
	pid_map_t *new_map;

	/* Allocate and initialise new tree. */
	new_map = malloc(sizeof(pid_map_t));
	if (new_map != NULL) {
		new_map->size = 0;
		new_map->root = NULL;
	}
	/* Return new instance. */
	return new_map;
}

/* Deallocates map instance from memory. */
void pmap_destroy(pid_map_t *map) {
	free_tree(map->root);
	free(map);
}

/* Inserts a new entry to the map. The new node is inserted with a simple
 * traversal of the binary search tree. After insertion, a helper function is
 * called to fix any red-black tree property violations. */
int pmap_insert(pid_map_t *map, pid_t pid, int cpu) {
	node_t *node, *next;

	/* Create and initialise new node, return on error. */
	node = malloc(sizeof(node_t));
	if (node == NULL) {
		return -1;
	}
	node->pid = pid;
	node->cpu = cpu;
	node->colour = RED; /* New nodes are red by default. */
	node->parent = NULL;
	node->left = NULL;
	node->right = NULL;
	/* Initialise next node pointer. */
	next = map->root;
	/* Just set the tree root node if map is empty. */
	if (next == NULL) {
		map->root = node;
	} else {
		/* Traverse tree and insert new node. */
		while (1) {
			if (pid < next->pid) {
				if (next->left == NULL) {
					next->left = node;
					break;
				} else {
					next = next->left;
				}
			} else if (pid > next->pid) {
				if (next->right == NULL) {
					next->right = node;
					break;
				} else {
					next = next->right;
				}
			} else {
				/* Tree already contains an equal entry, return with error. */
				free(node);
				return 1;
			}
		}
	}
	/* Set parent of new node. */
	node->parent = next;
	/* Fix any violated red-black tree properties and return. */
	insert_fix(map, node);
	/* Increment size. */
	map->size++;
	return 0;
}

/* Removes the entry with the specified PID from the specified map. The node is
 * removed using a standard binary search tree removal. A helper function is
 * called to restore the red-black tree properties if needed. */
int pmap_remove(pid_map_t *map, pid_t pid) {
	node_t *node, *pred, *child;

	/* Find node to remove. */
	node = find_node(map, pid);
	/* Return error if no matching entry is found. */
	if (node == NULL) {
		return -1;
	}
	/* If node has two children, we find the in-order predecessor (max node of
	 * left subtree) of the node and copy the PID and the CPU from the
	 * predecessor to the node. */
	if ((node->left != NULL) && (node->right != NULL)) {
		/* Find predecessor node. */
		pred = node->left;
		while (pred->right != NULL) {
			pred = pred->right;
		}
		/* Copy predecessor data to node. */
		node->pid = pred->pid;
		node->cpu = pred->cpu;
		/* Set new node to remove (predecessor). */
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
	if (get_colour(node) == BLACK) {
		if (get_colour(child) == RED) {
			child->colour = BLACK;
		} else {
			remove_fix(map, node);
		}
	}
	/* Unlink node from tree substituting it with the child. */
	if (node->parent == NULL) {
		map->root = child;
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
	/* Deallocate node. */
	free(node);
	/* Decrement size. */
	map->size--;
	return 0;
}

/* Get CPU allocation for specified PID. */
int pmap_get_cpu(pid_map_t *map, pid_t pid) {
	node_t *node;

	node = find_node(map, pid);
	if (node != NULL) {
		return node->cpu;
	} else {
		return -1;
	}
}

/* Update CPU allocation for the specified PID. */
int pmap_set_cpu(pid_map_t *map, pid_t pid, int cpu) {
	node_t *node;

	node = find_node(map, pid);
	if (node != NULL) {
		node->cpu = cpu;
		return 0;
	} else {
		return -1;
	}
}

/* Get size of specified map. */
size_t pmap_get_size(pid_map_t *map) {
	return map->size;
}

/* HELPER FUNCTIONS */

/* Returns the colour of the specified node. If the node is NULL, the colour
 * black is returned by convention. */
static inline colour_t get_colour(node_t *node) {
	if (node != NULL) {
		return node->colour;
	} else {
		return BLACK;
	}
}

/* Returns the get_sibling of the specified node. May be NULL if no sibling
 * exist. */
static inline node_t *get_sibling(node_t *node) {
	if (node == node->parent->left) {
		return node->parent->right;
	} else {
		return node->parent->left;
	}
}

/* Searches the specified map for an entry matching the specified process ID.
 * If found, a pointer to the entry (node) is returned, otherwise NULL. */
static node_t *find_node(pid_map_t *map, pid_t pid) {
	node_t *node;

	/* Initialise node pointer to tree root. */
	node = map->root;
	/* Traverse tree. Break out if matching entry is found. */
	while (node != NULL) {
		if (pid == node->pid) {
			break;
		} else if (pid < node->pid) {
			node = node->left;
		} else {
			node = node->right;
		}
	}
	/* Return node pointer. */
	return node;
}

/* Performs a left tree rotation around the specified node. */
static void rotate_left(pid_map_t *map, node_t *node) {
	node_t *new_root;

	/* Rotate by redirecting appropriate pointers. */
	new_root = node->right;
	new_root->parent = node->parent;
	if (node->parent == NULL) {
		map->root = new_root;
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
static void rotate_right(pid_map_t *map, node_t *node) {
	node_t *new_root;

	/* Rotate by redirecting appropriate pointers. */
	new_root = node->left;
	new_root->parent = node->parent;
	if (node->parent == NULL) {
		map->root = new_root;
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
static void insert_fix(pid_map_t *map, node_t *node) {
	node_t *uncle, *grandp;

	while (1) {
		/* If inserted node is tree root, just colour it black. */
		if (node->parent == NULL) {
			node->colour = BLACK;
			return;
		}
		/* If parent is black no tree property is violated by the insertion. */
		if (get_colour(node->parent) == BLACK) {
			return;
		}
		/* Get grandparent node. */
		grandp = node->parent->parent;
		/* Get uncle node. */
		if (node->parent == grandp->left) {
			uncle = grandp->right;
		} else {
			uncle = grandp->left;
		}
		/* If node has a red uncle, colour the uncle and parent black. Then
		 * colour the grandparent red and restart the fix operation starting
		 * from grandparent. */
		if (get_colour(uncle) == RED) {
			node->parent->colour = BLACK;
			uncle->colour = BLACK;
			grandp->colour = RED;
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
		rotate_left(map, node->parent);
		node = node->left;
	} else if ((node == node->parent->left)
			&& (node->parent == grandp->right)) {
		rotate_right(map, node->parent);
		node = node->right;
	}
	/* Second step (always executed): */
	node->parent->colour = BLACK;
	grandp->colour = RED;
	if (node == node->parent->left) {
		rotate_right(map, grandp);
	} else {
		rotate_left(map, grandp);
	}
}

/* Fixes any violated red-black tree properties after removal. Don't try this
 * at home... */
static void remove_fix(pid_map_t *map, node_t *node) {
	node_t *sibling;

	while (1) {
		/* Return if node is the tree root. */
		if (node->parent == NULL) {
			return;
		}
		sibling = get_sibling(node);
		/* If node has a red sibling, inverse colours of sibling and parent,
		 * then rotate around parent. */
		if (get_colour(sibling) == RED) {
			sibling->colour = BLACK;
			node->parent->colour = RED;
			if (node == node->parent->left) {
				rotate_left(map, node->parent);
			} else {
				rotate_right(map, node->parent);
			}
			sibling = get_sibling(node);
		}
		/* If parent, sibling, and siblings children are all black, we repaint
		 * the sibling red and restart operation from parent. */
		if (get_colour(node->parent) == BLACK && get_colour(sibling) == BLACK
				&& get_colour(sibling->left) == BLACK
				&& get_colour(sibling->right) == BLACK) {
			sibling->colour = RED;
			node = node->parent;
		} else {
			break;
		}
	}
	/* If parent is red but sibling and children of sibling are black, repaint
	 * parent black and sibling red. */
	if (get_colour(node->parent) == RED && get_colour(sibling) == BLACK
			&& get_colour(sibling->left) == BLACK
			&& get_colour(sibling->right) == BLACK) {
		sibling->colour = RED;
		node->parent->colour = BLACK;
		return;
	}
	/* If get_sibling is black but the colour of the sibling children differs,
	 * repaint the sibling red, repaint all sibling children black and then
	 * rotate either right or left around the sibling depending on the initial
	 * colours of the sibling children. */
	if (node == node->parent->left && get_colour(sibling->left) == RED
			&& get_colour(sibling->right) == BLACK) {
		sibling->colour = RED;
		sibling->left->colour = BLACK;
		rotate_right(map, sibling);
		sibling = get_sibling(node);
	} else if (node == node->parent->right && get_colour(sibling->left) == BLACK
			&& get_colour(sibling->right) == RED) {
		sibling->colour = RED;
		sibling->right->colour = BLACK;
		rotate_left(map, sibling);
		sibling = get_sibling(node);
	}
	/* Finally, the sibling is coloured as its parent and the parent is
	 * coloured black. Then if node is a left child the right child of the
	 * sibling is coloured black and the tree is rotated left around the node
	 * parent. The same action is performed in the mirrored case. */
	sibling->colour = get_colour(node->parent);
	node->parent->colour = BLACK;
	if (node == node->parent->left) {
		sibling->right->colour = BLACK;
		rotate_left(map, node->parent);
	} else {
		sibling->left->colour = BLACK;
		rotate_right(map, node->parent);
	}
}

/* Performs a post-order depth-first traversal to recursively deallocate memory
 * for all nodes rooted at the specified root. */
static void free_tree(node_t *root) {
	if (root == NULL) {
		return;
	} else {
		free_tree(root->left);
		free_tree(root->right);
		free(root);
	}
}

/* FOR DEBUG:
 * This code can safely be deleted. */

/* Returns the smallest value of the two integer parameters. */
static inline int min_value(int first, int second) {
	return (first <= second ? first : second);
}

/* Returns the number of black nodes on the shortest path from the specified
 * root to a descendant leaf. */
static int minumum_black_nodes(node_t *root, int count) {
	/* Increment count if node is black. */
	if (get_colour(root) == BLACK) {
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

/* Performs a recursive pre-order depth-first traversal of the tree asserting
 * that nodes on the path do not violate any red-black tree properties. */
static int assert_tree(node_t *root, int black_limit, int black_count) {
	int assertion;

	/* Reached end of path, return. */
	if (root == NULL) {
		return 0;
	}
	/* Assert node, returning immediately if a violation is found. */
	if (get_colour(root) == RED
			&& (get_colour(root->left) == RED || get_colour(root->right) == RED)) {
		return 2;
	} else if (get_colour(root) == BLACK) {
		black_count++;
		if (black_count > black_limit) {
			return 3;
		}
	}
	assertion = assert_tree(root->left, black_limit, black_count);
	if (assertion != 0) {
		return assertion;
	} else {
		return assert_tree(root->right, black_limit, black_count);
	}
}

/* Asserts that the tree adheres to all reb-black tree properties and returns 0
 * if successful. If a violation is found, the number of the violated property
 * is returned. Asserted properties and their values are:
 * 1: The root is black.
 * 2: Red nodes have only black children.
 * 3: Every path from a node to a descendant leaf contains the same number
 *    of black nodes.
 *  */
int pmap_assert_map_properties(pid_map_t *map) {
	int black_limit;
	/* Empty tree? */
	if (map->root == NULL) {
		return 0;
	}
	/* Assert property 1. */
	if (get_colour(map->root) != BLACK) {
		return 1;
	}
	black_limit = minumum_black_nodes(map->root, 0);
	return assert_tree(map->root, black_limit, 0);
}
