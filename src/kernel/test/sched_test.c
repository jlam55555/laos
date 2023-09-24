#include "sched/sched.h"

#include "common/libc.h"
#include "test/test.h"

struct scheduler scheduler;

void init_fn() {
  for (int i = 0; i < 10; ++i) {
    printf("Thread 2: %d Running in a thread!\n", i);
    sched_task_switch(sched_choose_task(&scheduler));
  }

  sched_task_destroy(scheduler.current_task);
}

DEFINE_TEST(sched, coroutines) {
  sched_init(&scheduler);

  sched_create_task(&scheduler, init_fn);
  sched_bootstrap_task(&scheduler);

  for (int i = 0; i < 10; ++i) {
    printf("Thread 1: %d Running in a thread!\n", i);
    sched_task_switch(sched_choose_task(&scheduler));
  }

  TEST_ASSERT(true);
}
