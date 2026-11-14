/*
 * Doubly linked lists of HTTP header (name, value) pairs.
 */

#include "http/scehttp.h"

/* Appends a new node with copies of name and value at the tail of list
   (list may be NULL). Returns the new node, not the head. */
struct sceHTTPHeaderList *sceHTTPAddHeaderList(struct sceHTTPHeaderList *list, const char *name, const char *value)
{
	char *name_dup;
	char *value_dup;
	struct sceHTTPHeaderList *node;

	if (name == 0 || value == 0)
		return 0;

	while (list != 0 && list->next != 0)
		list = list->next;

	name_dup = sceHTTPLibStrsave(name);
	if (name_dup == 0)
		return 0;
	value_dup = sceHTTPLibStrsave(value);
	if (value_dup == 0) {
		xfree(name_dup);
		return 0;
	}

	node = xmalloc(sizeof(struct sceHTTPHeaderList));
	if (node == 0) {
		xfree(name_dup);
		xfree(value_dup);
		return 0;
	}
	node->name = name_dup;
	node->value = value_dup;
	node->prev = list;
	node->next = 0;
	if (list != 0)
		list->next = node;
	return node;
}
int sceHTTPFreeHeaderList(struct sceHTTPHeaderList *node)
{
	struct sceHTTPHeaderList *next;

	while (node != 0) {
		next = node->next;
		if (node->name)
			xfree(node->name);
		if (node->value)
			xfree(node->value);
		xfree(node);
		node = next;
	}
	return 0;
}
/* Returns a copy of list, or 0 if an allocation fails. */
struct sceHTTPHeaderList *sceHTTPLibCloneHeaderList(struct sceHTTPHeaderList *list)
{
	struct sceHTTPHeaderList *newlist = 0;
	void *r;

	while (list != 0) {
		r = sceHTTPAddHeaderList(newlist, list->name, list->value);
		if (r == 0) {
			sceHTTPFreeHeaderList(newlist);
			return 0;
		}
		list = list->next;
		if (newlist == 0)
			newlist = (struct sceHTTPHeaderList *)r;
	}
	return newlist;
}
/* Appends list2 to list1 and returns the head. */
struct sceHTTPHeaderList *sceHTTPLibCatHeaderList(struct sceHTTPHeaderList *list1, struct sceHTTPHeaderList *list2)
{
	struct sceHTTPHeaderList *ret;

	if (list1 == 0)
		return list2;
	if (list2 == 0)
		return list1;
	ret = list1;
	while (list1 != 0 && list1->next != 0)
		list1 = list1->next;
	list1->next = list2;
	list2->prev = list1;
	return ret;
}
/* Removes and frees every node whose name matches (case-insensitive).
   Returns the new head. */
struct sceHTTPHeaderList *sceHTTPLibDelHeaderByName(struct sceHTTPHeaderList *list, const char *name)
{
	struct sceHTTPHeaderList *head = list;
	struct sceHTTPHeaderList *next;

	for (; list != 0; list = next) {
		next = list->next;
		if (sceHTTPLibStrcasecmp(list->name, name) != 0)
			continue;
		if (head == list)
			head = head->next;
		if (list->prev != 0)
			list->prev->next = list->next;
		if (list->next != 0)
			list->next->prev = list->prev;
		list->prev = 0;
		list->next = 0;
		sceHTTPFreeHeaderList(list);
	}
	return head;
}
