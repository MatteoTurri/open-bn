/*
 * do_DSAparams_dup: the DSAparams_dup macro of dsa.h as a function, the callback SSL_enable_DSA
 * registers with DSA_set_default_method_p_dup_cb().
 */
#include <stdio.h>
#include "objects.h"
#include "lhash.h"
#include "ssl_locl.h"

DSA *do_DSAparams_dup(dsa)
DSA *dsa;
	{
	return(DSAparams_dup(dsa));
	}
