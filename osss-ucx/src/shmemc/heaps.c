/* For license: see LICENSE file at top-level */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "state.h"
#include "shmemu.h"
#include "module.h"

#include <stdio.h>
#include <stdlib.h>

void
shmemc_heaps_init(void)
{
    size_t hs, ts;
    int r, i;
    char* sp_spec_tok;

    /* for now: could change with multiple heaps */
    proc.heaps.nheaps = 1 + proc.env.n_sps;

    hs = proc.heaps.nheaps * sizeof(*proc.heaps.heapsize);

    proc.heaps.heapsize = (size_t *) malloc(hs);

    shmemu_assert(proc.heaps.heapsize != NULL,
                  MODULE ": can't allocate memory for %lu heap%s",
                  (unsigned long) proc.heaps.nheaps,
                  shmemu_plural(proc.heaps.nheaps));

    ts = proc.heaps.nheaps * sizeof(*proc.heaps.type);

    proc.heaps.type = (symm_type_t*) malloc(ts);

    r = shmemu_parse_size(proc.env.heap_spec, &proc.heaps.heapsize[0]);
    shmemu_assert(r == 0,
                  MODULE ": couldn't work out requested heap size \"%s\"",
                  proc.env.heap_spec);

    proc.heaps.type[0] = SYMM_TYPE_MAIN;

    for (int i = 1; i < proc.heaps.nheaps; i++) {
        sp_spec_tok = strtok(proc.env.sp_specs[i - 1], ":");
        sp_spec_tok = strchr(sp_spec_tok, '=');
        r = shmemu_parse_size(sp_spec_tok + 1, &proc.heaps.heapsize[i]);
        shmemu_assert(r == 0,
                      MODULE ": couldn't work out requested partition size \"%s\"",
                      sp_spec_tok);

        sp_spec_tok = strtok(NULL, ":");
        sp_spec_tok = strstr(sp_spec_tok, "LIBOMP");

        if (sp_spec_tok != NULL) {
            proc.heaps.type[i] = SYMM_TYPE_LIBOMP;
        } else {
            proc.heaps.type[i] = SYMM_TYPE_UNKNOWN;
        }
    }
}

void
shmemc_heaps_finalize(void)
{
    free(proc.heaps.heapsize);
    free(proc.heaps.type);
}
