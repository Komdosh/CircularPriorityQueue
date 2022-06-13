<b>Circular relaxed concurrent priority queue (CPQ)</b> represents by a set of traditional priority queues
connected into one circular linked list. The main idea is close to [multiqueues](https://github.com/Komdosh/Multiqueues). The priority queue
consists of several traditional priority queues with fine-grained locks. For write operation thread-locks
only one priority queue. Each thread can perform read operation on any structure. It allows to threads
select suitable structure and prepare their operation for faster execution. 

The scheme of CPQ is present on Figure:

<img width="1150" alt="Non-Blocking Circular Priority Queue" src="https://user-images.githubusercontent.com/11743527/173334978-3a7b9ca1-3d75-4b27-a6df-5ae74aa9efef.png">


Related Article:

https://github.com/Komdosh/Publications/blob/master/2020/Computers-orig.pdf
