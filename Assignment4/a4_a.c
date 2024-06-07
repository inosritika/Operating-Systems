#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_FILENAME_LENGTH 256
#define MAX_FILE_CONTENT_LENGTH 1024
#define MAX_THREADS 200

struct Command {
  int type; // 0 for read, 1 for write, -1 to exit
  char filename[MAX_FILENAME_LENGTH];
  char content[MAX_FILE_CONTENT_LENGTH];
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

struct WriteFileArgs {
  char filename[MAX_FILENAME_LENGTH];
  int write_type; // 1 for content, 2 for append
  char content[MAX_FILE_CONTENT_LENGTH];
  struct read_write_lock *rwlock;
};

struct FileLock {
  sem_t read_lock;
  sem_t write_lock;
  pthread_mutex_t num_readers_lock;
  pthread_mutex_t num_writers_lock;
  pthread_cond_t zero_writers;
  int num_readers;
  int num_writers;
  bool writer_present;
};

#define FILE_LOCK_TABLE_SIZE 1000
struct FileLock file_lock_table[FILE_LOCK_TABLE_SIZE];

struct read_write_lock global_rwlock;

void InitializeGlobalLock(struct read_write_lock *rwlock) {
  sem_init(&rwlock->num_readers_lock, 0, 1);
  pthread_mutex_init(&rwlock->num_writers_lock, NULL);
  pthread_cond_init(&rwlock->zero_writers, NULL);
  sem_init(&rwlock->write_lock, 0, 1);
  rwlock->num_readers = 0;
  rwlock->num_writers = 0;
}

void InitializeFileLock(struct FileLock *file_lock) {
  sem_init(&file_lock->read_lock, 0, 1);
  sem_init(&file_lock->write_lock, 0, 1);
  pthread_mutex_init(&file_lock->num_readers_lock, NULL);
  pthread_mutex_init(&file_lock->num_writers_lock, NULL);
  pthread_cond_init(&file_lock->zero_writers, NULL);
  file_lock->num_readers = 0;
  file_lock->num_writers = 0;
  file_lock->writer_present = false;
}

void InitializeFileLockTable() {
  for (int i = 0; i < FILE_LOCK_TABLE_SIZE; i++) {
    InitializeFileLock(&file_lock_table[i]);
  }
}

int HashFilename(const char *filename) {
  unsigned long hash = 5381;
  int c;
  while ((c = *filename++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash % FILE_LOCK_TABLE_SIZE;
}

struct FileLock *GetFileLock(const char *filename) {
  int index = HashFilename(filename);
  return &file_lock_table[index];
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
  sem_wait(&queue->empty);
  pthread_mutex_lock(&queue->mutex);
  queue->rear = (queue->rear + 1) % queue->capacity;
  queue->commands[queue->rear] = *command;
  queue->size++;
  pthread_mutex_unlock(&queue->mutex);
  sem_post(&queue->full);
}

void DequeueCommand(struct CommandQueue *queue, struct Command *command) {
  sem_wait(&queue->full);
  pthread_mutex_lock(&queue->mutex);
  *command = queue->commands[queue->front];
  queue->front = (queue->front + 1) % queue->capacity;
  queue->size--;
  pthread_mutex_unlock(&queue->mutex);
  sem_post(&queue->empty);
}

void GReaderLock(struct read_write_lock *rwlock) {
  sem_wait(&rwlock->num_readers_lock);
  rwlock->num_readers++;
  printf("\n global reader lock called\n");
  if (rwlock->num_readers == 1) {
    sem_wait(&rwlock->write_lock);
  }
  sem_post(&rwlock->num_readers_lock);
}

void GReaderUnlock(struct read_write_lock *rwlock) {
  sem_wait(&rwlock->num_readers_lock);
  rwlock->num_readers--;
  printf("\n global reader unlock called\n");
  if (rwlock->num_readers == 0) {
    sem_post(&rwlock->write_lock);
    pthread_cond_broadcast(&rwlock->zero_writers);
  }
  sem_post(&rwlock->num_readers_lock);
}

void GWriterLock(struct read_write_lock *rwlock) {
  pthread_mutex_lock(&rwlock->num_writers_lock);
  rwlock->num_writers++;
  printf("\n global writer lock called\n");
  while (rwlock->num_readers > 0) {
    pthread_cond_wait(&rwlock->zero_writers, &rwlock->num_writers_lock);
  }
  pthread_mutex_unlock(&rwlock->num_writers_lock);
}
void GWriterUnlock(struct read_write_lock *rwlock) {
  pthread_mutex_lock(&rwlock->num_writers_lock);
  rwlock->num_writers--;
  printf("\n global writer unlock called\n");
  pthread_mutex_unlock(&rwlock->num_writers_lock);
}

void *ReadFile(void *arg) {
  struct WriteFileArgs *args = (struct WriteFileArgs *)arg;
  const char *filename = args->filename;
  struct read_write_lock *rwlock = args->rwlock;
  struct FileLock *file_lock = GetFileLock(filename);

  GReaderLock(rwlock);
  sem_wait(&file_lock->read_lock);
  pthread_mutex_lock(&file_lock->num_readers_lock);
  file_lock->num_readers++;
  if (file_lock->num_readers == 1) {
    sem_wait(&file_lock->write_lock);
  }
  pthread_mutex_unlock(&file_lock->num_readers_lock);
  sem_post(&file_lock->read_lock);

  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    perror("File open error");
    GReaderUnlock(rwlock);
    return NULL;
  }

  char content[MAX_FILE_CONTENT_LENGTH];
  size_t bytes_read = 0;
  int file_size = 0;
  char ch;

  while ((ch = fgetc(file)) != EOF) {
    if (bytes_read < sizeof(content) - 1) {
      content[bytes_read] = ch;
    }
    bytes_read++;
  }

  content[bytes_read] = '\0';

  if (bytes_read > 0) {
    printf("File Content: %s\n", content);
  } else {
    printf("File is empty or couldn't be read.\n");
  }

  printf("\nRead %s of %zu bytes with %d file-readers, %d global-readers and "
         "%d file-writers present, %d global-writers\n",
         filename, bytes_read, file_lock->num_readers, rwlock->num_readers,
         file_lock->num_writers, rwlock->num_writers);

  fclose(file);

  sem_wait(&file_lock->read_lock);
  pthread_mutex_lock(&file_lock->num_readers_lock);
  file_lock->num_readers--;
  if (file_lock->num_readers == 0) {
    sem_post(&file_lock->write_lock);
  }
  pthread_mutex_unlock(&file_lock->num_readers_lock);
  sem_post(&file_lock->read_lock);
  GReaderUnlock(rwlock);

  return NULL;
}

void *WriteFile(void *arg) {
  struct WriteFileArgs *args = (struct WriteFileArgs *)arg;
  const char *filename = args->filename;
  struct read_write_lock *rwlock = args->rwlock;
  struct FileLock *file_lock = GetFileLock(filename);
  long file_size = 0;
  GWriterLock(rwlock);
  sem_wait(&file_lock->write_lock);
  pthread_mutex_lock(&file_lock->num_writers_lock);
  file_lock->num_writers++;
  if (file_lock->num_writers == 1) {
    sem_wait(&file_lock->read_lock);
  }
  file_lock->writer_present = true;
  pthread_mutex_unlock(&file_lock->num_writers_lock);
  sem_post(&file_lock->write_lock);
  FILE *file = fopen(filename, "a");
  if (file == NULL) {
    perror("File open error");
    GWriterUnlock(rwlock);
    return NULL;
  }

  if (args->write_type == 1) {
    FILE *source_file = fopen(args->content, "r");
    if (source_file == NULL) {
      perror("Source file open error");
      GWriterUnlock(rwlock);
      fclose(file);
      return NULL;
    }
    fputc('\n', file);

    char buffer[MAX_FILE_CONTENT_LENGTH];
    size_t bytes_written = 1; 
    size_t bytes_read;
    int previous_char = EOF;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source_file)) > 0) {
      fwrite(buffer, 1, bytes_read, file);
      bytes_written += bytes_read;
    }
    file_size = bytes_written;

