// This file requires at least C99 to compile

/**
 * @file   todolist.c
 * @author Jean-Cédric Chappelier <jean-cedric.chappelier@epfl.ch>
 *
 * @copyright EPFL 2020
**/
/**
 * @section DESCRIPTION
 *
 * Template du second homework du cours CS-207, année 2020.
**/

#include <stdio.h>
#include <stdlib.h>

/* ====================================================================== *
 * Structures de données                                                  *
 * ====================================================================== */

/* Définissez ici les types et macros demandés :
 *    task_t,
 *    NO_TASK,
 *    task_print,
 *    queue_node_t,
 *    queue_t,
 *    priority_t,
 *    priority_print,
 * et priority_queue_t.
 */

// 'task_t' is of type void so that a '(task_t*)' is a '(void*)', namely a generic pointer
typedef const void task_t;

#define NO_TASK NULL

typedef void (*task_print)(const task_t *task);

typedef struct queue_node_t
{
    const task_t *elem;
    struct queue_node_t *next;
} queue_node_t;

typedef struct
{
    queue_node_t *first;
    queue_node_t *last;
} queue_t;

// 'priority_t' is an alias of 'size_t' to ensure it is a valid size (unsigned integer)
typedef size_t priority_t;

#define priority_print(p) printf("%zu", p)

typedef struct
{
    priority_t max;
    queue_t *queues;
} priority_queue_t;

// This conditional macro defines the maximum value for a 'size_t' variable
#ifndef SIZE_MAX
#define SIZE_MAX (~(size_t)0)
#endif

/* 
Macro defined in order to threshold maximum possible priority. Arbitrarily set to 600. 
Volontarily not the same as MAX_SIZE, so that we can choose a maximum priority easily, which is not necessarily 
the maximum value a 'size_t' variable can take.
*/
#define MAX_PRIORITY_SIZE 600

/* ====================================================================== *
 * Implémentation de queue_t                                              *
 * ====================================================================== */

queue_t *queue_init(queue_t *q)
{
    if (q != NULL)
    {
        q->first = NULL;
        q->last = NULL;
    }
    else
    {
        fprintf(stderr, "[queue_init] ERROR : Queue can't be initialized, null pointer\n");
    }
    return q;
}

// ----------------------------------------------------------------------
int queue_is_empty(const queue_t *q)
{
    if (q != NULL)
    {
        return (q->first == NULL && q->last == NULL);
    }
    else
    {
        fprintf(stderr, "[queue_is_empty] ERROR : pointer to queue is NULL\n");
        return -1;
    }
}

// ----------------------------------------------------------------------
const task_t *queue_pop(queue_t *q)
{
    if (q != NULL)
    {
        if (queue_is_empty(q))
        {
            // If the queue is empty, 'queue_pop' returns 'NO_TASK' as there is no task in it.
            return NO_TASK;
        }
        else
        {
            queue_node_t *to_pop = q->first;
            task_t *task_to_pop = to_pop->elem;
            q->first = to_pop->next;
            if (q->first == NULL)
            {
                q->last = NULL;
            }
            free(to_pop);
            return task_to_pop;
        }
    }
    else
    {
        fprintf(stderr, "[queue_pop] ERROR : Can't pop queue : pointer to it is 'NULL'\n");
        return NO_TASK;
    }
}

// ----------------------------------------------------------------------
queue_t *queue_push(queue_t *q, const task_t *value)
{
    if (q != NULL)
    {
        if (value != NULL)
        {
            // Instanciate a new node, soon to be added to the queue
            queue_node_t *node = malloc(sizeof(queue_node_t));
            node->elem = value;

            // As it will be at the end of the queue, its 'next' pointer will be NULL
            node->next = NULL;

            if (queue_is_empty(q))
            {
                q->last = node;
                q->first = node;
            }
            else
            {
                q->last->next = node;
                q->last = node;
            }
        }
        else
        {
            fprintf(stderr, "[queue_push] ERROR : Pointer to value is 'NULL'");
        }
    }
    else
    {
        fprintf(stderr, "[queue_push] ERROR : Pointer to queue is 'NULL'");
    }
    return q;
}

