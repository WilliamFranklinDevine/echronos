/*| headers |*/

/*| object_like_macros |*/
{{#mutexes.length}}
#define MUTEX_ID_NONE ((MutexIdOption) UINT8_MAX)
{{/mutexes.length}}

/*| type_definitions |*/
typedef {{prefix_type}}MutexId MutexIdOption;

/*| structure_definitions |*/

struct mutex {
    TaskIdOption holder;
};
{{#mutex.stats}}
struct mutex_stat {
    uint32_t mutex_lock_counter;
    uint32_t mutex_lock_contended_counter;
    {{prefix_type}}TicksRelative mutex_lock_max_wait_time;
};
{{/mutex.stats}}

/*| extern_definitions |*/

/*| function_definitions |*/

/*| state |*/
{{#mutexes.length}}
static struct mutex mutexes[{{mutexes.length}}] = {
{{#mutexes}}
    {TASK_ID_NONE},
{{/mutexes}}
};
static MutexIdOption waiters[{{tasks.length}}] = {
{{#tasks}}
    MUTEX_ID_NONE,
{{/tasks}}
};
{{#mutex.stats}}
bool {{prefix_func}}mutex_stats_enabled;
static struct mutex_stat mutex_stats[{{mutexes.length}}];
{{/mutex.stats}}
{{/mutexes.length}}

/*| function_like_macros |*/
{{#mutexes.length}}
#define assert_mutex_valid(mutex) api_assert(mutex < {{mutexes.length}}, ERROR_ID_INVALID_ID)
{{/mutexes.length}}

/*| functions |*/
{{#mutexes.length}}
static bool
_mutex_try_lock(const {{prefix_type}}MutexId m)
{
    const bool r = mutexes[m].holder == TASK_ID_NONE;

    precondition_preemption_disabled();

    if (r)
    {
        mutexes[m].holder = get_current_task();
    }

    postcondition_preemption_disabled();

    return r;
}
{{/mutexes.length}}

/*| public_functions |*/
{{#mutexes.length}}
void
{{prefix_func}}mutex_lock(const {{prefix_type}}MutexId m) {{prefix_const}}REENTRANT
{
{{#mutex.stats}}
    bool contended = false;
    const {{prefix_type}}TicksAbsolute wait_start_ticks = {{prefix_func}}timer_current_ticks;

{{/mutex.stats}}
    assert_mutex_valid(m);
    api_assert(mutexes[m].holder != get_current_task(), ERROR_ID_DEADLOCK);

    preempt_disable();

    while (!_mutex_try_lock(m))
    {
{{#mutex.stats}}
        contended = true;
{{/mutex.stats}}
        waiters[get_current_task()] = m;
        mutex_block_on(mutexes[m].holder);
    }

    preempt_enable();
{{#mutex.stats}}

    if ({{prefix_func}}mutex_stats_enabled)
    {
        mutex_stats[m].mutex_lock_counter += 1;
        if (contended)
        {
            {{prefix_type}}TicksRelative wait_time = {{prefix_func}}timer_current_ticks - wait_start_ticks;

            mutex_stats[m].mutex_lock_contended_counter += 1;
            if (wait_time > mutex_stats[m].mutex_lock_max_wait_time)
            {
                mutex_stats[m].mutex_lock_max_wait_time = wait_time;
            }
        }
    }
{{/mutex.stats}}
}

void
{{prefix_func}}mutex_unlock(const {{prefix_type}}MutexId m)
{
    {{prefix_type}}TaskId t;

    assert_mutex_valid(m);
    api_assert(mutexes[m].holder == get_current_task(), ERROR_ID_NOT_HOLDING_MUTEX);

    preempt_disable();

    for (t = {{prefix_const}}TASK_ID_ZERO; t <= {{prefix_const}}TASK_ID_MAX; t++)
    {
        if (waiters[t] == m)
        {
            waiters[t] = MUTEX_ID_NONE;
            mutex_unblock(t);
        }
    }

    mutexes[m].holder = TASK_ID_NONE;

    preempt_enable();
}

bool
{{prefix_func}}mutex_try_lock(const {{prefix_type}}MutexId m)
{
    bool r;

    assert_mutex_valid(m);

    preempt_disable();

    r = _mutex_try_lock(m);

    preempt_enable();

    return r;
}

RtosTaskId
{{prefix_func}}mutex_holder_get(const {{prefix_type}}MutexId m)
{
    assert_mutex_valid(m);
    return mutexes[m].holder;
}

{{#mutex.stats}}
void {{prefix_func}}mutex_stats_clear(void)
{
    /* memset would be preferable, but string.h is not available on all platforms */
    uint8_t mutex_index;
    for (mutex_index = 0; mutex_index < {{mutexes.length}}; mutex_index += 1)
    {
        mutex_stats[mutex_index].mutex_lock_counter = 0;
        mutex_stats[mutex_index].mutex_lock_contended_counter = 0;
        mutex_stats[mutex_index].mutex_lock_max_wait_time = 0;
    }
}
{{/mutex.stats}}
{{/mutexes.length}}