#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define MAX_THREADS 201
#define MAX_FILENAME_LENGTH 100

struct Node {
  int key;
  struct Node *left;
  struct Node *right;
  int height;
};

struct Node *rootnode = NULL;

int height(struct Node *N) {
  if (N == NULL)
    return 0;
  return N->height;
}

int max(int a, int b) { return (a > b) ? a : b; }

struct Command {
  int type; // 0 for insert, 1 for delete, 2 for contain, 3 for inorder
  int key;
};

struct CommandQueue {
  struct Command *commands;
  int capacity;
  int size;
  int front;
  int rear;
  pthread_mutex_t mutex;
  sem_t full;
  sem_t empty;
};

struct read_write_lock {
  sem_t num_readers_lock;
  pthread_mutex_t num_writers_lock;
  pthread_cond_t zero_writers;
  sem_t write_lock;
  int num_readers;
  int num_writers;
};

struct read_write_lock global_rwlock;

void InitializeGlobalLock(struct read_write_lock *rwlock) {
  sem_init(&rwlock->num_readers_lock, 0, 1);
  pthread_mutex_init(&rwlock->num_writers_lock, NULL);
  pthread_cond_init(&rwlock->zero_writers, NULL);
  sem_init(&rwlock->write_lock, 0, 1);
  rwlock->num_readers = 0;
  rwlock->num_writers = 0;
}

void InitializeCommandQueue(struct CommandQueue *queue, int capacity) {
  queue->commands = (struct Command *)malloc(capacity * sizeof(struct Command));
  queue->capacity = capacity;
  queue->size = 0;
  queue->front = 0;
  queue->rear = -1;
  pthread_mutex_init(&queue->mutex, NULL);
  sem_init(&queue->full, 0, 0);
  sem_init(&queue->empty, 0, capacity);
}

void EnqueueCommand(struct CommandQueue *queue, const struct Command *command) {
  queue->rear = (queue->rear + 1) % queue->capacity;
  queue->commands[queue->rear] = *command;
  queue->size++;
}

void DequeueCommand(struct CommandQueue *queue, struct Command *command) {
  *command = queue->commands[queue->front];
  queue->front = (queue->front + 1) % queue->capacity;
  queue->size--;
}

void GReaderLock(struct read_write_lock *rwlock) {
  sem_wait(&rwlock->num_readers_lock);
  rwlock->num_readers++;
  if (rwlock->num_readers == 1) {
    sem_wait(&rwlock->write_lock);
  }
  sem_post(&rwlock->num_readers_lock);
}

void GReaderUnlock(struct read_write_lock *rwlock) {
  sem_wait(&rwlock->num_readers_lock);
  rwlock->num_readers--;
  if (rwlock->num_readers == 0) {
    sem_post(&rwlock->write_lock);
    pthread_cond_broadcast(&rwlock->zero_writers);
  }
  sem_post(&rwlock->num_readers_lock);
}

void GWriterLock(struct read_write_lock *rwlock) {
  pthread_mutex_lock(&rwlock->num_writers_lock);
  rwlock->num_writers++;
  while (rwlock->num_readers > 0) {
    pthread_cond_wait(&rwlock->zero_writers, &rwlock->num_writers_lock);
  }
  pthread_mutex_unlock(&rwlock->num_writers_lock);
}

void GWriterUnlock(struct read_write_lock *rwlock) {

  pthread_mutex_lock(&rwlock->num_writers_lock);
  rwlock->num_writers--;
  pthread_mutex_unlock(&rwlock->num_writers_lock);
}

struct WriteFileArgs {
  int key;
  struct read_write_lock *rwlock;
  int write_type; // 0 for insert, 1 for delete, 2 for contain, 3 for inorder
};

void *leftRotate(struct Node *x) {
  struct Node *y = x->right;
  struct Node *T2 = y->left;
  y->left = x;
  x->right = T2;
  x->height = max(height(x->left), height(x->right)) + 1;
  y->height = max(height(y->left), height(y->right)) + 1;
  return y;
}

void *rightRotate(struct Node *x) {
  struct Node *y = x->left;
  struct Node *T2 = y->right;
  y->right = x;
  x->left = T2;
  x->height = max(height(x->left), height(x->right)) + 1;
  y->height = max(height(y->left), height(y->right)) + 1;
  return y;
}

int getBalance(struct Node *N) {
  if (N == NULL) {
    return 0;
  }
  return height(N->left) - height(N->right);
}

bool search(int key) {
  struct Node *temp = rootnode;
  while (temp != NULL) {
    if (key < temp->key) {
      temp = temp->left;
    } else if (key > temp->key) {
      temp = temp->right;
    } else {
      return true;
    }
  }
  return false;
}

int inorder_array[201];
int inorder_index = 0;

void inorder(struct Node *root) {
  struct Node *temp = root;
  if (temp == NULL) {
    return;
  }
  if (temp != NULL) {
    inorder(temp->left);
    inorder_array[inorder_index++] = temp->key;
    // printf(" %d ", temp->key);
    inorder(temp->right);
  }
}

