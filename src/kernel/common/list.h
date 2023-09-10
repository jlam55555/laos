/**
 * Circular doubly linked-list implementation. Based on Linux's `struct
 * list_head`.
 *
 * A linked list comprises a circular chain of doubly-linked `struct
 * list_head`s. Linked lists always have a sentinel node, which is initialized
 * (to point to itself) using `list_init()`. Such a linked list is called empty,
 * even though it has a single (sentinel) node. Other `struct list_head`s can be
 * added to a linked list using `list_add()`/`list_add_tail()`. A `struct
 * list_head` can be removed from its containing linked list using `list_del()`.
 *
 * `list_add()`, `list_add_tail()`, and `list_empty()` can be called with any
 * member of an initialized linked list for its `ll` argument -- it does not
 * have to be the sentinel node.
 *
 * An "initialized linked list" refers to a list which was created by
 * initializing an empty linked list with `list_init()`, followed by any
 * combination of `list_add()`/`list_add_tail()`/`list_del()` (assuming the
 * `list_del()` was never called on an empty list).
 *
 * The `list_entry()` macro is similar to the `list_entry()`/`container_of()`
 * macros in Linux. The `list_foreach()` is provided to simplify iterating over
 * a linked list.
 */

#ifndef COMMON_LIST_H
#define COMMON_LIST_H

#include <stdbool.h>

struct list_head {
  struct list_head *prev;
  struct list_head *next;
};

/**
 * Initialize a singleton linked-list with the sentinel node `ll`.
 */
void list_init(struct list_head *ll);

/**
 * Prepend `node` to `ll`. `ll` must be part of an initialized linked list.
 */
void list_add(struct list_head *ll, struct list_head *node);

/**
 * Append `node` to `ll`. `ll` must be part of an initialized linked list.
 */
void list_add_tail(struct list_head *ll, struct list_head *node);

/**
 * Delete `node` from its containing `ll`. `node` must be part of an initialized
 * linked list.
 */
void list_del(struct list_head *node);

/**
 * Check if a linked list is empty. `ll` must be part of an initialized linked
 * list. Must not be called on an empty linked list.
 */
bool list_empty(struct list_head *ll);

/**
 * Get a pointer to the containing struct of type `type`, in which the `struct
 * list_head` is a member named `field`. Equivalent to the macro of the same
 * name in Linux (see also: `container_of()`).
 */
#define list_entry(ll, type, field)                                            \
  ((type *)((void *)(ll)-offsetof(type, field)))

/**
 * Iterate over a linked list `ll`. For each iteration, the current `struct
 * list_head` is named `it`.
 *
 * Behavior is undefined if `list_del()` is called on elements other than the
 * current element (especially the sentinel/initial node) during traversal.
 * Deletion on the current element is explicitly safe, as a reference to the
 * next node is stored.
 */
#define list_foreach(ll, it)                                                   \
  for (struct list_head * (it) = (ll), *_next = (it)->next;                    \
       (_next = _next->next, (it) = _next->prev) != (ll);)

/**
 * Iterate over a linked list `ll`. For each iteration, the current `struct
 * list_head` is named `it`.
 *
 * This is thes simpler version and does not offer safe deletion of the current
 * element.
 */
#define list_foreach_const(ll, it)                                             \
  for (const struct list_head *(it) = (ll); ((it) = (it)->next) != (ll);)

#endif // COMMON_LIST_H
