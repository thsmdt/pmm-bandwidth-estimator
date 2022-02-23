#include <access/access_accounting.h>

#include <malloc.h>
#include <access/access_worker.h>
#include <bits/stdint-uintn.h>
#include <capstone/capstone.h>
#include <string.h>

#include <logger.h>

#define ACCESS_ACCOUNTING_MEMORYMOCK_DEFAULT_EXPIRY_IN_SEC 30
#define ACCESS_ACCOUNTING_MAX_INSTRUCTION_LEN_IN_BYTES 16
#define ACCESS_ACCOUNTING_INSTRUCTION_STEP_TYPE uint32_t
#define ACCESS_ACCOUNTING_STRUCT_INSN_LEN 32

struct access_accounting_instruction {
    char insn[ACCESS_ACCOUNTING_STRUCT_INSN_LEN];
    size_t access_size;
};

bool access_accounting_instruction_get(struct access_accounting *context, struct access_accounting_instruction *out, pid_t pid, void* insn_ptr);
size_t get_instruction(void* buffer, struct memmock_context *memmock, void* insn_ptr);

bool access_accounting_init(struct access_accounting *context, struct memmkcache_context *existing_memmock) {
    struct timespec default_expiry = {ACCESS_ACCOUNTING_MEMORYMOCK_DEFAULT_EXPIRY_IN_SEC, 0};
    context->memmock_from_outside = existing_memmock != NULL;
    context->memmock_cache = existing_memmock;
    if(!context->memmock_from_outside) {
        context->memmock_cache = malloc(sizeof(struct memmkcache_context));
        if(!context->memmock_cache) {
            LOG_ERROR("Unable to allocate memory for memory mock cache.");
            return false;
        }
        memmkcache_init(context->memmock_cache, &default_expiry);
    }

    thread_init(&context->worker_dump, access_worker, context);

    list_init(&context->current_entries);
    pthread_rwlock_init(&context->rw_lock, NULL);

    return true;
}

bool access_accounting_deinit(struct access_accounting *context) {
    if(!context->memmock_from_outside) {
        memmkcache_deinit(context->memmock_cache);
        free(context->memmock_cache);
    }

    thread_signal_stop(&context->worker_dump);
    thread_join(&context->worker_dump);
    thread_deinit(&context->worker_dump);

    struct access_entry *current, *nextup;
    pthread_rwlock_wrlock(&context->rw_lock);
    list_for_each_entry_safe(current, nextup, &context->current_entries, list) {
        access_entry_deinit(current);
        free(current);
    }
    pthread_rwlock_unlock(&context->rw_lock);

    pthread_rwlock_destroy(&context->rw_lock);
    return true;
}

bool access_accounting_write(struct access_accounting *context, pid_t pid, pid_t tid, void* insn_ptr) {
    bool ret = false;
    pthread_rwlock_wrlock(&context->rw_lock);

    struct access_entry *current, *match = NULL;
    list_for_each_entry(current, &context->current_entries, list) {
        if(current->pid == pid) {
            match = current;
            break;
        }
    }
    if(!match) {
        match = malloc(sizeof(struct access_entry));
        if(!match) {
            LOG_ERROR("Unable to allocate memory for new access' entry");
            goto error;
        }
        access_entry_init(match, pid);
        list_add_before(&context->current_entries, &match->list);
    }

    struct access_accounting_instruction instruction;
    if(!access_accounting_instruction_get(context, &instruction, pid, insn_ptr)) {
        LOG_WARN("Failed to retrieve any information about to be accounted write for pid=%d at ip=%p", pid, insn_ptr);
        goto error;
    }

    if (instruction.access_size == 0) {
        LOG_WARN("Could not determine access_size for write of pid=%d - estimations may be inaccurate", pid);
    }

    access_entry_track_write(match, tid, instruction.access_size, instruction.insn, insn_ptr);
    ret = true;

    error:
    pthread_rwlock_unlock(&context->rw_lock);
    return ret;
}

