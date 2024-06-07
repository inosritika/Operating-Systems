#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

// Define the Process struct
struct Process
{
    int pid;
    float startTime;
    float runTime;
};

// Define a linked list node for Process
struct Process_Node
{
    struct Process process;
    struct Process_Node *next;
};

struct Process_Node *process_head = NULL;

// Function to create a new Process_Node
struct Process_Node *createProcessNode(struct Process process)
{
    struct Process_Node *node = (struct Process_Node *)malloc(sizeof(struct Process_Node));
    if (node != NULL)
    {
        node->process = process;
        node->next = NULL;
    }
    return node;
}

// Function to merge two sorted linked lists
struct Process_Node *merge_start(struct Process_Node *left, struct Process_Node *right)
{
    if (left == NULL)
        return right;
    if (right == NULL)
        return left;

    struct Process_Node *result = NULL;

    if (left->process.startTime <= right->process.startTime)
    {
        result = left;
        result->next = merge_start(left->next, right);
    }
    else
    {
        result = right;
        result->next = merge_start(left, right->next);
    }

    return result;
}

// Function to perform merge sort on the linked list
void mergeSort(struct Process_Node **head)
{
    if (*head == NULL || (*head)->next == NULL)
        return;

    struct Process_Node *slow = *head;
    struct Process_Node *fast = (*head)->next;

    // Split the list into two halves
    while (fast != NULL)
    {
        fast = fast->next;
        if (fast != NULL)
        {
            slow = slow->next;
            fast = fast->next;
        }
    }

    struct Process_Node *left = *head;
    struct Process_Node *right = slow->next;
    slow->next = NULL;

    mergeSort(&left);
    mergeSort(&right);

    *head = merge_start(left, right);
}

// Function to sort a linked list of process nodes based on start time
void sortProcessList(struct Process_Node **head)
{
    mergeSort(head);
}

// Function to print a linked list of process nodes
void printProcessList()
{
    struct Process_Node *current = process_head;
    while (current != NULL)
    {
        printf("PID: %d, Start Time: %f, Run Time: %f\n", current->process.pid, current->process.startTime, current->process.runTime);
        current = current->next;
    }
}

// Function to add a Process_Node to the end of a linked list
void addProcessNodeToEnd(struct Process_Node *node)
{
    if (process_head == NULL)
    {
        process_head = node;
        process_head->next = NULL;
    }
    else
    {
        struct Process_Node *current = process_head;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = node;
        node->next = NULL;
    }

    sortProcessList(&process_head);
}

// Define the CPU struct
struct CPU
{
    int pid;
    float startTime;
    float endTime;
};

// Define a linked list node for CPU
struct CPU_Node
{
    struct CPU cpu;
    struct CPU_Node *next;
};

// Function to create a new CPU_Node
struct CPU_Node *createCPUNode(struct CPU cpu)
{ // correct
    struct CPU_Node *node = (struct CPU_Node *)malloc(sizeof(struct CPU_Node));
    if (node != NULL)
    {
        node->cpu = cpu;
        node->next = NULL;
    }
    return node;
}

// Function to add a CPU_Node to the end of a linked list
void addCPUNodeToEnd(struct CPU_Node **head, struct CPU_Node *node)
{
    if (*head == NULL)
    {
        *head = node;
    }
    else
    {
        struct CPU_Node *current = *head;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = node;
    }
}

struct CPU_Node *create_cpu_head()
{
    // Create a new CPU_Node object
    struct CPU_Node *newCPUNode = (struct CPU_Node *)malloc(sizeof(struct CPU_Node));
    if (newCPUNode != NULL)
    {
        // Set values for the CPU_Node
        newCPUNode->cpu.startTime = 0.0; // Set the start time
        newCPUNode->cpu.endTime = 0.0;   // Set the end time
        newCPUNode->cpu.pid = 0;         // Set the process ID
        newCPUNode->next = NULL;
        return newCPUNode;
    }
    else
    {
        printf("Failed to allocate memory!!\n");
        return NULL;
    }
}

void add_cpu_list(struct Process_Node *process_hd, struct CPU_Node **cpu_head, float time, float end_time)
{
    // Create a new CPU_Node object
    if ((*cpu_head)->next == NULL && (*cpu_head)->cpu.endTime == -1.0)
    {
        (*cpu_head)->cpu.startTime = time;
        (*cpu_head)->cpu.endTime = end_time;
        (*cpu_head)->cpu.pid = process_hd->process.pid;
        (*cpu_head)->next = NULL;
        return;
    }
    struct CPU_Node *newCPUNode = (struct CPU_Node *)malloc(sizeof(struct CPU_Node));
    struct CPU_Node *cpu_head_new = *cpu_head;
    while (cpu_head_new->next != NULL)
    {
        cpu_head_new = cpu_head_new->next;
    }
    if (newCPUNode != NULL)
    {
        newCPUNode->cpu.startTime = time;              // Set the start time
        newCPUNode->cpu.endTime = end_time;            // Set the end time
        newCPUNode->cpu.pid = process_hd->process.pid; // Set the process ID
        newCPUNode->next = NULL;
        cpu_head_new->next = newCPUNode;
        return;
    }
    return;
}

struct Hashmap
{
    int pid;
    struct CPU_Node *cpu_head;
};

// Define a structure for the MinHeapNode
struct MinHeapNode
{
    struct Process_Node *processNode;
    float priority;
};

// Define the MinHeap structure
struct MinHeap
{
    int size;
    int capacity;
    struct MinHeapNode *array;
};