// ----------------------------------------------------------------------
void queue_print(const queue_t *q, task_print print)
{
    if (q != NULL)
    {
        if (!queue_is_empty(q))
        {
            queue_node_t *current = q->first;

            // Iterate along the queue as long as no queue element points to NULL, meaning the end of the queue is reached
            while (current != NULL && current->elem != NULL)
            {
                print(current->elem);
                printf(", ");
                // Update current element to its 'next' field, pointing towards next element
                current = current->next;
            }
        }
        else
        {
            printf("<empty queue>");
        }
    }
    else
    {
        fprintf(stderr, "[queue_print] ERROR : Pointer to queue is 'NULL'");
    }
}

// ----------------------------------------------------------------------
void queue_clear(queue_t *q)
{
    if (q != NULL)
    {
        if (!queue_is_empty(q))
        {

            queue_node_t *to_clear = q->first;
            queue_node_t *next_to_clear = to_clear->next;

            // This loops frees all elements until the current element's 'next' pointer points to NULL,
            // meaning it reached the end of the loop
            while (next_to_clear != NULL)
            {
                to_clear->elem = NULL;
                to_clear->next = NULL;
                free(to_clear);
                to_clear = next_to_clear;
                next_to_clear = next_to_clear->next;
            }

            // Deal with the case in which next pointed element is NULL
            if (next_to_clear == NULL)
            {
                // 'last' and 'first' pointers of q are set to NULL and the last element is freed
                q->last = NULL;
                q->first = NULL;
                free(to_clear);
            }
        }
    }
    else
    {
        fprintf(stderr, "[queue_clear] ERROR : Pointer to queue is 'NULL'");
    }
}

/* ====================================================================== *
 * Implémentation de priority_queue_t                                     *
 * ====================================================================== */

// ----------------------------------------------------------------------

/*
pri_queue_enlarge 

This an auxiliary method which aims at setting a higher max priority for the queue, by reallocating the memory
area pointed by pq->queues with the new max priority, and initializing the portion of memory added as empty queues.

Finally, it updates the value of pq->max with the parameter new_max
*/
priority_queue_t *pri_queue_enlarge(priority_queue_t *pq, const priority_t old_max, const priority_t new_max)
{
    if (new_max > old_max)
    {
        // Check for overflow in order to reallocate pq->queues with a new size of new_max + 1 (0 -> new_max)
        if ((new_max + 1 > SIZE_MAX / sizeof(queue_t)) || ((pq->queues = realloc(pq->queues, sizeof(queue_t) * (new_max + 1))) == NULL))
        {
            fprintf(stderr, "[pri_queue_enlarge] ERROR : overflow while reallocating\n");
            return pq;
        }
        // Initializes as queues the memory chunk of size (new_max - old_max - 1) added to the area pointed by pq->queues
        for (size_t i = old_max + 1; i <= new_max; ++i)
        {
            queue_init(&pq->queues[i]);
        }
        // Max priority is now the larger and updated one
        pq->max = new_max;
        return pq;
    }
    else
    {
        fprintf(stderr, "[pri_queue_enlarge] ERROR : new_max isn't bigger than old_max\n");
        return pq;
    }
}

// ----------------------------------------------------------------------

/*
pri_queue_reduce

This an auxiliary method which aims at setting a lower max priority for the queue, by reallocating the memory
area pointed by pq->queues with the new max priority, namely clipping the memory zone formerly used by pq->queues 
(dereferencing the zone between pq->queues[new_max] and pq->queues[pq->max])

Finally, it updates the value of pq->max with the parameter new_max
*/
priority_queue_t *pri_queue_reduce(priority_queue_t *pq, const priority_t new_max)
{
    if (new_max < pq->max)
    {
        // Check for overflow in order to reallocate pq->queues with a new size of
        // new_max + 1 (0 -> new_max)
        // (but should be good since initially bigger)
        if ((new_max + 1 > SIZE_MAX / sizeof(queue_t)) || ((pq->queues = realloc(pq->queues, sizeof(queue_t) * (new_max + 1))) == NULL))
        {
            fprintf(stderr, "[pri_queue_reduce] ERROR : overflow while reallocating\n");
            return pq;
        }
        // Update the value for max priority
        pq->max = new_max;
    }
    else
    {
        fprintf(stderr, "[pri_queue_reduce] ERROR : new_max is not smaller that current max\n");
    }
    return pq;
}

