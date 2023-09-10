#include "common/list.h"

#include "arch/x86_64/pt.h"
#include "test/test.h"

/**
 * Returns the length of the list and verifies that all pointers are correct
 * between items. If the pointers are incorrect, then return -1.
 *
 * This also exercises the `list_foreach_const()` macro.
 */
static signed _list_length(const struct list_head *ll) {
  size_t elems = 0;

  if (!ll || !ll->next || !ll->prev) {
    return -1;
  }

#ifdef DEBUG
  printf("GOT LL   ll=%lx ll->next=%lx ll->prev=%lx\r\n", VM_TO_IDM(ll),
         VM_TO_IDM(ll->next), VM_TO_IDM(ll->prev));
#endif // DEBUG
  list_foreach_const(ll, it) {
#ifdef DEBUG
    printf("GOT ELEM it=%lx it->next=%lx it->prev=%lx\r\n", VM_TO_IDM(it),
           VM_TO_IDM(it->next), VM_TO_IDM(it->prev));
#endif // DEBUG
    if (it->next->prev != it || it->prev->next != it) {
      printf("BAD\r\n");
      return -1;
    }
    ++elems;
  }
#ifdef DEBUG
  printf("DONE elems=%lu\r\n", elems);
#endif // DEBUG
  return elems;
}

static bool _list_invalid(const struct list_head *ll) {
  return _list_length(ll) == -1;
}

DEFINE_TEST(list, init) {
  struct list_head ll;
  list_init(&ll);
  TEST_ASSERT(list_empty(&ll));
  TEST_ASSERT(!_list_length(&ll));
}

DEFINE_TEST(list, add) {
  struct list_head ll, el1, el2, el3;
  list_init(&ll);

  // Test that after each add, the newest element is at the front of the list
  // and the list is valid and of the correct length.
  list_add(&ll, &el1);
  TEST_ASSERT(ll.next == &el1);
  TEST_ASSERT(_list_length(&ll) == 1);
  list_add(&ll, &el2);
  TEST_ASSERT(ll.next == &el2);
  TEST_ASSERT(_list_length(&ll) == 2);
  list_add(&ll, &el3);
  TEST_ASSERT(ll.next == &el3);
  TEST_ASSERT(_list_length(&ll) == 3);

  // Check that the remaining elements in the list are in the correct order.
  TEST_ASSERT(el3.next == &el2);
  TEST_ASSERT(el2.next == &el1);
  TEST_ASSERT(el1.next == &ll);
}

DEFINE_TEST(list, add_tail) {
  struct list_head ll, el1, el2, el3;
  list_init(&ll);

  // Test that after each add, the newest element is at the front of the list
  // and the list is valid and of the correct length.
  list_add_tail(&ll, &el1);
  TEST_ASSERT(ll.prev == &el1);
  TEST_ASSERT(_list_length(&ll) == 1);
  list_add_tail(&ll, &el2);
  TEST_ASSERT(ll.prev == &el2);
  TEST_ASSERT(_list_length(&ll) == 2);
  list_add_tail(&ll, &el3);
  TEST_ASSERT(ll.prev == &el3);
  TEST_ASSERT(_list_length(&ll) == 3);

  // Check that the remaining elements in the list are in the correct order.
  TEST_ASSERT(el3.prev == &el2);
  TEST_ASSERT(el2.prev == &el1);
  TEST_ASSERT(el1.prev == &ll);
}

DEFINE_TEST(list, del) {
  struct list_head ll, el1, el2;
  list_init(&ll);

  list_add(&ll, &el1);
  TEST_ASSERT(_list_length(&ll) == 1);

  list_add(&ll, &el2);
  TEST_ASSERT(_list_length(&ll) == 2);

  list_del(&el1);
  TEST_ASSERT(_list_length(&ll) == 1);

  list_add(&ll, &el1);
  TEST_ASSERT(_list_length(&ll) == 2);

  list_del(&el2);
  TEST_ASSERT(_list_length(&ll) == 1);

  list_del(&el1);
  TEST_ASSERT(list_empty(&ll));
}

DEFINE_TEST(list, entry) {
  // Typical linked-list setup. The sentinel node is stored in some container
  // structure, and the elements have an embedded `struct list_head` member.
  struct container {
    int a, b;
    struct list_head ll_head;
    int c, d;
  } container;
  struct element {
    struct list_head ll;
    int val;
  } element;

  // This is the real test.
  TEST_ASSERT(list_entry(&container.ll_head, struct container, ll_head) ==
              &container);
  TEST_ASSERT(list_entry(&element.ll, struct element, ll) == &element);
}

DEFINE_TEST(list, foreach) {
  struct list_head ll;
  struct element {
    struct list_head ll;
    int val;
  } elems[8];

  list_init(&ll);
  for (unsigned i = 0; i < 8; ++i) {
    elems[i].val = i;
    list_add_tail(&ll, &elems[i].ll);
  }

  TEST_ASSERT(_list_length(&ll) == 8);

  // `list_foreach_const)(` doesn't currently offer an index, so this has to be
  // handled manually.
  int i = 0;
  list_foreach_const(&ll, it) {
    TEST_ASSERT(list_entry(it, struct element, ll)->val == i++);
  }
}

DEFINE_TEST(list, foreach_delete) {
  struct list_head ll;
  struct element {
    struct list_head ll;
    int val;
  } elems[8];

  list_init(&ll);
  for (unsigned i = 0; i < 8; ++i) {
    elems[i].val = i;
    list_add_tail(&ll, &elems[i].ll);
  }

  TEST_ASSERT(_list_length(&ll) == 8);

  list_foreach(&ll, it) {
    // Akin to doing a cleanup task.
    list_entry(it, struct element, ll)->val = 0;
    // Remove element from linked list.
    list_del(it);
  }

  TEST_ASSERT(list_empty(&ll));
  for (unsigned i = 0; i < 8; ++i) {
    TEST_ASSERT(_list_invalid(&elems[i].ll));
    TEST_ASSERT(!elems[i].val);
  }
}

DEFINE_TEST(list, foreach_invalid) {
  struct list_head ll = {.prev = NULL, .next = NULL};
  TEST_ASSERT(_list_invalid(&ll));
}