// Function to create a new MinHeapNode
struct MinHeapNode *createMinHeapNode(struct Process_Node *processNode, float priority)
{
    struct MinHeapNode *newNode = (struct MinHeapNode *)malloc(sizeof(struct MinHeapNode));
    if (newNode != NULL)
    {
        newNode->processNode = processNode;
        newNode->priority = priority;
    }
    else
    {
        newNode = createMinHeapNode(processNode, priority);
    }
    return newNode;
}

// Function to create a MinHeap
struct MinHeap *createMinHeap(int capacity)
{
    struct MinHeap *minHeap = (struct MinHeap *)malloc(sizeof(struct MinHeap));
    if (minHeap != NULL)
    {
        minHeap->size = 0;
        minHeap->capacity = capacity;
        minHeap->array = (struct MinHeapNode *)malloc(capacity * sizeof(struct MinHeapNode));
    }
    return minHeap;
}

// Function to swap two MinHeapNode elements
void swapMinHeapNodes(struct MinHeapNode *a, struct MinHeapNode *b)
{
    struct MinHeapNode temp = *a;
    *a = *b;
    *b = temp;
}

// Function to heapify a subtree with the given root
void minHeapify(struct MinHeap *minHeap, int idx)
{
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < minHeap->size && minHeap->array[left].priority < minHeap->array[smallest].priority)
        smallest = left;

    if (right < minHeap->size && minHeap->array[right].priority < minHeap->array[smallest].priority)
        smallest = right;

    if (smallest != idx)
    {
        swapMinHeapNodes(&minHeap->array[smallest], &minHeap->array[idx]);
        minHeapify(minHeap, smallest);
    }
}

// Function to check if the heap is empty
bool isEmpty(struct MinHeap *minHeap)
{
    return (minHeap->size == 0);
}

