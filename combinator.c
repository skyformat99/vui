#include "statemachine.h"

#include "combinator.h"


vui_frag* vui_frag_union(vui_frag* lhs, vui_frag* rhs)
{
	// TODO: union

	vui_frag_kill(rhs);

	return lhs;
}

vui_frag* vui_frag_cat(vui_frag* lhs, vui_frag* rhs)
{
	rhs->entry = vui_frag_release(lhs, rhs->entry);

	return rhs;
}

vui_frag* vui_frag_catv(vui_frag** frags)
{
	vui_frag* frag = frags[0];

	if (frag != NULL)
	{
		int i = 1;
		while (frags[i] != NULL)
		{
			frag = vui_frag_cat(frags[i], frag);
		}
	}

	return frag;
}