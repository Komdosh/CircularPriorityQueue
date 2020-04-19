#define BOOST_DISABLE_ASSERTS

#include <boost/assert.hpp>
#include <iostream>
#include <cassert>
#include <x86intrin.h>
#include "macos/cpu_helper.h"
#include "macos/pthread_helper.h"
#include "CircularPriorityQueue.hpp"
#include "boost/lockfree/queue.hpp"

#define MAX_INSERTED_NUM 1000000
#define INSERT_ELEMENTS 20000000
#define DELETE_ELEMENTS 10000000
#define RANDOM_ELEMENTS 4000000
//#define INSERT_ELEMENTS 10000000
//#define DELETE_ELEMENTS 500000
//#define RANDOM_ELEMENTS 1000000
#define CPU_FRQ 4.3E9
#define CORES 8
#define METHODS 3*2
#define REPEATS 10

using namespace std;

struct threadData {
    int threadId;
};
int threadsCount = CORES;

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

void printCSVThroughputByThreadsTable(int repeat) {
    cout << endl;
    cout << "Repeat: " << repeat << endl;
    cout << "Thread,InsertTraditional,DeleteTraditional,RandomTraditional,InsertRelaxed,DeleteRelaxed,RandomRelaxed"
         << endl;
    for (int i = 0; i < CORES; ++i) {
        cout << i + 1 << ",";
        for (int j = 0; j < METHODS; ++j) {
            cout << throughputByThread[i][j] << ((j + 1 != METHODS) ? "," : "");
        }
        cout << endl;
    }
}

void saveThroughput(const string &type, int threadId, uint64_t start, long numOfElement) {
    double secs = (__rdtsc() - start) / CPU_FRQ;
    auto throughput = static_cast<long>(numOfElement / secs);
    cout << "[" << type << "] " << throughput << " throughput per sec" << endl;
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

    unsigned int seed = 0;
    int operationsCount = INSERT_ELEMENTS / threadsCount;

    uint64_t start = __rdtsc();
    for (int i = 0; i < operationsCount; ++i) {
        int insertedNum = rand_r(&seed) % MAX_INSERTED_NUM;
        priorityQueue->push(insertedNum);
    }
    saveThroughput("INSERT", threadData->threadId, start, operationsCount);

    pthread_barrier_wait(&barrier);
    int *a;
    operationsCount = DELETE_ELEMENTS / threadsCount;
    start = __rdtsc();
    for (int i = 0; i < operationsCount; ++i) {
        priorityQueue->pop(a);
    }
    saveThroughput("DELETE", threadData->threadId, start, operationsCount);

    pthread_barrier_wait(&barrier);
    seed = 0;
    operationsCount = RANDOM_ELEMENTS / threadsCount;
    start = __rdtsc();
    for (int i = 0; i < operationsCount; ++i) {
        int mode = rand_r(&seed) % 4;
        if (mode == 0) {
            priorityQueue->pop(a);
        } else {
            int insertedNum = rand_r(&seed) % MAX_INSERTED_NUM;
            priorityQueue->push(insertedNum);
        }
    }
    saveThroughput("RANDOM", threadData->threadId, start, operationsCount);
    pthread_barrier_wait(&barrier);

    pthread_exit(nullptr);
}

void *RunCPQExperiment(void *threadarg) {
    struct threadData *threadData;
    threadData = (struct threadData *) threadarg;


    unsigned int seed = 0;
    int operationsCount = INSERT_ELEMENTS / threadsCount;

    uint64_t start = __rdtsc();
    for (int i = 0; i < operationsCount; ++i) {
        int insertedNum = rand_r(&seed) % MAX_INSERTED_NUM;
        cpq->push(insertedNum);
    }
    saveThroughput("INSERT", threadData->threadId, start, operationsCount);

    pthread_barrier_wait(&barrier);
    operationsCount = DELETE_ELEMENTS / threadsCount;
    start = __rdtsc();
    for (int i = 0; i < operationsCount; ++i) {
        cpq->pop();
    }
    saveThroughput("DELETE", threadData->threadId, start, operationsCount);

    pthread_barrier_wait(&barrier);
    seed = 0;
    operationsCount = RANDOM_ELEMENTS / threadsCount;
    start = __rdtsc();
    for (int i = 0; i < operationsCount; ++i) {
        int mode = rand_r(&seed) % 2;
        if (mode == 0) {
            int insertedNum = rand_r(&seed) % MAX_INSERTED_NUM;
            cpq->push(insertedNum);
        } else {
            cpq->pop();
        }
    }
    saveThroughput("RANDOM", threadData->threadId, start, operationsCount);
    pthread_barrier_wait(&barrier);

    return nullptr;
}

void runExperiment(const cpu_set_t *cpuset, int numOfThreads, int repeat, bool useTraditional) {
    pthread_t threads[numOfThreads];
    struct threadData td[numOfThreads];
    throughputDelete = new long[numOfThreads]{0};
    throughputInsert = new long[numOfThreads]{0};
    throughputRandom = new long[numOfThreads]{0};
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

        cout << "FINISHED" << endl;
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
    throughputByThread[numOfThreads - 1][relaxedOffset] += throughputInsertSum / REPEATS;
    throughputByThread[numOfThreads - 1][relaxedOffset + 1] += throughputDeleteSum / REPEATS;
    throughputByThread[numOfThreads - 1][relaxedOffset + 2] += throughputRandomSum / REPEATS;

//    delete throughputDelete;
//    delete throughputInsert;
//    delete throughputRandom;
}

int main(int argc, char *argv[]) {

    cout
            << "CircularPriorityQueue [numOfMaxCores:8] [skipTraditional:0/1] [startThreads:1..numOfMaxCores]"
            << endl;
    cpu_set_t cpuset[CORES];

    for (int i = 0; i < CORES; i++) {
        CPU_ZERO(&cpuset[i]);
        CPU_SET(i, &cpuset[i]);
    }

    initThroughputByThread();

    int numOfThreads = atoi(argv[1]);
    bool skipTraditional = atoi(argv[2]);
    int startThreads = atoi(argv[3]);

    threadsCount = numOfThreads;
    cout
            << "Run parameters [numOfMaxCores:" << numOfThreads << "] [skipTraditional:" << skipTraditional
            << "] [startThreads:" << startThreads << "]"
            << endl;

    if (skipTraditional == 0) {
        for (int threads = startThreads; threads < numOfThreads + 1; ++threads) {
            for (int repeat = 0; repeat < REPEATS; ++repeat) {
                priorityQueue = new boost::lockfree::queue<int>(128);
                pthread_barrier_init(&barrier, nullptr, threads);
                runExperiment(cpuset, threads, repeat, true);
                pthread_barrier_destroy(&barrier);
                printCSVThroughputByThreadsTable(repeat);
            }
        }
    }


    for (int threads = startThreads; threads < numOfThreads + 1; ++threads) {
        for (int repeat = 0; repeat < REPEATS; ++repeat) {
            cpq = new CircularPriorityQueue<int>();
            pthread_barrier_init(&barrier, nullptr, threads);
            runExperiment(cpuset, threads, repeat, false);
            pthread_barrier_destroy(&barrier);
            printCSVThroughputByThreadsTable(repeat);
        }
    }

    pthread_exit(nullptr);
}