# Concurrency Primitives
Lock vs Lock-free:

## Lock
Use a mutual exclusion primitive to ensure exclusive access. Simpler correctness.
- Preserve invariant by holding lock
- Block with std::condition_variable
- Issues: deadlock, ordering, priority inversion, convoying (long processes increase average wait)
- Use when: low contention, small critical section

## Lock-free
Built from atomic operations with progress guarantee. Think of smaller scale locks on a fundamental per-thread level.
- Preserve invariant by atomic logic. Often needs extra data.
- Block with atomic::wait() / sleep()
- ABA problem, starvation, hazard pointers for dynamic memory
- Use when: simple enough to reason about. Can have bad tail latency since not wait-free.

# Taxonomy (lower -> higher level)
Mutexes: mutex/shared_mutex
RAII wrappers; operates based in supported interface
lock_guard < unique_lock (both)
             shared_lock (only shared)
condition_variable (sleep/wait mechanism)



std::mutex
- Is the actual mutual exclusive lock. Usually not accessed directly, put in unique_lock or lock_guard

std::shared_mutex
- A bit awkward naming, but means either many readers hold in shared mode, or one writer holds in exclusive mode

std::lock_guard
- Just RAII, holds on to the mutex as long as in scope.

*std::unique_lock, std::condition_variable*
- More flexible version of lock_guard, if you want to defer, check ownership, try locking, etc.
- Mainly use cv.wait(lock, predicate), which is same as (while(!predicate)) { cv.wait(lock); }
- After that, it is guaranteed that predicate = true, and that we hold the lock.
- Prefer unlocking before notifying for performance
https://en.cppreference.com/w/cpp/thread/condition_variable.html
- This example is helpful. We have a worker and main thread; we can "pass" to worker thread by setting ready = true in a lock, then cv.notify_one(). Then, it passes the cv.wait(lock, [](){return ready;}), which does work then "sends back" to main by setting processed = true, lock.unlock(), cv.notify_one(). Main has cv.wait(lock, [](){return processed;}). Finally worker.join() -> merge process back.
- Must use a while loop to ensure condition is satisfied, since it is possible you can have spurious wakeups.


*std::shared_lock*
Differs from unique lock by allowing many threads to own the lock for reading, or one for writing
- Implementation-wise, involves a reader count, a flag for active writer, and waiting writers
- Pros: Use when reads are frequent and having parallel readers helps
- Cons: possible writer starvation and less predictable performance
- Note that the API usage does not protect you against the shared / unique distinction semantics; it is possible to perform mutating actions while holding a read lock, but it would violate the contract. The stdlib provides low-level features, so a safer API would be wrapping the data and lock together

std::counting_semaphore
- Does not follow RAII: Anyone can acquire/release, shared count
- Also has binary_semaphore variant.

std::atomic<>

std::atomic_flag
- Basically atomic<bool>, but guaranteed to be lock free, which is beneficial because being backed by a mutex/OS lock blocks longer and is at risk of deadlock. Does not have load()/store()

