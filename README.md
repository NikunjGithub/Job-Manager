Job-Manager
===========

Manage number of tasks with boost thread pool


Create a queue of jobs.
A Manager which waits for new JOBS in the queue.
Once a job is received a notification is sent to waiting manager about the same.
Worker maintains a handle to Manager. When all the tasks assigned are complete Manger is informed.
Manager on getting a call for end, stops waiting for new JOBS in queue and exits.
