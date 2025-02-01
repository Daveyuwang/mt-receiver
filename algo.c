/**
 * Implementation of frame processing algorithms
 * 
 * This file includes two main algorithms:
 * 1. Process 100-byte frames by removing duplicates and sorting
 * 2. Quick search for byte value 62 in 500-byte frames
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* Constants */
#define FRAME_LEN_100 100
#define FRAME_LEN_500 500
#define SEARCH_BYTE 62

/* Function prototypes */
int process_byte_frame(uint8_t *data, int data_len, uint8_t *result, int *result_len);
int binary_search_for_byte(uint8_t *data, int data_len, uint8_t target);
int linear_search_for_byte(uint8_t *data, int data_len, uint8_t target);
void print_data(uint8_t *data, int data_len);
int generate_test_data(uint8_t *buffer, int size);



/**
 * process_byte_frame:
 * process input data array (length = data_len)
 *  1. remove duplicated bytes (only keep the first occurrence)
 *  2. sort the unique bytes in ascending order
 * 
 * write result to output array, store the result length in result_len
 * return 0 if success, otherwise any non-zero value
 */
int process_byte_frame(uint8_t *data, int data_len, uint8_t *result, int *result_len){
    // check input parameters
    if (data == NULL || result == NULL || result_len == NULL){
        return -1;
    }

    // counter for each byte value
    uint16_t count[256] = {0};

    // count the occurrences of each byte
    for(int i = 0; i < data_len; i++){
        count[data[i]]++;
    }

    // collect all unique bytes
    // naturally sorted
    *result_len = 0;
    for(int i = 0; i < 256; i++){
        if(count[i] > 0){
            result[*result_len] = (uint8_t)i;
            (*result_len)++;
        }
    }
    return 0;
}

/**
 * binary_search_for_byte:
 * search for target byte in input data array (length = data_len)
 * return the index of the first occurrence of target byte
 * return -1 if target byte is not found
 * use binary search for efficiency
 */
int binary_search_for_byte(uint8_t *data, int data_len, uint8_t target){
    // check input parameters
    if(data == NULL || data_len <= 0){
        return -1;
    }

    int left = 0;   // left index
    int right = data_len - 1; // right index

    while(left <= right){   // binary search loop
        int mid = left + (right - left) / 2; // middle index
        if(data[mid] == target){    // target found
            return mid;             // return the index of the target byte found
        }
        else if(data[mid] < target){ // target is in the right half
            left = mid + 1;          // move the left index to the right
        }
        else{
            right = mid - 1;         // move the right index to the left
        }
    }
    return -1;
}

/**
 * linear_search_for_byte:
 * search for target byte in input data array (length = data_len)
 * return the index of the first occurrence of target byte
 * return -1 if target byte is not found
 */
int linear_search_for_byte(uint8_t *data, int data_len, uint8_t target){
    // check input parameters
    if(data == NULL || data_len <= 0){
        return -1;
    }

    // simple linear search
    for(int i = 0; i < data_len; i++){
        if(data[i] == target){   // target found
            return i;           // return the index of the target byte found
        }
    }
    return -1;                  // target not found
}

/**
 * print_data:
 * print the input data array (length = data_len)
 * print 10 bytes per line
 */
void print_data(uint8_t *data, int data_len){
    for(int i = 0; i < data_len; i++){
        printf("%d ", data[i]);
        if((i+1) % 10 == 0) printf("\n");
    }
    printf("\n");
}

/**
 * generate_test_data:
 * generate test data array (length = size)
 * fill the array with random byte values
 * return 0 if success, otherwise any non-zero value
 */
int generate_test_data(uint8_t *buffer, int size){
    for(int i = 0; i < size; i++){
        buffer[i] = rand() % 256;
    }
    return 0;
}

int main(){
    // initialize random seed
    srand(time(NULL));

    // Test 1: process 100-byte frame
    printf("\n=== Test 1: Process 100-byte frame ===\n\n");
    uint8_t data[FRAME_LEN_100];
    uint8_t result[FRAME_LEN_100];
    int result_len;
    generate_test_data(data, FRAME_LEN_100);
    printf("Original data:\n");
    print_data(data, FRAME_LEN_100);
    
    if(process_byte_frame(data, FRAME_LEN_100, result, &result_len) == 0){
        printf("Processed result (%d unique bytes):\n", result_len);
        print_data(result, result_len);
    }else{
        printf("Error: process_byte_frame failed\n");
    }

    // Check if the result is sorted and unique
    for(int i = 0; i < result_len - 1; i++){
        if(result[i] > result[i+1]){
            printf("Error: result is not sorted\n");
            return -1;
        }
        if(result[i] == result[i+1]){
            printf("Error: result has duplicate bytes\n");
            return -1;
        }
    }

    // Test 2: binary search for byte 62 in 500-byte frame
    printf("\n=== Test 2: Binary search for byte 62 in 500-byte frame ===\n\n");
    uint8_t data_500[FRAME_LEN_500];
    generate_test_data(data_500, FRAME_LEN_500);

    // clear the existing byte 62
    for(int i = 0; i < FRAME_LEN_500; i++){
        if(data_500[i] == SEARCH_BYTE){
            while(data_500[i] == SEARCH_BYTE){
                data_500[i] = rand() % 256;
            }
        }
    }

    // insert the target byte at a random position
    int known_pos = rand() % FRAME_LEN_500;
    data_500[known_pos] = SEARCH_BYTE;
    
    struct timespec start, end;

    printf("Searching for value %d (inserted at position %d)\n", SEARCH_BYTE, known_pos);

    clock_gettime(CLOCK_MONOTONIC, &start);
    int linear_result = linear_search_for_byte(data_500, FRAME_LEN_500, SEARCH_BYTE);
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Linear search result: %d\n", linear_result);
    printf("Time consumed (nanoseconds) for linear search: %ld\n", (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec));

    // Sort data for binary search
    for (int i = 0; i < FRAME_LEN_500 - 1; i++) {
        for (int j = 0; j < FRAME_LEN_500 - i - 1; j++) {
            if (data_500[j] > data_500[j + 1]) {
                uint8_t temp = data_500[j];
                data_500[j] = data_500[j + 1];
                data_500[j + 1] = temp;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    // perform efficient search in sorted array
    int binary_result = binary_search_for_byte(data_500, FRAME_LEN_500, SEARCH_BYTE);

    // Record the end time after binary search and sorting are complete
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate time consumed in nanoseconds
    long time_taken_ns = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);

    printf("Binary search result: %d\n", binary_result);
    // Print the time consumed by the sorting and binary search operation
    printf("Time consumed (nanoseconds) for binary search: %ld\n", time_taken_ns);
    return 0;
}
