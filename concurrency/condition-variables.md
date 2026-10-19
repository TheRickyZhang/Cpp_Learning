A condition variable solves the problem of not doing wasted work when a thread is blocked by some condition

Conceptually, a condition variable is just a list of threads. It is always used with a lock, but the lock isn't included within the condition variable because the lock is tied to the data it protects, not the condition, so it is possible for it to be used with multiple condition variables.

Here is the typical structure of usage:
We have producer threads and consumer threads, and shared state is passed as references to both.
The canonical answer is a MPMC queue. The shared state is the queue, ie buffer and head/tail indices.

The producer:
- Locks
- Modifies shared state (pushes item to queue)
- Notifies the condition variable
- Unlocks

(Note that notifying the condition variable often moved outside lock release, since otherwise cv might be notified, but the producer still holds lock)

The consumer:
- Locks
- While the predicate is false, cv.wait. (only proceed if queue is not empty and we hold lock)
- Process item


