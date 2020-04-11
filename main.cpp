#include <iostream>
#include <x86intrin.h>
#include "macos/cpu_helper.h"
#include "macos/pthread_helper.h"
#include "CircularPriorityQueue.h"

#define MAX_INSERTED_NUM 1000000
//#define INSERT_PER_THREAD 40000000
//#define DELETE_PER_THREAD 2000000
//#define RANDOM_PER_THREAD 4000000
#define INSERT_PER_THREAD 3000000
#define DELETE_PER_THREAD 50000
#define RANDOM_PER_THREAD 500000
#define CPU_FRQ 4.3E9
#define CORES 8
#define METHODS 3*2

using namespace std;

struct threadData {
    int threadId;
};


CircularPriorityQueue<int> *cpq;
long *throughputInsert;
long *throughputDelete;
long *throughputRandom;

long throughputByThread[CORES][METHODS];

boost::lockfree::queue<int> *priorityQueue;

pthread_barrier_t barrier;

void initThroughputByThread() {
    for (int i = 0; i < CORES; ++i) {
        for (int j = 0; j < METHODS; ++j) {
            throughputByThread[i][j] = 0;
        }
    }
}

void printCSVThroughputByThreadsTable() {
    cout << "Thread,InsertTraditional,DeleteTraditional,RandomTraditional,InsertRelaxed,DeleteRelaxed,RandomRelaxed"
         << endl;
    for (int i = 0; i < CORES; ++i) {
        cout << i+1 << ",";
        for (int j = 0; j < METHODS; ++j) {
            cout << throughputByThread[i][j] << ((j + 1 != METHODS) ? "," : "");
        }
        cout << endl;
    }
}

void saveThroughput(const string &type, int threadId, uint64_t start, long numOfElement) {
    double secs = (__rdtsc() - start) / CPU_FRQ;
    auto throughput = static_cast<long>(numOfElement / secs);
    cout << "[" << type << "] " <<  throughput << " throughput per sec" << endl;
    if (type == "DELETE") {
        throughputDelete[threadId] = throughput;
    } else if (type == "INSERT") {
        throughputInsert[threadId] = throughput;
    } else if (type == "RANDOM") {
        throughputRandom[threadId] = throughput;
    }
}

void *RunTraditionalPQExperiment(void *threadarg) {
    struct threadData *threadData;
    threadData = (struct threadData *) threadarg;
    uint64_t start = __rdtsc();

    unsigned int seed = 0;
    for (int i = 0; i < INSERT_PER_THREAD; ++i) {
        int insertedNum = rand_r(&seed) % MAX_INSERTED_NUM;
        priorityQueue->push(insertedNum);
    }
    saveThroughput("INSERT", threadData->threadId, start, INSERT_PER_THREAD);

    int *a;
    start = __rdtsc();
    for (int i = 0; i < DELETE_PER_THREAD; ++i) {
        priorityQueue->pop(a);
    }
    saveThroughput("DELETE", threadData->threadId, start, DELETE_PER_THREAD);

    start = __rdtsc();
    for (int i = 0; i < RANDOM_PER_THREAD; ++i) {
        int mode = rand_r(&seed) % 2;
        if (mode == 0)
            priorityQueue->push(i);
        else
            priorityQueue->pop(a);
    }
    saveThroughput("RANDOM", threadData->threadId, start, RANDOM_PER_THREAD);

    pthread_exit(nullptr);
}

