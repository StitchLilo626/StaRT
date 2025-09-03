/**
 * @file list.c
 * @brief Intrusive doubly-linked circular list primitives.
 * @version 1.0.2
 * @date 2025-08-26
 * @author
 *   StitchLilo626
 * @note
 *   History:
 *     - 2025-08-26 1.0.2 StitchLilo626: Translate comments to English.
 */

#include "sdef.h"
#include "start.h"

/**
 * @brief Initialize list head (points to itself).
 */
void s_list_init(s_plist l)
{
    l->next = l->prev = l;
}

/**
 * @brief Insert node n right after node l.
 */
void s_list_insert_after(s_plist l, s_plist n)
{
    l->next->prev = n;
    n->next       = l->next;
    l->next       = n;
    n->prev       = l;
}

/**
 * @brief Insert node n right before node l.
 */
void s_list_insert_before(s_plist l, s_plist n)
{
    l->prev->next = n;
    n->prev       = l->prev;
    l->prev       = n;
    n->next       = l;
}

/**
 * @brief Remove node d from list and self-point it.
 */
void s_list_delete(s_plist d)
{
    d->next->prev = d->prev;
    d->prev->next = d->next;
    d->next = d->prev = d;
}

/**
 * @brief Check if list head is empty.
 * @return 1 if empty else 0.
 */
int s_list_isempty(s_plist l)
{
    return l->next == l;
}
