#include "svc.h"

/**
 * Creates a new file and any necessary folders at a path
 * @return
 *  0 if successful
 *  1 if failed
 */
int create_file(char* path) {
  char temp[MAX_FILE_PATH + 20];
  strcpy(temp, path);

  // terminate path at end of folders
  char* end = strrchr(temp, '/');
  if (end != NULL) *end = '\0';
  else *temp = '\0';

  // create each folder in the path
  char* tok = strtok(temp, "/");
  char agg[MAX_FILE_PATH + 20] = "";
  while (tok != NULL) {
    strcat(agg, tok);
    strcat(agg, "/");
    mkdir(agg, 0777);
    tok = strtok(NULL, "/");
  }

  // create the file
  FILE* fp = fopen(path, "w+");
  if (fp == NULL) printf("It didnt work...\n");
  fclose(fp);
  return 0;
}

/**
 * Copies a file from src to dest
 * @param check_hash: hash to check against the destination's hash to see if
 *                    a change has occurred and a copy is necessary
 * @return
 *  0 if successful
 *  1 if failed
 */
int copy_file(char* dest_path, char* src_path, int check_hash) {
  // check if the file has changed enough to require an overwrite
  int hash = hash_file(NULL, dest_path);
  if (check_hash == hash) return 0;

  // create file if it does not exist
  if (hash == -2) create_file(dest_path);

  // open the source and destination files
  FILE* dest = fopen(dest_path, "w+");
  if (dest == NULL) {
    return -1;
  }
  FILE* src = fopen(src_path, "r");
  if (src == NULL) {
    if (dest != NULL) fclose(dest);
    return -1;
  }

  // copy contents over
  int ch;
  while( ( ch = fgetc(src) ) != EOF )
    fputc(ch, dest);
  if (dest != NULL) fclose(dest);
  if (src != NULL) fclose(src);

  return 0;
}

/**
 * Returns the status of a file through comparing initial_hash and current_hash
 * @return
 *  0 if add
 *  1 if remove
 *  2 if modify
 *  -1 if no change
 */
int get_status(struct svc_file* f) {
  if (f->hash == f->initial_hash) {
    return -1;
  }
  if (f->hash == -1) {
    if (f->initial_hash != -1) {
      return 1;
    }
  } else {
    if (f->initial_hash == -1) {
      return 0;
    } else {
      return 2;
    }
  }
  return -1;
}

/**
 * Adds an empty branch
 * @return  pointer to new branch
 */
struct branch* add_branch(void* helper) {
  struct helper* h = (struct helper*) helper;
  h->n_branches += 1;
  if (h->n_branches > h->branch_capacity) {
    h->branch_capacity = h->branch_capacity * 1.5;
    h->branches = realloc(h->branches, h->branch_capacity * sizeof(struct branch));
  }
  struct branch* new_branch = &h->branches[h->n_branches - 1];

  new_branch->tracked_capacity = 10;
  new_branch->n_tracked = 0;
  new_branch->tracked = malloc(new_branch->tracked_capacity * sizeof(struct svc_file));
  strcpy(new_branch->commit, "zzzzzz");
  return new_branch;
}

/**
 * Add an empty commit and sets its previous commit
 * @return   pointer to new commit
 */
struct commit* add_commit(void* helper) {
  struct helper* h = (struct helper*) helper;
  struct branch* head = &h->branches[h->head];

  h->n_commits += 1;
  if (h->n_commits > h->commit_capacity) {
    h->commit_capacity = h->commit_capacity * 1.5;
    h->commits = realloc(h->commits, h->commit_capacity * sizeof(struct commit));
  }
  struct commit* new_commit = &h->commits[h->n_commits - 1];

  strcpy(new_commit->branch_name, head->name);
  new_commit->prev = malloc(2 * sizeof(commit_id));
  if (strcmp(head->commit, "zzzzzz") == 0) {
    new_commit->n_prev = 0;
  } else {
    new_commit->n_prev = 1;
    strcpy(new_commit->prev[0], head->commit);
  }

  new_commit->n_files = 0;
  return new_commit;
}

/**
 * Used for quicksort
 */
int cmpfunc (const void * a, const void * b) {
  return strcasecmp((char*) a, (char*) b) > 0;
}

/**
 * Checks if a branch has uncommitted changes
 * @return
 *  1 if yes
 *  0 if no
 */
