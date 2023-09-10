/**
 * Linked-list implementation.
 *
 * TODO(jlam55555): Working on documentation and testing.
 */

#ifndef COMMON_LIST_H
#define COMMON_LIST_H

#include <stdbool.h>

struct list_head {
  struct list_head *prev;
  struct list_head *next;
};

static inline void list_init(struct list_head *ll) {
  ll->next = ll;
  ll->prev = ll;
}

static inline void list_add(struct list_head *ll, struct list_head *node) {
  node->next = ll->next;
  ll->next = node;
  node->prev = ll;
  node->next->prev = node;
}

static inline void list_add_tail(struct list_head *ll, struct list_head *node) {
  node->prev = ll->prev;
  ll->prev = node;
  node->next = ll;
  node->prev->next = node;
}

static inline void list_del(struct list_head *node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
}

static inline bool list_empty(struct list_head *ll) { return ll->next == ll; }

#define list_entry(ll, type, field)                                            \
  ((type *)((void *)(ll)-offsetof(type, field)))

#endif // COMMON_LIST_H
