dphil is a solution to the dining philosopher's problem. The solution utilizes a mutex and semaphores in order to prevent more than
one philosopher from being in the critical region at a time. Each philosopher is represented as a seperate process, and they require a fork
on either side of them in order to eat.

In order to compile the program just call make.

To execute the program call ./dphil <N> <KNIVES> <TIME> 
Where:
<N> : number of philosophers
<0/K> : 0 – version (a) of the dining philosophers where obtaining a resources from the shared pool is not necessary in order to eat
            K – the knife version with the problem where K is the number of knives available in the shared knife pool.
<TIME> : the random amount of time for a philosopher to sleep or eat between 0-TIME seconds.

To end the program enter ctrl + z on the keyboard. 
This will terminate the parent and child processes and properly destory the shared memory segment and array of semaphores.
