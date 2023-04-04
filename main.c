#include <mpi.h>
#include "element_list.h"
#include "offset.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    element_list_t *element_list = new_element_list();
    print_offsets_on_root(element_list->offsets);

    int *new_offsets;
    new_offset_single_proc(element_list->offsets, 0, &new_offsets);
    print_offsets_on_root(new_offsets);

    print_element_list(element_list);

    element_list_partition(&element_list, new_offsets);

    printf("---- finish_partition ----\n");
    print_element_list(element_list);

    free_element_list(element_list);

    // Finalize the MPI environment.
    MPI_Finalize();
    return 0;
}
