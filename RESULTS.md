# RESULTS

## Context-Switching Approach

For this assignment, I implemented cooperative user-level threads in user space. Each thread gets its own stack from `malloc()`, and each one has a small thread control block that keeps track of its thread ID, state, function, argument, stack, and saved CPU context.

The context switch itself is based on the xv6 style of saving registers, along with a small assembly helper in `user/uthread_switch.S`. When one thread yields, the switch routine saves the current thread's callee-saved registers and stack pointer, loads the next thread's saved stack pointer, restores its registers, and returns to wherever that thread last left off.

New threads start in a small trampoline function. That function calls the thread's assigned routine with its argument, and when the routine finishes, the thread is marked as done and control moves to another runnable thread. Since the scheduler is cooperative, threads only switch when they call `thread_yield()` themselves or when a thread finishes.

The mutex follows the same cooperative idea. If a thread tries to lock a mutex that is already taken, it keeps yielding until the lock becomes available. I also tracked the owner of the lock so only the thread that acquired it can unlock it.

## Limitations

- Maximum thread count: 8 threads total, including the main thread
- Stack size: 4096 bytes per thread
- Scheduling model: cooperative only, with no timer interrupts or preemption
- Address space model: all threads share the same address space and heap
- Join behavior: only basic join semantics are implemented
- Mutex behavior: simple spin-and-yield mutex intended for cooperative scheduling only

## Demonstration Result

The producer/consumer demo in `user/test_pc.c` ran successfully with two producers and one consumer. In the final run, all 200 items were produced and consumed, the buffer finished empty, and the program printed `PASS`. That shows the demo completed without deadlock and without any detected duplicate or invalid values.
