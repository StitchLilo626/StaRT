#include "sdef.h"
#include "start.h"


void s_list_init(s_plist l)
{
	l->next=l->prev=l;
}

void s_list_insert_after(s_plist l,s_plist n)
{
	l->next->prev=n;
	n->next=l->next;
	l->next=n;
	n->prev=l;
}

void s_list_insert_before(s_plist l,s_plist n)
{
	l->prev->next=n;
	n->prev=l->prev;
	l->prev=n;
	n->next=l;
}

void s_list_delete(s_plist d)
{
	d->next->prev=d->prev;
	d->prev->next=d->next;
	d->next=d->prev=d;
}

int s_list_isempty(s_plist l)
{
    return l->next == l;
}
