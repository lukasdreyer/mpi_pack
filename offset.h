// offset.h
#ifndef OFFSET_H
#define OFFSET_H

void print_offsets_on_root(int *offsets);

void new_offset_single_proc(int * old_offset, int proc, int ** new_offset_p);

/* offsets needs to be allocated */
void compute_num_global_and_offsets (int num_local_elements, int* num_global_elements, int* offsets);

int offset_search(int *offsets, int element_search_id);

/* currently [first, last] */
void first_and_last_proc(int *old_offsets, int* new_offsets, int *first, int *last);
/* currently [first, last) */
void offset_overlap(int *offsets1, int rank1, int *offsets2, int rank2, int* first, int *last);

#endif /* OFFSET_H */