    fclose(source_file);
  } else if (args->write_type == 2) {
    size_t content_length = strlen(args->content);
    fprintf(file, "\n%s", args->content);
    // printf("564-------%s-----------\n", args->content);
    file_size = content_length;
  }

  printf("\nWriting to %s added %ld bytes with %d file-readers, %d "
         "global-readers and %d file-writers, %d global-writers present\n",
         args->filename, file_size, file_lock->num_readers, rwlock->num_readers,
         file_lock->num_writers, rwlock->num_writers);

  fclose(file);

  sem_wait(&file_lock->write_lock);
  pthread_mutex_lock(&file_lock->num_writers_lock);
  file_lock->num_writers--;
  if (file_lock->num_writers == 0) {
    file_lock->writer_present = false;
    sem_post(&file_lock->read_lock);
  }
  pthread_mutex_unlock(&file_lock->num_writers_lock);
  sem_post(&file_lock->write_lock);

  GWriterUnlock(rwlock);

  return NULL;
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
        strcpy(args[thread_count].filename, command.filename);
        pthread_create(&threads[thread_count], NULL, ReadFile,
&args[thread_count]);
        thread_count++;
      }
    } else if (command.type == 1) {
      if (thread_count < MAX_THREADS) {
        args[thread_count].rwlock = rwlock;
        strcpy(args[thread_count].filename, command.filename);
        args[thread_count].write_type = 1; // 1 for write
        strcpy(args[thread_count].content, command.content);
        pthread_create(&threads[thread_count], NULL, WriteFile,
&args[thread_count]);
        thread_count++;
      }
    } else if (command.type == 2) {
      if (thread_count < MAX_THREADS) {
        args[thread_count].rwlock = rwlock;
        strcpy(args[thread_count].filename, command.filename);
        args[thread_count].write_type = 2; // 2 for append
        strcpy(args[thread_count].content, command.content);
        pthread_create(&threads[thread_count], NULL, WriteFile,
&args[thread_count]);
        thread_count++;
      }
    } else if (command.type == -1) {
      break;
    }
  }

  for (int i = 0; i < thread_count; i++) {
    pthread_join(threads[i], NULL);
  }

  return;
}