// ----------------------------------------------------------------------
priority_queue_t *pri_queue_init(priority_queue_t *pq)
{
    if (pq != NULL)
    {
        // Initially, max priority is 0 since the priority queue is empty
        pq->max = 0;

        // Initially set just one queue of priority 0
        pq->queues = malloc(sizeof(queue_t));
        queue_init(pq->queues);

        return pq;
    }
    else
    {
        fprintf(stderr, "[pri_queue_init] ERROR : pointer to pq NULL\n");
        return pq;
    }
}

// ----------------------------------------------------------------------
const task_t *pri_queue_pop(priority_queue_t *pq)
{
    if (pq != NULL)
    {
        queue_t *queue_to_pop = &(pq->queues[pq->max]);

        if (!queue_is_empty(queue_to_pop))
        {
            task_t *popped = queue_pop(queue_to_pop);

            // If the queue we want to pop is empty, it means that the max priority has to be updated
            if (queue_is_empty(queue_to_pop))
            {
                size_t new_max = pq->max - 1;

                /* This loops iterates on the priority_queue, starting from max_priority - 1 (we already know that the queue 
                corresponding to max priority is empty), until if it finds a non-empty queue, whose corresponding priority 
                new_max will then be the new max priority.
                */
                while (queue_is_empty(&(pq->queues[new_max])) && new_max > 0)
                {
                    new_max--;
                }

                // Reduce the size of the memory area pointed by pq->queues to be new_max + 1 long (from 0 to new_max)
                pri_queue_reduce(pq, new_max);
            }
            return popped;
        }
        else
        {
            return NO_TASK;
        }
    }
    else
    {
        fprintf(stderr, "[pri_queue_pop] ERROR : pointer to pq NULL\n");
        return pq;
    }
}

// ----------------------------------------------------------------------
int pri_queue_is_empty(const priority_queue_t *pq)
{
    if (pq != NULL)
    {
        /* Following my choice to initialize the priority queue with an empty queue corresponding to priority 0, the priority queue
        is considered to be empty if pq->max is 0 and the queue of priority 0 is empty.
        */
        return (pq->max == 0 && queue_is_empty(&(pq->queues[0])));
    }
    else
    {
        fprintf(stderr, "[pri_queue_pop] ERROR : pointer to pq NULL\n");
        return -1;
    }
}

// ----------------------------------------------------------------------
priority_queue_t *pri_queue_push(priority_queue_t *pq, const task_t *task,
                                 const priority_t priority)
{
    if (pq != NULL)
    {
        // Check if the priority is below the threshold value MAX_PRIORITY_SIZE
        if (task != NULL && priority <= MAX_PRIORITY_SIZE)
        {
            // If priority is larger than pq->max, the size of the priority queue will be augmented and the new maximum priority set
            if (priority > pq->max)
            {
                pq = pri_queue_enlarge(pq, pq->max, priority);
            }

            // General case
            queue_t *current_queue = &(pq->queues[priority]);
            current_queue = queue_push(current_queue, task);
            return pq;
        }
        else if (task == NULL)
        {
            fprintf(stderr, "[pri_queue_push] ERROR : pointer to task NULL\n");
            return pq;
        }
        else
        {
            fprintf(stderr, "[pri_queue_push] ERROR : priority is too big. Did not modify the priority queue\n");
            return pq;
        }
    }
    else
    {
        fprintf(stderr, "[pri_queue_push] ERROR : pointer to pq NULL\n");
        return NULL;
    }
}

// ----------------------------------------------------------------------
// provided
static void print_subqueue(const priority_t p, const queue_t *q, task_print print_f)
{
    priority_print(p);
    fputs(" : ", stdout);
    queue_print(q, print_f);
    putchar('\n');
}

// ----------------------------------------------------------------------
void pri_queue_print(const priority_queue_t *pq, task_print print_func)
{
    if (pq != NULL)
    {
        // Only prints in the case of initialized priority_queue_t
        if (pq->queues != NULL)
        {
            for (size_t i = pq->max; i <= pq->max; i--)
            {
                print_subqueue(i, &(pq->queues[i]), print_func);
            }
        }
    }
    else
    {
        fprintf(stderr, "[pri_queue_print] ERROR : pointer to pq is NULL\n");
    }
}

// ----------------------------------------------------------------------
void pri_queue_clear(priority_queue_t *pq)
{
    if (pq != NULL)
    {
        // Iterates on all queues contained in the priority queue and clears them all individually
        for (size_t i = 0; i <= pq->max; ++i)
        {
            queue_clear(&(pq->queues[i]));
        }

        // Reduces the size of the priority queue to its initial settings, with new_max = 0
        pri_queue_reduce(pq, 0);
    }
    else
    {
        fprintf(stderr, "[pri_queue_clear] ERROR : pointer to pq is NULL\n");
    }
}

