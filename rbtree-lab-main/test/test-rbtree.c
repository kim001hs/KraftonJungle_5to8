#include "rbtree.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// fork와 waitpid, 그리고 signal 설명을 위한 헤더 파일
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

/* -------------------------------------------------------------------------- */
/* 기존 테스트 함수들                                                             */
/* -------------------------------------------------------------------------- */
// new_rbtree should return rbtree struct with null root node
void test_init(void) {
  rbtree *t = new_rbtree();
  assert(t != NULL);
#ifdef SENTINEL
  assert(t->nil != NULL);
  assert(t->root == t->nil);
#else
  assert(t->root == NULL);
#endif
  delete_rbtree(t);
}

// root node should have proper values and pointers
void test_insert_single(const key_t key) {
  rbtree *t = new_rbtree();
  node_t *p = rbtree_insert(t, key);
  assert(p != NULL);
  assert(t->root == p);
  assert(p->key == key);
  // assert(p->color == RBTREE_BLACK);  // color of root node should be black
#ifdef SENTINEL
  assert(p->left == t->nil);
  assert(p->right == t->nil);
  assert(p->parent == t->nil);
#else
  assert(p->left == NULL);
  assert(p->right == NULL);
  assert(p->parent == NULL);
#endif
  delete_rbtree(t);
}

// find should return the node with the key or NULL if no such node exists
void test_find_single(const key_t key, const key_t wrong_key) {
  rbtree *t = new_rbtree();
  node_t *p = rbtree_insert(t, key);

  node_t *q = rbtree_find(t, key);
  assert(q != NULL);
  assert(q->key == key);
  assert(q == p);

  q = rbtree_find(t, wrong_key);
  assert(q == NULL);

  delete_rbtree(t);
}

// erase should delete root node
void test_erase_root(const key_t key) {
  rbtree *t = new_rbtree();
  node_t *p = rbtree_insert(t, key);
  assert(p != NULL);
  assert(t->root == p);
  assert(p->key == key);
  rbtree_erase(t, p);
#ifdef SENTINEL
  assert(t->root == t->nil);
#else
  assert(t->root == NULL);
#endif

  delete_rbtree(t);
}

static void insert_arr(rbtree *t, const key_t *arr, const size_t n) {
  for (size_t i = 0; i < n; i++) {
    rbtree_insert(t, arr[i]);
  }
}

static int comp(const void *p1, const void *p2) {
  const key_t *e1 = (const key_t *)p1;
  const key_t *e2 = (const key_t *)p2;
  if (*e1 < *e2) {
    return -1;
  } else if (*e1 > *e2) {
    return 1;
  } else {
    return 0;
  }
};

// min/max should return the min/max value of the tree
void test_minmax(key_t *arr, const size_t n) {
  // null array is not allowed
  assert(n > 0 && arr != NULL);

  rbtree *t = new_rbtree();
  assert(t != NULL);

  insert_arr(t, arr, n);
  assert(t->root != NULL);
#ifdef SENTINEL
  assert(t->root != t->nil);
#endif

  qsort((void *)arr, n, sizeof(key_t), comp);
  node_t *p = rbtree_min(t);
  assert(p != NULL);
  assert(p->key == arr[0]);

  node_t *q = rbtree_max(t);
  assert(q != NULL);
  assert(q->key == arr[n - 1]);

  rbtree_erase(t, p);
  p = rbtree_min(t);
  assert(p != NULL);
  assert(p->key == arr[1]);

  if (n >= 2) {
    rbtree_erase(t, q);
    q = rbtree_max(t);
    assert(q != NULL);
    assert(q->key == arr[n - 2]);
  }

  delete_rbtree(t);
}

void test_to_array(rbtree *t, const key_t *arr, const size_t n) {
  assert(t != NULL);

  insert_arr(t, arr, n);
  qsort((void *)arr, n, sizeof(key_t), comp);

  key_t *res = calloc(n, sizeof(key_t));
  rbtree_to_array(t, res, n);
  for (int i = 0; i < n; i++) {
    assert(arr[i] == res[i]);
  }
  free(res);
}