void *RunCPQExperiment(void *threadarg) {
    struct threadData *threadData;
    threadData = (struct threadData *) threadarg;
    uint64_t start = __rdtsc();

    unsigned int seed = 0;
    for (int i = 0; i < INSERT_PER_THREAD; ++i) {
        int insertedNum = rand_r(&seed) % MAX_INSERTED_NUM;
        cpq->push(insertedNum);
    }
    saveThroughput("INSERT", threadData->threadId, start, INSERT_PER_THREAD);
    pthread_barrier_wait (&barrier);
    start = __rdtsc();
    for (int i = 0; i < DELETE_PER_THREAD; ++i) {
        cpq->pop();
    }
    saveThroughput("DELETE", threadData->threadId, start, DELETE_PER_THREAD);
    pthread_barrier_wait (&barrier);
    start = __rdtsc();
    for (int i = 0; i < RANDOM_PER_THREAD; ++i) {
        int mode = rand_r(&seed) % 2;
        if (mode == 0)
            cpq->push(i);
        else
            cpq->pop();
    }
    saveThroughput("RANDOM", threadData->threadId, start, RANDOM_PER_THREAD);

    pthread_exit(nullptr);
}

void runExperiment(const cpu_set_t *cpuset, int numOfThreads, bool useTraditional) {
    pthread_t threads[numOfThreads];
    struct threadData td[numOfThreads];
    throughputDelete = new long[numOfThreads];
    throughputInsert = new long[numOfThreads];
    throughputRandom = new long[numOfThreads];
    void *(*routine)(void *) = useTraditional ? RunTraditionalPQExperiment : RunCPQExperiment;

    for (int i = 0; i < numOfThreads; i++) {
        td[i].threadId = i;

        int rc = pthread_create(&threads[i], nullptr, routine, (void *) &td[i]);

        int s = pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpuset[i % CORES]);
        if (s != 0) {
            printf("Thread %d affinities was not set", i);
            pthread_exit(nullptr);
        }

        if (rc) {
            cout << "Error: thread wasn't created," << rc << endl;
            exit(-1);
        }
    }

    long throughputDeleteSum = 0;
    long throughputInsertSum = 0;
    long throughputRandomSum = 0;

    for (int i = 0; i < numOfThreads; i++) {
        pthread_join(threads[i], nullptr);
        throughputDeleteSum += throughputDelete[i];
        throughputInsertSum += throughputInsert[i];
        throughputRandomSum += throughputRandom[i];
    }

//    cout << "SUM " << (useTraditional ? "TRADITIONAL" : "RELAXED") << " THROUGHPUT PUSH: " << throughputInsertSum
//         << endl;
//    cout << "SUM " << (useTraditional ? "TRADITIONAL" : "RELAXED") << " THROUGHPUT POP: " << throughputDeleteSum
//         << endl;
//    cout << "SUM " << (useTraditional ? "TRADITIONAL" : "RELAXED") << " THROUGHPUT RANDOM: " << throughputRandomSum
//         << endl;

    int relaxedOffset = useTraditional ? 0 : 3;
    throughputByThread[numOfThreads - 1][relaxedOffset] = throughputDeleteSum;
    throughputByThread[numOfThreads - 1][relaxedOffset + 1] = throughputInsertSum;
    throughputByThread[numOfThreads - 1][relaxedOffset + 2] = throughputRandomSum;

    delete throughputDelete;
    delete throughputInsert;
    delete throughputRandom;
}

int main(int argc, char *argv[]) {
    cpu_set_t cpuset[CORES];

    for (int i = 0; i < CORES; i++) {
        CPU_ZERO(&cpuset[i]);
        CPU_SET(i, &cpuset[i]);
    }

    initThroughputByThread();

    int numOfThreads = atoi(argv[1]);


//    for (int threads = 1; threads < numOfThreads + 1; ++threads) {
//        priorityQueue = new boost::lockfree::queue<int>(128);
//        pthread_barrier_init(&barrier, nullptr, threads);
//        runExperiment(cpuset, threads, true);
//        delete priorityQueue;
//    }
//    printCSVThroughputByThreadsTable();

    for (int threads = 1; threads < numOfThreads + 1; ++threads) {
        cpq = new CircularPriorityQueue<int>();
        pthread_barrier_init(&barrier, nullptr, threads);
        runExperiment(cpuset, threads, false);
        delete cpq;
        printCSVThroughputByThreadsTable();
    }

    printCSVThroughputByThreadsTable();
    pthread_exit(nullptr);
}