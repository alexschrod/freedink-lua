#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dinkc_sp_custom_hash.h"

# include <stdbool.h>
struct str_int
{
  char key[200]; // same size as slist[0][]
  int val;
};

/* Auxiliary functions for hash */
static size_t dinkc_sp_custom_hasher(const void *x, size_t tablesize)
{
  return hash_string(((struct str_int*)x)->key, tablesize);
  // We could also call 'hash_pjw' from module 'hash-pjw'
}
static bool dinkc_sp_custom_comparator(const void* a, const void* b)
{
  return !strcmp(((struct str_int*)a)->key,
		 ((struct str_int*)b)->key);
}

/**
 * Return a new hash table
 */
dinkc_sp_custom_hash dinkc_sp_custom_new()
{
  Hash_tuning* default_tuner = NULL;
  int start_size = 10;

  return hash_initialize(start_size, default_tuner,
			 dinkc_sp_custom_hasher, dinkc_sp_custom_comparator,
			 free);
}

/**
 * Free all memory
 */
void dinkc_sp_custom_free(dinkc_sp_custom_hash hash)
{
  hash_free(hash);
}

/**
 * Create a new int value for key 'key', or replace existing value if
 * 'key' is already mapped.
 */
void dinkc_sp_custom_set(dinkc_sp_custom_hash hash, char key[200], int val)
{
  struct str_int search;
  strcpy(search.key, key);
  void* slot = hash_lookup(hash, &search);
  if (slot != NULL)
    {
      ((struct str_int*)slot)->val = val;
    }
  else
    {
      struct str_int* newslot = malloc(sizeof(struct str_int));
      strcpy(newslot->key, key);
      ((struct str_int*)newslot)->val = val;
      hash_insert(hash, newslot);
    }
}

/**
 * Get the int value associated with 'key'. Returns -1 if not found
 * (DinkC limitation: no way to return NULL or similar).
 */
int dinkc_sp_custom_get(dinkc_sp_custom_hash hash, char key[200])
{
  struct str_int search;
  strcpy(search.key, key);
  void* slot = hash_lookup(hash, &search);
  if (slot != NULL)
    return ((struct str_int*)slot)->val;
  else
    return -1;
}


#ifdef TEST

int main(void)
{
  dinkc_sp_custom_hash myhash = dinkc_sp_custom_new();

  dinkc_sp_custom_set(myhash, "foo", -1);
  dinkc_sp_custom_set(myhash, "foo", 3);
  dinkc_sp_custom_set(myhash, "foo", -1);
  dinkc_sp_custom_set(myhash, "foo", 4);

  dinkc_sp_custom_set(myhash, "bar", 34);

  printf("%d\n", dinkc_sp_custom_get(myhash, "foo"));
  printf("%d\n", dinkc_sp_custom_get(myhash, "bar"));

  dinkc_sp_custom_free(myhash);
  return 0;
}

#endif