int uncommitted_changes(struct branch* b) {
  for (int i = 0; i < b->n_tracked; i++) {
    struct svc_file curr = b->tracked[i];
    if (curr.initial_hash != -1 || curr.hash != -1) {
      int hash = hash_file(NULL, curr.path);
      if (hash == -2) hash = -1;
      if (hash != curr.initial_hash) return 1;
    }
  }
  return 0;
}

/**
 * Initialise everything
 * @return  helper
 */
void *svc_init(void) {
  struct helper* h = malloc(sizeof(struct helper));
  // init branch
  h->n_branches = 0;
  h->branch_capacity = 10;
  h->branches = malloc(h->branch_capacity * sizeof(struct branch));
  struct branch* master = add_branch(h);
  strcpy(master->name, "master");
  h->head = 0;

  // init commits
  h->n_commits = 0;
  h->commit_capacity = 10;
  h->commits = malloc(h->commit_capacity * sizeof(struct commit));

  return h;
}

/**
 * Cleanup everything
 */
void cleanup(void *helper) {
  struct helper* h = (struct helper*) helper;
  // cleanup branches
  for (int i = 0; i < h->n_branches; i++) {
    free(h->branches[i].tracked);
    h->branches[i].tracked = NULL;
  }
  free(h->branches);
  h->branches = NULL;

  // cleanup commits
  for (int i = 0; i < h->n_commits; i++) {
    free(h->commits[i].files);
    h->commits[i].files = NULL;
    free(h->commits[i].prev);
    h->commits[i].prev = NULL;
    free(h->commits[i].message);
    h->commits[i].message = NULL;
  }
  free(h->commits);
  h->commits = NULL;

  // cleanup helper
  free(h);
  h = NULL;

  // delete commits folder
  system("rm -r -f commits");
  return;
}

/**
 * Generates file hash
 * @return
 *  hash
 *  -1 if file path null
 *  -2 if no file exists at given path
 */
int hash_file(void *helper, char *file_path) {
  if (file_path == NULL) return -1;

  FILE* fp = fopen(file_path, "r");
  if (fp == NULL) {
    return -2;
  }

  int hash = 0;
  int c;

  // loop thru file_path
  for (int i = 0; i < strlen(file_path); i++) {
    hash = (hash + (int)(file_path[i])) % 1000;
  }
  // loop thru file
  while ((c = fgetc(fp)) != EOF) {
    hash = (hash + c) % 2000000000;
  }

  fclose(fp);
  return hash;
}

/**
 * Creates a commit
 * @return
 *  NULL if no changes since last commit or message is NULL
 *  commit id if successful
 */
char *svc_commit(void *helper, char *message) {
  struct helper* h = (struct helper*) helper;
  struct branch* head = &h->branches[h->head];

  if (NULL == message) return NULL;

  // Generate the commit id, keeping track of number of changes
  int id = 0;
  for (int i = 0; i < strlen(message); i++) {
    id = (id + ((int) message[i]) ) % 1000;
  }

  int n_changes = 0;

  for (int i = 0; i < head->n_tracked; i++) {
    struct svc_file* curr = &head->tracked[i];
    if (curr->hash != -1) {
      curr->hash = hash_file(helper, curr->path);
      if (curr->hash == -2) curr->hash = -1;
    }

    int status = get_status(curr);

    switch (status) {
      case 0: // add
        id = id + 376591;
        n_changes++;
        break;
      case 1: // remove
        id = id + 85973;
        n_changes++;
        break;
      case 2: // modify
        id = id + 9573681;
        n_changes++;
        break;
    }
    if (status != -1) {
      for (int i = 0; i < strlen(curr->path); i++) {
        id = ( id * ((int) curr->path[i] % 37) ) % 15485863 + 1;
      }
    }
  }

  // if no changes detected return NULL
  if (!n_changes) return NULL;

  // convert to hexadecimal string
  commit_id hex = "000000";
  commit_id temp;
  sprintf(temp, "%x", id);
  strcpy(hex+6-strlen(temp), temp);

  // create commit
  struct commit* new_commit = add_commit(helper);
  strcpy(new_commit->id, hex);

  // copy tracked files into the commit
  new_commit->files = malloc(head->n_tracked * sizeof(struct svc_file));
  new_commit->n_files = head->n_tracked;
  for (int i = 0; i < new_commit->n_files; i++) {
    // copy file
    new_commit->files[i] = head->tracked[i];
    // update initial_hash of tracked files
    head->tracked[i].initial_hash = head->tracked[i].hash;
  }

  strcpy(new_commit->branch_name, head->name);
  new_commit->message = strdup(message);

  // point the branch's commit to the new commit
  strcpy(head->commit, new_commit->id);

  // backup all tracked files (not deleted)

  char stored_path[MAX_FILE_PATH+20];
  int match = 0;
  for (int i = 0; i < new_commit->n_files; i++) {
    match = 0;
    // struct svc_file* current_file = &new_commit->files[i];
    if (new_commit->files[i].hash != -1) {
      // See if the hash matches any hashes in prev commit
      // if so, point stored_path to the stored_path of the file in prev commit
      struct commit* prev = get_commit(helper, new_commit->prev[0]);
      if (prev != NULL) {
        for (int j = 0; j < prev->n_files; j++) {
          if (new_commit->files[i].hash == prev->files[j].hash) {
            strcpy(new_commit->files[i].stored_path, prev->files[j].stored_path);
            match = 1;
          }
        }
      }

      if (!match) {
        // if hash does not exist in prev commit (file has changed), store a new copy
        sprintf(stored_path, "commits/%s/%s", new_commit->id, new_commit->files[i].path);
        copy_file(stored_path, new_commit->files[i].path, -1);
        strcpy(new_commit->files[i].stored_path, stored_path);
      }
    }
  }

  return new_commit->id;
}

