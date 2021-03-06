#include "ctable.h"
#include "cmem.h"
#include "cvalue.h"
#include "cobj.h"

#include <string.h>

#define MAX_TABLE_FILL 0.75

void cosmoT_initTable(CState *state, CTable *tbl, int startCap) {
    tbl->capacity = startCap;
    tbl->count = 0;
    tbl->table = NULL; // to let out GC know we're initalizing
    tbl->table = cosmoM_xmalloc(state, sizeof(CTableEntry) * startCap);

    // init everything to NIL
    for (int i = 0; i < startCap; i++) {
        tbl->table[i].key = cosmoV_newNil();
        tbl->table[i].val = cosmoV_newNil();
    }
}

void cosmoT_addTable(CState *state, CTable *from, CTable *to) {
    for (int i = 0; i < from->capacity; i++) {
        CTableEntry *entry = &from->table[i];

        if (!(IS_NIL(entry->key))) {
            CValue *newVal = cosmoT_insert(state, to, entry->key);
            *newVal = entry->val;
        }
    }
}

void cosmoT_clearTable(CState *state, CTable *tbl) {
    cosmoM_freearray(state, CTableEntry, tbl->table, tbl->capacity);
}

uint32_t getObjectHash(CObj *obj) {
    switch(obj->type) {
        case COBJ_STRING:
            return ((CObjString*)obj)->hash;
        default:
            return 0;
    }
}

uint32_t getValueHash(CValue *val) {
    switch (val->type) {
        case COSMO_TOBJ:
            return getObjectHash(val->val.obj);
        case COSMO_TNUMBER:
            // how the fuck
        // TODO: add support for other types
        default:
            return 0;
    }
}

// mask should always be (capacity - 1)
static CTableEntry *findEntry(CTableEntry *entries, int mask, CValue key) {
    uint32_t hash = getValueHash(&key);
    uint32_t indx = hash & mask; // since we know the capacity will *always* be a power of 2, we can use bitwise & to perform a MUCH faster mod operation
    CTableEntry *tomb = NULL;

    // keep looking for an open slot in the entries array
    while (true) {
        CTableEntry *entry = &entries[indx];

        if (IS_NIL(entry->key)) {
            // check if it's an empty bucket or a tombstone
            if (IS_NIL(entry->val)) {
                // it's empty! if we found a tombstone, return that so it'll be reused
                return tomb != NULL ? tomb : entry;
            } else {
                // its a tombstone!
                tomb = entry;
            }
        } else if (cosmoV_equal(entry->key, key)) {
            return entry;
        }

        indx = (indx + 1) & mask; // fast mod here too
    }
}

static void growTbl(CState *state, CTable *tbl, size_t newCapacity) {
    CTableEntry *entries = cosmoM_xmalloc(state, sizeof(CTableEntry) * newCapacity);
    int newCount;

    // set all nodes as NIL : NIL
    for (int i = 0; i < newCapacity; i++) {
        entries[i].key = cosmoV_newNil();
        entries[i].val = cosmoV_newNil();
    }

    // move over old values to the new buffer
    for (int i = 0; i < tbl->capacity; i++) {
        CTableEntry *oldEntry = &tbl->table[i];
        if (IS_NIL(oldEntry->key))
            continue; // skip empty keys

        // get new entry location & update the node
        CTableEntry *newEntry = findEntry(entries, newCapacity - 1, oldEntry->key);
        newEntry->key = oldEntry->key;
        newEntry->val = oldEntry->val;
        newCount++; // inc count
    }

    // free the old table
    cosmoM_freearray(state, CTableEntry, tbl->table, tbl->capacity);

    tbl->table = entries;
    tbl->capacity = newCapacity;
    tbl->count = newCount;
}

// returns a pointer to the allocated value
COSMO_API CValue* cosmoT_insert(CState *state, CTable *tbl, CValue key) {
    // make sure we have enough space allocated
    if (tbl->count + 1 > tbl->capacity * MAX_TABLE_FILL) {
        int newCap = tbl->capacity * GROW_FACTOR;
        growTbl(state, tbl, newCap);
    }

    // insert into the table
    CTableEntry *entry = findEntry(tbl->table, tbl->capacity - 1, key); // -1 for our capacity mask

    if (IS_NIL(entry->key) && IS_NIL(entry->val)) // is it empty?
        tbl->count++;

    entry->key = key;
    return &entry->val;
}

bool cosmoT_get(CTable *tbl, CValue key, CValue *val) {
    if (tbl->count == 0) {
        *val = cosmoV_newNil();
        return false; // sanity check
    }
    
    CTableEntry *entry = findEntry(tbl->table, tbl->capacity - 1, key);
    *val = entry->val;
    
    return !(IS_NIL(entry->key));
}

bool cosmoT_remove(CTable *tbl, CValue key) {
    if (tbl->count == 0) return 0; // sanity check

    CTableEntry *entry = findEntry(tbl->table, tbl->capacity - 1, key);
    if (IS_NIL(entry->key)) // sanity check
        return false;

    // crafts tombstone
    entry->key = cosmoV_newNil(); // this has to be nil
    entry->val = cosmoV_newBoolean(false); // doesn't reall matter what this is, as long as it isn't nil

    return true;
}

CObjString *cosmoT_lookupString(CTable *tbl, const char *str, size_t length, uint32_t hash) {
    if (tbl->count == 0) return 0; // sanity check
    uint32_t indx = hash & (tbl->capacity - 1); // since we know the capacity will *always* be a power of 2, we can use bitwise & to perform a MUCH faster mod operation

    // keep looking for an open slot in the entries array
    while (true) {
        CTableEntry *entry = &tbl->table[indx];

        // check if it's an empty slot (meaning we dont have it in the table)
        if (IS_NIL(entry->key) && IS_NIL(entry->val)) {
            return NULL;
        } else if (IS_STRING(entry->key) && cosmoV_readString(entry->key)->length == length && memcmp(cosmoV_readString(entry->key)->str, str, length) == 0) {
            // it's a match!
            return (CObjString*)entry->key.val.obj;
        }

        indx = (indx + 1) & (tbl->capacity - 1); // fast mod here too
    }
}

// for debugging purposes
void cosmoT_printTable(CTable *tbl, const char *name) {
    printf("==== [[%s]] ====\n", name);
    for (int i = 0; i < tbl->capacity; i++) {
        CTableEntry *entry = &tbl->table[i];
        if (!(IS_NIL(entry->key))) {
            printValue(entry->key);
            printf(" - ");
            printValue(entry->val);
            printf("\n");
        }
    }
}