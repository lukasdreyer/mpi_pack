#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "offset.h"

void print_offsets_on_root(int *offsets){
    int mpi_rank, mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    if(mpi_rank == 0){
        for(int i=0; i<=mpi_size; i++){
            printf("%i ", offsets[i]);
        }
        printf("\n");
    }
       
}

void new_offset_single_proc(int * old_offset, int proc, int ** new_offset_p){
    int mpi_rank, mpi_size, num_global_elements;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    *new_offset_p = calloc((mpi_size + 1), sizeof(int));
    int *new_offset = *new_offset_p;
    num_global_elements = old_offset[mpi_size];
    for(int i=proc+1; i<=mpi_size; i++){
        new_offset[i] = num_global_elements;
    }
}

void compute_num_global_and_offsets (int num_local_elements, int* num_global_elements, int* offsets){
    /* compute global number of elements*/
    MPI_Allreduce(&num_local_elements, num_global_elements,
                    1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    /* compute offset of local first element in global list*/ 
    int local_offset;
    MPI_Scan (&num_local_elements, &local_offset, 1, MPI_INT,
               MPI_SUM, MPI_COMM_WORLD);
    /* MPI_Scan is inklusive, thus it counts our own data.
     * Therefore, we have to subtract it again */
    local_offset -= num_local_elements;
    
    /* gather offsets*/
    MPI_Allgather(&local_offset, 1, MPI_INT, offsets, 1, MPI_INT, MPI_COMM_WORLD);

    int mpi_size;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    offsets[mpi_size] = *num_global_elements;
}

/* TODO: implement with binary search */
int offset_search(int *offsets, int element_search_id){
    int mpi_size;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    for (int i=0; i<mpi_size; i++){
        if(offsets[i+1] > element_search_id) return i;
    }
}

void first_and_last_proc(int *old_offsets, int *new_offsets, int *first, int *last){
    int mpi_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    *first = offset_search(new_offsets, old_offsets[mpi_rank]);
    *last = offset_search(new_offsets, old_offsets[mpi_rank+1]-1);
}


static int min_int(int a, int b){
    return a<b?a:b;
}
static int max_int(int a, int b){
    return a>b?a:b;
}

void offset_overlap(int *offsets1, int rank1, int *offsets2, int rank2, int* first, int *last){
    *first = max_int(offsets1[rank1], offsets2[rank2]);
    *last = min_int(offsets1[rank1+1], offsets2[rank2+1]);
}