void inorder_new(struct Node *root) {
  GWriterLock(&global_rwlock);
  inorder(root);
  // printf("\n------inorder_new print-------\n");
  for(int i=0;i<inorder_index;i++){
      printf(" %d ", inorder_array[i]);
  }
  printf("\n");
  inorder_index = 0;
  GWriterUnlock(&global_rwlock);
}

void preorder(struct Node *root) {
  struct Node *temp = root;
  if (temp == NULL) {
    return;
  }
  if (temp != NULL) {
    printf(" %d ", temp->key);
    preorder(temp->left);
    preorder(temp->right);
  }
}
void preorder_new(){
  GReaderLock(&global_rwlock);
  preorder(rootnode);
  GReaderUnlock(&global_rwlock);
}

void *ReadFile(void *arg) { // 0 for insert, 1 for delete, 2 for contain, 3 for
                            // inorder, -1 for pre-order
  struct WriteFileArgs *args = (struct WriteFileArgs *)arg;
  struct read_write_lock *rwlock = args->rwlock;
  int key = args->key;
  int write_type = args->write_type;
  if (write_type == 0 || write_type == 1)
    return NULL;
  if (write_type == 2) {
    // printf("\nThread Checking if Tree Contains key : %d\n", args->key);
    if (search(key) == 0) {
      printf("no\n");
    } else {
      printf("yes\n");
    }
  } else if (write_type == 3) {
    // printf("\nThread printing inorder traversal\n");
    inorder_new(rootnode);
  }
  return NULL;
}

struct Node *insert(struct Node *root, int key) {
  if (root == NULL) {
    struct Node *temp = (struct Node *)malloc(sizeof(struct Node));
    temp->key = key;
    temp->left = NULL;
    temp->right = NULL;
    temp->height = 1;
    return temp;
  } else if (key < root->key) {
    root->left = insert(root->left, key);
  } else if (key > root->key) {
    root->right = insert(root->right, key);
  } else {
    return root;
  }
  root->height = 1 + max(height(root->left), height(root->right));
  int balance = getBalance(root);
  if (balance > 1 && key < root->left->key) {
    return rightRotate(root);
  }
  if (balance < -1 && key > root->right->key) {
    return leftRotate(root);
  }
  if (balance > 1 && key > root->left->key) {
    root->left = leftRotate(root->left);
    return rightRotate(root);
  }
  if (balance < -1 && key < root->right->key) {
    root->right = rightRotate(root->right);
    return leftRotate(root);
  }
  return root;
}

void *minValueNode(struct Node *node) {
  struct Node *current = node;
  while (current->left != NULL) {
    current = current->left;
  }
  return current;
}

struct Node *deleteNode(struct Node *root, int key) {
  if (root == NULL) {
    return root;
  }
  if (key < root->key) {
    root->left = deleteNode(root->left, key);
  } else if (key > root->key) {
    root->right = deleteNode(root->right, key);
  } else {
    if ((root->left == NULL) || (root->right == NULL)) {
      struct Node *temp = root->left ? root->left : root->right;
      if (temp == NULL) {
        temp = root;
        root = NULL;
      } else {
        *root = *temp;
      }
      free(temp);
    } else {
      struct Node *temp = minValueNode(root->right);
      root->key = temp->key;
      root->right = deleteNode(root->right, temp->key);
    }
  }
  if (root == NULL) {
    return root;
  }
  root->height = 1 + max(height(root->left), height(root->right));
  int balance = getBalance(root);
  if (balance > 1 && getBalance(root->left) >= 0) {
    return rightRotate(root);
  }
  if (balance > 1 && getBalance(root->left) < 0) {
    root->left = leftRotate(root->left);
    return rightRotate(root);
  }
  if (balance < -1 && getBalance(root->right) <= 0) {
    return leftRotate(root);
  }
  if (balance < -1 && getBalance(root->right) > 0) {
    root->right = rightRotate(root->right);
    return leftRotate(root);
  }
  return root;
}

struct Node *insert_new(struct Node *root, int key) {
  GWriterLock(&global_rwlock);
  root = insert(root, key);
  GWriterUnlock(&global_rwlock);
  return root;
}

struct Node *deleteNode_new(struct Node *root, int key) {
  GWriterLock(&global_rwlock);
  if(search(key) == 1)
  {root = deleteNode(root, key);
  }
  // else 
    // printf("Key %d not found\n", key);
  GWriterUnlock(&global_rwlock);
  return root;
}
void *WriteFile(void *arg) {
  struct WriteFileArgs *args = (struct WriteFileArgs *)arg;
  int key = args->key;
  int write_type = args->write_type;

  if (write_type == 0) {
    // printf("\nThread Insertion of key : %d\n",  args->key);
    rootnode = insert_new(rootnode, key);
  } else if (write_type == 1) {
    // printf("\nThread Deletion of key : %d\n",  args->key);
    rootnode = deleteNode_new(rootnode, key);
  }
  return NULL;
}

