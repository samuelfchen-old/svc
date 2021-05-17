#ifndef svc_h
#define svc_h

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_FILE_PATH 260
#define MAX_BRANCH_NAME 50

typedef struct resolution {
    // NOTE: DO NOT MODIFY THIS STRUCT
    char *file_name;
    char *resolved_file;
} resolution;

typedef char commit_id[7];

struct svc_file {
  char path[MAX_FILE_PATH + 1];
  char stored_path[MAX_FILE_PATH + 20];
  int hash;
  int initial_hash;
};

struct commit {
  commit_id id;
  char* message;
  char branch_name[MAX_BRANCH_NAME + 1];

  struct svc_file* files;
  int n_files;

  commit_id* prev;
  size_t n_prev;
};

struct branch {
  char name[MAX_BRANCH_NAME + 1];

  commit_id commit;

  struct svc_file* tracked;
  size_t n_tracked;
  size_t tracked_capacity;
};

struct helper {
  int head;

  struct branch* branches;
  size_t n_branches;
  size_t branch_capacity;

  struct commit* commits;
  size_t n_commits;
  size_t commit_capacity;
};

int create_file(char* path);
int copy_file(char* dest_path, char* src_path, int check_hash);
// int copy_file(char* dest_path, char* src_path);
int get_status(struct svc_file* f);
int cmpfunc (const void * a, const void * b);
int uncommitted_changes(struct branch* b);
struct branch* add_branch(void* helper);
struct commit* add_commit(void* helper);


void *svc_init(void);

void cleanup(void *helper);

int hash_file(void *helper, char *file_path);

char *svc_commit(void *helper, char *message);

void *get_commit(void *helper, char *commit_id);

char **get_prev_commits(void *helper, void *commit, int *n_prev);

void print_commit(void *helper, char *commit_id);

int svc_branch(void *helper, char *branch_name);

int svc_checkout(void *helper, char *branch_name);

char **list_branches(void *helper, int *n_branches);

int svc_add(void *helper, char *file_name);

int svc_rm(void *helper, char *file_name);

int svc_reset(void *helper, char *commit_id);

char *svc_merge(void *helper, char *branch_name, resolution *resolutions, int n_resolutions);

#endif
