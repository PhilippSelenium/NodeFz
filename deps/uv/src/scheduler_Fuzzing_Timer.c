#include "scheduler_Fuzzing_Timer.h"
#include "scheduler.h"

#include <unistd.h> /* usleep, unlink */
#include <string.h> /* memcpy */
#include <stdlib.h> /* srand, rand */
#include <time.h>   /* time */
#include <assert.h>

static int SCHEDULER_FUZZING_TIMER_MAGIC = 65468789;

/* implDetails for the fuzzing timer scheduler. */

static struct
{
  int magic;

  int mode;
  scheduler_fuzzing_timer_args_t args;
  int delay_range; /* max_delay - min_delay */
} fuzzingTimer_implDetails;

/***********************
 * Private API declarations
 ***********************/

/* Returns non-zero if the scheduler_fuzzing_timer looks valid (e.g. is initialized properly). */
int scheduler_fuzzing_timer__looks_valid (void);

/* Returns the amount of time to sleep in useconds based on fuzzingTimer_implDetails. */
useconds_t scheduler_fuzzing_timer__pick_sleep_time (void);

/***********************
 * Public API definitions
 ***********************/

void
scheduler_fuzzing_timer_init (scheduler_mode_t mode, void *args, schedulerImpl_t *schedulerImpl)
{
  assert(args != NULL);
  assert(schedulerImpl != NULL);
  assert(schedulerImpl != NULL);

  srand(time(NULL));

  /* Populate schedulerImpl. */
  schedulerImpl->register_lcbn = scheduler_fuzzing_timer_register_lcbn;
  schedulerImpl->next_lcbn_type = scheduler_fuzzing_timer_next_lcbn_type;
  schedulerImpl->thread_yield = scheduler_fuzzing_timer_thread_yield;
  schedulerImpl->emit = scheduler_fuzzing_timer_emit;
  schedulerImpl->lcbns_remaining = scheduler_fuzzing_timer_lcbns_remaining;
  schedulerImpl->schedule_has_diverged = scheduler_fuzzing_timer_schedule_has_diverged;

  /* Set implDetails. */
  fuzzingTimer_implDetails.magic = SCHEDULER_FUZZING_TIMER_MAGIC;
  fuzzingTimer_implDetails.mode = mode;
  fuzzingTimer_implDetails.args = *(scheduler_fuzzing_timer_args_t *) args;

  assert(fuzzingTimer_implDetails.args.min_delay <= fuzzingTimer_implDetails.args.max_delay);
  fuzzingTimer_implDetails.delay_range = fuzzingTimer_implDetails.args.max_delay - fuzzingTimer_implDetails.args.min_delay;

  return;
}

void
scheduler_fuzzing_timer_register_lcbn (lcbn_t *lcbn)
{
  assert(scheduler_fuzzing_timer__looks_valid());
  assert(lcbn != NULL && lcbn_looks_valid(lcbn));

  return;
}

enum callback_type
scheduler_fuzzing_timer_next_lcbn_type (void)
{
  assert(scheduler_fuzzing_timer__looks_valid());
  return CALLBACK_TYPE_ANY;
}

void
scheduler_fuzzing_timer_thread_yield (schedule_point_t point, void *pointDetails)
{
  /* SPDs whose inputs/outputs we may need to examine. */
  spd_getting_work_t *spd_getting_work = NULL;
  spd_getting_done_t *spd_getting_done = NULL;

  /* Whether to sleep. */
  int could_sleep = 1;
  int sleep_prob = (rand() % 100);

  assert(scheduler_fuzzing_timer__looks_valid());
  /* Ensure {point, pointDetails} are consistent. Afterwards we know the inputs are correct. */
  assert(schedule_point_looks_valid(point, pointDetails));

  /* Don't sleep at certain schedule points, where doing so merely delays forward progress. */
  if (point == SCHEDULE_POINT_TP_AFTER_PUT_DONE /* This thread is not about to do anything. */
   || point == SCHEDULE_POINT_AFTER_EXEC_CB /* This thread holds the mutex, and has already finished its CB. */
     )
  {
    could_sleep = 0;
  }

  /* Inject sleep. */
  if (could_sleep && sleep_prob < fuzzingTimer_implDetails.args.delay_perc)
  {
    useconds_t sleep_fuzz = scheduler_fuzzing_timer__pick_sleep_time();
    mylog(LOG_SCHEDULER, 1, "scheduler_fuzzing_timer_thread_yield: Sleeping for %llu usec (%s)\n", sleep_fuzz, schedule_point_to_string(point));
    usleep(sleep_fuzz);
  }

  /* Any extra steps required. */

  /* - Supply output for points that want it. */
  switch (point)
  {
    /* As a fuzzing timer scheduler, we simply tell threads to honor the FIFO queue. 
     * Any variation in work and done item execution order must come from the sleeps we inject. */
    case SCHEDULE_POINT_TP_GETTING_WORK:
      assert(scheduler__get_thread_type() == THREAD_TYPE_THREADPOOL);
      spd_getting_work = (spd_getting_work_t *) pointDetails;
      spd_getting_work->index = 0;
      break;
    case SCHEDULE_POINT_LOOPER_GETTING_DONE:
      assert(scheduler__get_thread_type() == THREAD_TYPE_LOOPER);
      spd_getting_done = (spd_getting_done_t *) pointDetails;
      spd_getting_done->index = 0;
      break;
    default:
      /* Nothing to do. */
      break;
  }

}

void
scheduler_fuzzing_timer_emit (char *output_file)
{
  assert(scheduler_fuzzing_timer__looks_valid());
  unlink(output_file);
  return;
}

int
scheduler_fuzzing_timer_lcbns_remaining (void)
{
  assert(scheduler_fuzzing_timer__looks_valid());
  return -1;
}

int
scheduler_fuzzing_timer_schedule_has_diverged (void)
{
  assert(scheduler_fuzzing_timer__looks_valid());
  return -1;
}

/***********************
 * Private API definitions.
 ***********************/

int
scheduler_fuzzing_timer__looks_valid (void)
{
  return (fuzzingTimer_implDetails.magic == SCHEDULER_FUZZING_TIMER_MAGIC);
}

useconds_t
scheduler_fuzzing_timer__pick_sleep_time (void)
{
  useconds_t sleep_fuzz = 0;

  assert(scheduler_fuzzing_timer__looks_valid());

  if (fuzzingTimer_implDetails.args.min_delay == fuzzingTimer_implDetails.args.max_delay)
    sleep_fuzz = fuzzingTimer_implDetails.args.min_delay;
  else
    sleep_fuzz = fuzzingTimer_implDetails.args.min_delay + (rand() % fuzzingTimer_implDetails.delay_range);

  return sleep_fuzz;
}