int main() {
  InitializeGlobalLock(&global_rwlock);

  InitializeFileLockTable();

  struct CommandQueue commands;
  InitializeCommandQueue(&commands, 200);

  FILE *inputFile = fopen("input.txt", "r"); 
  if (inputFile == NULL) {
    perror("Error opening input file");
    return 1;
  }
  char command[MAX_FILENAME_LENGTH + MAX_FILE_CONTENT_LENGTH];
  while (fgets(command, sizeof(command), inputFile) != NULL) {
    char *token = strtok(command, " ");
    if (strcmp(token, "exit\n") == 0) {
      EnqueueCommand(&commands, &(struct Command){-1, "", ""});
      break;
    } else if (strcmp(token, "read") == 0) {
      token = strtok(NULL, " \n");
      if (token == NULL) {
        fprintf(stderr, "Invalid 'read' command\n");
        continue;
      }
      struct Command readCommand;
      readCommand.type = 0;
      strcpy(readCommand.filename, token);
      EnqueueCommand(&commands, &readCommand);
    } else if (strcmp(token, "write") == 0) {
      token = strtok(NULL, " \n");
      if (token == NULL) {
        fprintf(stderr, "Invalid 'write' command\n");
        continue;
      }
      int write_type = atoi(token);
      token = strtok(NULL, " \n");
      if (token == NULL) {
        fprintf(stderr, "Invalid 'write' command\n");
        continue;
      }
      char filename[MAX_FILENAME_LENGTH];
      strcpy(filename, token);
      if (write_type == 1) {
        token = strtok(NULL, " \n");
        if (token == NULL) {
          fprintf(stderr, "Invalid 'write' command\n");
          continue;
        }
        struct Command writeCommand;
        writeCommand.type = 1;
        strcpy(writeCommand.filename, filename);
        strcpy(writeCommand.content, token);
        EnqueueCommand(&commands, &writeCommand);
      } else if (write_type == 2) {
        token = strtok(NULL, "");
        if (token == NULL) {
          fprintf(stderr, "Invalid 'write' command\n");
          continue;
        }
        struct Command writeCommand;
        writeCommand.type = 2;
        strcpy(writeCommand.filename, filename);
        strcpy(writeCommand.content, token);
        EnqueueCommand(&commands, &writeCommand);
      } else {
        fprintf(stderr, "Invalid 'write' type\n");
      }
    } else {
      fprintf(stderr, "Unknown command\n");
    }
  }
  fclose(inputFile);
  StartWorkerThreads(&commands, &global_rwlock);
  free(commands.commands);
  exit(1);
  return 0;
}