/**
 *
 * @return
 *  Pointer to commit
 *  NULL if commit does not exist, commit_id is NULL
 */
void *get_commit(void *helper, char *commit_id) {
  struct helper* h = (struct helper*) helper;

  if (NULL == commit_id) return NULL;

  for (int i = 0; i < h->n_commits; i++) {
    if (strcmp(h->commits[i].id, commit_id) == 0) {
      return &h->commits[i];
    }
  }

  return NULL;
}

/**
 * Gets list of parent commits. If commit is NULL, or it is the very first
 * commit, this function should set the contents of n_prev to 0 and return NULL
 * @return
 *  NULL if n_prev is null, commit is NULl or is first commit
 */
char **get_prev_commits(void *helper, void *commit, int *n_prev) {
  struct commit * c = (struct commit *) commit;

  if (commit == NULL) {
    *n_prev = 0;
  } else {
    *n_prev = c->n_prev;
  }
  if (*n_prev == 0) return NULL;

  char** ret = malloc(c->n_prev * sizeof(char *));
  if (ret == NULL) printf("Could not malloc return in get_prev_commits\n");
  for (int i = 0; i < c->n_prev; i++) {
    ret[i] = c->prev[i];
  }
  *n_prev = c->n_prev;

  return ret;
}

/**
 * Prints commit in this format:
 * 74cde7 [master]: Initial commit
   + Tests/test1.in
   + hello.py
   Tracked files (2):
   [       564] Tests/test1.in
   [      2027] hello.py
 */
void print_commit(void *helper, char *commit_id) {
  void* commit = get_commit(helper, commit_id);

  if (commit == NULL) {
      printf("Invalid commit id\n");
      return;
  }
  struct commit * c = (struct commit*) commit;

  printf("%s [%s]: %s\n", c->id, c->branch_name, c->message);

  int n_changes = 0;
  for (int i = 0; i < c->n_files; i++) {
      int status = get_status(&c->files[i]);
      switch (status) {
        case 0: // add
          printf("    + %s\n", c->files[i].path);
          n_changes++;
          break;
        case 1: // remove
          printf("    - %s\n", c->files[i].path);
          n_changes++;
          break;
        case 2: // change
          printf("    / %s [%10d -> %10d]\n", c->files[i].path, c->files[i].initial_hash, c->files[i].hash);
          n_changes++;
          break;
      }
  }
  // Count how many files there are that still exist
  int n_tracked = 0;
  for (int i = 0; i < c->n_files; i++) {
      if (c->files[i].hash != -1) {
        n_tracked++;
      }
  }
  printf("\n    Tracked files (%d):\n", n_tracked);
  for (int i = 0; i < c->n_files; i++) {
      if (c->files[i].hash != -1) {
        printf("    [%10d] %s\n", c->files[i].hash, c->files[i].path);
      }
  }
  return;
}

