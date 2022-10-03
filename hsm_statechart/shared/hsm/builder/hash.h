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
 *
 *    from: @(#)hash.h    8.1 (Berkeley) 6/6/93
 * $FreeBSD: src/usr.bin/make/hash.h,v 1.8 1999/08/28 01:03:30 peter Exp $
 */

/* hash.h --
 *
 *     This file contains definitions used by the hash module,
 *     which maintains hash tables.
 */

#ifndef    _HASH
#define    _HASH

typedef void *ClientData;

/*
 * The following defines one entry in the hash table.
 */

typedef struct Hash_Entry hash_entry_t;
struct Hash_Entry
{
    hash_entry_t         *next;    /* entries associated with the same bucket. */
    unsigned        clientFlags;   /* on the plus side, the string is now separate */
    ClientData      clientData;    /* Arbitrary piece of data associated. */
    unsigned          namehash;    /* hash value of key */
} ;

typedef struct Hash_Table hash_table_t;
struct Hash_Table 
{
    hash_entry_t **bucketPtr; /* Pointers to Hash_Entry, one for each bucket in the table. */
    int     size;             /* Actual size of array. */
    int     numEntries;      /* Number of entries in the table. */
    int     mask;            /* Used to select bits for hashing. */
};

/*
 * Macros.
 */

/*
 * ClientData Hash_GetValue(h)
 *     Hash_Entry *h;
 */
#define Hash_GetValue(h) ((h)->clientData)

/*
 * Hash_SetValue(h, val);
 *     Hash_Entry *h;
 *     char *val;
 */
#define Hash_SetValue(h, val) ((h)->clientData = (ClientData) (val))

int Hash_InitTable(hash_table_t*);
void Hash_DeleteTable(hash_table_t*, int);

hash_entry_t *Hash_FindEntry(hash_table_t*, unsigned int);
hash_entry_t *Hash_CreateEntry(hash_table_t*, unsigned int, int *);


#endif /* _HASH */
