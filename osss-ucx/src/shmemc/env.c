/* For license: see LICENSE file at top-level */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "state.h"
#include "shmemu.h"
#include "shmemc.h"
#include "boolean.h"
#include "collectives/defaults.h"
#include "module.h"

#include <stdio.h>
#include <stdlib.h>             /* getenv */
#include <ctype.h>
#include <string.h>
#include <strings.h>

/*
 * for string formatting
 */
#define BUFSIZE 16

/*
 * detect whether option enabled
 */

static bool
option_enabled_test(const char *str)
{
    if (str == NULL) {
        return false;
        /* NOT REACHED */
    }
    if (tolower(*str) == 'y') {
        return true;
        /* NOT REACHED */
    }
    if (strncasecmp(str, "on", 2) == 0) {
        return true;
        /* NOT REACHED */
    }
    if (strtol(str, NULL, 10) != 0L) {
        return true;
        /* NOT REACHED */
    }
    return false;
}

/*
 * read & save all our environment variables
 */

#define CHECK_ENV(_e, _name)                    \
    do {                                        \
        (_e) = getenv("SHMEM_" #_name);         \
    } while (0)

#define CHECK_ENV_WITH_DEPRECATION(_e, _name)   \
    do {                                        \
        CHECK_ENV(_e, _name);                   \
        if ((_e) == NULL) {                     \
            (_e) = getenv("SMA_" #_name);       \
        }                                       \
    } while (0)

void
shmemc_env_init(void)
{
    char *e;
    int r, i;
    char *delay;
    char env_buf[30];

    /*
     * defined in spec
     */

    proc.env.print_version = false;
    proc.env.print_info    = false;
    proc.env.debug         = false;

    CHECK_ENV_WITH_DEPRECATION(e, VERSION);
    if (e != NULL) {
        proc.env.print_version = option_enabled_test(e);
    }
    CHECK_ENV_WITH_DEPRECATION(e, INFO);
    if (e != NULL) {
        proc.env.print_info = option_enabled_test(e);
    }
    CHECK_ENV_WITH_DEPRECATION(e, DEBUG);
    if (e != NULL) {
        proc.env.debug = option_enabled_test(e);
    }
    CHECK_ENV_WITH_DEPRECATION(e, SYMMETRIC_SIZE);
    proc.env.heap_spec = strdup(
                                e != NULL
                                ? e
                                : SHMEM_DEFAULT_HEAP_SIZE
                                ); /* free@end */

    CHECK_ENV(e, MAX_PARTITIONS);
    proc.env.max_sps = (e != NULL) ? atoi(e) : 8;   /* TODO: cap w/ omp_get_num_devices */
    proc.env.n_sps = 0;

    proc.env.sp_specs = (char**)calloc(proc.env.max_sps, sizeof(char*));

    for (i = 0; i < proc.env.max_sps; i++) {
        snprintf(env_buf, sizeof(env_buf), "SHMEM_SYMMETRIC_PARTITION%d", i);
        e = getenv(env_buf);
        if (e != NULL) {
            proc.env.sp_specs[i] = strdup(e);
            proc.env.n_sps++;
        } else {
            break;
        }
    }

    /*
     * this implementation also has...
     */

    proc.env.logging           = false;
    proc.env.logging_events    = NULL;
    proc.env.logging_file      = NULL;

    CHECK_ENV(e, LOGGING);
    if (e != NULL) {
        proc.env.logging = option_enabled_test(e);
    }
    CHECK_ENV(e, LOGGING_FILE);
    if (e != NULL) {
        proc.env.logging_file = strdup(e); /* free@end */
    }
    CHECK_ENV(e, LOGGING_EVENTS);
    if (e != NULL) {
        proc.env.logging_events = strdup(e); /* free@end */
    }

    proc.env.coll.barrier       = NULL;
    proc.env.coll.barrier_all   = NULL;
    proc.env.coll.sync          = NULL;
    proc.env.coll.sync_all      = NULL;
    proc.env.coll.broadcast     = NULL;
    proc.env.coll.collect       = NULL;
    proc.env.coll.fcollect      = NULL;
    proc.env.coll.alltoall      = NULL;
    proc.env.coll.alltoalls     = NULL;
    proc.env.coll.reductions    = NULL;

    CHECK_ENV(e, BARRIER_ALGO);
    proc.env.coll.barrier =
        strdup( (e != NULL) ? e : COLLECTIVES_DEFAULT_BARRIER );
    CHECK_ENV(e, BARRIER_ALL_ALGO);
    proc.env.coll.barrier_all =
        strdup( (e != NULL) ? e : COLLECTIVES_DEFAULT_BARRIER_ALL );
    CHECK_ENV(e, SYNC_ALGO);
    proc.env.coll.sync =
        strdup( (e != NULL) ? e : COLLECTIVES_DEFAULT_SYNC );
    CHECK_ENV(e, SYNC_ALL_ALGO);
    proc.env.coll.sync_all =
        strdup( (e != NULL) ? e : COLLECTIVES_DEFAULT_SYNC_ALL );
    CHECK_ENV(e, BROADCAST_ALGO);
    proc.env.coll.broadcast =
        strdup( (e != NULL) ? e : COLLECTIVES_DEFAULT_BROADCAST );
    CHECK_ENV(e, COLLECT_ALGO);
    proc.env.coll.collect =
        strdup( (e != NULL) ? e : COLLECTIVES_DEFAULT_COLLECT );
    CHECK_ENV(e, FCOLLECT_ALGO);
    proc.env.coll.fcollect =
        strdup( (e != NULL) ? e : COLLECTIVES_DEFAULT_FCOLLECT );
    CHECK_ENV(e, ALLTOALL_ALGO);
    proc.env.coll.alltoall =
        strdup( (e != NULL) ? e : COLLECTIVES_DEFAULT_ALLTOALL );
    CHECK_ENV(e, ALLTOALLS_ALGO);
    proc.env.coll.alltoalls =
        strdup( (e != NULL) ? e : COLLECTIVES_DEFAULT_ALLTOALLS );
    CHECK_ENV(e, REDUCE_ALGO);
    /* TODO currently ignored */
    proc.env.coll.reductions =
        strdup( (e != NULL) ? e : COLLECTIVES_DEFAULT_REDUCTIONS );
    /* collectives to free@end */

    proc.env.progress_threads = NULL;

    CHECK_ENV(e, PROGRESS_THREADS);
    if (e != NULL) {
        proc.env.progress_threads = strdup(e); /* free@end */
    }

    delay = "1000";             /* magic number */
    proc.env.progress_delay_ns = strtol(delay, NULL, 10);

    CHECK_ENV(e, PROGRESS_DELAY);

    r = shmemu_parse_size(e != NULL ? e : delay,
                          &proc.env.progress_delay_ns);
    shmemu_assert(r == 0,
                  MODULE ": couldn't work out requested "
                  "progress delay time \"%s\"",
                  e != NULL ? e : delay);

    proc.env.prealloc_contexts = 64; /* magic number */

    CHECK_ENV(e, PREALLOC_CTXS);
    if (e != NULL) {
        long n = strtol(e, NULL, 10);

        if (n < 0) {
            n = proc.env.prealloc_contexts;
        }
        proc.env.prealloc_contexts = (size_t) n;
    }

    proc.env.memfatal = true;

    CHECK_ENV(e, MEMERR_FATAL);
    if (e != NULL) {
        proc.env.memfatal = option_enabled_test(e);
    }
}

#undef CHECK_ENV
#undef CHECK_ENV_WITH_DEPRECATION

void
shmemc_env_finalize(void)
{
    int i;

    free(proc.env.logging_file);
    free(proc.env.logging_events);
    free(proc.env.heap_spec);

    for (i = 0; i < proc.env.n_sps; i++) {
        free(proc.env.sp_specs[i]);
    }
    free(proc.env.sp_specs);

    free(proc.env.coll.reductions);
    free(proc.env.coll.alltoalls);
    free(proc.env.coll.alltoall);
    free(proc.env.coll.fcollect);
    free(proc.env.coll.collect);
    free(proc.env.coll.sync_all);
    free(proc.env.coll.sync);
    free(proc.env.coll.barrier_all);
    free(proc.env.coll.barrier);
    free(proc.env.coll.broadcast);

    free(proc.env.progress_threads);

}

/*
 * all terminals are 80 columns, right? :)
 */

static const int var_width = 22;
static const int val_width = 10;
static const int hr_width  = 74;

inline static void
hr(FILE *stream, const char *prefix)
{
    int i;

    fprintf(stream, "%s", prefix);
    for (i = 0; i < hr_width; ++i) {
        fprintf(stream, "-");
    }
    fprintf(stream, "\n");
}

void
shmemc_print_env_vars(FILE *stream, const char *prefix)
{
    fprintf(stream, "%sEnvironment Variable Information.  "
            "See oshrun(1) for more.\n",
            prefix);
    fprintf(stream, "%s\n", prefix);
    fprintf(stream, "%s%-*s %-*s %s\n",
            prefix,
            var_width, "Variable",
            val_width, "Value",
            "Description");
    hr(stream, prefix);
    fprintf(stream, "%s\n", prefix);
    fprintf(stream, "%s%s\n",
            prefix,
            "From specification:");
    fprintf(stream, "%s\n", prefix);
    fprintf(stream, "%s%-*s %-*s %s\n",
            prefix,
            var_width, "SHMEM_VERSION",
            val_width, shmemu_human_option(proc.env.print_version),
            "print library version at start-up");
    fprintf(stream, "%s%-*s %-*s %s\n",
            prefix,
            var_width, "SHMEM_INFO",
            val_width, shmemu_human_option(proc.env.print_info),
            "print this information");
    {
        char buf[BUFSIZE];

        /* TODO hardwired index */
        (void) shmemu_human_number(proc.heaps.heapsize[0], buf, BUFSIZE);
        fprintf(stream, "%s%-*s %-*s %s\n",
                prefix,
                var_width, "SHMEM_SYMMETRIC_SIZE",
                val_width, buf,
                "requested size of the symmetric heap");
    }
    fprintf(stream, "%s%-*s %-*s %s\n",
            prefix,
            var_width, "SHMEM_DEBUG",
            val_width, shmemu_human_option(proc.env.debug),
            "enable sanity checking ("
#if ! defined(ENABLE_DEBUG)
            "not "
#endif /* ! ENABLE_DEBUG */
            "configured)"
            );

    fprintf(stream, "%s\n", prefix);
    fprintf(stream, "%s%s\n",
            prefix,
            "Specific to this implementation:");
    fprintf(stream, "%s\n", prefix);
    fprintf(stream, "%s%-*s %-*s %s\n",
            prefix,
            var_width, "SHMEM_LOGGING",
            val_width, shmemu_human_option(proc.env.logging),
            "enable logging messages ("
#if ! defined(ENABLE_LOGGING)
            "not "
#endif /* ! ENABLE_LOGGING */
            "configured)"
            );

    fprintf(stream, "%s%-*s %-*s %s\n",
            prefix,
            var_width, "SHMEM_LOGGING_EVENTS",
            val_width, "...",   /* could be far too long to show */
            "types of logging events to show");
    fprintf(stream, "%s%-*s %-*s %s\n",
            prefix,
            var_width, "SHMEM_LOGGING_FILE",
            val_width, proc.env.logging_file ? proc.env.logging_file : "unset",
            "file for logging information");

#define DESCRIBE_COLLECTIVE(_name, _envvar)                             \
    do {                                                                \
        fprintf(stream, "%s%-*s %-*s: %s\n",                            \
                prefix,                                                 \
                var_width, "SHMEM_" #_envvar "_ALGO",                   \
                val_width,                                              \
                proc.env.coll._name ? proc.env.coll._name : "unset",    \
                "algorithm for \"" #_name "\" routine");                \
    } while (0)

    DESCRIBE_COLLECTIVE(barrier, BARRIER);
    DESCRIBE_COLLECTIVE(barrier_all, BARRIER_ALL);
    DESCRIBE_COLLECTIVE(sync, SYNC);
    DESCRIBE_COLLECTIVE(sync_all, SYNC_ALL);
    DESCRIBE_COLLECTIVE(broadcast, BROADCAST);
    DESCRIBE_COLLECTIVE(collect, COLLECT);
    DESCRIBE_COLLECTIVE(fcollect, FCOLLECT);
    DESCRIBE_COLLECTIVE(alltoall, ALLTOALL);
    DESCRIBE_COLLECTIVE(alltoalls, ALLTOALLS);
    DESCRIBE_COLLECTIVE(reductions, REDUCE);

    fprintf(stream, "%s%-*s %-*s %s\n",
            prefix,
            var_width, "SHMEM_PROGRESS_THREADS",
            val_width,
            proc.env.progress_threads ? proc.env.progress_threads : "none",
            "PEs that need progress threads");
    fprintf(stream, "%s%-*s %-*lu %s",
            prefix,
            var_width, "SHMEM_PROGRESS_DELAY",
            val_width,
            (unsigned long) proc.env.progress_delay_ns,
            "delay between progress polls (ns)");
    if (proc.env.progress_threads == NULL) {
        fprintf(stream, " [not used]");
    }
    fprintf(stream, "\n");
    fprintf(stream, "%s%-*s %-*lu %s\n",
            prefix,
            var_width, "SHMEM_PREALLOC_CTXS",
            val_width, (unsigned long) proc.env.prealloc_contexts,
            "pre-allocate contexts at startup");
    fprintf(stream, "%s%-*s %-*s %s\n",
            prefix,
            var_width, "SHMEM_MEMERR_FATAL",
            val_width, proc.env.memfatal ? "yes" : "no",
            "abort if symmetric memory corruption");

    /* ---------------------------------------------------------------- */

    fprintf(stream, "%s\n", prefix);
    hr(stream, prefix);
    fprintf(stream, "%s\n", prefix);
    fprintf(stream, "\n");

    fflush(stream);
}
