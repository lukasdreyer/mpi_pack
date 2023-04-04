#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include "element_list.h"
#include "offset.h"

#define MPI_PARTITION_ELEMENT_LIST 666

const int num_int_data[numTypes] = {3,5};
const int num_double_data[numTypes] = {2,6};

element_t *
new_element(types_t type){
    int mpi_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    element_t *element = malloc(sizeof(element_t));
    element->type = type;
    element->int_data = malloc(sizeof(int) * num_int_data[type]);
    element->double_data = malloc(sizeof(double) * num_double_data[type]);
    for(int i = 0; i<num_int_data[type];i++){
        element->int_data[i] = i + 10*mpi_rank;
    }
    for(int i = 0; i<num_double_data[type];i++){
        element->double_data[i] = i + 20.*mpi_rank;
    }
//    element->num_user_data = 0;
//    element->user_data = NULL;
//    element->flatten = NULL;
//    element->expand = NULL;
    return element;
}
void print_element_list(element_list_t *element_list){
    int mpi_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    element_t *element;
    printf("rank: %i, num_local_elements:%i \n", mpi_rank, element_list->num_local_elements);
    for(int ielement=0; ielement<element_list->num_local_elements;ielement++){
        printf("rank: %i, element:%i \n", mpi_rank, ielement);
        element = element_list->elements[ielement];
        for(int idata=0; idata<num_int_data[element->type]; idata++){
            printf("%i ", element->int_data[idata]);
        }
        printf("\n");
        for(int idata=0; idata<num_double_data[element->type]; idata++){
            printf("%f ", element->double_data[idata]);
        }
        printf("\n");
    }
}

/* create an element list with mpi_rank many elements per rank and alternating types on each rank*/
element_list_t *
new_element_list(){
    int mpi_size, mpi_rank;
    element_list_t* element_list = malloc(sizeof(element_list_t));

    /* malloc memory for mpi_rank many elements */
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    element_list->num_local_elements = mpi_rank + 1;
    element_list->elements = malloc (element_list->num_local_elements * sizeof(element_t));
    
    /* create elements with alternating types */
    for(int i=0; i<element_list->num_local_elements; i++){
        element_list->elements[i] = new_element(i%2);
    }
    
    /* create list of global offsets */
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    element_list->offsets = malloc((mpi_size+1)*sizeof(int));

    compute_num_global_and_offsets(element_list->num_local_elements, &element_list->num_global_elements, element_list->offsets);

    return element_list;
}


element_list_t *
init_element_list(int *offsets){
    int mpi_size,mpi_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    element_list_t* element_list = malloc(sizeof(element_list_t));
    element_list->num_local_elements = offsets[mpi_rank+1] - offsets[mpi_rank];

    printf("init element list: rank: %i, num_local_elements: %i\n", mpi_rank, element_list->num_local_elements);

    element_list->elements = malloc (element_list->num_local_elements * sizeof(element_t));
    element_list->offsets = offsets;
    element_list->num_global_elements = offsets[mpi_size+1];
}


void element_list_partition(element_list_t **element_list_p, int *new_offsets){
    int mpi_rank, mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    int range_start, range_end;
    element_list_t *element_list;
    element_list = *element_list_p;
    element_t *element;

    int first_send, last_send, num_send;
    first_and_last_proc(element_list->offsets, new_offsets, &first_send, &last_send);
    num_send = last_send - first_send + 1;

    for (int iproc=first_send; iproc<=last_send; iproc++){
        offset_overlap(element_list->offsets, mpi_rank, new_offsets, iproc, &range_start, &range_end);
        int send_size=0;
        int pack_size;

        for (int ilocalelement = 0; ilocalelement < range_end - range_start; ilocalelement++){
            element = element_list->elements[ilocalelement];
            MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &pack_size);
            send_size += pack_size;
            MPI_Pack_size(num_int_data[element->type], MPI_INT, MPI_COMM_WORLD, &pack_size);
            send_size += pack_size;
            MPI_Pack_size(num_double_data[element->type], MPI_DOUBLE, MPI_COMM_WORLD, &pack_size);
            send_size += pack_size;
        }
        void *send_buffer = malloc(send_size);
        int position = 0;
        for (int ilocalelement = 0; ilocalelement < range_end - range_start; ilocalelement++){
            element = element_list->elements[ilocalelement];
            MPI_Pack(&element->type, 1, MPI_INT, send_buffer, send_size, &position, MPI_COMM_WORLD);
            MPI_Pack(element->int_data, num_int_data[element->type], MPI_INT, send_buffer, send_size, &position, MPI_COMM_WORLD);
            MPI_Pack(element->double_data, num_double_data[element->type], MPI_DOUBLE, send_buffer, send_size, &position, MPI_COMM_WORLD);
        }
        MPI_Bsend(send_buffer, position, MPI_PACKED, iproc, MPI_PARTITION_ELEMENT_LIST, MPI_COMM_WORLD);
        free(send_buffer);
    }

    /* Free the old elements, but keep the offsets */
    int *old_offsets = malloc((mpi_size+1) * sizeof(int));
    for(int i=0; i<=mpi_size; i++){
        old_offsets[i] = element_list->offsets[i];
    }
    free_element_list(element_list);
    *element_list_p = init_element_list(new_offsets);
    element_list = *element_list_p;

    /* Find out, from how many procs we recieve messages from */
    int first_recv, last_recv, num_recv;
    first_and_last_proc(new_offsets, old_offsets, &first_recv, &last_recv);
    num_recv = last_recv - first_recv + 1;
    /* Loop over expected messages */
    while(num_recv > 0){
        num_recv --;

        //Probe and receive message
        MPI_Status status;
        MPI_Probe(MPI_ANY_SOURCE, MPI_PARTITION_ELEMENT_LIST, MPI_COMM_WORLD, &status);
        int recv_from_proc = status.MPI_SOURCE;
        int recv_count;
        MPI_Get_count(&status, MPI_PACKED, &recv_count);
        void *recv_buffer = malloc(recv_count);
        MPI_Recv(recv_buffer, recv_count, MPI_PACKED, recv_from_proc, MPI_PARTITION_ELEMENT_LIST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //Unpack message
        offset_overlap(new_offsets, mpi_rank, old_offsets, recv_from_proc, &range_start, &range_end);
        int position =0;
        for (int ilocalelement = 0; ilocalelement < range_end - range_start; ilocalelement++){
            element = malloc(sizeof(element_t));
            MPI_Unpack(recv_buffer, recv_count, &position, &element->type, 1, MPI_INT, MPI_COMM_WORLD);
            element->int_data = malloc(num_int_data[element->type] * sizeof(int));
            MPI_Unpack(recv_buffer, recv_count, &position, element->int_data, num_int_data[element->type], MPI_INT, MPI_COMM_WORLD);
            element->double_data = malloc(num_double_data[element->type] * sizeof(double));
            MPI_Unpack(recv_buffer, recv_count, &position, element->double_data, num_double_data[element->type], MPI_DOUBLE, MPI_COMM_WORLD);
            element_list->elements[range_start + ilocalelement] = element;
        }
    }
    free (old_offsets);
}

void free_element(element_t *element){
    free(element->int_data);
    free(element->double_data);
    free(element);
}

void free_element_list(element_list_t *element_list){
    for(int i=0; i<element_list->num_local_elements; i++){
        free_element(element_list->elements[i]);
    }
    free(element_list);
}
