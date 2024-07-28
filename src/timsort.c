#include "timsort.h"
#include "track.h"
#define RUN 32

static int
compare(Track* a, Track* b)
{
  gchar* a_lower = g_ascii_strdown(a->title, -1);
  gchar* b_lower = g_ascii_strdown(b->title, -1);
  int result = g_strcmp0(a_lower, b_lower);

  g_free(a_lower);
  g_free(b_lower);

  return result;
}

static void
insertionSort(GPtrArray* arr, int left, int right)
{
  for (int i = left + 1; i <= right; i++) {
    Track* temp = g_ptr_array_index(arr, i);
    int j = i - 1;
    while (j >= left && compare(g_ptr_array_index(arr, j), temp) > 0) {
      g_ptr_array_index(arr, j + 1) = g_ptr_array_index(arr, j);
      j--;
    }
    g_ptr_array_index(arr, j + 1) = temp;
  }
}

static void
merge(GPtrArray* arr, int l, int m, int r)
{
  int len1 = m - l + 1;
  int len2 = r - m;

  GPtrArray* left = g_ptr_array_sized_new(len1);
  GPtrArray* right = g_ptr_array_sized_new(len2);

  for (int i = 0; i < len1; i++)
    g_ptr_array_add(left, g_ptr_array_index(arr, l + i));
  for (int i = 0; i < len2; i++)
    g_ptr_array_add(right, g_ptr_array_index(arr, m + 1 + i));

  int i = 0, j = 0, k = l;
  while (i < len1 && j < len2) {
    if (compare(g_ptr_array_index(left, i), g_ptr_array_index(right, j)) <= 0) {
      g_ptr_array_index(arr, k++) = g_ptr_array_index(left, i++);
    } else {
      g_ptr_array_index(arr, k++) = g_ptr_array_index(right, j++);
    }
  }

  while (i < len1) {
    g_ptr_array_index(arr, k++) = g_ptr_array_index(left, i++);
  }

  while (j < len2) {
    g_ptr_array_index(arr, k++) = g_ptr_array_index(right, j++);
  }

  g_ptr_array_free(left, FALSE);
  g_ptr_array_free(right, FALSE);
}

void
timSort(GPtrArray* arr)
{
  int n = arr->len;

  for (int i = 0; i < n; i += RUN) {
    insertionSort(arr, i, MIN((i + RUN - 1), (n - 1)));
  }

  for (int size = RUN; size < n; size = 2 * size) {
    for (int left = 0; left < n; left += 2 * size) {
      int mid = MIN((left + size - 1), (n - 1));
      int right = MIN((left + 2 * size - 1), (n - 1));

      if (mid < right)
        merge(arr, left, mid, right);
    }
  }
}