void test_multi_instance() {
  rbtree *t1 = new_rbtree();
  assert(t1 != NULL);
  rbtree *t2 = new_rbtree();
  assert(t2 != NULL);

  key_t arr1[] = {10, 5, 8, 34, 67, 23, 156, 24, 2, 12, 24, 36, 990, 25};
  const size_t n1 = sizeof(arr1) / sizeof(arr1[0]);
  insert_arr(t1, arr1, n1);
  qsort((void *)arr1, n1, sizeof(key_t), comp);

  key_t arr2[] = {4, 8, 10, 5, 3};
  const size_t n2 = sizeof(arr2) / sizeof(arr2[0]);
  insert_arr(t2, arr2, n2);
  qsort((void *)arr2, n2, sizeof(key_t), comp);

  key_t *res1 = calloc(n1, sizeof(key_t));
  rbtree_to_array(t1, res1, n1);
  for (int i = 0; i < n1; i++) {
    assert(arr1[i] == res1[i]);
  }

  key_t *res2 = calloc(n2, sizeof(key_t));
  rbtree_to_array(t2, res2, n2);
  for (int i = 0; i < n2; i++) {
    assert(arr2[i] == res2[i]);
  }

  free(res2);
  free(res1);
  delete_rbtree(t2);
  delete_rbtree(t1);
}

// Search tree constraint
// The values of left subtree should be less than or equal to the current node
// The values of right subtree should be greater than or equal to the current
// node

static bool search_traverse(const node_t *p, key_t *min, key_t *max,
                            node_t *nil) {
  if (p == nil) {
    return true;
  }

  *min = *max = p->key;

  key_t l_min, l_max, r_min, r_max;
  l_min = l_max = r_min = r_max = p->key;

  const bool lr = search_traverse(p->left, &l_min, &l_max, nil);
  if (!lr || l_max > p->key) {
    return false;
  }
  const bool rr = search_traverse(p->right, &r_min, &r_max, nil);
  if (!rr || r_min < p->key) {
    return false;
  }

  *min = l_min;
  *max = r_max;
  return true;
}

void test_search_constraint(const rbtree *t) {
  assert(t != NULL);
  node_t *p = t->root;
  key_t min, max;
#ifdef SENTINEL
  node_t *nil = t->nil;
#else
  node_t *nil = NULL;
#endif
  assert(search_traverse(p, &min, &max, nil));
}

// Color constraint
// 1. Each node is either red or black. (by definition)
// 2. All NIL nodes are considered black.
// 3. A red node does not have a red child.
// 4. Every path from a given node to any of its descendant NIL nodes goes
// through the same number of black nodes.

bool touch_nil = false;
int max_black_depth = 0;

static void init_color_traverse(void) {
  touch_nil = false;
  max_black_depth = 0;
}

static bool color_traverse(const node_t *p, const color_t parent_color,
                           const int black_depth, node_t *nil) {
  if (p == nil) {
    if (!touch_nil) {
      touch_nil = true;
      max_black_depth = black_depth;
    } else if (black_depth != max_black_depth) {
      printf("\ndhka\n");
      return false;
    }
    return true;
  }
  if (parent_color == RBTREE_RED && p->color == RBTREE_RED) {
    printf("\ndhka222\n");
    return false;
  }
  int next_depth = ((p->color == RBTREE_BLACK) ? 1 : 0) + black_depth;
  return color_traverse(p->left, p->color, next_depth, nil) &&
         color_traverse(p->right, p->color, next_depth, nil);
}

void test_color_constraint(const rbtree *t) {
  assert(t != NULL);
#ifdef SENTINEL
  node_t *nil = t->nil;
#else
  node_t *nil = NULL;
#endif
  node_t *p = t->root;
  assert(p == nil || p->color == RBTREE_BLACK);

  init_color_traverse();
  assert(color_traverse(p, RBTREE_BLACK, 0, nil));
}

