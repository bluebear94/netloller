#include <wchar.h>

struct interval {
  int first;
  int last;
};

/* auxiliary function for binary search in interval table */
static int bisearch(wchar_t ucs, const struct interval *table, int max);

int mk_wcwidth(wchar_t ucs);

int mk_wcswidth(const wchar_t *pwcs, size_t n);

int mk_wcwidth_cjk(wchar_t ucs);

int mk_wcswidth_cjk(const wchar_t *pwcs, size_t n);
