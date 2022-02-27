#pragma once

#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

static bool is_digit(char character) {
    return character >= '0' && character <= '9';
}

// returns position after parsed digit
static size_t get_digit(char* list, size_t len, int* digit) {
    size_t digit_stop = 0;
    for(; digit_stop<len && is_digit(list[digit_stop]); digit_stop++);
    if(digit_stop == 0) {
        return 0;
    }

    char backup_char_at_digit_stop = list[digit_stop];
    list[digit_stop] = '\0';
    if(sscanf(list, "%d", digit) != 1) {
        return false;
    }
    list[digit_stop] = backup_char_at_digit_stop;
    return digit_stop;
}

static bool cpu_in_cpulist(char* list, int cpuid) {
    size_t list_len = strlen(list);

    size_t list_index = 0;
    while(list_index < list_len) {
        int digit = 0, digit_end;
        size_t digit_len = get_digit(list + list_index, list_len-list_index, &digit);
        if(digit_len == 0) {
            return false;
        }
        list_index += digit_len;

        switch(list[list_index]) {
            case ',':
            case '\0':
                if(cpuid == digit) {
                    return true;
                }
                break;
            case '-':
                list_index += 1; // add separator
                digit_len = get_digit(list+list_index, list_len-list_index, &digit_end);
                if(digit_len == 0) {
                    return false;
                }
                list_index += digit_len;

                if(cpuid >= digit && cpuid <= digit_end) {
                    return true;
                }
                break;
        }
        list_index += 1;
    }
    return false;
}