// rbtree should keep search tree and color constraints
void test_rb_constraints(const key_t arr[], const size_t n) {
  rbtree *t = new_rbtree();
  assert(t != NULL);

  insert_arr(t, arr, n);
  assert(t->root != NULL);

  test_color_constraint(t);
  test_search_constraint(t);

  delete_rbtree(t);
}

// rbtree should manage distinct values
void test_distinct_values() {
  const key_t entries[] = {10, 5, 8, 34, 67, 23, 156, 24, 2, 12};
  const size_t n = sizeof(entries) / sizeof(entries[0]);
  test_rb_constraints(entries, n);
}

// rbtree should manage values with duplicate
void test_duplicate_values() {
  const key_t entries[] = {10, 5, 5, 34, 6, 23, 12, 12, 6, 12};
  const size_t n = sizeof(entries) / sizeof(entries[0]);
  test_rb_constraints(entries, n);
}

void test_minmax_suite() {
  key_t entries[] = {10, 5, 8, 34, 67, 23, 156, 24, 2, 12};
  const size_t n = sizeof(entries) / sizeof(entries[0]);
  test_minmax(entries, n);
}

void test_to_array_suite() {
  rbtree *t = new_rbtree();
  assert(t != NULL);

  key_t entries[] = {10, 5, 8, 34, 67, 23, 156, 24, 2, 12, 24, 36, 990, 25};
  const size_t n = sizeof(entries) / sizeof(entries[0]);
  test_to_array(t, entries, n);

  delete_rbtree(t);
}

void test_find_erase(rbtree *t, const key_t *arr, const size_t n) {
  for (int i = 0; i < n; i++) {
    node_t *p = rbtree_insert(t, arr[i]);
    assert(p != NULL);
  }
  for (int i = 0; i < n; i++) {
    node_t *p = rbtree_find(t, arr[i]);
    // printf("arr[%d] = %d\n", i, arr[i]);
    assert(p != NULL);
    assert(p->key == arr[i]);
    rbtree_erase(t, p);
  }
  for (int i = 0; i < n; i++) {
    node_t *p = rbtree_find(t, arr[i]);
    assert(p == NULL);
  }
  for (int i = 0; i < n; i++) {
    node_t *p = rbtree_insert(t, arr[i]);
    assert(p != NULL);
    node_t *q = rbtree_find(t, arr[i]);
    assert(q != NULL);
    assert(q->key == arr[i]);
    assert(p == q);
    rbtree_erase(t, p);
    q = rbtree_find(t, arr[i]);
    assert(q == NULL);
  }
}

void test_find_erase_fixed() {
  const key_t arr[] = {10, 5, 8, 34, 67, 23, 156, 24, 2, 12, 24, 36, 990, 25};
  const size_t n = sizeof(arr) / sizeof(arr[0]);
  rbtree *t = new_rbtree();
  assert(t != NULL);
  test_find_erase(t, arr, n);

  delete_rbtree(t);
}

void test_find_erase_rand(const size_t n, const unsigned int seed) {
  srand(seed);
  rbtree *t = new_rbtree();
  key_t *arr = calloc(n, sizeof(key_t));
  for (int i = 0; i < n; i++) {
    arr[i] = rand();
  }

  test_find_erase(t, arr, n);

  free(arr);
  delete_rbtree(t);
}


/* -------------------------------------------------------------------------- */
/* 테스트를 도와주는 헬퍼                                                          */
/* -------------------------------------------------------------------------- */


typedef enum { TEST_PASSED, TEST_FAILED } test_status_t;

typedef struct {
  const char *name;
  const char *args;
  test_status_t status;
  int signal;
} test_result_t;