/**
 * Creates a new branch
 * @return
 *  -1 if branch name is invalid or null
 *  -2 if branch name already exists
 *  -3 if there are uncommitted changes
 *  0 if successful
 *
 */
int svc_branch(void *helper, char *branch_name) {
  struct helper* h = (struct helper*) helper;
  struct branch* head = &h->branches[h->head];

  // check if branch name valid
  if (NULL == branch_name) return -1;
  const char* valid = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMOPQRSTUVWXYZ0123456789_/-";
  char* check = branch_name;
  while (*check) {
    if (strchr(valid, *check) == NULL) {
      return -1;
    }
    check++;
  }

  // check if branch already exists
  for (int i = 0; i < h->n_branches; i++) {
    if (strcmp(h->branches[i].name, branch_name) == 0) {
      return -2;
    }
  }

  // check for uncommitted changes
  if (uncommitted_changes(head)) return -3;

  // create new branch
  struct branch* new_branch = add_branch(helper);
  // update head pointer (in the case of realloc)
    head = &h->branches[h->head];

  // set attr
  strcpy(new_branch->commit, head->commit);
  strcpy(new_branch->name, branch_name);

  new_branch->n_tracked = head->n_tracked;
  if (new_branch->n_tracked > new_branch->tracked_capacity) {
    new_branch->tracked_capacity = new_branch->tracked_capacity * 1.5;
    new_branch->tracked = realloc(new_branch->tracked, new_branch->tracked_capacity * sizeof(struct svc_file));
  }
  for (int i = 0; i < head->n_tracked; i++) {
    strcpy(new_branch->tracked[i].path, head->tracked[i].path);
    new_branch->tracked[i].hash = head->tracked[i].hash;
    new_branch->tracked[i].initial_hash = head->tracked[i].initial_hash;
  }

  return 0;
}

/**
 * Switches to a different branch. Should restore to the state of the last commit
 * on new branch
 * @return
 *  -1 if branch_name is NULL or no branch exists
 *  -2 if uncommitted changes
 *  0 if successful
 */
int svc_checkout(void *helper, char *branch_name) {
  struct helper* h = (struct helper*) helper;
  struct branch* head = &h->branches[h->head];

  if (NULL == branch_name) return -1;

  if (uncommitted_changes(head)) return -2;

  // see if branch exists
  int found = 0;
  for (int i = 0; i < h->n_branches; i++) {
    if (strcmp(h->branches[i].name, branch_name) == 0) {
      // change the head
      head = &h->branches[i];
      h->head = i;
      found = 1;
    }
  }

  if (!found) return -1;

  // reset to the branch's commit
  svc_reset(helper, head->commit);

  return 0;
}

/**
 * List the branches in order of creation
 * @return
 *  NULL if n_branches is NULL
 *  A list of pointers to branch names if successful
 */
char **list_branches(void *helper, int *n_branches) {
  struct helper* h = (struct helper*)helper;

  if (n_branches == NULL) return NULL;
  *n_branches = h->n_branches;
  char** ret = malloc(h->n_branches * sizeof(char*));
  for (int i = 0; i < h->n_branches; i++) {
    printf("%s\n", h->branches[i].name);
    ret[i] = h->branches[i].name;
  }

  return ret;
}

/**
 * Add a file to version control
 * @return
 *  -1 if file_name is NULL
 *  -2 if already being tracked
 *  -3 if file does not exist
 *  hash value of file if successful
 */
int svc_add(void *helper, char *file_name) {
  struct helper* h = (struct helper*) helper;
  struct branch* head = &h->branches[h->head];

  if (NULL == file_name) return -1;

  int hash = hash_file(helper, file_name);
  if (hash == -2) return -3;

  int exists = 0;

  // Check in tracked
  for (int i = 0; i < head->n_tracked; i++) {
    if (strcmp(head->tracked[i].path, file_name) == 0) {
      if (head->tracked[i].hash != -1) return -2;
      else {
        head->tracked[i].hash = hash;
        exists = 1;
      }
    }
  }

  if (!exists) {
    // Add to tracked
    head->n_tracked += 1;
    if (head->n_tracked > head->tracked_capacity) {
      head->tracked_capacity = head->tracked_capacity * 1.5;
      head->tracked = realloc(head->tracked, head->tracked_capacity * sizeof(struct svc_file));
    }
    struct svc_file* new_add = &head->tracked[head->n_tracked-1];

    strcpy(new_add->path, file_name);
    new_add->hash = hash;
    new_add->initial_hash = -1;

  }

  // sort tracked files
  qsort(head->tracked, head->n_tracked, sizeof(struct svc_file), cmpfunc);

  return hash;
}

