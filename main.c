/**
 * Banker's Algorithm
 * By: Jake Carlson
 * 2017-03-24
 * This program implements Banker's Algorithm in C.
 */

// includes
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>

// constants
#define NUMBER_OF_CUSTOMERS 5       /* total number of customer processes:  n */
#define NUMBER_OF_RESOURCES 3       /* total number of different resources: m */

// global banking arrays
int available[NUMBER_OF_RESOURCES];      /* available amount of each resource */
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];      /* max demand     */
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];   /* current alloc  */
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];         /* remaining need */

// mutex for writing to the global banking arrays
pthread_mutex_t banker_mutex;

// prototypes
void banker_initalization_pipeline(char const *[]);
bool are_valid_args(int, char const *[]);
void init_available(char const *[]);
void init_maximum();
void init_allocation();
void init_need();
void print_state();
void *customer_process(void*);
int request_resources(int customer_num, int request[]);
int release_resources(int customer_num, int request[]);
bool is_safe_state();
bool need_lt_work(int, int[]);
bool all_true(bool[], int);
void print_arr(int[], int);

// main driver
int main(int argc, char const *argv[])
{
    // validate arguments
    if (!are_valid_args(argc, argv))
    {
        fprintf(stderr, "Invalid arguments\n");
        return 1;
    }

    // initalize global banking arrays
    banker_initalization_pipeline(argv);

    // launch customer threads
    pthread_t thread_id[NUMBER_OF_CUSTOMERS];
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        int *arg = malloc(sizeof(*arg));
        *arg = i;
        pthread_create(&thread_id[i], NULL, customer_process, arg);
    }

    // join threads
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        pthread_join(thread_id[i], NULL);
    }

    return 0;
}

// pipeline for initalizing global banking arrays
void banker_initalization_pipeline(char const *argv[])
{
    // initialize global banking mutex
    pthread_mutex_init(&banker_mutex, NULL);

    // initialize array of available resources
    init_available(argv);

    // initialize matrix of maximum requested resouces per customer
    init_maximum();

    // initialize allocation matrix
    init_allocation();

    // initalize need matrix
    init_need();

    // print initial state to console
    print_state();
}

/**
 * checks that there are a correct number of arguments provided and that they
 * are integers
*/
bool are_valid_args(int argc, char const *argv[])
{
    // check size of argv
    if (argc - 1 != NUMBER_OF_RESOURCES)
    {
        printf("%i\n", argc);
        return 0;
    }
    // check individual arguments
    for (int i = 1; i <= NUMBER_OF_RESOURCES; i++)
    {
        if (atoi(argv[i]) == 0)
        {
            return 0;
        }
    }
    return 1;
}

// initializes the global available array to the arguments provided
void init_available(char const *argv[])
{
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        available[i] = atoi(argv[i + 1]);
    }
}

/**
 * initializes the maximum resource use per customer, it is guaranteed that the
 * max resources requested will not exceed the number of available resources
 */
void init_maximum()
{
    // seed with time for random number gen
    srand((unsigned int)time(NULL));

    // loops through customers
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        // loops through resources
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
        {
            // assigns max resource use to a random number moded by
            // resource availability
            maximum[i][j] = rand() % available[j];
        }
    }
}

// initalizes all entries in the allocation matrix to zero
void init_allocation()
{
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
        {
            allocation[i][j] = 0;
        }
    }
}

// calculates the need matrix as need[i,j] = maximum[i,j] - allocation[i,j]
void init_need()
{
    // acquire locks for need, maximum, and allocation

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
        {
            need[i][j] = maximum[i][j] - allocation[i][j];
        }
    }
}

// prints the state the the banking arrays, assumes three resources
void print_state()
{
    printf("\t alloc  \t  max   \t  need  \t avail  \n");
    // print table bars
    for (int i = 0; i < 4; i++)
    {
        printf("\t");
        for (int j = 0; j < (NUMBER_OF_RESOURCES * 2 +
             NUMBER_OF_RESOURCES - 1); j++)
            printf("-");
    }
    printf("\n\t");
    // print resource numbers as column headers
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
            printf("R%i ", j);
        printf("\t");
    }
    printf("\n");
    // print values for each process to the table
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        printf("P%i\t", i);
        print_arr(allocation[i], NUMBER_OF_RESOURCES);
        printf("\t");
        print_arr(maximum[i], NUMBER_OF_RESOURCES);
        printf("\t");
        print_arr(need[i], NUMBER_OF_RESOURCES);

        if (i == 0)
        {
            printf("\t");
            print_arr(available, NUMBER_OF_RESOURCES);
        }
        printf("\n");
    }
}

