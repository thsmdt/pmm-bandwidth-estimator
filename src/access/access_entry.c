#include <access/access_entry.h>

#include <string.h>
#include <malloc.h>

#include <logger.h>

bool access_entry_init(struct access_entry *context, pid_t pid) {
    context->pid = pid;
    context->bytes_written_total = 0;
    context->write_access_total = 0;
#ifdef FEATUREFLAG_ACCESS_STORE_INSTRUCTIONS
    list_init(&context->bytes_written_by_insn);
#endif
}

bool access_entry_deinit(struct access_entry *context) {
#ifdef FEATUREFLAG_ACCESS_STORE_INSTRUCTIONS
    struct access_entry_insn *current, *nextup;
    list_for_each_entry_safe(current, nextup, &context->bytes_written_by_insn, list) {
        list_remove(&current->list);
        access_entry_insn_deinit(current);
        free(current);
    }
#endif
}

bool access_entry_track_write(struct access_entry *context, size_t access_size, const char* insn, void* _) {
    context->bytes_written_total += access_size;
    context->write_access_total += 1;

#ifdef FEATUREFLAG_ACCESS_STORE_INSTRUCTIONS
    struct access_entry_insn *current, *match = NULL;
    list_for_each_entry(current, &context->bytes_written_by_insn, list) {
        if(strcmp(current->insn, insn) == 0) {
            match = current;
            break;
        }
    }
    if(!match) {
        match = malloc(sizeof(struct access_entry_insn));
        if(!match) {
            LOG_ERROR("Unable to allocate memory for new access' instruction tracking entry");
            return false;
        }
        access_entry_insn_init(match, insn);
        list_add_after( &context->bytes_written_by_insn, &match->list);
    }

    access_entry_insn_track(match, access_size);
#endif
    return true;
}

#ifdef FEATUREFLAG_ACCESS_STORE_INSTRUCTIONS
bool access_entry_insn_init(struct access_entry_insn *context, const char* insn) {
    context->bytes_written_total = 0;
    context->write_access_total = 0;
    size_t insn_buffer_len = strlen(insn) + 1;
    char* insn_buffer = malloc(insn_buffer_len);
    if(!insn_buffer) {
        LOG_ERROR("Unable to allocate memory for new access' instruction tracking entry label='%s'", insn);
        return false;
    }

    strcpy(insn_buffer, insn);
    context->insn = insn_buffer;

    return true;
}

bool access_entry_insn_deinit(struct access_entry_insn *context) {
    free(context->insn);
    return true;
}

bool access_entry_insn_track(struct access_entry_insn *context, size_t access_size) {
    context->write_access_total += 1;
    context->bytes_written_total += access_size;
}
#endif