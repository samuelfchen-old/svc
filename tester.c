#include <assert.h>
#include "svc.h"

void example1() {
  void *helper = svc_init();

  assert(hash_file(helper, "hello.py") == 2027);

  assert(hash_file(helper, "fake.c") == -2);

  assert(svc_commit(helper, "No changes") == NULL);

  assert(svc_add(helper, "Tests/test1.in") == 564);


  assert(svc_add(helper, "hello.py") == 2027);


  assert(svc_add(helper, "Tests/test1.in") == -2);

  // assert(strcmp(svc_commit(helper, "Initial commit"), "74cde7") == 0);
  printf("%s\n" ,svc_commit(helper, "Initial commit"));

  void *commit = get_commit(helper, "74cde7");

  int n_prev;
  char **prev_commits = get_prev_commits(helper, commit, &n_prev);
  assert(prev_commits == NULL);
  free(prev_commits);
  assert(n_prev == 0);

  print_commit(helper, "74cde7");

  int n;
  char **branches = list_branches(helper, &n);
  assert(n == 1);
  free(branches);

  cleanup(helper);
  return;
}

void reset_example() {
  void *helper = svc_init();

  svc_add(helper, "hello.py");

  char* id = svc_commit(helper, "first commit");

  system("rm -r hello.py");

  svc_reset(helper, id);

  cleanup(helper);
}

void branch() {
  void *helper = svc_init();

  assert(hash_file(helper, "hello.py") == 2027);

  assert(hash_file(helper, "fake.c") == -2);

  assert(svc_commit(helper, "No changes") == NULL);

  assert(svc_add(helper, "Tests/test1.in") == 564);


  assert(svc_add(helper, "hello.py") == 2027);


  assert(svc_add(helper, "Tests/test1.in") == -2);

  // assert(strcmp(svc_commit(helper, "Initial commit"), "74cde7") == 0);
  printf("ID: %s\n" ,svc_commit(helper, "Initial commit"));

  //branch
  svc_branch(helper, "branch1");
  svc_checkout(helper, "branch1");

  // printf()
  remove("hello.py");

  char* id = svc_commit(helper, "commit 2");

  print_commit(helper, id);

  cleanup(helper);

}

void self_example() {
  void *helper = svc_init();

  // TODO: write your own tests here
  // Hint: you can use assert(EXPRESSION) if you want
  // e.g.  assert((2 + 3) == 5);

  int add_1 = svc_add(helper, "svc.h");

  // printf("%d\n", add_1);

  char* id = svc_commit(helper, "commit 1");
  int add_2 = svc_add(helper, "a/hello.txt");

  char* id2 = svc_commit(helper, "commit 2");

  int rm_1 = svc_rm(helper, "svc.h");
  char* id3 = svc_commit(helper, "commit 3");

  struct helper * h = (struct helper *) helper;
  printf("%s\n", id3);
  // printf("%s\n", h->head->prev_commit->id);

  struct commit * c = get_commit(helper, "31afb7");

  // printf("%ld\n", c->n_files);
  // printf("%s\n", c->files[0].name);
  //
  // printf("%s\n", c->prev[0]->id);
  // printf("%s\n", c->id);

  int n_prev;
  char** i = get_prev_commits(helper, c, &n_prev);
  printf("%d\n", n_prev);
  printf("%s\n", *i);
  free(i);

  cleanup(helper);
}

void other() {
  void *helper = svc_init();
  svc_add(helper, "../e.pdf");
  char* init_c = svc_commit(helper, "Initial Commit");

  svc_rm(helper, "invalid.c");
  svc_rm(helper, "../e.pdf");

  svc_commit(helper, "Removed pdf");

  svc_branch(helper, "Second Branch");

  svc_reset(helper, init_c);

  svc_checkout(helper, "Second Branch");
  svc_add(helper, "../duck-fat.MP4");
  // svc_add(helper, "file4.jpg");
  svc_commit(helper, "Second Branch Commit");
  svc_checkout(helper, "master");
  svc_merge(helper, "Second Branch Commit", NULL, 0);

  // TODO: write your own tests here
  // Hint: you can use assert(EXPRESSION) if you want
  // e.g.  assert((2 + 3) == 5);

  cleanup(helper);
}

void long_test() {
  void* helper = svc_init();

  svc_add(helper, "../duck-fat.MP4");

  svc_add(helper, "../10.zip");

  char* id = strdup(svc_commit(helper, "first commit"));

  printf("first id: %s\n", id);

  svc_rm(helper, "../duck-fat.MP4");

  char* id2 = strdup(svc_commit(helper, "second commit"));
  printf("second id: %s\n", id2);
  print_commit(helper, id2);

  svc_branch(helper, "fix");

  svc_reset(helper, id);

  svc_checkout(helper, "fix");

  FILE* f = fopen("new.file", "w+");
  svc_add(helper, "new.file");
  char* id3 = strdup(svc_commit(helper, "third commit"));
  fclose(f);


  svc_checkout(helper, "master");

  int n_res = 0;

  char* id4 = svc_merge(helper, "fix", NULL, n_res);

  // print_commit(helper, id4);

  free(id);
  free(id2);
  free(id3);
  // free(id4);

  cleanup(helper);
}

void short_test() {
  void* helper = svc_init();
  svc_add(helper, "10.zip");
  svc_commit(helper, "first commit");

  cleanup(helper);
}

int main(int argc, char **argv) {
    // example1();
    // reset_example();
    // branch();
    long_test();
    // other();
    // short_test();
    return 0;
}
