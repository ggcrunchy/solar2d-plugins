/*
 * Copyright (c) 1988, 1989, 1990 The Regents of the University of California.
 * Copyright (c) 1988, 1989 by Adam de Boor
 * Copyright (c) 1989 by Berkeley Softworks
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the University of
 *    California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* hash.c --
 *
 *     This module contains routines to manipulate a hash table.
 *     See hash.h for a definition of the structure of the hash
 *     table.  Hash tables grow automatically as the amount of
 *     information increases.
 */
#include "hash.h"
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


#ifndef FALSE 
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif


/*
 * Forward references to local procedures that are used before they're
 * defined:
 */

static int RebuildTable(hash_table_t *);

/*
 * The following defines the ratio of # entries to # buckets
 * at which we rebuild the table to make it larger.
 */

#define rebuildLimit 8

/*
 *---------------------------------------------------------
 *
 * Hash_InitTable --
 *
 *    This routine just sets up the hash table.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    Memory is allocated for the initial bucket area.
 *
 *---------------------------------------------------------
 */

int 
Hash_InitTable( hash_table_t *table )                 
{
    int buckets=16;
    hash_entry_t **hp= (hash_entry_t **)calloc(buckets, sizeof(hash_entry_t*)); 
    if (hp) {
        table->numEntries = 0;
        table->size = buckets;
        table->mask = buckets - 1;
        table->bucketPtr = hp;
    }        
    return hp!=0;
}    

/*
 *---------------------------------------------------------
 *
 * Hash_DeleteTable --
 *
 *    This routine removes everything from a hash table
 *    and frees up the memory space it occupied (except for
 *    the space in the hash_table_t structure).
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    Lots of memory is freed up.
 *
 *---------------------------------------------------------
 */

void
Hash_DeleteTable(hash_table_t *table, int freeClientData)
{
    hash_entry_t **hp, *h, *nexth = NULL;
    int i;
    
    // free all the buckets
    for (hp = table->bucketPtr, i = table->size; --i >= 0;) {
        for (h = *hp++; h != NULL; h = nexth) {
            nexth = h->next;
            if (freeClientData) {
                free(h->clientData);
            }        
            free((char *)h);
        }
    }

    // free the table
    free((char *)table->bucketPtr);

    /*
     * Set up the hash table to cause memory faults on any future access
     * attempts until re-initialization.
     */
    table->bucketPtr = NULL;
}

/*
 *---------------------------------------------------------
 *
 * Hash_FindEntry --
 *
 *     Searches a hash table for an entry corresponding to hash.
 *
 * Results:
 *    The return value is a pointer to the entry for key,
 *    if key was present in the table.  If key was not
 *    present, NULL is returned.
 *
 * Side Effects:
 *    None.
 *
 *---------------------------------------------------------
 */

hash_entry_t*
Hash_FindEntry(hash_table_t *table, unsigned int hash)
{
    hash_entry_t *e;
    for (e = table->bucketPtr[hash & table->mask]; e != NULL; e = e->next) {
        if (e->namehash == hash) {
            break;
        }
    }        
    return e;
}

/*
 *---------------------------------------------------------
 *
 * Hash_CreateEntry --
 *
 *    Searches a hash table for an entry corresponding to
 *    key.  If no entry is found, then one is created.
 *
 * Results:
 *    The return value is a pointer to the entry.  If *newPtr
 *    isn'table NULL, then *newPtr is filled in with TRUE if a
 *    new entry was created, and FALSE if an entry already existed
 *    with the given key.
 *
 * Side Effects:
 *    Memory may be allocated, and the hash buckets may be modified.
 *---------------------------------------------------------
 */

hash_entry_t *
Hash_CreateEntry(hash_table_t *table, /* Hash table to search. */
                unsigned int hash,  /* A hash key. */
                int *newPtr /* Filled in with TRUE if new entry created,
                             * FALSE otherwise. */
                )                             
{
    hash_entry_t *e;
    for (e = table->bucketPtr[hash & table->mask]; e != NULL; e = e->next) {
        if (e->namehash == hash) {
            if (newPtr != NULL) {
                *newPtr = FALSE;
            }                
            break;
        }
    }
    if (!e) {
        /*
         * The desired entry isn'table there.  Before allocating a new entry,
         * expand the table if necessary (and this changes the resulting
         * bucket chain).
         */
        if ((table->numEntries < (rebuildLimit * table->size)) || RebuildTable(table)) 
        { 
            e = (hash_entry_t *) malloc(sizeof(hash_entry_t));/*emalloc(sizeof(*e) + keylen);*/
            if (e) {
                hash_entry_t **hp= &table->bucketPtr[hash & table->mask];
                e->next = *hp;              // point at whatever was in the bucket
                *hp = e;                    // put us in the bucket
                e->clientData = NULL;
                e->clientFlags= 0;
                e->namehash = hash;
                ++table->numEntries;
                if (newPtr != NULL) {
                    *newPtr = TRUE;
                }                
            }
        }        
    }        
    return (e);
}

/*
 *---------------------------------------------------------
 *
 * RebuildTable --
 *    This local routine makes a new hash table that
 *    is larger than the old one.
 *
 * Results:
 *     None.
 *
 * Side Effects:
 *    The entire hash table is moved, so any bucket numbers
 *    from the old table are invalid.
 *
 *---------------------------------------------------------
 */

static int 
RebuildTable(hash_table_t *table)
{
    hash_entry_t *e, *next = NULL, **hp, **xp;
    int i, mask;
        hash_entry_t **oldhp;
    int oldsize;

    oldhp = table->bucketPtr;
    oldsize = i = table->size;
    i <<= 1;
    hp = (hash_entry_t **) calloc(i, sizeof(hash_entry_t*)); /*emalloc(sizeof(*hp) * i)*/
    if (hp) {
        table->size = i;
        table->mask = mask = i - 1;
        table->bucketPtr = hp;             
        for (hp = oldhp, i = oldsize; --i >= 0;) 
        {
            for (e = *hp++; e != NULL; e = next) 
            {
                next = e->next;
                xp = &table->bucketPtr[e->namehash & mask];
                e->next = *xp;
                *xp = e;
            }
        }
        free((char *)oldhp);
    }        
    return hp!=0;
}


