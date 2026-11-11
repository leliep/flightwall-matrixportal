#pragma once
#include <cstddef>
#include <cstdint>
// Hardware boundary: two files, durable active-slot metadata, no settings writes.
bool lookupStorageMount();
bool lookupStorageInitialize();
size_t lookupStorageCapacity();
size_t lookupStorageSize();
bool lookupStorageRead(char* data, size_t size);
bool lookupStorageCommit(const char* data, size_t size);