// Function to insert a new element into the heap
void heapinsert(struct MinHeap *minHeap, struct Process_Node *processNode, float priority)
{
    if (minHeap->size == minHeap->capacity)
    {
        return;
    }

    int i = minHeap->size;
    minHeap->size++;
    minHeap->array[i].processNode = processNode; // check if memory needed
    minHeap->array[i].priority = priority;

    while (i > 0 && minHeap->array[(i - 1) / 2].priority > minHeap->array[i].priority)
    {
        swapMinHeapNodes(&minHeap->array[i], &minHeap->array[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

// Function to extract the element with the highest priority from the heap
struct MinHeapNode extractMin(struct MinHeap *minHeap)
{
    if (isEmpty(minHeap))
    {
        struct MinHeapNode nullNode = {NULL, -1};
        return nullNode;
    }

    if (minHeap->size == 1)
    {
        minHeap->size--;
        return minHeap->array[0];
    }

    struct MinHeapNode root = minHeap->array[0];
    minHeap->array[0] = minHeap->array[minHeap->size - 1];
    minHeap->size--;
    minHeapify(minHeap, 0);

    return root;
}

struct CPU_Node *getlist(struct Hashmap **hashlist, int id)
{ // correct
    for (int i = 0; i < 30000; i++)
    {
        if ((*hashlist)[i].pid == id)
        {
            return (*hashlist)[i].cpu_head;
        }
    }
    return NULL;
}

void add_head(struct Hashmap **schedule, struct Process_Node *process_hd)
{
    for (int i = 0; i < 30000; i++)
    {
        if ((*schedule)[i].cpu_head == NULL)
        {
            struct CPU_Node *newCPUNode = (struct CPU_Node *)malloc(sizeof(struct CPU_Node));
            if (newCPUNode != NULL)
            {
                newCPUNode->cpu.startTime = -1.0;              // Set the start time
                newCPUNode->cpu.endTime = -1.0;                // Set the end time
                newCPUNode->cpu.pid = process_hd->process.pid; // Set the process ID
                newCPUNode->next = NULL;
                (*schedule)[i].cpu_head = newCPUNode;
                (*schedule)[i].pid = process_hd->process.pid;
                return;
            }
        }
    }
    return;
}
void printSchedule(struct Hashmap *schedule)
{
    int size = 30000;
    for (int i = 0; i < size; i++)
    {
        if (schedule[i].cpu_head == NULL)
        {
            return;
        }
        printf("pid printed %d \n", schedule[i].pid);
        printf("cpu pid printed %f \n", schedule[i].cpu_head->cpu.pid);
        printf("starttime printed %f \n", schedule[i].cpu_head->cpu.startTime);
        printf("cpu endtime %f \n", schedule[i].cpu_head->cpu.endTime);
    }
    return;
}
int process_size(struct Process_Node *pr_head)
{
    struct Process_Node *head = pr_head;
    int i = 0;
    while (head != NULL)
    {
        i++;
        head = head->next;
    }
    return i;
}

// Define a Queue_Node struct
struct Queue_Node
{
    struct Process_Node *processNode;
    struct Queue_Node *next;
};

// Define a Queue struct
struct Queue
{
    struct Queue_Node *front;
    struct Queue_Node *rear;
    int size;
};

// Function to create an empty queue
struct Queue *createQueue()
{
    struct Queue *queue = (struct Queue *)malloc(sizeof(struct Queue));
    if (queue != NULL)
    {
        queue->front = queue->rear = NULL;
        queue->size = 0;
    }
    return queue;
}

// Function to check if the queue is empty
int isEmpty_q(struct Queue *queue)
{
    return (queue == NULL || queue->size == 0);
}

// Function to enqueue a Process_Node
void enqueue(struct Queue *queue, struct Process_Node *node)
{
    if (queue == NULL || node == NULL)
    {
        fprintf(stderr, "Invalid queue or node.\n");
        return;
    }

    struct Queue_Node *qNode = (struct Queue_Node *)malloc(sizeof(struct Queue_Node));
    if (qNode == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for Queue_Node.\n");
        return;
    }

    qNode->processNode = node;
    qNode->next = NULL;

    if (isEmpty_q(queue))
    {
        queue->front = queue->rear = qNode;
    }
    else
    {
        queue->rear->next = qNode;
        queue->rear = qNode;
    }

    queue->size++;
}

// Function to dequeue a Process_Node
struct Process_Node *dequeue(struct Queue *queue)
{
    if (isEmpty_q(queue))
    {
        fprintf(stderr, "Queue is empty. Cannot dequeue.\n");
        return NULL;
    }

    struct Queue_Node *tempNode = queue->front;
    queue->front = queue->front->next;
    queue->size--;

    struct Process_Node *processNode = tempNode->processNode;
    free(tempNode);
    return processNode;
}

// Function to get the size of the queue
int getSize(struct Queue *queue)
{
    if (queue != NULL)
    {
        return queue->size;
    }
    return 0;
}

// Function to free the memory of a queue
void freeQueue(struct Queue *queue)
{
    while (!isEmpty_q(queue))
    {
        struct Process_Node *tempNode = dequeue(queue);
        free(tempNode);
    }
    // free(queue);
}

// Define the maximum number of elements in the map
#define MAX_MAP_SIZE 30000

// Define the struct for a key-value pair
struct KeyValuePair
{
    int key;
    int value;
};

// Define the struct for the map
struct IntMap
{
    struct KeyValuePair *data; // Change to a pointer
    int size;
};

// Modify the signature of initializeMap to accept a double pointer
void initializeMap(struct IntMap **map, int initialSize)
{
    // Allocate memory for the IntMap structure
    *map = (struct IntMap *)malloc(sizeof(struct IntMap));

    if (*map == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for the map.\n");
        exit(1); // Handle memory allocation failure as needed
    }

    // Allocate memory for the data array based on initialSize
    (*map)->data = (struct KeyValuePair *)malloc(initialSize * sizeof(struct KeyValuePair));

    if ((*map)->data == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for the map data.\n");
        // free(*map); // Free the allocated map structure
        exit(1);    // Handle memory allocation failure as needed
    }

    (*map)->size = 0; // Initialize size to 0
}

// Insert a key-value pair into the map
void insert(struct IntMap *map, int key, int value)
{
    map->data[map->size].key = key;
    map->data[map->size].value = value;
    map->size++;
}

// Search for a key and return its corresponding value
int search(struct IntMap *map, int key)
{
    for (int i = 0; i < map->size; i++)
    {
        if (map->data[i].key == key)
        {
            return map->data[i].value;
        }
    }
    return -1; // Key not found
}

// Free the memory allocated for the map
void freeMap(struct IntMap *map)
{
    // free(map->data);
}
void addAll(struct Queue *destQueue, struct Queue *srcQueue)
{
    if (destQueue == NULL || srcQueue == NULL)
    {
        fprintf(stderr, "Invalid destination or source queue.\n");
        return;
    }

    while (!isEmpty_q(srcQueue))
    {
        struct Process_Node *node = dequeue(srcQueue);
        enqueue(destQueue, node);
    }
}
struct Process_Node *getfirstque(struct Queue *q)
{
    if (isEmpty_q(q))
    {
        printf("queue is empty\n");
        return NULL;
    }
    return q->front->processNode;
};
// Function to remove an element by key from the map
void removeByKey(struct IntMap *map, int key)
{
    int index = -1; // Initialize index to -1, indicating that the key was not found
    // Find the index of the key
    for (int i = 0; i < map->size; i++)
    {
        if (map->data[i].key == key)
        {
            index = i;
            break; // Key found, break out of the loop
        }
    }
    if (index != -1)
    {
        // Shift elements to the left to remove the element at index
        for (int i = index; i < map->size - 1; i++)
        {
            map->data[i] = map->data[i + 1];
        }
        map->size--; // Decrease the size of the map
    }
}

// schedules start here:
struct Hashmap *scheduleRR(int slice)
{
    struct Hashmap *schedule = (struct Hashmap *)malloc(30000 * sizeof(struct Hashmap));
    for (int i = 0; i < 30000; i++)
    {
        schedule[i].cpu_head = NULL;
        schedule[i].pid = -1;
    }
    struct Queue *queue = createQueue();
    struct Process_Node *process_hd = process_head;
    float time = 0;
    int i = 0;
    while (queue->size > 0 || i < process_size(process_head))
    {
        if (queue->size == 0)
        {
            time = fmax(time, process_hd->process.startTime);
        }
        for (; i < process_size(process_head) && process_hd->process.startTime <= time + slice; i++)
        {
            enqueue(queue, process_hd);
            process_hd = process_hd->next;
        }
        struct Process_Node *p = dequeue(queue);

        time = fmax(time, p->process.startTime);
        struct CPU_Node *cpu_head_check = getlist(&schedule, p->process.pid);
        if (cpu_head_check == NULL)
        {
            add_head(&schedule, p);
        }
        struct CPU_Node *cpu_head_check_new = getlist(&schedule, p->process.pid);
        float t = time + fmin(slice, p->process.runTime);
        add_cpu_list(p, &cpu_head_check_new, time, t);
        time = time + fmin(slice, p->process.runTime);
        if (p->process.runTime > slice)
        {
            struct Process process_new = {p->process.pid, p->process.startTime, p->process.runTime - slice};
            struct Process_Node *new_process_node = createProcessNode(process_new);
            enqueue(queue, new_process_node);
        }
    }
    return schedule;
}

struct Hashmap *scheduleFCFS()
{
    struct Hashmap *schedule = (struct Hashmap *)malloc(30000 * sizeof(struct Hashmap));
    for (int i = 0; i < 30000; i++)
    {
        schedule[i].cpu_head = NULL;
        schedule[i].pid = -1;
    }
    float time = 0;
    struct Process_Node *process_hd = process_head;
    while (process_hd != NULL)
    {
        time = fmax(time, process_hd->process.startTime);
        struct CPU_Node *cpu_head_check = getlist(&schedule, process_hd->process.pid);

        if (cpu_head_check == NULL)
        {
            add_head(&schedule, process_hd);
            cpu_head_check = getlist(&schedule, process_hd->process.pid);
        }
        struct CPU_Node *cpu_head_check_new = getlist(&schedule, process_hd->process.pid);
        add_cpu_list(process_hd, &cpu_head_check_new, time, time + process_hd->process.runTime);
        time += process_hd->process.runTime;
        process_hd = process_hd->next;
    }
    return schedule;
}

struct Hashmap *scheduleSJF()
{
    struct Hashmap *schedule = (struct Hashmap *)malloc(30000 * sizeof(struct Hashmap));
    for (int i = 0; i < 30000; i++)
    {
        schedule[i].cpu_head = NULL;
        schedule[i].pid = 0;
    }
    struct Process_Node *process_hd = process_head;
    struct MinHeap *heap = createMinHeap(30000);
    int i = 0;
    float time = 0;
    while (i < process_size(process_head) || heap->size > 0)
    {
        if (heap->size == 0)
            time = fmax(time, process_hd->process.startTime);
        for (; i < process_size(process_head) && process_hd->process.startTime <= time; i++)
        {
            heapinsert(heap, process_hd, process_hd->process.runTime);
            process_hd = process_hd->next;
        }

        struct MinHeapNode p = extractMin(heap);
        struct Process proc = p.processNode->process;
        time = fmax(time, proc.startTime);
        struct CPU_Node *cpu_head_check = getlist(&schedule, proc.pid);
        if (cpu_head_check == NULL)
        {
            add_head(&schedule, p.processNode);
            cpu_head_check = getlist(&schedule, proc.pid);
        }
        struct CPU_Node *cpu_head_check_new = getlist(&schedule, proc.pid);
        add_cpu_list(p.processNode, &cpu_head_check_new, time, time + proc.runTime);
        time += proc.runTime;
    }

    return schedule;
}

struct Hashmap *scheduleSTCS()
{
    struct Hashmap *schedule = (struct Hashmap *)malloc(30000 * sizeof(struct Hashmap));
    for (int i = 0; i < 30000; i++)
    {
        schedule[i].cpu_head = NULL;
        schedule[i].pid = 0;
    }
    struct MinHeap *heap = createMinHeap(30000);
    struct Process_Node *process_hd = process_head;
    int i = 0;
    float time = 0;
    int count = 0;
    while (i < process_size(process_head) || heap->size > 0)
    {
        if (heap->size == 0)
            time = fmax(time, process_hd->process.startTime);
        for (; process_hd != NULL && i < process_size(process_head) && process_hd->process.startTime <= time; i++)
        {
            heapinsert(heap, process_hd, process_hd->process.runTime);
            process_hd = process_hd->next;
        }
        struct MinHeapNode p = extractMin(heap);
        struct Process proc = p.processNode->process;
        for (; process_hd != NULL && (i < process_size(process_head)) && (process_hd->process.startTime < (time + proc.runTime)) && ((process_hd->process.runTime + process_hd->process.startTime) >= (time + proc.runTime)); i++)
        {
            heapinsert(heap, process_hd, process_hd->process.runTime);
            process_hd = process_hd->next;
        }
        struct CPU_Node *cpu_head_check = getlist(&schedule, proc.pid);
        if (cpu_head_check == NULL)
        {
            add_head(&schedule, p.processNode);
            cpu_head_check = getlist(&schedule, proc.pid);
        }
        if (process_hd != NULL && i < process_size(process_head) && process_hd->process.startTime < time + proc.runTime)
        {
            struct Process_Node *temp = process_hd;
            process_hd = process_hd->next;
            i++;
            struct CPU_Node *cpu_head_check_new = getlist(&schedule, proc.pid);
            add_cpu_list(p.processNode, &cpu_head_check_new, time, temp->process.startTime);
            heapinsert(heap, temp, temp->process.runTime);
            struct Process_Node *new_process = (struct Process_Node *)malloc(sizeof(struct Process_Node));
            new_process->process.pid = proc.pid;
            new_process->process.runTime = time + proc.runTime - temp->process.startTime;
            new_process->process.startTime = temp->process.startTime;
            new_process->next = NULL;
            heapinsert(heap, new_process, new_process->process.runTime); // remaining time priority
            time = temp->process.startTime;
        }
        else
        {
            time = fmax(time, proc.startTime);
            struct CPU_Node *cpu_head_check_new = getlist(&schedule, proc.pid);
            add_cpu_list(p.processNode, &cpu_head_check_new, time, time + proc.runTime);
            time = time + proc.runTime;
        }
        count++;
    }
    return schedule;
};

struct Hashmap *scheduleMLFQ(int slice1, int slice2, int slice3, int boost)
{
    struct Hashmap *schedule = (struct Hashmap *)malloc(500 * sizeof(struct Hashmap));
    for (int i = 0; i < 500; i++)
    {
        schedule[i].cpu_head = NULL;
        schedule[i].pid = 0;
    }
    int boost_real = boost;
    struct IntMap *sl2;
    initializeMap(&sl2, 500); // Start with an initial size of 500
    struct IntMap *sl3;
    initializeMap(&sl3, 500); // Start with an initial size of 500
    struct Process_Node *process_hd = process_head;
    struct Queue *q1 = createQueue();
    struct Queue *q2 = createQueue();
    struct Queue *q3 = createQueue();
    int i = 0, time = 0;
    while (i < process_size(process_head) || q1->size > 0 || q2->size > 0 || q3->size > 0)
    {
        if (process_hd != NULL && q1->size == 0 && q2->size == 0 && q3->size == 0)
        {
            time == fmax(time, process_hd->process.startTime);
            enqueue(q1, process_hd);
            i++;
            process_hd = process_hd->next;
        }
        if (time >= boost)
        {
            addAll(q1, q2);
            freeQueue(q2);
            addAll(q1, q3);
            freeQueue(q3);
            boost += boost_real; //make it boost+=bt;
        }
        while (q1->size > 0)
        {
            struct Process_Node *p_node = dequeue(q1);
            struct Process p = p_node->process;
            time = fmax(time, p.startTime);
            for (; process_hd != NULL && i < process_size(process_head) && process_hd->process.startTime <= time + slice1; i++)
            {
                enqueue(q1, process_hd);
                process_hd = process_hd->next;
            }
            struct CPU_Node *cpu_head_check = getlist(&schedule, p.pid);
            if (cpu_head_check == NULL)
            {
                add_head(&schedule, p_node); // for first time adding cpu node
            }
            if (time + fmin(slice1, p.runTime) > boost)
            {
                struct CPU_Node *cpu_head_check_new = getlist(&schedule, p.pid);
                add_cpu_list(p_node, &cpu_head_check_new, time, boost);
                enqueue(q1, createProcessNode((struct Process){p.pid, time + p.runTime - boost, boost}));
                addAll(q1, q2);
                freeQueue(q2);
                addAll(q1, q3);
                freeQueue(q3);
                time = boost;
                boost += boost_real;
            }
            else
            {
                struct CPU_Node *cpu_head_check_new = getlist(&schedule, p.pid);
                add_cpu_list(p_node, &cpu_head_check_new, time, time + fmin(slice1, p.runTime));
                time = time + fmin(slice1, p.runTime);
                if (p.runTime > slice1)
                {
                    struct Process process = {p.pid, p.runTime - slice1, time};
                    struct Process_Node *proc_node = createProcessNode(process);
                    enqueue(q2, proc_node);
                    dequeue(q1);
                }
            }
        }
        while (q2->size > 0 && q1->size == 0)
        {
            struct Process_Node *p = dequeue(q2);
            if (search(sl2, p->process.pid) == -1)
            {
                insert(sl2, p->process.pid, slice2);
            }
            time = fmax(time, p->process.startTime);
            if (i < process_size(process_head) && process_hd->process.startTime < time + fmin(p->process.runTime, search(sl2, p->process.pid)))
            {
                enqueue(q1, process_hd);
                i++;
                process_hd = process_hd->next;
            }

            if (q1->size == 0)
            {
                if (time + fmin(search(sl2, p->process.pid), p->process.runTime) > boost)
                {
                    struct CPU_Node *cpu_head_check_new = getlist(&schedule, p->process.pid);
                    add_cpu_list(p, &cpu_head_check_new, time, boost);
                    enqueue(q2, createProcessNode((struct Process){p->process.pid, time + p->process.runTime - boost, boost}));
                    addAll(q1, q2);
                    freeQueue(q2);
                    addAll(q1, q3);
                    freeQueue(q3);
                    freeMap(sl2);
                    time = boost;
                    boost += boost_real;
                }
                else
                {
                    struct CPU_Node *cpu_head_check_new = getlist(&schedule, p->process.pid);
                    add_cpu_list(p, &cpu_head_check_new, time, time + fmin(search(sl2, p->process.pid), p->process.runTime));
                    time = time + fmin(search(sl2, p->process.pid), p->process.runTime);

                    if (p->process.runTime > search(sl2, p->process.pid))
                    {
                        enqueue(q3, createProcessNode((struct Process){p->process.pid, p->process.runTime - search(sl2, p->process.pid), time}));
                        removeByKey(sl2, p->process.pid);
                    }
                }
            }
            else
            {
                struct Process_Node *t = getfirstque(q1);
                if (boost < t->process.startTime)
                {
                    struct CPU_Node *cpu_head_check_new = getlist(&schedule, p->process.pid);
                    add_cpu_list(p, &cpu_head_check_new, time, boost);
                    enqueue(q2, createProcessNode((struct Process){p->process.pid, time + p->process.runTime - boost, boost}));
                    addAll(q2, q3);
                    freeQueue(q3);
                    addAll(q2, q1);
                    freeQueue(q1);
                    addAll(q1, q2);
                    freeQueue(q2);
                    freeMap(sl2);
                    time = boost;
                    boost += boost_real;
                }
                else
                {
                    struct CPU_Node *cpu_head_check_new = getlist(&schedule, p->process.pid);
                    add_cpu_list(p, &cpu_head_check_new, time, t->process.startTime);
                    enqueue(q2, createProcessNode((struct Process){p->process.pid, time + p->process.runTime - t->process.startTime, t->process.startTime}));
                    insert(sl2, p->process.pid, search(sl2, p->process.pid) - t->process.startTime + time);
                }
            }
        }
        while (q3->size > 0 && q1->size == 0 && q2->size == 0)
        {
            struct Process_Node *p = dequeue(q3);
            if (search(sl3, p->process.pid) == -1)
                insert(sl3, p->process.pid, slice3);
            time = fmax(time, p->process.startTime);
            if (i < process_size(process_head) && process_hd->process.startTime <= time + fmin(search(sl3, p->process.pid), p->process.runTime))
            {
                enqueue(q1, process_hd);
                process_hd = process_hd->next;
                i++;
            }

            if (q1->size == 0)
            {
                if (time + fmin(search(sl3, p->process.pid), p->process.runTime) > boost)
                {
                    struct CPU_Node *cpu_head_check_new = getlist(&schedule, p->process.pid);
                    add_cpu_list(p, &cpu_head_check_new, time, boost);
                    enqueue(q3, createProcessNode((struct Process){p->process.pid, time + p->process.runTime - boost, boost}));
                    addAll(q1, q2);
                    freeQueue(q2);
                    addAll(q1, q3);
                    freeQueue(q3);
                    freeMap(sl3);
                    time = boost;
                    boost += boost_real;
                }
                else
                {
                    struct CPU_Node *cpu_head_check_new = getlist(&schedule, p->process.pid);
                    add_cpu_list(p, &cpu_head_check_new, time, time + fmin(search(sl3, p->process.pid), p->process.runTime));
                    time = time + fmin(search(sl3, p->process.pid), p->process.runTime);
                    if (p->process.runTime > search(sl3, p->process.pid))
                    {
                        enqueue(q3, createProcessNode((struct Process){p->process.pid, p->process.runTime - search(sl3, p->process.pid), time}));
                    }
                    removeByKey(sl3, p->process.pid);
                }
            }
            else
            {
                struct Process_Node *t = getfirstque(q1);
                if (boost < t->process.startTime)
                {
                    struct CPU_Node *cpu_head_check_new = getlist(&schedule, p->process.pid);
                    add_cpu_list(p, &cpu_head_check_new, time, boost);
                    enqueue(q3, createProcessNode((struct Process){p->process.pid, time + p->process.runTime - boost, boost}));
                    addAll(q2, q3);
                    freeQueue(q3);
                    addAll(q2, q1);
                    freeQueue(q1);
                    addAll(q1, q2);
                    freeQueue(q2);
                    freeMap(sl3);
                    time = boost;
                    boost += boost_real;
                }
                else
                {
                    struct CPU_Node *cpu_head_check_new = getlist(&schedule, p->process.pid);
                    add_cpu_list(p, &cpu_head_check_new, time, t->process.startTime);
                    enqueue(q3, createProcessNode((struct Process){p->process.pid, time + p->process.runTime - t->process.startTime, t->process.startTime}));
                    insert(sl3, p->process.pid, search(sl3, p->process.pid) + time - t->process.startTime);
                }
            }
        }
    }
    return schedule;
}
void clear_schedule(struct Hashmap **schedule)
{
    // Free allocated memory for the linked lists and schedule
    for (int i = 0; i < 30000; i++)
    {
        struct CPU_Node *currentCPU = (*schedule)[i].cpu_head;
        while (currentCPU != NULL)
        {
            struct CPU_Node *temp = currentCPU;
            currentCPU = currentCPU->next;
            free(temp);
        }
    }
    free(*schedule);
}
struct Process_Node *get_process_from_id(int id)
{
    struct Process_Node *head = process_head;
    while (head != NULL)
    {
        if (head->process.pid == id)
        {
            return head;
        }
        head = head->next;
    }
    return NULL;
}
float *return_tt_rt(struct Hashmap *schedule)
{
    float *array = (float *)malloc(3 * sizeof(float));

    if (array == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    // Initialize the elements of the array
    array[0] = 0.0; // turnaround time
    array[1] = 0.0; // response time
    array[2] = 0.0; // count

    // Print the CPU schedule1 to the output file
    for (int i = 0; i < 30000; i++)
    {
        if (schedule[i].cpu_head == NULL)
        {
            break;
        }
        struct Process_Node *process = get_process_from_id(schedule[i].pid);
        struct CPU_Node *currentCPU = schedule[i].cpu_head;
        float arrival_time = process->process.startTime;
        float run_time = process->process.runTime;
        float start_time = currentCPU->cpu.startTime;
        while (currentCPU->next != NULL)
        {
            array[2]++;
            currentCPU = currentCPU->next;
        }
        array[2] += 1;
        float end_time = currentCPU->cpu.endTime;
        array[0] += (end_time - arrival_time);
        array[1] += (start_time - arrival_time);
    }

    return array;
}

// Comparator function for qsort to sort based on the second column (index 1)
int compare(const void *a, const void *b)
{
    const float *arr1 = *(const float(*)[3])a; // Cast a to a pointer to a 2D array of floats
    const float *arr2 = *(const float(*)[3])b; // Cast b to a pointer to a 2D array of floats

    // Compare the second element of the arrays (arr1[1] and arr2[1])
    if (arr1[1] < arr2[1])
        return -1;
    if (arr1[1] > arr2[1])
        return 1;
    return 0;
}
// q: function that generated random numbers using exponential distribution

float timeNow = 0;
float lambda1 = 0.1; // You can change this value
float lambda2 = 3; // You can change this value

// Function to generate exponential random variable with parameter lambda
float generateExponentialRandomVariable(double lambda)
{
    double u = rand() / (RAND_MAX + 1.0);
    float ret_value = -log(1 - u) / lambda;
    return ret_value;
}

float *createProcess(int pid)
{
    // Calculate the start time and run time using transformed random variables
    float *array = (float *)malloc(2 * sizeof(float));

    if (array == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1); // Exit the program with an error code
    }

    // Initialize the elements of the array
    array[0] = 0.0;
    array[1] = 0.0;
    float startTimeRandom = generateExponentialRandomVariable(lambda1);
    float runTimeRandom = generateExponentialRandomVariable(lambda2);
    timeNow += startTimeRandom;
    array[0] = timeNow;
    array[1] = runTimeRandom;
    return array;
}

int main(int argc, char *argv[])
{
    
    if (argc != 8) {
        fprintf(stderr, "Usage: %s input.txt output.txt TsRR TsMLFQ1 TsMLFQ2 TsMLFQ3 BMLFQ\n", argv[0]);
        return 1;
    }

    // Extract command-line arguments
    char *inputFilePath = argv[1];
    char *outputFilePath = argv[2];
    float TsRR = atoi(argv[3]);
    int TsMLFQ1 = atoi(argv[4]);
    int TsMLFQ2 = atoi(argv[5]);
    int TsMLFQ3 = atoi(argv[6]);
    int BMLFQ = atoi(argv[7]);

    
    // printf("Input file path: %s\n", inputFilePath);
    // printf("Output file path: %s\n", outputFilePath);
    // printf("TsRR: %d\n", TsRR);
    // printf("TsMLFQ1: %d\n", TsMLFQ1);
    // printf("TsMLFQ2: %d\n", TsMLFQ2);
    // printf("TsMLFQ3: %d\n", TsMLFQ3);
    // printf("BMLFQ: %d\n", BMLFQ);

    // //Open the input text file for reading
    // FILE *inputFile = fopen(inputFilePath, "r");
    // if (inputFile == NULL)
    // {
    //     perror("Error opening input file");
    //     return 1;
    // }

     //Read input from the file and add processes to the linked list
    //  int pid;
    //  float runTime, startTime;
    //  while (fscanf(inputFile, "%d %f %f", &pid, &runTime, &startTime) == 3)
    //  {
    //      addProcessNodeToEnd(createProcessNode((struct Process){pid, runTime, startTime}));
    //  }

    // Seed the random number generator with the current time
    srand((unsigned)time(NULL));
    // Generate and print some random processes
    // Open the output text file for writing
    FILE *outinFile = fopen("outin.txt", "w");
    if (outinFile == NULL)
    {
        perror("Error opening outin file");
        // fclose(inputFile);
        return 1;
    }

    for (int i = 1; i <= 3000; i++)
    {
        float *arr = createProcess(i);
        addProcessNodeToEnd(createProcessNode((struct Process){i, arr[0], arr[1]}));
        fprintf(outinFile, "j%d  %f  %f \n ", i, arr[0], arr[1]);

        printf("Process %d: Arrival Time = %f, Runtime = %f\n", i, arr[0], arr[1]);
    }
    fprintf(outinFile, "\n");
    fclose(outinFile);

    // Open the output text file for writing
    FILE *outputFile = fopen(outputFilePath, "w");
    if (outputFile == NULL)
    {
        perror("Error opening output file");
        // fclose(inputFile);
        return 1;
    }

    // printProcessList();

    // fcfs starts $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    printf("FCFS Started---------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    struct Hashmap *schedule1 = scheduleFCFS();
    float *arr = return_tt_rt(schedule1);

    int count = arr[2];
    float(*output_array)[3]; // Declare a pointer to a 2D array
    output_array = (float(*)[3])malloc(count * sizeof(float[3]));
    output_array[0][0] = 3.897;
    if (output_array == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    int k = 0;
    for (int i = 0; i < 30000; i++)
    {
        if (schedule1[i].cpu_head == NULL)
            break;
        struct CPU_Node *currentCPU = schedule1[i].cpu_head;
        while (currentCPU != NULL)
        {
            output_array[k][0] = (float)currentCPU->cpu.pid;
            output_array[k][1] = currentCPU->cpu.startTime;
            output_array[k][2] = currentCPU->cpu.endTime;
            k++;
            currentCPU = currentCPU->next;
        }
    }

    qsort(output_array, count, sizeof(output_array[0]), compare);

    // Print the CPU schedule1 to the output file
    for (int i = 0; i < count; i++)
    {
        fprintf(outputFile, "j%d  %f  %f  ", (int)output_array[i][0], output_array[i][1], output_array[i][2]);
    }
    fprintf(outputFile, "\n");
    fprintf(outputFile, "%f %f\n\n", arr[0] / (float)process_size(process_head), arr[1] / (float)process_size(process_head));
    printf("%f %f\n", arr[0] / (float)process_size(process_head), arr[1] / (float)process_size(process_head));


    FILE *gantt_fcfsFile = fopen("gantt_fcfs.txt", "w");
    if (gantt_fcfsFile == NULL)
    {
        perror("Error opening outin file");
        // fclose(inputFile);
        return 1;
    }

    for (int i = 0; i < count; i++)
    {
        fprintf(gantt_fcfsFile, "%d  %f  %f \n", (int)output_array[i][0], output_array[i][1], output_array[i][2]);
    }
    fprintf(gantt_fcfsFile, "\n");
    fclose(gantt_fcfsFile);
    

    clear_schedule(&schedule1);

    // rr starts $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    printf("RR Started -----------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    float slice;
    slice=TsRR;
    struct Hashmap *schedule3 = scheduleRR(slice);
    float *arr3 = return_tt_rt(schedule3);
    int count3 = arr3[2];

    float(*output_array3)[3]; // Declare a pointer to a 2D array
    output_array3 = (float(*)[3])malloc(count3 * sizeof(float[3]));
    if (output_array3 == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    k = 0;
    for (int i = 0; i < 30000; i++)
    {
        if (schedule3[i].cpu_head == NULL)
            break;
        struct CPU_Node *currentCPU = schedule3[i].cpu_head;
        while (currentCPU != NULL)
        {
            output_array3[k][0] = currentCPU->cpu.pid;
            output_array3[k][1] = currentCPU->cpu.startTime;
            output_array3[k][2] = currentCPU->cpu.endTime;
            k++;
            currentCPU = currentCPU->next;
        }
    }

    qsort(output_array3, count3, sizeof(output_array3[0]), compare);

    // Print the CPU schedule1 to the output file
    for (int i = 0; i < count3; i++)
    {
        fprintf(outputFile, "j%d  %f  %f  ", (int)output_array3[i][0], output_array3[i][1], output_array3[i][2]);
    }
    fprintf(outputFile, "\n");
    fprintf(outputFile, "%f %f\n\n", arr3[0] / (float)process_size(process_head), arr3[1] / (float)process_size(process_head));
    printf("%f %f\n\n", arr3[0] / (float)process_size(process_head), arr3[1] / (float)process_size(process_head));


    FILE *gantt_rrFile = fopen("gantt_rr.txt", "w");
    if (gantt_rrFile == NULL)
    {
        perror("Error opening outin file");
        // fclose(inputFile);
        return 1;
    }

    for (int i = 0; i < count3; i++)
    {
        fprintf(gantt_rrFile, "%d  %f  %f \n", (int)output_array3[i][0], output_array3[i][1], output_array3[i][2]);
    }
    fprintf(gantt_rrFile, "\n");
    fclose(gantt_rrFile);

    clear_schedule(&schedule3);

    // sjf starts $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    printf("SJF Started --------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    struct Hashmap *schedule2 = scheduleSJF();
    float *arr2 = return_tt_rt(schedule2);

    int count2 = arr2[2];
    float(*output_array2)[3]; // Declare a pointer to a 2D array
    output_array2 = (float(*)[3])malloc(count2 * sizeof(float[3]));

    if (output_array2 == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    k = 0;
    for (int i = 0; i < 30000; i++)
    {
        if (schedule2[i].cpu_head == NULL)
            break;
        struct CPU_Node *currentCPU = schedule2[i].cpu_head;
        while (currentCPU != NULL)
        {
            output_array2[k][0] = currentCPU->cpu.pid;
            output_array2[k][1] = currentCPU->cpu.startTime;
            output_array2[k][2] = currentCPU->cpu.endTime;
            k++;
            currentCPU = currentCPU->next;
        }
    }

    qsort(output_array2, count2, sizeof(output_array2[0]), compare);

    // Print the CPU schedule1 to the output file
    for (int i = 0; i < count2; i++)
    {
        fprintf(outputFile, "j%d  %f  %f  ", (int)output_array2[i][0], output_array2[i][1], output_array2[i][2]);
    }
    fprintf(outputFile, "\n");
    fprintf(outputFile, "%f %f\n\n", arr2[0] / (float)process_size(process_head), arr2[1] / (float)process_size(process_head));
    printf("%f %f\n\n", arr2[0] / (float)process_size(process_head), arr2[1] / (float)process_size(process_head));


    FILE *gantt_sjfFile = fopen("gantt_sjf.txt", "w");
    if (gantt_sjfFile == NULL)
    {
        perror("Error opening outin file");
        // fclose(inputFile);
        return 1;
    }

    for (int i = 0; i < count2; i++)
    {
        fprintf(gantt_sjfFile, "%d  %f  %f \n", (int)output_array2[i][0], output_array2[i][1], output_array2[i][2]);
    }
    fprintf(gantt_sjfFile, "\n");
    fclose(gantt_sjfFile);

    clear_schedule(&schedule2);

    // stcs starts $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    printf("STCS Started---------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    struct Hashmap *schedule4 = scheduleSTCS();
    float *arr4 = return_tt_rt(schedule4);
    int count4 = arr4[2];

    float(*output_array4)[3]; // Declare a pointer to a 2D array
    output_array4 = (float(*)[3])malloc(count4 * sizeof(float[3]));
    if (output_array4 == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    k = 0;
    for (int i = 0; i < 30000; i++)
    {
        if (schedule4[i].cpu_head == NULL)
            break;
        struct CPU_Node *currentCPU = schedule4[i].cpu_head;
        while (currentCPU != NULL)
        {
            output_array4[k][0] = currentCPU->cpu.pid;
            output_array4[k][1] = currentCPU->cpu.startTime;
            output_array4[k][2] = currentCPU->cpu.endTime;
            k++;
            currentCPU = currentCPU->next;
        }
    }

    qsort(output_array4, count4, sizeof(output_array4[0]), compare);

    // Print the CPU schedule1 to the output file
    for (int i = 0; i < count4; i++)
    {
        fprintf(outputFile, "j%d  %f  %f  ", (int)output_array4[i][0], output_array4[i][1], output_array4[i][2]);
    }
    fprintf(outputFile, "\n");
    fprintf(outputFile, "%f %f\n", arr4[0] / (float)process_size(process_head), arr4[1] / (float)process_size(process_head));
    printf("%f %f\n\n", arr4[0] / (float)process_size(process_head), arr4[1] / (float)process_size(process_head));


    FILE *gantt_stcsFile = fopen("gantt_stcs.txt", "w");
    if (gantt_stcsFile == NULL)
    {
        perror("Error opening outin file");
        // fclose(inputFile);
        return 1;
    }

    for (int i = 0; i < count4; i++)
    {
        fprintf(gantt_stcsFile, "%d  %f  %f \n", (int)output_array4[i][0], output_array4[i][1], output_array4[i][2]);
    }
    fprintf(gantt_rrFile, "\n");
    fclose(gantt_rrFile);

    clear_schedule(&schedule4);

    // Close the input and output files
    // fclose(inputFile);
    fclose(outputFile);

    return 0;
}

// main ends $$$$$$$