const char *get_used_functions(const char *test_name) {
  if (strcmp(test_name, "test_init") == 0) return "new_rbtree, delete_rbtree";
  if (strcmp(test_name, "test_insert_single") == 0) return "new_rbtree, rbtree_insert, delete_rbtree";
  if (strcmp(test_name, "test_find_single") == 0) return "new_rbtree, rbtree_insert, rbtree_find, delete_rbtree";
  if (strcmp(test_name, "test_erase_root") == 0) return "new_rbtree, rbtree_insert, rbtree_erase, delete_rbtree";
  if (strcmp(test_name, "test_minmax_suite") == 0) return "new_rbtree, rbtree_insert, rbtree_min, rbtree_max, rbtree_erase, delete_rbtree";
  if (strcmp(test_name, "test_to_array_suite") == 0) return "new_rbtree, rbtree_insert, rbtree_to_array, delete_rbtree";
  if (strcmp(test_name, "test_distinct_values") == 0) return "new_rbtree, rbtree_insert, delete_rbtree (그리고 RB-Tree 규칙 검사)";
  if (strcmp(test_name, "test_duplicate_values") == 0) return "new_rbtree, rbtree_insert, delete_rbtree (그리고 RB-Tree 규칙 검사)";
  if (strcmp(test_name, "test_multi_instance") == 0) return "new_rbtree, rbtree_insert, rbtree_to_array, delete_rbtree";
  if (strcmp(test_name, "test_find_erase_fixed") == 0) return "new_rbtree, rbtree_insert, rbtree_find, rbtree_erase, delete_rbtree";
  if (strcmp(test_name, "test_find_erase_rand") == 0) return "new_rbtree, rbtree_insert, rbtree_find, rbtree_erase, delete_rbtree";
  return "알 수 없음";
}

const char *get_test_description(const char *test_name) {
  if (strcmp(test_name, "test_init") == 0) return "새로운 RB-Tree가 올바르게 초기화되는지 확인해요! (tree->root == tree->nil)";
  if (strcmp(test_name, "test_insert_single") == 0) return "노드 한 개를 삽입했을 때, root가 잘 설정되는지 확인해요!";
  if (strcmp(test_name, "test_find_single") == 0) return "노드 한 개를 넣고, 잘 찾아지는지 확인해요!";
  if (strcmp(test_name, "test_erase_root") == 0) return "단 하나뿐인 루트 노드를 지웠을 때, 트리가 비어있는 상태가 되는지 확인해요!";
  if (strcmp(test_name, "test_find_erase_fixed") == 0) return "미리 정해진 값들을 넣고, 순서대로 잘 지워지는지 확인해요!";
  if (strcmp(test_name, "test_minmax_suite") == 0) return "여러 값을 넣었을 때, 최솟값과 최댓값을 잘 찾는지 확인해요!";
  if (strcmp(test_name, "test_to_array_suite") == 0) return "트리의 모든 값을 배열로 잘 변환하는지 확인해요!";
  if (strcmp(test_name, "test_distinct_values") == 0) return "중복 없는 값들을 넣었을 때, RB-Tree의 규칙(색깔, 순서)을 잘 지키는지 확인해요!";
  if (strcmp(test_name, "test_duplicate_values") == 0) return "중복 있는 값들을 넣었을 때, RB-Tree의 규칙(색깔, 순서)을 잘 지키는지 확인해요!";
  if (strcmp(test_name, "test_multi_instance") == 0) return "여러 개의 트리를 동시에 만들어도 서로 영향을 주지 않는지 확인해요!";
  if (strcmp(test_name, "test_find_erase_rand") == 0) return "랜덤한 값들을 넣고 지웠을 때도, 문제가 없는지 확인해요!";
  return "설명이 없는 테스트에요...";
}

#define MAX_TESTS 11
test_result_t results[MAX_TESTS];
int test_count = 0;

