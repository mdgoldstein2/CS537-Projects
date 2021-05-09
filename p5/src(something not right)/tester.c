// tester class for debugging
#include "types.h"
#include "user.h"
#include "mmu.h"

int main() {
    struct pt_entry entries[2];  // pt_entries for storage
    int result = getpgtable(entries, 2);
    printf(2, "pdx?; %d\n", entries[1].pdx);
    printf(2, "ptx?; %d\n", entries[1].ptx);
    printf(2, "success?: %d\n", result);
    printf(2, "present?: %d\n", entries[1].present);
    printf(2, "encrypted?: %d\n", entries[1].encrypted);
    printf(2, "addr to encrypt?: %d\n", PGADDR(entries[1].pdx, entries[1].ptx, 0));
    printf(2, "addr of array?: %d\n", PGROUNDDOWN((uint) entries));    

    int result2 = mencrypt((char*) PGADDR(entries[1].pdx, entries[1].ptx, 0), 1);
    printf(2, "ok?: %d\n", result2);
    result = getpgtable(entries, 2);
    printf(2, "pdx?; %d\n", entries[1].pdx);
    printf(2, "ptx?; %d\n", entries[1].ptx);
    printf(2, "success?: %d\n", result);
    printf(2, "present?: %d\n", entries[1].present);
    printf(2, "encrypted?: %d\n", entries[1].encrypted);

    exit();
}
