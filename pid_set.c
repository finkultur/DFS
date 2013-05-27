/* pid_set.c
 *
 * Implementation of the pid set module. */

#include <stdlib.h>
#include "pid_set.h"

/* Type definition for a tree entry. */
typedef struct pid_set_entry_struct entry_t;

/* Type definition for an entry color. */
typedef enum pid_set_entry_color color_t;

/* Entry color data type. */
enum pid_set_entry_color {
	RED = 0, BLACK = 1
};

/* Tree entry data type. */
struct pid_set_entry_struct {
	process_t process;

	color_t color;
	entry_t *parent;
	entry_t *left;
	entry_t *right;
};

/* Set instance data type. */
struct pid_set_struct {
	size_t size;
	entry_t *root;
};

/* Helper functions, see definitions below. */
static inline color_t get_color(entry_t *entry);
static inline entry_t *get_sibling(entry_t *entry);
static entry_t *find_entry(pid_set_t *set, pid_t pid);
static entry_t *get_minimum_entry(entry_t *entry, entry_t *min_entry, int node);
static entry_t *create_entry(pid_t pid, int entry, int cpu, int class);
static void free_tree_entries(entry_t *entry);
static void rotate_left(pid_set_t *set, entry_t *entry);
static void rotate_right(pid_set_t *set, entry_t *entry);
static void insert_fix(pid_set_t *set, entry_t *entry);
static void remove_fix(pid_set_t *set, entry_t *entry);

/* Creates a new set instance. */
pid_set_t *pid_set_create(void)
{
	/* Allocate a new set. */
	return calloc(1, sizeof(pid_set_t));
}

/* Deallocates set from memory. */
void pid_set_destroy(pid_set_t *set)
{
	free_tree_entries(set->root);
	free(set);
}

/* Inserts a new entry to the set. The new entry is inserted with a simple
 * traversal of the binary search tree. After insertion, a helper function is
 * called to fix any red-black tree property violations. */
int pid_set_insert(pid_set_t *set, pid_t pid, int node, int cpu, int class)
{
	entry_t *entry, *next;
	/* If set is empty. */
	if (set->size == 0) {
		entry = create_entry(pid, node, cpu, class);
		if (entry != NULL) {
			entry->color = BLACK;
			set->root = entry;
			set->size++;
			return 0;
		} else {
			return -1;
		}
	}
	/* Traverse tree and insert new entry. */
	entry = NULL;
	next = set->root;
	while (1) {
		if (pid < next->process.pid) {
			if (next->left == NULL) {
				entry = create_entry(pid, node, cpu, class);
				next->left = entry;
				break;
			} else {
				next = next->left;
			}
		} else if (pid > next->process.pid) {
			if (next->right == NULL) {
				entry = create_entry(pid, node, cpu, class);
				next->right = entry;
				break;
			} else {
				next = next->right;
			}
		} else {
			/* Set contains an equal entry. */
			return 1;
		}
	}
	if (entry != NULL) {
		entry->parent = next;
		insert_fix(set, entry);
		set->size++;
		return 0;
	} else {
		return -1;
	}
}

/* Removes the matching entry from the set. The entry is removed using a
 * standard binary search tree removal and after removal a helper function is
 * called to restore the red-black tree properties if needed. */
int pid_set_remove(pid_set_t *set, pid_t pid)
{
	entry_t *entry, *pred, *child;
	/* Find entry to remove. */
	entry = find_entry(set, pid);
	if (entry == NULL) {
		return -1;
	}
	/* If entry has two children, we find the in-order predecessor of the entry
	 * and copy the process descriptor. */
	if (entry->left != NULL && entry->right != NULL) {
		pred = entry->left;
		while (pred->right != NULL) {
			pred = pred->right;
		}
		entry->process = pred->process;
		entry = pred;
	}
	/* Check if the entry has a child (at most one here). */
	if (entry->left != NULL) {
		child = entry->left;
	} else if (entry->right != NULL) {
		child = entry->right;
	} else {
		child = NULL;
	}
	/* If entry is black we may have to adjust the tree to maintain the
	 * red-black tree properties after removal. */
	if (get_color(entry) == BLACK) {
		if (get_color(child) == RED) {
			child->color = BLACK;
		} else {
			remove_fix(set, entry);
		}
	}
	/* Unlink entry from tree substituting it with the child. */
	if (entry->parent == NULL) {
		set->root = child;
	} else {
		if (entry == entry->parent->left) {
			entry->parent->left = child;
		} else {
			entry->parent->right = child;
		}
	}
	if (child != NULL) {
		child->parent = entry->parent;
	}
	set->size--;
	free(entry);
	return 0;
}