/**
 * Removes a file from version control
 * @return
 *  -1 if file name is NULL
 *  -2 if file is not being tracked on current branch
 *  last known hash
 */
int svc_rm(void *helper, char *file_name) {
  struct helper* h = (struct helper*)helper;
  struct branch* head = &h->branches[h->head];

  if (file_name == NULL) return -1;

  for (int i = 0; i < head->n_tracked; i++) {
    if (strcmp(head->tracked[i].path, file_name) == 0) {
      // change hash to -1
      struct svc_file* f = &head->tracked[i];
      if (f->hash == -1) return -2;
      int hash = f->hash;
      f->hash = -1;
      return hash;
    }
  }
  return -2;
}

/**
 * Resets files to specific commit (along current branch)
 * @return
 *  -1 if commit id is NULL
 *  -2 if no commit under id exitst
 *  0 if successful
 */
int svc_reset(void *helper, char *commit_id) {
  struct helper* h = (struct helper*) helper;
  struct branch* head = &h->branches[h->head];

  if (NULL == commit_id) return -1;

  struct commit* restore = (struct commit*) get_commit(helper, commit_id);

  if (NULL == restore) return -2;

  // Restore tracked files
  for (int i = 0; i < restore->n_files; i++) {
    if (restore->files[i].hash != -1) copy_file(restore->files[i].path, restore->files[i].stored_path, restore->files[i].hash);
  }

  // Copy branch's tracked files to the commit's tracked files
  head->n_tracked = restore->n_files;
  if (head->n_tracked > head->tracked_capacity) {
    head->tracked_capacity = head->tracked_capacity * 1.5;
    head->tracked = realloc(head->tracked, head->tracked_capacity * sizeof(struct svc_file));
  }

  for (int i = 0; i < restore->n_files; i++) {
    head->tracked[i] = restore->files[i];
    head->tracked[i].initial_hash = head->tracked[i].hash;
  }

  if (commit_id != head->commit) strcpy(head->commit, commit_id);

  return 0;
}

/**
 * Merges two branches
 * @return
 *  NULL if branch_name is null
 *  NULL if branch not found
 *  NULL if uncommitted changes
 *  NULL if trying to merge branch with itself
 *  new commit id if successful
 */
char *svc_merge(void *helper, char *branch_name, struct resolution *resolutions, int n_resolutions) {
  struct helper* h = (struct helper*) helper;
  struct branch* head = &h->branches[h->head];

  if (NULL == branch_name) {
    printf("Invalid branch name\n");
    return NULL;
  }

  if (uncommitted_changes(head)) {
    printf("Changes must be committed\n");
    return NULL;
  }

  int found = 0;
  struct branch* merge = NULL;
  for (int i = 0; i < h->n_branches; i++) {
    if (strcmp(h->branches[i].name, branch_name) == 0) {
      merge = &h->branches[i];
      found = 1;
    }
  }
  if (!found) {
    printf("Branch not found\n");
    return NULL;
  }

  if (head == merge) {
    printf("Cannot merge a branch with itself\n");
    return NULL;
  }

  for (int i = 0; i < merge->n_tracked; i++) {
    // Update any necessary files
    if (merge->tracked[i].hash != -1) {
      copy_file(merge->tracked[i].path, merge->tracked[i].stored_path, merge->tracked[i].hash);

      // add to tracked files
      svc_add(helper, merge->tracked[i].path);
    }
  }

  // resolutions
  for (int i = 0; i < n_resolutions; i++) {
    if (resolutions[i].resolved_file != NULL) {
      copy_file(resolutions[i].file_name, resolutions[i].resolved_file, -1);
      svc_add(helper, resolutions[i].file_name);
    } else {
      remove(resolutions[i].file_name);
    }
  }

  // commit
  char commit_message[MAX_BRANCH_NAME+30];
  sprintf(commit_message, "Merged branch %s", merge->name);
  char* new_commit_id = svc_commit(helper, commit_message);
  struct commit* new_commit = get_commit(helper, new_commit_id);

  new_commit->n_prev += 1;
  strcpy(new_commit->prev[new_commit->n_prev - 1], merge->commit);

  printf("Merge successful\n");
  return new_commit_id;
}