int create_thread(pthread_t *thread, void *(*start_routine)(void *),
                  void *arg) {
  if (pthread_create(thread, NULL, start_routine, arg) != 0) {
    perror("pthread_create");
    return -1; // Return an error code
  }
  return 0; // Success
}

// Wrapper function for pthread_join
int join_thread(pthread_t thread) {
  if (pthread_join(thread, NULL) != 0) {
    perror("pthread_join");
    return -1; // Return an error code
  }
  return 0; // Success
}

void StartWorkerThreads(struct CommandQueue *queue,
                        struct read_write_lock *rwlock) {
  pthread_t threads[MAX_THREADS];
  struct WriteFileArgs args[MAX_THREADS];
  int thread_count = 0;

  while (1) {
    struct Command command;
    DequeueCommand(queue, &command);

    if (command.type == 0) {
      if (thread_count < MAX_THREADS) {
        args[thread_count].rwlock = rwlock;
        args[thread_count].key = command.key;
        args[thread_count].write_type = 0; // 0 for insert
        // printf("Insertion of key : %d\n", command.key);
        if (create_thread(&threads[thread_count], WriteFile,
&args[thread_count]) == 0) {
          thread_count++;
        }
      }
    }

    else if (command.type == 1) {
      if (thread_count < MAX_THREADS) {
        args[thread_count].rwlock = rwlock;
        args[thread_count].key = command.key;
        args[thread_count].write_type = 1;
        // printf("Deletion of key : %d\n", command.key);
        if (create_thread(&threads[thread_count], WriteFile,
&args[thread_count]) == 0) {
          thread_count++;
        }
      }
    } else if (command.type == 2) {
      if (thread_count < MAX_THREADS) {
        args[thread_count].rwlock = rwlock;
        args[thread_count].key = command.key;
        args[thread_count].write_type = 2;
        // printf("Check if Tree Contains key : %d\n", command.key);
        if (create_thread(&threads[thread_count], ReadFile,
&args[thread_count]) == 0) {
          thread_count++;
        }
      }
    } else if (command.type == 3) {
      // Create a thread for insertion
      if (thread_count < MAX_THREADS) {
        args[thread_count].rwlock = rwlock;
        args[thread_count].key = command.key;
        args[thread_count].write_type = 3; // 0 for insert
        // printf("Printing inorder traversal\n");
        if (create_thread(&threads[thread_count], ReadFile,
&args[thread_count]) == 0) {
          thread_count++;
        }
      }
    }

    else {
      // printf("\nExiting\n");
      break;
    }
  }

  for (int i = 0; i < thread_count; i++) {
    if (join_thread(threads[i]) == 0) {
      // printf("\nThread %d joined successfully\n", i);
    }
  }
}

int main() {
  InitializeGlobalLock(&global_rwlock);

  struct CommandQueue commands;
  InitializeCommandQueue(&commands, 201);

  FILE *inputFile = fopen("input.txt", "r"); // Open the input file for reading
  if (inputFile == NULL) {
    perror("Error opening input file");
    return 1;
  }
  char command[MAX_FILENAME_LENGTH];
  while (fgets(command, sizeof(command), inputFile) != NULL) {
    char *token = strtok(command, " ");
    if (strcmp(token, "insert") == 0) {
      token = strtok(NULL, " \n");
      if (token == NULL) {
        fprintf(stderr, "Invalid 'insert' command\n");
        continue;
      }
      int key = atoi(token);
      EnqueueCommand(&commands, &(struct Command){0, key});
    } else if (strcmp(token, "delete") == 0) {
      token = strtok(NULL, " \n");
      if (token == NULL) {
        fprintf(stderr, "Invalid 'delete' command\n");
        continue;
      }
      int key = atoi(token);
      EnqueueCommand(&commands, &(struct Command){1, key});
    } else if (strcmp(token, "contains") == 0) {
      token = strtok(NULL, " \n");
      if (token == NULL) {
        fprintf(stderr, "Invalid 'contain' command\n");
        continue;
      }
      int key = atoi(token);
      EnqueueCommand(&commands, &(struct Command){2, key});
    } else if (strcmp(token, "in") == 0) {
        token = strtok(NULL, " \n");
        if (token != NULL && strcmp(token, "order") == 0) {
          EnqueueCommand(&commands, &(struct Command){3, 0});
        } else {
          fprintf(stderr, "Invalid command: in %s\n", token);
        }
      }
    else if (strcmp(token, "exit") == 0) {
      EnqueueCommand(&commands, &(struct Command){-1, 0});
    } else {
      fprintf(stderr, "Invalid command: %s\n", token);
    }
  }
  fclose(inputFile);
  StartWorkerThreads(&commands, &global_rwlock);
  free(commands.commands);
  // printf("\n---------In preorder--------\n");
  // printf("rootnode is key %d\n", rootnode->key);
  preorder(rootnode);
  printf("\n");
  // printf("\n***************Program ends here*************\n");
  return 0;
}

