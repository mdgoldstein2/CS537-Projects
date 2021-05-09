// tester class for debugging
#include "types.h"
#include "user.h"
#include "mmu.h"

int main() {
    struct pt_entry entries[10];  // pt_entries for storage
    char *ptr = sbrk(PGSIZE*2);  // page to encrypt

    int result = getpgtable(entries, 10, 0);
    for (int index = 0; index < result; index++) {
        printf(2, "%d: pdx: %d, ptx: %d, ppage: %d, present: %d, writable: %d, encrypted: %d, accessed: %d, user: %d\n",
                index, entries[index].pdx, entries[index].ptx, entries[index].ppage, entries[index].present,
                        entries[index].writable, entries[index].encrypted, entries[index].ref, entries[index].user);
    }    

    *ptr = 'b';

    result = getpgtable(entries, 10, 0);
    for (int index = 0; index < result; index++) {
        printf(2, "%d: pdx: %d, ptx: %d, ppage: %d, present: %d, writable: %d, encrypted: %d, accessed: %d, user: %d\n",
                index, entries[index].pdx, entries[index].ptx, entries[index].ppage, entries[index].present,
                        entries[index].writable, entries[index].encrypted, entries[index].ref, entries[index].user);
    }

    exit();
}
