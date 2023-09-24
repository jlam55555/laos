#include "common/list.h"

#include <assert.h>
#include <stddef.h>

void list_init(struct list_head *ll) {
  assert(ll);
  ll->next = ll;
  ll->prev = ll;
}

void list_add(struct list_head *ll, struct list_head *node) {
  assert(ll);
  assert(node);
  assert(ll->next);

  node->next = ll->next;
  ll->next = node;
  node->prev = ll;
  node->next->prev = node;
}

void list_add_tail(struct list_head *ll, struct list_head *node) {
  assert(ll);
  assert(node);
  assert(ll->prev);

  node->prev = ll->prev;
  ll->prev = node;
  node->next = ll;
  node->prev->next = node;
}

void list_del(struct list_head *node) {
  assert(node);
  assert(node->prev);
  assert(node->next);
  assert(!list_empty(node));

  node->prev->next = node->next;
  node->next->prev = node->prev;

  // De-initialize the node.
  node->next = node->prev = NULL;
}

bool list_empty(struct list_head *ll) {
  assert(ll);
  return ll->next == ll;
}

size_t list_length(const struct list_head *ll) {
  assert(ll);
  assert(ll->next);

  size_t rval = 0;
  list_foreach_const(ll, _) { ++rval; }
  return rval;
}
