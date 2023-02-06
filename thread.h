/**
 *
 * @file Defines the public interface for the Thread Library.
 */
#ifndef THREAD_H
#define THREAD_H

/**
 * Error codes for the Thread Library
 */
typedef enum
{
  ERROR_TID_INVALID = -1,
  ERROR_THREAD_BAD = -2,
  ERROR_SYS_THREAD = -3,
  ERROR_SYS_MEM = -4,
  ERROR_OTHER = -5
} ThreadError;

/**
 * The maximum number of threads supported by the library.
 */
#define MAX_THREADS 256

/**
 * The minimum stack size, in bytes, of a thread.
 */
#define THREAD_STACK_SIZE 32768

/**
 * The identifier for a thread. Valid ids are non-negative and less than
 * MAX_THREADS.
 */
typedef int Tid;

/**
 * Initialize the user-level thread library.
 *
 * This must be called before using other functions in this library.
 *
 * @return 0 on success, ERROR_OTHER otherwise.
 */
int
ThreadInit(void);

/**
 * Get the identifier of the calling thread.
 *
 * @return The calling thread's identifier.
 */
Tid
ThreadId(void);

/**
 * Create a new thread that runs the function f with the argument arg.
 *
 * This function may fail if:
 *  - no more threads can be created (ERROR_SYS_THREAD), or
 *  - there is no more memory available (ERROR_SYS_MEM), or
 *  - something unexpected failed (ERROR_OTHER)
 *
 * @param f A pointer to the function that this thread will execute.
 * @param arg The argument passed to f.
 *
 * @return If successful, the new thread's identifier. Otherwise, the
 * appropriate error code.
 */
Tid
ThreadCreate(void (*f)(void*), void* arg);

/**
 * Suspend the calling thread and run the next ready thread. The calling thread
 * will be scheduled again after all *currently* ready threads have run.
 *
 * When the calling thread is the only ready thread, it yields to itself.
 *
 * @return The thread identifier yielded to.
 */
Tid
ThreadYield();

/**
 * Suspend the calling thread and run the thread with identifier tid. The
 * calling thread will be scheduled again after all *currently* ready threads
 * have run.
 *
 * This function may fail if:
 *  - the identifier is invalid (ERROR_TID_INVALID), or
 *  - the identifier is valid but that thread cannot run
 * (ERROR_THREAD_BAD)
 *
 * @param tid The identifier of the thread to run.
 *
 * @return If successful, the identifier yielded to. Otherwise, the appropriate
 * error code.
 */
int
ThreadYieldTo(Tid tid);

/**
 * Exit the calling thread. If the caller is the last thread in the system, then
 * the program exits with exit code 0. If the caller is not the last thread in
 * the system, then the next ready thread will be run.
 *
 * This function may fail when switching to another ready thread. In this case,
 * the program exits with exit code -1.
 *
 * @return This function does not return.
 */
void
ThreadExit();

/**
 * Kill the thread whose identifier is tid.
 *
 * This function may fail if:
 *  - the identifier is invalid (ERROR_TID_INVALID), or
 *  - the identifier is of the calling thread (ERROR_THREAD_BAD), or
 *  - the identifier is valid but not an active thread (ERROR_SYS_THREAD)
 *
 * @param tid The identifier of the thread to kill.
 *
 * @return If successful, the killed thread's identifier. Otherwise, the
 * appropriate error code.
 */
Tid
ThreadKill(Tid tid);

#endif /* THREAD_H */