#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "table.h"
#include "object.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void freeTable(Table* table) {
  reallocate(table, sizeof(Entry) * table->capacity, 0);
  initTable(table);
}

static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
  uint32_t index = key->hash % capacity;
  Entry* tombstone = NULL;
  while (true) {
    Entry* entry = entries + index;
    if (entry->key == NULL) {
      if (entry->value.type == VALUE_NIL) {
        return tombstone != NULL ? tombstone : entry;
      } else if (tombstone == NULL) {
        tombstone = entry;
      }
    } else if (entry->key == key) {
      return entry;
    }
    index = (index + 1) % capacity;
  }
}

static void adjustCapacity(Table* table, int newCapacity) {
  Entry* newEntries =
    reallocate(NULL, 0, sizeof(Entry) * newCapacity);
  for (int i = 0; i < newCapacity; ++i) {
    newEntries[i].key = NULL;
    newEntries[i].value = nil_value();
  }
  table->count = 0;
  for (int i = 0; i < table->capacity; ++i) {
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;
    Entry* dest = findEntry(newEntries, newCapacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }
  reallocate(table->entries, sizeof(Entry) * table->capacity, 0);
  table->entries = newEntries;
  table->capacity = newCapacity;
}

bool tableSet(Table* table, ObjString* key, Value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int newCap = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, newCap);
  }
  Entry* entry = findEntry(table->entries, table->capacity, key);
  bool newKey = entry->key == NULL;
  if (newKey && entry->value.type == VALUE_NIL) table->count++;
  entry->key = key;
  entry->value = value;
  return newKey;
}

bool tableGet(Table* table, ObjString* key, Value* value_returned) {
  if (table->count == 0) return false;
  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->key != NULL) {
    *value_returned = entry->value;
    return true;
  }
  return false;
}

bool tableDelete(Table* table, ObjString* key) {
  if (table->count == 0) return false;
  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;
  entry->key = NULL;
  entry->value = bool_value(true);
  return true;
}

void tableCopyTo(Table* from, Table* to) {
  for (int i = 0; i < from->capacity; ++i) {
    if (from->entries[i].key != NULL)
      tableSet(to, from->entries[i].key, from->entries[i].value);
  }
}

ObjString* tableFindString(Table* strings, const char* chars, int length,
                            uint32_t hash) {
  if (strings->count == 0) return NULL;
  uint32_t index = hash % strings->capacity;
  while (true) {
    Entry* entry = strings->entries + index;
    if (entry->key == NULL) {
      if (entry->value.type == VALUE_NIL) {
        return NULL;
      }
    } else if (entry->key->hash == hash &&
               memcmp(entry->key->chars, chars, length) == 0) {
      return entry->key;
    }
    index = (index + 1) % strings->capacity;
  }
}
