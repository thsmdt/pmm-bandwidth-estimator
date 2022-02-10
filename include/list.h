#pragma once

#include <stddef.h>

struct list_node {
    struct list_node *next;
    struct list_node *prev;
};

#define DELCARE_LIST_INIT(name) { &(name), &(name) }

#define DECLARE_LIST(name) \
    struct list_node name = DELCARE_LIST_INIT(name)

static inline void list_init(struct list_node *entry) {
    entry->prev = entry;
    entry->next = entry;
}

static inline void list_add_after(struct list_node *list_node, struct list_node *added) {
    struct list_node *next = list_node->next;
    struct list_node *prev = list_node;
    added->next = next;
    added->prev = prev;
    next->prev = added;
    prev->next = added;
}

static inline void list_add_before(struct list_node *list_node, struct list_node *added) {
    struct list_node *next = list_node;
    struct list_node *prev = list_node->prev;
    added->next = next;
    added->prev = prev;
    next->prev = added;
    prev->next = added;
}

static inline void list_remove(struct list_node *entry) {
    struct list_node *prev = entry->prev;
    struct list_node *next = entry->next;
    prev->next = next;
    next->prev = prev;
}

static inline size_t list_count(struct list_node *node) {
    struct list_node *current = node->next;
    size_t count = 0;
    while(node != current) {
        current = current->next;
        count++;
    }

    return count;
}

#define list_foreach(item, list) \
    for (item = list; item != list; item = item->next)

#define container_of(ptr, type, member) ({         \
    const typeof( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *)__mptr - offsetof(type,member) );})

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)


#define list_entry_is_head(pos, head, member)				\
	(&pos->member == (head))


#define list_for_each_entry(pos, head, member)				\
	for (pos = list_first_entry(head, typeof(*pos), member);	\
	     !list_entry_is_head(pos, head, member);			\
	     pos = list_next_entry(pos, member))

#define list_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = list_next_entry(pos, member), 				\
		n = list_next_entry(pos, member);				\
	     !list_entry_is_head(pos, head, member);				\
	     pos = n, n = list_next_entry(n, member))

#define list_for_each_entry_continue(pos, head, member) 		\
	for (pos = list_next_entry(pos, member);			\
	     !list_entry_is_head(pos, head, member);			\
	     pos = list_next_entry(pos, member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_first_entry(head, typeof(*pos), member),	\
		n = list_next_entry(pos, member);			\
	     !list_entry_is_head(pos, head, member); 			\
	     pos = n, n = list_next_entry(n, member))