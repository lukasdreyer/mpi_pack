// element_list.h
#ifndef ELEMENT_LIST_H
#define ELEMENT_LIST_H

typedef int(*mpi_pack_size_fn)(void * user_data);
typedef void(*mpi_pack_fn)(void *user_data, void *pack_buffer, int buffer_size, int *position);

/* allocates memory for user_data */
typedef void(*mpi_unpack_fn)(void *pack_buffer, int buffer_size, void **user_data_p);

typedef enum types{type1, type2, numTypes}types_t;

extern const int num_int_data[numTypes];
extern const int num_double_data[numTypes];

typedef struct element{
    types_t type;
    int *int_data;
    double *double_data;

    int num_user_data;
    void **user_data;
    mpi_pack_size_fn *pack_size_fn;
    mpi_pack_fn *pack_fn;
    mpi_unpack_fn *unpack_fn;
}element_t;

typedef struct element_list{
    int num_local_elements;
    int num_global_elements;
    element_t **elements;
    int *offsets;
}element_list_t;


element_list_t * new_element_list();

/* Takes over offsets */
element_list_t *init_element_list(int *offsets);

void print_element_list(element_list_t *element_list);

/* takes over new offsets, reallocates memory for *element_list_p */
void element_list_partition(element_list_t **element_list_p, int *new_offsets);

void free_element_list(element_list_t *element_list);

#endif /* ELEMENT_LIST_H */