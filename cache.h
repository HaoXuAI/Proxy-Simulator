/**
 * cache.h 
 * The cache.h file helps to declare all thosed needed structure and
 * functions in cache.c.
 * @Author: Hao Xu
 * @Andrew id: haox1
 */

/**
 * doubly linked list cache_struct is used to store request and data
 * returned from server, and next and prev pointers are pointing to
 * next and previous cache in list.
 *
 */
typedef struct cacheStruct
{
    char *request;
    char *data;
    struct cacheStruct *next;
    struct cacheStruct *prev;
} cache_struct;

/**
 * functions are used in cache.c, please read comment in cache.c to get
 * details about its usage. 
 *
 */
void addFirst(char *request, char *buf);
void initCache(char *request, char *buf);
char *findCache(char *request);
int removeLast(int size);
void removeHelper(cache_struct *cache_s);
void updateCache(cache_struct *cache);
int evictCache(char *request, char *buf);
char *readCache(char *request);
void freeCache();
