// tester class for debugging
#include "types.h"
#include "user.h"
#include "mmu.h"

int main() {
    struct pt_entry entries[2];  // pt_entries for storage
    char *ptr = sbrk(PGSIZE);  // page to encrypt
    *ptr = 'a';

    printf(2, "encpage addr: %d\n", ptr);

    int result = getpgtable(entries, 10);
    for (int index = 0; index < result; index++) {
        printf(2, "%d: pdx: %d, ptx: %d, ppage: %d, present: %d, writable: %d, encrypted: %d\n",
                index, entries[index].pdx, entries[index].ptx, entries[index].ppage, entries[index].present,
                        entries[index].writable, entries[index].encrypted);
    }

    int result2 = mencrypt(ptr, 1);
    printf(2, "\nok?: %d\n\n", result2);
    
    result = getpgtable(entries, 10);
    for (int index = 0; index < result; index++) {
        printf(2, "%d: pdx: %d, ptx: %d, ppage: %d, present: %d, writable: %d, encrypted: %d\n",
                index, entries[index].pdx, entries[index].ptx, entries[index].ppage, entries[index].present,
                        entries[index].writable, entries[index].encrypted);
    }

    *ptr = 'b';
    printf(2, "\nis b?: %c\n\n", *ptr);

    result = getpgtable(entries, 10);
    for (int index = 0; index < result; index++) {
        printf(2, "%d: pdx: %d, ptx: %d, ppage: %d, present: %d, writable: %d, encrypted: %d\n",
                index, entries[index].pdx, entries[index].ptx, entries[index].ppage, entries[index].present,
                        entries[index].writable, entries[index].encrypted);
    }

    exit();
}
