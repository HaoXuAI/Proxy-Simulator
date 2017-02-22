#include "cache.h"
#include "csapp.h"
#include <stdio.h>

/********************************************************************
 * cache.c file                                                     *
 * The cache .c assembly all needed data and functions to access,   *
 * store, updata information from and to proxy. the doubly linked   *
 * list strategy is LIFO -- always store data at first place in the *
 * list, and always remove data from the last place in list. If the *
 * last cache size is not enough to even the size need for new data,*
 * it will enlarge its size to include its previous cache again and *
 * again.                                                           *
 * @Author: Hao Xu                                                  *
 * @andrew id: haox1                                                *
 ********************************************************************/


/**
 * two simply initialized pointers cacheHead and cacheTail
 * are representing the first and last cache in list.
 *
 */
cache_struct *cacheHead = NULL;
cache_struct *cacheTail = NULL;

/**
 * add First make a new cache if the list is empty, and add it into
 * first place of list if it's not.
 */
void addFirst(char *request, char *buf) {

    if (cacheHead == NULL) {

        initCache(request, buf);
    } else {

        cache_struct *cacheNew;
        cacheNew = Malloc(sizeof(cache_struct));

        cacheNew->request = Malloc(strlen(request));

        strcpy(cacheNew->request, request);
        
        cacheNew->data = Malloc(strlen(buf));

        strcpy(cacheNew->data, buf);
        cacheNew->prev = NULL;

        cacheNew->next = cacheHead;

        cacheHead->prev = cacheNew;
        cacheHead = cacheNew;
    }
}

/**
 * initCache initialize the first cache in list with returned
 * request and data.
 */
void initCache(char *request, char *buf) {

    cacheHead = Malloc(sizeof(cache_struct));

    cacheHead->data = Malloc(strlen(buf));

    strcpy(cacheHead->data, buf);
    cacheHead->request = Malloc(strlen(request));
    strcpy(cacheHead->request, request);
    cacheHead->next = NULL;
    cacheHead->prev = NULL;
}

/**
 * findCache find the first fit size cache in list, if it's
 * found return the data to proxy and updata it to first place
 * of list. If cannot find it simply return null.
 */
char *findCache(char *request) {

    cache_struct *cache_s = cacheHead;
    while (cache_s != NULL) {

        if (strcmp(request, cache_s->request) == 0) {
 
            char *content = cache_s->data;
            updateCache(cache_s);
            return content;
        }

        cache_s = cache_s->next;

    }

    return NULL;
}

/**
 * removeLast remove the last caches in list with fit size. it
 * will enlage by including previous cache in list.
 */
int removeLast(int size) {

    cache_struct *cache_s = cacheTail;

    int testSize = 0;
    int newSize = 0;

    while (cache_s != NULL) {

        newSize = strlen(cache_s->data);

        if ((testSize += newSize) >= size) {
            cache_struct *cache_tmp = cache_s;
            while (cache_tmp != NULL) {
                removeHelper(cache_tmp);
                cache_tmp = cache_tmp->next;
            }
            break;
        }
        cache_s = cache_s->prev;
    }
    return testSize;
}

/**
 * removeHelper helps to remove the cache in list.
 */
void removeHelper(cache_struct *cache_s) {

    cache_struct *cache_prev = cache_s->prev;
    cache_struct *cache_next = cache_s->next;

    if (cache_prev == NULL && cache_next == NULL) {
        cacheHead = NULL;
        cacheTail = NULL;
        return;
    }
    if (cache_prev != NULL && cache_next == NULL) {
        cache_prev->next = NULL;
        cacheTail = cache_prev;
        return;
    }
    if (cache_prev == NULL && cache_next != NULL) {
        cache_next->prev = NULL;
        cacheHead = cache_next;
        return;
    }
    if (cache_prev != NULL && cache_next != NULL) {
        cache_next->prev = cache_prev;
        cache_prev->next = cache_next;
        return;
    }
}

/**
 * updataCache update the cache to first place in list.
 */
void updateCache(cache_struct *cache) {

    cache_struct *cache_next = cache->next;
    cache_struct *cache_prev = cache->prev;

    if (cache_prev == NULL && cache_next == NULL) {
        return;
    } 
    if (cache_prev == NULL && cache_next != NULL) {
        return;
    }
    if (cache_prev != NULL && cache_next == NULL) {
        cacheTail = cache_prev;
        cache_prev->next = NULL;
    }

    if (cache_prev != NULL && cache_next != NULL) {
        cache_next->prev = cache_prev;
        cache_prev->next = cache_next;
    }

    addFirst(cache->request, cache->data);
}

/**
 * evictCache make a new cache at first place of list, remove
 * last caches in list, and return evicted (removed) caches whole
 * size.
 */
int evictCache(char *request, char *buf) {
    int size = strlen(buf);
    addFirst(request, buf);
    return removeLast(size);
}

/**
 * freeCache free all cache, cache request and cache data in list.
 */
void freeCache() {
    while (cacheHead != NULL) {
        Free(cacheHead->request);
        Free(cacheHead->data);
        Free(cacheHead);
        cacheHead = cacheHead->next;
    }
    while (cacheTail != NULL) {
        Free(cacheTail->request);
        Free(cacheTail->data);
        Free(cacheTail);
        cacheTail = cacheTail->prev;
    }
    
}