/* Returns the size of the set. */
size_t pid_set_get_size(pid_set_t *set)
{
	return set->size;
}

/* Returns the process descriptor for the specified process ID. */
process_t *pid_set_get_process(pid_set_t *set, pid_t pid)
{
	entry_t *entry = find_entry(set, pid);
	if (entry != NULL) {
		return &entry->process;
	} else {
		return NULL;
	}
}

/* Returns the process descriptor of the entry with the minumum class for the
 * specified node. */
process_t *pid_set_get_minimum_process(pid_set_t *set, int node)
{
	entry_t *entry = get_minimum_entry(set->root, NULL, node);
	if (entry != NULL) {
		return &entry->process;
	} else {
		return NULL;
	}
}

/* HELPER FUNCTIONS */

/* Returns the color of the entry. If the entry is NULL, black is returned by
 * convention. */
static inline color_t get_color(entry_t *entry)
{
	if (entry != NULL) {
		return entry->color;
	} else {
		return BLACK;
	}
}

/* Returns the sibling of the entry. May be NULL if no sibling exist. */
static inline entry_t *get_sibling(entry_t *entry)
{
	if (entry == entry->parent->left) {
		return entry->parent->right;
	} else {
		return entry->parent->left;
	}
}

/* Searches the set for an entry matching the process ID. If found, a pointer
 * to the entry is returned, otherwise NULL. */
static entry_t *find_entry(pid_set_t *set, pid_t pid)
{
	/* Traverse tree. Break out if matching entry is found. */
	entry_t *entry = set->root;
	while (entry != NULL) {
		if (pid < entry->process.pid) {
			entry = entry->left;
		} else if (pid > entry->process.pid) {
			entry = entry->right;
		} else {
			break;
		}
	}
	return entry;
}

/* Recursively traverses all entries returning the one with the lowest class
 * value for the node. The first one found is returned if multiple equal valued
 * entries exist. */
entry_t *get_minimum_entry(entry_t *entry, entry_t *min_entry, int node)
{
	entry_t *candidate;
	if (entry == NULL) {
		return min_entry;
	} else if ((entry->process.node == node) && (min_entry == NULL)) {
		candidate = get_minimum_entry(entry->left, entry, node);
		candidate = get_minimum_entry(entry->right, candidate, node);
	} else if ((entry->process.node == node)
			&& (entry->process.class < min_entry->process.class)) {
		candidate = get_minimum_entry(entry->left, entry, node);
		candidate = get_minimum_entry(entry->right, candidate, node);
	} else {
		candidate = get_minimum_entry(entry->left, min_entry, node);
		candidate = get_minimum_entry(entry->right, candidate, node);
	}
	return candidate;
}

/* Creates and initializes a new entry. */
static entry_t *create_entry(pid_t pid, int node, int cpu, int class)
{
	entry_t *entry = malloc(sizeof(entry_t));
	if (entry != NULL) {
		entry->process.pid = pid;
		entry->process.node = node;
		entry->process.cpu = cpu;
		entry->process.class = class;
		entry->color = RED; /* New entries are red by default. */
		entry->parent = NULL;
		entry->left = NULL;
		entry->right = NULL;
	}
	return entry;
}

/* Performs a post-order depth-first traversal to recursively deallocate memory
 * for all entries in the tree rooted at the specified entry. */
static void free_tree_entries(entry_t *entry)
{
	if (entry == NULL) {
		return;
	} else {
		free_tree_entries(entry->left);
		free_tree_entries(entry->right);
		free(entry);
	}
}

/* Performs a left tree rotation around the specified entry. */
static void rotate_left(pid_set_t *set, entry_t *entry)
{
	/* Rotate by redirecting appropriate pointers. */
	entry_t *new_root = entry->right;
	new_root->parent = entry->parent;
	if (entry->parent == NULL) {
		set->root = new_root;
	} else if (entry == entry->parent->left) {
		entry->parent->left = new_root;
	} else {
		entry->parent->right = new_root;
	}
	entry->parent = new_root;
	entry->right = new_root->left;
	if (new_root->left != NULL) {
		new_root->left->parent = entry;
	}
	new_root->left = entry;
}

/* Performs a right tree rotation around the specified entry. */
static void rotate_right(pid_set_t *set, entry_t *entry)
{
	/* Rotate by redirecting appropriate pointers. */
	entry_t *new_root = entry->left;
	new_root->parent = entry->parent;
	if (entry->parent == NULL) {
		set->root = new_root;
	} else if (entry == entry->parent->left) {
		entry->parent->left = new_root;
	} else {
		entry->parent->right = new_root;
	}
	entry->parent = new_root;
	entry->left = new_root->right;
	if (new_root->right != NULL) {
		new_root->right->parent = entry;
	}
	new_root->right = entry;
}

