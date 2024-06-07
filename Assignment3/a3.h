// Era Sarda(2020MT10801), Ritika Soni (2020MT10838)
// functions implemented are my_malloc, my_free, my_realloc, my_calloc, my_debug
// my_malloc: allocates the requested size of memory and returns a pointer to it
// my_free: frees the memory pointed by the pointer
// my_calloc: allocates the requested size of memory and sets all the bytes to 0
// my_debug: prints the free list and heap info

#define _DEFAULT_SOURCE
//^ Defined for MAP_ANONYMOUS to be available
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static void *page;
const int total_page_size = 8 * 1024; // 8 KB allocated total to the page

struct info_heap
{
  int num_alloc_blocks;   // allocated blocks
  int largest_free_space; // largest space in heap without header
  int heap_size;          // current size without header
};

struct free_list_node
{                              // header for free list node and allocated node
  int size_fl;                 // size of free list node and allocated node
  struct free_list_node *next; // pointer to next free list node
  struct free_list_node *prev; // pointer to previous free list node
  bool free_fl;                // 0-free, 1-allocated
  int magic;                   // alternate to free to check if node is allocated or not
};

struct info_heap *info_abt_heap = NULL;
struct free_list_node *head_free_list; // head of free list

int initialize(void)
{ // function to initialize the heap
  page = mmap(NULL, total_page_size, PROT_READ | PROT_WRITE,
              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (page == MAP_FAILED)
  {
    return errno;
  }
  info_abt_heap = (struct info_heap *)page;
  head_free_list =
      (struct free_list_node *)((char *)page + sizeof(struct info_heap));
  head_free_list->size_fl = total_page_size - sizeof(struct free_list_node) - sizeof(struct info_heap);
  // head_free_list->size_fl = 100;
  head_free_list->next = NULL;
  head_free_list->prev = NULL;
  info_abt_heap->heap_size = 0;        // initially heap is empty
  info_abt_heap->num_alloc_blocks = 0; // initially no blocks allocated
  info_abt_heap->largest_free_space = head_free_list->size_fl;
  return 0;
}

void my_debug()
{
  struct free_list_node *temp = head_free_list;
  while (temp != NULL)
  {
    fprintf(stderr, "Free list (%p): size = %d, free = %d, prev = %p, next = %p\n", temp, temp->size_fl, temp->free_fl, temp->prev, temp->next);
    temp = temp->next;
  }
  // print the heap info
  fprintf(stderr, "Heap size: %d\n", info_abt_heap->heap_size);
  fprintf(stderr, "Largest free space: %d\n", info_abt_heap->largest_free_space);
  fprintf(stderr, "Number of allocated blocks: %d\n", info_abt_heap->num_alloc_blocks);
}

void *my_malloc(int size)
{
  int save_size = size; // stores the size of the user requested space
  if (size <= 0)
  {
    fprintf(stderr, "Invalid size requested from malloc\n");
    return NULL;
  }

  if (head_free_list->size_fl == 0) // no space available
  {
    fprintf(stderr, "No space available in the heap\n");
    page = mmap(
        head_free_list, info_abt_heap->heap_size + total_page_size,
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,
        0); // check if we have to use some pointer from previous page or not
    if (page == MAP_FAILED)
    {
      return NULL;
    }
    head_free_list->size_fl = total_page_size - sizeof(struct free_list_node);
    head_free_list->next = NULL;
    head_free_list->prev = NULL;
    head_free_list->free_fl = 0;
  }

  int size_with_header = sizeof(struct free_list_node) + size; // size of the user requested space with header

  struct free_list_node *ptr = NULL; // used to point to the node which is to be allocated
  struct free_list_node *node = head_free_list;
  int min = total_page_size + 1;
  int max = -1;
  struct free_list_node *second_last_node = NULL;

  while (node != NULL) // best fit algorithm
  {
    if (node->size_fl > max)
    {
      max = node->size_fl;
    }
    if (node->size_fl >= size_with_header)
    {
      if (min > node->size_fl)
      {
        min = node->size_fl;
        ptr = node;
      }
    }
    if (node->next == NULL)
      second_last_node = node; // saved the pointer to last free node
    node = node->next;
  }
  if (ptr == NULL) // no node found as the size of the free node is less than the requested size
  {
    fprintf(stderr, "No space available in the heap\n"); // do we need to print this?
    void *new_page = mmap(
        NULL, info_abt_heap->heap_size + total_page_size,
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); // check if we have to use some pointer from previous page or not
    if (page == MAP_FAILED)
    {
      return NULL;
    }
    fprintf(stderr, "No space in free list so sbrk called\n");
    struct free_list_node *new_node_temp = (struct free_list_node *)new_page;
    // add new_node_temp to free list with sorting according to address
    node = head_free_list;
    while (node != NULL && node->next != NULL && node < new_node_temp)
    {
      node = node->next;
    }
    if (node->next == NULL && node < new_node_temp)
    {
      new_node_temp->next = NULL;
      new_node_temp->prev = node;
      node->next = new_node_temp;
    }
    else if (node == head_free_list && node > new_node_temp)
    {
      new_node_temp->next = node;
      node->prev = new_node_temp;
      new_node_temp->prev = NULL;
      head_free_list = new_node_temp;
    }
    else if (node > new_node_temp)
    {
      new_node_temp->next = node;
      new_node_temp->prev = node->prev;
      node->prev = new_node_temp;
    }
    new_node_temp->size_fl = total_page_size - sizeof(struct free_list_node);
    new_node_temp->free_fl = 0; // free list node
    return my_malloc(size);
  }

  struct free_list_node *new_node = ptr;      // node to be allocated
  if (ptr->prev == NULL && ptr->next == NULL) // only one node in free list
  {
    if (ptr->size_fl - size == sizeof(struct free_list_node))
    {
      head_free_list->free_fl = 1; //  not free
    }
    else
    {
      head_free_list = (struct free_list_node *)((char *)ptr + size_with_header);
      head_free_list->free_fl = 0; // not free
    }
    head_free_list->next = NULL;
    head_free_list->prev = NULL;
    head_free_list->size_fl = ptr->size_fl - sizeof(struct free_list_node) - size;
  }
  else if (ptr->prev == NULL) // first node as next is not null
  {
    if (ptr->size_fl - size == sizeof(struct free_list_node))
    {
      head_free_list = ptr->next;
    }
    else
    {
      head_free_list = (struct free_list_node *)((char *)ptr + size_with_header);
      head_free_list->next = ptr->next;
      ptr->next->prev = head_free_list;
    }
    head_free_list->prev = NULL;
    head_free_list->size_fl = ptr->size_fl - size_with_header;
  }
  else if (ptr->next == NULL) // last node
  {
    if (ptr->size_fl - size == sizeof(struct free_list_node))
    {
      ptr->prev->next = NULL;
    }
    else
    {
      struct free_list_node *temp = ptr->prev;
      int size_temp = ptr->size_fl;
      ptr = (struct free_list_node *)((char *)ptr + size_with_header);
      temp->next = ptr;
      ptr->prev = temp;
      ptr->next = NULL;
      ptr->size_fl = size_temp - size_with_header;
      ptr->free_fl = 0; // free hai
    }
  }
  else // middle node
  {
    if (ptr->size_fl - size == sizeof(struct free_list_node))
    {
      ptr->prev->next = ptr->next;
      ptr->next->prev = ptr->prev;
    }
    else
    {
      struct free_list_node *temp1 = ptr->prev;
      struct free_list_node *temp2 = ptr;
      ptr = (struct free_list_node *)((char *)ptr + size_with_header);
      temp1->next = ptr;
      ptr->prev = temp1;
      ptr->next = temp2->next;
      temp2->next->prev = ptr;
      ptr->size_fl = temp2->size_fl - size_with_header;
      ptr->free_fl = 0;
    }
    new_node->size_fl = size;
  }
  ptr = head_free_list;
  new_node->size_fl = size;
  new_node->magic = 0x1234567;
  new_node->free_fl = 1;
  new_node->prev = NULL;
  new_node->next = NULL;

  // Heap info update
  info_abt_heap->num_alloc_blocks++;
  info_abt_heap->heap_size += size;

  node = head_free_list;
  max = -1;
  while (node != NULL)
  {
    if (node->size_fl > max)
    {
      max = node->size_fl;
    }
    node = node->next;
  }
  info_abt_heap->largest_free_space = max;
  return (void *)((char *)(new_node) + sizeof(struct free_list_node));
}

void *my_free(void *ptr)
{
  if (ptr == NULL)
  {
    fprintf(stderr, "Invalid pointer\n");
    return NULL;
  }
  struct free_list_node *node = (struct free_list_node *)((char *)ptr - sizeof(struct free_list_node)); // header of free process already free
  if (node->free_fl == 0)
  {
    fprintf(stderr, "Invalid pointer\n");
    return NULL;
  }

  node->free_fl = 0; // free
  info_abt_heap->heap_size -= node->size_fl;

  struct free_list_node *temp_head = head_free_list;
  if (temp_head <
      node) // node to be freed is present before the first node of free list
  {
    while (temp_head != NULL && temp_head->next != NULL && temp_head->next < node) // to check if node to be freed is present between two nodes of free list
    {
      temp_head = temp_head->next;
    }
    if ((struct free_list_node *)((char *)temp_head + temp_head->size_fl + sizeof(struct free_list_node)) == node) // node to be frees is present just after  the free node in free list
    {
      temp_head->size_fl += sizeof(struct free_list_node) + node->size_fl;
      node = temp_head;
      node->free_fl = temp_head->free_fl;
      node->next = temp_head->next;
      node->prev = temp_head->prev;
      node->size_fl = temp_head->size_fl;
    }
    else
    {
      node->next = temp_head->next;
      node->prev = temp_head;
      temp_head->next = node;

      if (node->next != NULL)
      {
        node->next->prev = node;
      }
      else
      {
        node->next = NULL;
      }
    }

    if (node->next != NULL && node->next == (struct free_list_node *)((char *)node + node->size_fl + sizeof(struct free_list_node)))
    {
      node->size_fl += sizeof(struct free_list_node) + node->next->size_fl;
      if (node->next->next == NULL)
      {
        node->next = NULL;
      }
      else
      {
        node->next = node->next->next;
        node->next->prev = node;
      }
    }
  }
  else
  {
    if ((struct free_list_node *)((char *)node + node->size_fl + sizeof(struct free_list_node)) == temp_head)
    {
      node->size_fl += sizeof(struct free_list_node) + temp_head->size_fl;
      node->next = temp_head->next;
      if (temp_head->next != NULL)
        temp_head->next->prev = node;
      head_free_list = node;
    }
    else
    {
      node->next = temp_head;
      temp_head->prev = node;
      head_free_list = node;
      node->prev = NULL;
    }
  }
  // heap info update
  info_abt_heap->num_alloc_blocks--;

  struct free_list_node *temp = head_free_list;
  int max = 0;
  while (temp != NULL)
  {
    if (temp->size_fl > max)
    {
      max = temp->size_fl;
    }
    temp = temp->next;
  }
  info_abt_heap->largest_free_space = max;
  return NULL;
}

void *my_calloc(int num, int size)
{ // num is number of elements, size is size of each element
  int total_size = num * size;
  void *ptr = my_malloc(total_size); // allocate space
  memset(ptr, 0, total_size);        // set all the bytes to 0
  return ptr;
}

void *my_realloc(void *ptr, int size)
{
  if (ptr == NULL) // if ptr is null then realloc is same as malloc
  {
    return my_malloc(size);
  }

  if (size <= 0)
  {
    fprintf(stderr, "Invalid size requested from realloc\n");
    return NULL;
  }
  struct free_list_node *node = (struct free_list_node *)((char *)ptr - sizeof(struct free_list_node));
  if (node->free_fl == 0)
  {
    fprintf(stderr, "Invalid pointer\n");
    return NULL;
  }

  if (node->size_fl >= size) // if size of the node is greater than the requested size then no need to allocate new node
  {
    return ptr;
  }
  void *new_node = my_malloc(size); // allocate new node
  if (new_node == NULL)             // if malloc fails
  {
    fprintf(stderr, "Malloc failed\n");
    return NULL;
  }
  memcpy(new_node, ptr, node->size_fl); // copy the data from old node to new
  my_free(ptr);                         // free the old node
  return new_node;
}
