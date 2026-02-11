/* Doubly linked list macros compatible with Linux kernel's version
 * Copyright (c) 2015 by Takashi Iwai <tiwai@suse.de>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 */

#ifndef _UCAM_LIST_H
#define _UCAM_LIST_H

#include <stddef.h>

struct ucam_list_head {
	struct ucam_list_head *next;
	struct ucam_list_head *prev;
};

/* one-shot definition of a list head */
#define UCAM_LIST_HEAD(x) \
	struct ucam_list_head x = { &x, &x }

/* initialize a list head explicitly */
static inline void UCAM_INIT_LIST_HEAD(struct ucam_list_head *p)
{
	p->next = p->prev = p;
}

#define ucam_list_entry_offset(p, type, offset) \
	((type *)((char *)(p) - (offset)))

/* list_entry - retrieve the original struct from ucam_list_head
 * @p: ucam_list_head pointer
 * @type: struct type
 * @member: struct field member containing the ucam_list_head
 */
#define ucam_list_entry(p, type, member) \
	ucam_list_entry_offset(p, type, offsetof(type, member))

/* list_for_each - iterate over the linked list
 * @p: iterator, a ucam_list_head pointer variable
 * @list: ucam_list_head pointer containing the list
 */
#define ucam_list_for_each(p, list) \
	for (p = (list)->next; p != (list); p = p->next)

/* list_for_each_safe - iterate over the linked list, safe to delete
 * @p: iterator, a ucam_list_head pointer variable
 * @s: a temporary variable to keep the next, a ucam_list_head pointer, too
 * @list: ucam_list_head pointer containing the list
 */
#define ucam_list_for_each_safe(p, s, list) \
	for (p = (list)->next; s = p->next, p != (list); p = s)

/* list_add - prepend a list entry at the head
 * @p: the new list entry to add
 * @list: the list head
 */
static inline void ucam_list_add(struct ucam_list_head *p, struct ucam_list_head *list)
{
	struct ucam_list_head *first = list->next;

	p->next = first;
	first->prev = p;
	list->next = p;
	p->prev = list;
}

/* list_add_tail - append a list entry at the tail
 * @p: the new list entry to add
 * @list: the list head
 */
static inline void ucam_list_add_tail(struct ucam_list_head *p, struct ucam_list_head *list)
{
	struct ucam_list_head *last = list->prev;

	last->next = p;
	p->prev = last;
	p->next = list;
	list->prev = p;
}

/* list_del - delete the given list entry */
static inline void ucam_list_del(struct ucam_list_head *p)
{
	p->prev->next = p->next;
	p->next->prev = p->prev;
}

/* list_empty - returns 1 if the given list is empty */
static inline int ucam_list_empty(const struct ucam_list_head *p)
{
	return p->next == p;
}

#endif /* _UCAM_LIST_H */