// ----------------------------------------------------------------------
void pri_queue_delete(priority_queue_t *pq)
{
    if (pq != NULL)
    {
        pri_queue_clear(pq);

        // Frees the memory area pointed by pq->queues
        free(pq->queues);
        pq->queues = NULL;
    }
    else
    {
        fprintf(stderr, "[pri_queue_delete] ERROR : pointer to pq is NULL\n");
    }
}

/*  ====================================================================== *
 *  ====================================================================== *
 *  Code fourni                                                            *
 *  ====================================================================== *
 *  ====================================================================== */

#define array_size(T) (sizeof(T) / sizeof((T)[0]))
#define min2(x, y) ((x) > (y) ? (y) : (x))
#define push_task(i)    \
    if (nb_tasks > (i)) \
    queue_push(&q, (const char *)tasks + (i)*task_size)

void test(const task_t *tasks, size_t nb_tasks, size_t task_size, task_print print)
{
    /* -------------------------------------------------- *
     * Tests queue_t                                      *
     * -------------------------------------------------- */
    puts("======\nQueue example:");

    queue_t q;
    queue_t *check = queue_init(&q);
    if (check != &q)
        puts("Houlélé !");
    push_task(0);
    push_task(1);
    push_task(2);

    queue_print(&q, print);
    putchar('\n');

    while (!queue_is_empty(&q))
    {
        print(queue_pop(&q));
        puts(" <-- pop");
    }

    push_task(0);
    push_task(1);
    queue_clear(&q);
    puts("---");
    queue_print(&q, print);

    /* -------------------------------------------------- *
     * Tests priority_queue_t                             *
     * -------------------------------------------------- */
    puts("\n-----\nPriority queue example:");

    priority_queue_t pq;
    pri_queue_init(&pq);

    const priority_t priors[] = {
        5, 4, 2, 7, 1, 2, 5, 4, 3, 2, 6, 1, 4, 3};

    const size_t end = min2(nb_tasks, array_size(priors));
    for (size_t i = 0; i < end; ++i)
    {
        pri_queue_push(&pq, (const char *)tasks + i * task_size, priors[i]);
    }
    pri_queue_print(&pq, print);

    while (!pri_queue_is_empty(&pq))
    {
        print(pri_queue_pop(&pq));
        printf(" <-- pop (max p = ");
        priority_print(pq.max);
        puts(")");
    }

    if (nb_tasks > 1)
    {
        pri_queue_push(&pq, tasks, 3);
        pri_queue_push(&pq, (const char *)tasks + task_size, 1);
        pri_queue_push(&pq, (const char *)tasks + task_size, 3);
        puts("---");
        pri_queue_print(&pq, print);
        pri_queue_clear(&pq);
        puts("---");
        pri_queue_print(&pq, print);
    }

    if (nb_tasks > 1)
    {
        pri_queue_push(&pq, tasks, 5);
        pri_queue_push(&pq, (const char *)tasks + task_size, 0);
        pri_queue_push(&pq, (const char *)tasks + task_size, 3);
        puts("---");
        pri_queue_print(&pq, print);
    }

    pri_queue_delete(&pq);
    puts("---");
    pri_queue_print(&pq, print);
}

// ----------------------------------------------------------------------
void print_string(const char **p_s)
{
    if (p_s != NULL)
    {
        fputs(*p_s, stdout);
    }
    else
    {
        fputs("<NULL>", stdout);
    }
}

// ----------------------------------------------------------------------
void print_int(const int *p_i)
{
    if (p_i != NULL)
    {
        printf("%d", *p_i);
    }
    else
    {
        fputs("<NULL>", stdout);
    }
}

#define TEST(T, Func) \
    test(T, array_size(T), sizeof((T)[0]), (task_print)Func)

// ----------------------------------------------------------------------
int main()
{
    const char *const s_tasks[] = {
        "pain", "confiture", "lessive", "beurre", "savon", "sel",
        "noix", "miel"};
    TEST(s_tasks, print_string);

    const int i_tasks[] = {
        111, 112, 113, 114, 115, 116, 117, 118};
    TEST(i_tasks, print_int);

    return 0;
}
