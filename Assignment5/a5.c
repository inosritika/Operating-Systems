#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
 
// function to search for a file/directory
void search_directory(const char *objective_file_dir, const char *working_directory)
{
   DIR *dir;
   struct dirent *entry;
   struct stat statbuf;
 
   if ((dir = opendir(working_directory)) == NULL)
   {
      perror("Error opening directory");
      exit(EXIT_FAILURE);
   }
 
   // Iterate through the entries in the directory
   chdir(working_directory);
   while ((entry = readdir(dir)) != NULL)
   {
      if (stat(entry->d_name, &statbuf) == -1)
      {
         perror("Error getting file status");
         exit(EXIT_FAILURE);
      }
 
      if (S_ISDIR(statbuf.st_mode))
      {
         if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
         {
            continue;
         }
         // printf("Searching in subdirectory: %s\n", working_directory, entry->d_name);
         // Check if the entry matches the objective_file_dir
         if (strcmp(entry->d_name, objective_file_dir) == 0)
         {
            // Handle the case when the objective_file_dir is the current directory
            printf("Directory found: %s\\%s\n", working_directory, entry->d_name);
            // printf("Type: Directory\n");
            // Additional handling, if needed
 
            closedir(dir);
            exit(EXIT_SUCCESS);
         }
 
         // Recursively search in the subdirectory
         char path_to_sub_directory[PATH_MAX];
         snprintf(path_to_sub_directory, sizeof(path_to_sub_directory), "%s\\%s", working_directory, entry->d_name);
         search_directory(objective_file_dir, path_to_sub_directory);
      }
      else
      {
         // Constructing absolute path for the entry
         char full_path[PATH_MAX];
         snprintf(full_path, sizeof(full_path), "%s\\%s", working_directory, entry->d_name);
         // Check if the entry matches the objective_file_dir
         if (strcmp(entry->d_name, objective_file_dir) == 0)
         {
            printf("File found: %s\n", full_path);
            if (S_ISREG(statbuf.st_mode))
            {
               // printf("Type: File\n");
            }
            else
            {
               fprintf(stderr, "Unknown type found.\n");
            }
 
            closedir(dir);
            exit(EXIT_SUCCESS);
         }
      }
   }
 
   chdir("..");
   closedir(dir);
}
 
int main(int argc, char *argv[])
{
   // Check if the correct number of arguments is provided
   if (argc != 2)
   {
      fprintf(stderr, "Usage: %s <objective_file_dir>\n", argv[0]);
      exit(EXIT_FAILURE);
   }
 
   // current working directory
   char working_directory[PATH_MAX];
   if (getcwd(working_directory, sizeof(working_directory)) == NULL)
   {
      perror("Error getting current working directory");
      exit(EXIT_FAILURE);
   }
 
   // Calling search function
   search_directory(argv[1], working_directory);
 
   // If the objective_file_dir is not found, print an appropriate message
   fprintf(stderr, "The target '%s' was not found in the current directory.\n", argv[1]);
   exit(EXIT_FAILURE);
}