// contains the logic for a customer process to request and release resources
void *customer_process(void * args)
{
    int customer_num = *((int *) args);
    // seed with time for each thread
    srand((unsigned int)time(NULL));

    bool run = true;
    while (run)
    {
        int request[NUMBER_OF_RESOURCES];

        // determine resources to request based on need vector
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        {
            // request will always be less than need
            if (need[customer_num][i] == 0 || need[customer_num][i] == 1)
                request[i] = need[customer_num][i];
            else
                request[i] = rand() % need[customer_num][i];
        }

        // request resources
        while (request_resources(customer_num, request) == -1)
            ; /* busy wait */

        int release[NUMBER_OF_RESOURCES];

        // determine resources to release based on current allocation
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        {
            // release will always be less than allocation
            if (allocation[customer_num][i] == 0 ||
                allocation[customer_num][i] == 1)
                release[i] = allocation[customer_num][i];
            else
                release[i] = rand() % allocation[customer_num][i];
        }

        // release resources
        while (release_resources(customer_num, release) == -1)
            ; /* busy wait */

        // if the need vector is empty set run to false
        run = false;
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        {
            if (need[customer_num][i] > 0)
                run = true;
        }
        // check to release remaining allocated resouces
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        {
            if (allocation[customer_num][i] > 0)
                run = true;
        }
    }

    // exit thread
    pthread_exit(NULL);
}

/**
 * locks the global banking arrays and determines if a request for resources
 * can be granted
 */
int request_resources(int customer_num, int request[])
{
    // acquire banking mutex
    pthread_mutex_lock(&banker_mutex);

    // check that request < available
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        // if the request cannot be granted the mutex is released and the
        // requesting thread must wait
        if (available[i] < request[i])
        {
            printf("\n[P%i] avail < req: ", customer_num);
            print_arr(request, NUMBER_OF_RESOURCES);
            printf("\n");
            // print_state();
            pthread_mutex_unlock(&banker_mutex);
            return -1;
        }
    }

    // simulate allocation
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        available[i] -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i] -= request[i];
    }

    // if the resulting state is not safe, the previous state is restored and
    // the thread must wait
    if (!is_safe_state())
    {
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        {
            available[i] += request[i];
            allocation[customer_num][i] -= request[i];
            need[customer_num][i] += request[i];
        }
        printf("\n[P%i] state unsafe: ", customer_num);
        print_arr(request, NUMBER_OF_RESOURCES);
        printf("\n");
        // print_state();
        pthread_mutex_unlock(&banker_mutex);
        return -1;
    }
    // otherwise, the transaction is completed
    printf("\n[P%i] request completed: ", customer_num);
    print_arr(request, NUMBER_OF_RESOURCES);
    printf("\n");
    // print_state();
    pthread_mutex_unlock(&banker_mutex);
    return 0;
}

/**
 * locks the global banking arrays and determines if a request to release
 * resources can be granted
 */
int release_resources(int customer_num, int request[])
{
    // acquire banking mutex
    pthread_mutex_lock(&banker_mutex);

    // increment available and decrement allocation by amount of released
    // resources
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        available[i] += request[i];
        allocation[customer_num][i] -= request[i];
    }

    // release banking mutex and return successful
    printf("\n[P%i] release completed: ", customer_num);
    print_arr(request, NUMBER_OF_RESOURCES);
    printf("\n");
    print_state();
    pthread_mutex_unlock(&banker_mutex);
    return 0;
}

/**
 * returns true if a sequence of process executions exists in which all
 * processes can finish, returns false otherwise. Will only be called from a
 * thread holding the banker_mutex
 */
bool is_safe_state()
{
    // safety algorithm data structures
    int work[NUMBER_OF_RESOURCES];
    bool finish[NUMBER_OF_CUSTOMERS];

    // initalize data structures
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        work[i] = available[i];
    }
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        finish[i] = false;
    }

    // loop through all customers
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        // if a job can be completed with the available resources we mark it as
        // finished and add its allocated resources to the resource pool
        if (finish[i] == false && need_lt_work(i, work))
        {
            // update work with allocation for customer
            for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
            {
                work[j] = work[j] + allocation[i][j];
            }
            // mark job as complete
            finish[i] = true;
        }
    }

    // return result of all_true, returns true if all processes can finish
    return all_true(finish, NUMBER_OF_CUSTOMERS);
}

// safety algorithm helper function to determine if need is less then work
bool need_lt_work(int proc_num, int work[])
{
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        if (need[proc_num][i] > work[i])
            return false;
    }
    return true;
}

// ands all booleans in an array to determine if they are all true
bool all_true(bool arr[], int len)
{
    bool result = true;

    for (int i = 0; i < len; i++)
        result = result && arr[i];

    return result;
}

// prints an array of integers of length size
void print_arr(int arr[], int size)
{
    for (int i = 0; i < size; i++)
        printf("%i  ", arr[i]);
}
