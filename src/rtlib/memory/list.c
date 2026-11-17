/*
 * Project: FreeBASIC runtime library
 * File: memory/list.c
 * Purpose: generic internal lists based on static arrays.
 * Responsibilities: Raw memory operations and internal list storage.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <stdint.h>

/** Initializes a list.
 *
 * This list implementation is based on a static array.
 *
 * @param list      Pointer to list structure to initialize.
 * @param table     Pointer to the pool of available list elements.
 * @param elem_size Size of elements in the array.
 * @param size      Number of elements in the array.
 */
void fb_hListInit( FB_LIST *list, void *table, size_t elem_size, size_t size )
{
	size_t i;
#if defined(_MSC_VER)
	size_t alignment = (size_t)__alignof( FB_LISTELEM );
	#else
	size_t alignment = (size_t)__alignof__( FB_LISTELEM );
	#endif
	FB_LISTELEM *next;
	FB_LISTELEM *entry;
	void *element_storage;
	unsigned char *elem = (unsigned char *)table;

	if( list == NULL )
		return;
	fb_hListDynInit( list );

	/* Each free-list node stores pointers, so its pool and stride must stay aligned. */
	if( size == 0 || table == NULL || elem_size < sizeof( FB_LISTELEM ) ||
		((uintptr_t)table % alignment) != 0 || (elem_size % alignment) != 0 ||
		(size > SIZE_MAX / elem_size) )
		return;

	element_storage = elem;
	list->fhead = element_storage;

	for( i = 0; i < size; i++ )
	{
		if( i < size-1 )
		{
			element_storage = elem + elem_size;
			next = element_storage;
		}
		else
			next = NULL;
		element_storage = elem;
		entry = element_storage;
		entry->prev = NULL;
		entry->next = next;

		elem += elem_size;
	}
}

/** Allocate a new list element.
 *
 * This function gets an element from the list of free elements
 * ( struct FB_LIST_::fhead ) and adds to the tail. It also increases the
 * number of used elements ( struct FB_LIST_::cnt ).
 *
 * @param list      Pointer to the list structure.
 *
 * @return A new element.
 */
FB_LISTELEM *fb_hListAllocElem( FB_LIST *list )
{
	FB_LISTELEM *elem;

	/* take from free list */
	elem = list->fhead;
	if( elem == NULL )
		return NULL;

	list->fhead = elem->next;

    /* add to entry used list */
    fb_hListDynElemAdd( list, elem );

	return elem;
}

/** Free a list element.
 *
 * This function frees a list element by removing it from the list of
 * used elements and adding it to the list of free elements
 * ( struct FB_LIST_::fhead ). It also decreses the number of used
 * elements ( struct FB_LIST_::cnt ).
 *
 * @param list      Pointer to the list structure.
 * @param elem      List element to add to the list of free elements.
 */
void fb_hListFreeElem( FB_LIST *list, FB_LISTELEM *elem )
{
    /* remove entry from the list of used elements */
    fb_hListDynElemRemove( list, elem );

	/* add to free list */
	elem->next = list->fhead;
	list->fhead = elem;
}

/* end of memory/list.c */