#define RUN_TEST(test_func, ...)                                               \
  do {                                                                         \
                                                                                 \
    printf("\n----------------------------------\n");                           \
    printf("👉 다음 테스트: [%s(%s)]", #test_func, #__VA_ARGS__);              \
    printf("\n   계속하려면 Enter 키를 눌러주세요...");                         \
    while(getchar() != '\n' && getchar() != EOF); /* 버퍼를 비우며 Enter를 기다려요 */ \
    \
    results[test_count].name = #test_func;                                     \
    results[test_count].args = #__VA_ARGS__;                                   \
    \
    pid_t pid = fork();                                                      \
    if (pid == -1) {                                                         \
      perror("fork"); exit(1);                                               \
    } else if (pid == 0) { /* 자식 프로세스 */                                \
      test_func(__VA_ARGS__);                                                  \
      exit(0); /* 성공! */                                                   \
    } else { /* 부모 프로세스 */                                             \
      int status;                                                            \
      waitpid(pid, &status, 0);                                              \
      if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {                   \
        results[test_count].status = TEST_PASSED;                            \
        printf("  -> ✅ 통과!\n");                                            \
      } else {                                                               \
        results[test_count].status = TEST_FAILED;                            \
        if (WIFSIGNALED(status)) {                                           \
          results[test_count].signal = WTERMSIG(status);                     \
        } else {                                                             \
          results[test_count].signal = 0;                                    \
        }                                                                    \
        printf("  -> ❌ 실패!\n");                                            \
      }                                                                      \
    }                                                                          \
    test_count++;                                                              \
  } while (0)

int main(void) {
  printf("🎉 RB-Tree 테스트를 시작합니다! 🎉\n");

  RUN_TEST(test_init);
  RUN_TEST(test_insert_single, 1024);
  RUN_TEST(test_find_single, 512, 1024);
  RUN_TEST(test_erase_root, 128);
  RUN_TEST(test_find_erase_fixed);
  RUN_TEST(test_minmax_suite);
  RUN_TEST(test_to_array_suite);
  RUN_TEST(test_distinct_values);
  RUN_TEST(test_duplicate_values);
  RUN_TEST(test_multi_instance);
  RUN_TEST(test_find_erase_rand, 10000, 17);

  int passed_count = 0;
  int failed_count = 0;
  
  printf("\n\n---\n");
  printf("📋✨ 최종 테스트 결과 보고서 ✨📋\n");
  printf("---\n");

  for (int i = 0; i < test_count; i++) {
    if (results[i].status == TEST_FAILED) {
      if (failed_count == 0) {
        printf("\n❌ 실패한 테스트 ❌\n");
      }
      failed_count++;
      printf("\n[%d] %s(%s)\n", failed_count, results[i].name, results[i].args);
      printf("   - 설명: %s\n", get_test_description(results[i].name));
      printf("   - 사용 함수: %s\n", get_used_functions(results[i].name));
      if (results[i].signal != 0) {
        printf("   - 발생 오류: %s (Signal %d)\n", strsignal(results[i].signal), results[i].signal);
        if (results[i].signal == SIGABRT) {
          printf("   - 힌트: assert() 문이 실패하면 보통 이 오류가 발생해요!\n");
        } else if (results[i].signal == SIGSEGV) {
          printf("   - 힌트: 잘못된 메모리 주소에 접근했어요! 포인터를 확인해보세요!\n");
        }
      }
    } else if (results[i].status == TEST_PASSED) {
      passed_count++;
    }
  }

  if (failed_count == 0) {
      printf("\n❌ 실패한 테스트가 하나도 없어요! 완벽해요! ❌\n");
  }

  printf("\n✅ 통과한 테스트 ✅\n");
  if (passed_count == 0) {
      printf("   통과한 테스트가 없어요... ( •́ ̯•̀ )\n");
  } else {
    for (int i = 0; i < test_count; i++) {
      if (results[i].status == TEST_PASSED) {
        printf("   - %s(%s)\n", results[i].name, results[i].args);
      }
    }
  }

  printf("\n----------------------------------\n");
  printf("총 %d개 테스트 중, ✅ 통과: %d개, ❌ 실패: %d개\n", test_count, passed_count, failed_count);
  printf("----------------------------------\n\n");

  return failed_count > 0 ? 1 : 0;
}