/* Fixes any violated red-black tree properties after insertion. */
static void insert_fix(pid_set_t *set, entry_t *entry)
{
	entry_t *uncle, *grandp;
	while (1) {
		/* If inserted entry is tree root, just color it black. */
		if (entry->parent == NULL) {
			entry->color = BLACK;
			return;
		}
		/* If parent is black no tree property is violated by the insertion. */
		if (get_color(entry->parent) == BLACK) {
			return;
		}
		grandp = entry->parent->parent;
		if (entry->parent == grandp->left) {
			uncle = grandp->right;
		} else {
			uncle = grandp->left;
		}
		/* If entry has a red uncle, color the uncle and parent black. Then
		 * color the grandparent red and restart the fix operation starting
		 * from grandparent. */
		if (get_color(uncle) == RED) {
			entry->parent->color = BLACK;
			uncle->color = BLACK;
			grandp->color = RED;
			entry = grandp;
		} else {
			break;
		}
	}
	/* If no uncle exist or if the uncle is black the (sub)tree must be
	 * rotated to maintain the red-black tree properties. The rotation is
	 * performed in two steps where the first step may not be needed
	 * depending on the position of the entry.
	 *
	 * First step (may not be needed): */
	if ((entry == entry->parent->right) && (entry->parent == grandp->left)) {
		rotate_left(set, entry->parent);
		entry = entry->left;
	} else if ((entry == entry->parent->left) && (entry->parent
			== grandp->right)) {
		rotate_right(set, entry->parent);
		entry = entry->right;
	}
	/* Second step (always executed): */
	entry->parent->color = BLACK;
	grandp->color = RED;
	if (entry == entry->parent->left) {
		rotate_right(set, grandp);
	} else {
		rotate_left(set, grandp);
	}
}

/* Fixes any violated red-black tree properties after removal. Don't try this
 * at home... */
static void remove_fix(pid_set_t *set, entry_t *entry)
{
	entry_t *sibling;
	while (1) {
		/* Return if entry is the tree root. */
		if (entry->parent == NULL) {
			return;
		}
		sibling = get_sibling(entry);
		/* If entry has a red sibling, inverse colors of sibling and parent,
		 * then rotate around parent. */
		if (get_color(sibling) == RED) {
			sibling->color = BLACK;
			entry->parent->color = RED;
			if (entry == entry->parent->left) {
				rotate_left(set, entry->parent);
			} else {
				rotate_right(set, entry->parent);
			}
			sibling = get_sibling(entry);
		}
		/* If parent, sibling, and siblings children are all black, we repaint
		 * the sibling red and restart operation from parent. */
		if (get_color(entry->parent) == BLACK && get_color(sibling) == BLACK
				&& get_color(sibling->left) == BLACK && get_color(
				sibling->right) == BLACK) {
			sibling->color = RED;
			entry = entry->parent;
		} else {
			break;
		}
	}
	/* If parent is red but sibling and children of sibling are black, repaint
	 * parent black and sibling red. */
	if (get_color(entry->parent) == RED && get_color(sibling) == BLACK
			&& get_color(sibling->left) == BLACK && get_color(sibling->right)
			== BLACK) {
		sibling->color = RED;
		entry->parent->color = BLACK;
		return;
	}
	/* If get_sibling is black but the color of the sibling children differs,
	 * repaint the sibling red, repaint all sibling children black and then
	 * rotate either right or left around the sibling depending on the initial
	 * colors of the sibling children. */
	if (entry == entry->parent->left && get_color(sibling->left) == RED
			&& get_color(sibling->right) == BLACK) {
		sibling->color = RED;
		sibling->left->color = BLACK;
		rotate_right(set, sibling);
		sibling = get_sibling(entry);
	} else if (entry == entry->parent->right && get_color(sibling->left)
			== BLACK && get_color(sibling->right) == RED) {
		sibling->color = RED;
		sibling->right->color = BLACK;
		rotate_left(set, sibling);
		sibling = get_sibling(entry);
	}
	/* Finally, the sibling is colored as its parent and the parent is
	 * colored black. Then if entry is a left child the right child of the
	 * sibling is colored black then the tree is rotated left around the entry
	 * parent. The same action is performed in the mirrored case. */
	sibling->color = get_color(entry->parent);
	entry->parent->color = BLACK;
	if (entry == entry->parent->left) {
		sibling->right->color = BLACK;
		rotate_left(set, entry->parent);
	} else {
		sibling->left->color = BLACK;
		rotate_right(set, entry->parent);
	}
}
