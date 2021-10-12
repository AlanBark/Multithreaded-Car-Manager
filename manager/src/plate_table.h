// Inroduction to hash tables in C

#include <inttypes.h> // for portable integer declarations
#include <stdbool.h>  // for bool type
#include <stdio.h>    // for printf()
#include <stdlib.h>   // for malloc(), free(), NULL
#include <string.h>   // for strcmp()

// An item inserted into a hash table.
// As hash collisions can occur, multiple items can exist in one bucket.
// Therefore, each bucket is a linked list of items that hashes to that bucket.
typedef struct item item_t;
struct item
{
    char *key;
    item_t *next;
};


void item_print(item_t *i);

// A hash table mapping a string to an integer.
typedef struct htab htab_t;
struct htab
{
    item_t **buckets;
    size_t size;
};

void htab_print(htab_t *h);

// Initialise a new hash table with n buckets.
// pre: true
// post: (return == false AND allocation of table failed)
//       OR (all buckets are null pointers)
bool htab_init(htab_t *h, size_t n);

// The Bernstein hash function.
// A very fast hash function that works well in practice.
size_t djb_hash(char *s);

// Calculate the offset for the bucket for key in hash table.
size_t htab_index(htab_t *h, char *key);

// Find pointer to head of list for key in hash table.
item_t *htab_bucket(htab_t *h, char *key);

// Find an item for key in hash table.
// pre: true
// post: (return == NULL AND item not found)
//       OR (strcmp(return->key, key) == 0)
item_t *htab_find(htab_t *h, char *key);

// Add a key with value to the hash table.
// pre: htab_find(h, key) == NULL
// post: (return == false AND allocation of new item failed)
//       OR (htab_find(h, key) != NULL)
bool htab_add(htab_t *h, char *key);

// Delete an item with key from the hash table.
// pre: htab_find(h, key) != NULL
// post: htab_find(h, key) == NULL
void htab_delete(htab_t *h, char *key);

// Destroy an initialised hash table.
// pre: htab_init(h)
// post: all memory for hash table is released
void htab_destroy(htab_t *h);