bool access_accounting_instruction_get(struct access_accounting *context, struct access_accounting_instruction *out, pid_t pid, void* insn_ptr) {
    bool ret = false;
    out->access_size = 0;
    out->insn[0] = '\0';

    struct memmock_context *memmock = memmkcache_get(context->memmock_cache, pid);
    if(!memmock) {
        LOG_WARN("Could not acquire memory mock for pid=%d", pid);
        goto error;
    }

    uint8_t ins_buffer[ACCESS_ACCOUNTING_MAX_INSTRUCTION_LEN_IN_BYTES];
    size_t ins_buffer_len = get_instruction(ins_buffer, memmock, insn_ptr);
    if(ins_buffer_len == 0) {
        LOG_WARN("Could not read the executed instruction of pid=%d at ip=%p", pid, insn_ptr);
        goto error;
    }

    csh handle;
    if(cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK)
        goto error;
    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    cs_insn *insn;
    size_t count = cs_disasm(handle, ins_buffer, ins_buffer_len, (uint64_t)insn_ptr, 0, &insn);
    if(count < 1) {
        LOG_WARN("Unable to decode instruction for pid=%d at ip=%p with content: %x%x%x%x%x (length_read=%zu)", pid, insn_ptr,
                  ((ACCESS_ACCOUNTING_INSTRUCTION_STEP_TYPE*)ins_buffer)[0], ((ACCESS_ACCOUNTING_INSTRUCTION_STEP_TYPE*)ins_buffer)[1],
                  ((ACCESS_ACCOUNTING_INSTRUCTION_STEP_TYPE*)ins_buffer)[2], ((ACCESS_ACCOUNTING_INSTRUCTION_STEP_TYPE*)ins_buffer)[3],
                  ((ACCESS_ACCOUNTING_INSTRUCTION_STEP_TYPE*)ins_buffer)[4], ins_buffer_len);
        goto error_cs;
    }
    int relevant_insn = 0;

    cs_detail *detail = insn[relevant_insn].detail;
    if(detail == NULL) {
        LOG_DEBUG("detail == NULL for instruction: 0x%"PRIx64":\t%s\t\t%s\n", insn[relevant_insn].address, insn[relevant_insn].mnemonic, insn[relevant_insn].op_str);
        goto error_inst;
    }

    strncpy(out->insn, insn[relevant_insn].mnemonic, ACCESS_ACCOUNTING_STRUCT_INSN_LEN-1);
    out->insn[ACCESS_ACCOUNTING_STRUCT_INSN_LEN-1] = '\0';

    cs_x86 *detail_x86 = &(detail->x86);
    for (size_t op_id = 0; op_id < detail_x86->op_count; op_id++) {
        cs_x86_op *op = &(detail_x86->operands[op_id]);
        if (op->type == X86_OP_MEM) {
            //printf("%20s size=%d\n", insn[relevant_insn].mnemonic, op->size);
            out->access_size += op->size;
        }
    }

    ret = true;

    error_inst:
    cs_free(insn, count);
    error_cs:
    cs_close(&handle);
    error:
    return ret;
}

size_t get_instruction(void* buffer, struct memmock_context *memmock, void* insn_ptr) {
    void* translated_ip = memmock_get(memmock,insn_ptr);

    size_t read_bytes;
    for(read_bytes = 0; read_bytes < ACCESS_ACCOUNTING_MAX_INSTRUCTION_LEN_IN_BYTES;
        read_bytes += sizeof(ACCESS_ACCOUNTING_INSTRUCTION_STEP_TYPE)) {
        void* next_bytes_readable = memmock_get(memmock,insn_ptr+read_bytes+sizeof(ACCESS_ACCOUNTING_INSTRUCTION_STEP_TYPE)-1);
        if(!next_bytes_readable) {
            LOG_DEBUG("Populated instruction memory buffer partially: Otherwise out of bounds for in file=%s",
                      memmock_get_label(memmock, insn_ptr));
            // TODO: which process? which offset?
            break;
        }
        *(ACCESS_ACCOUNTING_INSTRUCTION_STEP_TYPE*)(buffer+read_bytes) =
                *(ACCESS_ACCOUNTING_INSTRUCTION_STEP_TYPE*)(translated_ip+read_bytes);

    }

    return read_bytes;
}