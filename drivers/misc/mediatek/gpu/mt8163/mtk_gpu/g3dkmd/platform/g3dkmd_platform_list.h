/*
 * Copyright (c) 2014 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _G3DKMD_PLATFORM_LIST_H_
#define _G3DKMD_PLATFORM_LIST_H_

/* 1. linux list list */
/* header */
#if defined(linux) && !defined(linux_user_mode) /* linux kernel */
#include <linux/kernel.h>
#include <linux/list.h>
#else /* WIN or linux_user */
#include "platform/list_head/list.h"
#endif
/* structs */

#define g3dkmd_list_head_t struct list_head

/* macros */

#define G3DKMD_LIST_HEAD_INIT   LIST_HEAD_INIT
#define G3DKMD_LIST_HEAD        LIST_HEAD
#define G3DKMD_INIT_LIST_HEAD   INIT_LIST_HEAD
/**
 * g3dkmd_list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
#define g3dkmd_list_add list_add

/**
 * g3dkmd_list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
#define g3dkmd_list_add_tail list_add_tail

/**
 * g3dkmd_list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is in an undefined state.
 */
#define g3dkmd_list_del list_del

/**
 * g3dkmd_list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
#define g3dkmd_list_del_init  list_del_init

/**
 * g3dkmd_list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
#define g3dkmd_list_move list_move

/**
 * g3dkmd_list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
#define g3dkmd_list_move_tail list_move_tail

/**
 * g3dkmd_list_empty - tests whether a list is empty
 * @head: the list to test.
 */
#define g3dkmd_list_empty lis_empty

/**
 * g3dkmd_list_splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
#define g3dkmd_list_splice list_splice

/**
 * g3dkmd_list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
#define g3dkmd_list_splice_init list_splice_init

/**
 * g3dkmd_list_entry - get the struct for this entry
 * @ptr:    the &struct list_head pointer.
 * @type:    the type of the struct this is embedded in.
 * @member:    the name of the list_struct within the struct.
 */
#define g3dkmd_list_entry       list_entry

/**
 * g3dkmd_list_for_each    -    iterate over a list
 * @pos:    the &struct list_head to use as a loop counter.
 * @head:    the head for your list.
 */
#define g3dkmd_list_for_each list_for_each
/**
 * g3dkmd_list_for_each_prev    -    iterate over a list backwards
 * @pos:    the &struct list_head to use as a loop counter.
 * @head:    the head for your list.
 */
#define g3dkmd_list_for_each_prev list_for_each_prev

/**
 * g3dkmd_list_for_each_safe    -    iterate over a list safe against removal of list entry
 * @pos:    the &struct list_head to use as a loop counter.
 * @n:        another &struct list_head to use as temporary storage
 * @head:    the head for your list.
 */
#define g3dkmd_list_for_each_safe list_for_each_safe

/**
 * g3dkmd_list_for_each_entry    -    iterate over list of given type
 * @pos:    the type * to use as a loop counter.
 * @head:    the head for your list.
 * @member:    the name of the list_struct within the struct.
 */
#define g3dkmd_list_for_each_entry list_for_each_entry

/**
 * g3dkmd_list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:    the type * to use as a loop counter.
 * @n:        another type * to use as temporary storage
 * @head:    the head for your list.
 * @member:    the name of the list_struct within the struct.
 */
#define g3dkmd_list_for_each_entry_safe list_for_each_entry_safe

#endif /* _G3DKMD_PLATFORM_LIST_H_ */
