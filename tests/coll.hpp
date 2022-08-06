#include <iostream>
#include <cstdint>
#include <cstring>
#include <cassert>

#include <shmem.h>
#include <ucc/api/ucc.h>


ucc_lib_h ucc_lib;
ucc_context_h ucc_ctx;
ucc_team_h ucc_team;


ucc_status_t coll_oob_allgather(void* sbuf, void* rbuf, size_t sz, void* coll_info, void** req)
{
    (void)coll_info;
    *req = (void*)0xdeadbeef;

    static long pSync[SHMEM_COLLECT_SYNC_SIZE];
    for (int i = 0; i < SHMEM_COLLECT_SYNC_SIZE; i++) {
        pSync[i] = SHMEM_SYNC_VALUE;
    }

    // Round up to multiple of 4 Bytes, since we use fcollect32
    const size_t sz4 = [&] {
        if (sz % 4 == 0) {
            return sz;
        } else {
            return ((sz / 4) + 1) * 4;
        }
    } ();

    uint8_t* sbuf_symm = (uint8_t*)shmem_malloc(sz4);
    uint8_t* rbuf_symm = (uint8_t*)shmem_malloc(sz4 * shmem_n_pes());

    memcpy(sbuf_symm, sbuf, sz);

    shmem_sync_all();

    // if (shmem_my_pe() == 0) {
    //     std::cout << "OOB AllGather size: " << sz << '\n';
    // }

    shmem_fcollect32(rbuf_symm, sbuf_symm, sz4 / 4, 0, 0, shmem_n_pes(), pSync);

    shmem_sync_all();

    // Byte-by-Byte copy back
    for (int pe = 0; pe < shmem_n_pes(); pe++) {
        for (size_t b = 0; b < sz; b++) {
            ((uint8_t*)rbuf)[pe * sz + b] = rbuf_symm[pe * sz4 + b];
        }
    }

    shmem_free(rbuf_symm);
    shmem_free(sbuf_symm);

    return UCC_OK;
}


ucc_status_t coll_oob_allgather_test(void* req)
{
    (void)req;
    return UCC_OK;
}


ucc_status_t coll_oob_allgather_free(void* req)
{
    (void)req;
    return UCC_OK;
}


void coll_init()
{
    ucc_lib_config_h lib_cfg;
    assert(UCC_OK == ucc_lib_config_read(nullptr, nullptr, &lib_cfg));
    // ucc_lib_config_print(lib_cfg, stdout, "", UCC_CONFIG_PRINT_CONFIG);

    ucc_lib_params_t lib_params;
    lib_params.mask        = UCC_LIB_PARAM_FIELD_THREAD_MODE;
    lib_params.thread_mode = UCC_THREAD_SINGLE;
    assert(UCC_OK == ucc_init(&lib_params, lib_cfg, &ucc_lib));

    ucc_lib_config_release(lib_cfg);

    ucc_context_config_h ctx_cfg;
    assert(UCC_OK == ucc_context_config_read(ucc_lib, nullptr, &ctx_cfg));
    // ucc_context_config_print(ctx_cfg, stdout, "", UCC_CONFIG_PRINT_CONFIG);

    ucc_context_params_t ctx_params;
    ctx_params.mask          = UCC_CONTEXT_PARAM_FIELD_OOB;
    ctx_params.oob.allgather = coll_oob_allgather;
    ctx_params.oob.req_test  = coll_oob_allgather_test;
    ctx_params.oob.req_free  = coll_oob_allgather_free;
    ctx_params.oob.coll_info = nullptr;
    ctx_params.oob.n_oob_eps = shmem_n_pes();
    ctx_params.oob.oob_ep    = shmem_my_pe();
    assert(UCC_OK == ucc_context_create(ucc_lib, &ctx_params, ctx_cfg, &ucc_ctx));

    while (UCC_OK != ucc_context_progress(ucc_ctx)) {}

    ucc_context_config_release(ctx_cfg);

    ucc_team_params_t team_params;
    team_params.mask          = UCC_TEAM_PARAM_FIELD_EP
                              | UCC_TEAM_PARAM_FIELD_EP_RANGE
                              | UCC_TEAM_PARAM_FIELD_OOB;
    team_params.oob.allgather = coll_oob_allgather;
    team_params.oob.req_test  = coll_oob_allgather_test;
    team_params.oob.req_free  = coll_oob_allgather_free;
    team_params.oob.coll_info = nullptr;
    team_params.oob.n_oob_eps = shmem_n_pes();
    team_params.oob.oob_ep    = shmem_my_pe();
    team_params.ep            = shmem_my_pe();
    team_params.ep_range      = UCC_COLLECTIVE_EP_RANGE_CONTIG;
    assert(UCC_OK == ucc_team_create_post(&ucc_ctx, 1, &team_params, &ucc_team));
    do {
        ucc_context_progress(ucc_ctx);
    } while (UCC_INPROGRESS == ucc_team_create_test(ucc_team));
}


void coll_finalize()
{
    assert(UCC_OK == ucc_team_destroy(ucc_team));
    assert(UCC_OK == ucc_context_destroy(ucc_ctx));
    assert(UCC_OK == ucc_finalize(ucc_lib));
}


template <typename T>
ucc_datatype_t coll_select_type()
{
    if (std::is_same<T, int>::value)     return UCC_DT_INT32;
    if (std::is_same<T, int8_t>::value)  return UCC_DT_INT8;
    if (std::is_same<T, int16_t>::value) return UCC_DT_INT16;
    if (std::is_same<T, int32_t>::value) return UCC_DT_INT32;
    if (std::is_same<T, int64_t>::value) return UCC_DT_INT64;
    if (std::is_same<T, uint8_t>::value) return UCC_DT_UINT8;
    if (std::is_same<T, float>::value)   return UCC_DT_FLOAT32;
    if (std::is_same<T, double>::value)  return UCC_DT_FLOAT64;
    return UCC_DT_PREDEFINED_LAST;
}


void shmemx_sync_all()
{
    ucc_coll_req_h req;
    ucc_coll_args_t args;
    memset(&args, 0, sizeof(ucc_coll_args_t));
    args.mask      = UCC_COLL_ARGS_FIELD_FLAGS;
    args.flags     = UCC_COLL_ARGS_FLAG_TIMEOUT;
    args.timeout   = 5;
    args.coll_type = UCC_COLL_TYPE_BARRIER;

    assert(UCC_OK == ucc_collective_init(&args, &req, ucc_team));
    assert(UCC_OK == ucc_collective_post(req));
    do {
        ucc_context_progress(ucc_ctx);
    } while (UCC_OK != ucc_collective_test(req));
    assert(UCC_OK == ucc_collective_finalize(req));
}


void shmemx_bcast(void* buf, size_t sz, int root, ucc_memory_type_t type)
{
    ucc_coll_req_h req;
    ucc_coll_args_t args;
    memset(&args, 0, sizeof(ucc_coll_args_t));
    args.mask              = UCC_COLL_ARGS_FIELD_FLAGS;
    args.flags             = UCC_COLL_ARGS_FLAG_TIMEOUT;
    args.timeout           = 5;
    args.coll_type         = UCC_COLL_TYPE_BCAST;
    args.root              = root;
    args.src.info.buffer   = buf;
    args.src.info.count    = sz;
    args.src.info.datatype = UCC_DT_UINT8;
    args.src.info.mem_type = type;

    assert(UCC_OK == ucc_collective_init(&args, &req, ucc_team));
    assert(UCC_OK == ucc_collective_post(req));
    do {
        ucc_context_progress(ucc_ctx);
    } while (UCC_OK != ucc_collective_test(req));
    assert(UCC_OK == ucc_collective_finalize(req));
}


template <typename T>
void shmemx_reduce(T* dst, const T* src, ucc_reduction_op_t op, ucc_memory_type_t type)
{
    ucc_coll_req_h req;
    ucc_coll_args_t args;
    memset(&args, 0, sizeof(ucc_coll_args_t));
    args.mask              = UCC_COLL_ARGS_FIELD_FLAGS;
    args.flags             = UCC_COLL_ARGS_FLAG_TIMEOUT;
    args.timeout           = 5;
    args.coll_type         = UCC_COLL_TYPE_ALLREDUCE;
    args.op                = op;
    args.dst.info.buffer   = dst;
    args.dst.info.count    = 1;
    args.dst.info.datatype = coll_select_type<T>();
    args.dst.info.mem_type = type;

    if (dst == src) {
        args.flags |= UCC_COLL_ARGS_FLAG_IN_PLACE;
    } else {
        args.src.info.buffer   = reinterpret_cast<void*>(const_cast<T*>(src));
        args.src.info.count    = 1;
        args.src.info.datatype = coll_select_type<T>();
        args.src.info.mem_type = type;
    }

    assert(UCC_OK == ucc_collective_init(&args, &req, ucc_team));
    assert(UCC_OK == ucc_collective_post(req));
    do {
        ucc_context_progress(ucc_ctx);
    } while (UCC_OK != ucc_collective_test(req));
    assert(UCC_OK == ucc_collective_finalize(req));
}


template <typename T>
void shmemx_alltoall(T* dst, const T* src, size_t nelems, ucc_memory_type_t type)
{
    (void)type;
    const int mype = shmem_my_pe();
    const int npes = shmem_n_pes();

    for (int p = 0; p < npes; p++) {
        shmem_putmem_nbi(dst + nelems * mype, src + nelems * p, nelems * sizeof(T), p);
    }
    shmem_quiet();
}
// template <typename T>
// void shmemx_alltoall(T* dst, const T* src, size_t nelems, ucc_memory_type_t type)
// {
//     ucc_coll_req_h req;
//     ucc_coll_args_t args;
//     memset(&args, 0, sizeof(ucc_coll_args_t));
//     args.mask              = UCC_COLL_ARGS_FIELD_FLAGS;
//     args.flags             = UCC_COLL_ARGS_FLAG_TIMEOUT;
//     args.timeout           = 5;
//     args.coll_type         = UCC_COLL_TYPE_ALLTOALL;
//     args.dst.info.buffer   = dst;
//     args.dst.info.count    = nelems * shmem_n_pes();;
//     args.dst.info.datatype = coll_select_type<T>();
//     args.dst.info.mem_type = type;
//
//     if (dst == src) {
//         args.flags |= UCC_COLL_ARGS_FLAG_IN_PLACE;
//     } else {
//         args.src.info.buffer   = reinterpret_cast<void*>(const_cast<T*>(src));
//         args.src.info.count    = nelems * shmem_n_pes();;
//         args.src.info.datatype = coll_select_type<T>();
//         args.src.info.mem_type = type;
//     }
//
//     assert(UCC_OK == ucc_collective_init(&args, &req, ucc_team));
//     assert(UCC_OK == ucc_collective_post(req));
//     do {
//         ucc_context_progress(ucc_ctx);
//     } while (UCC_OK != ucc_collective_test(req));
//     assert(UCC_OK == ucc_collective_finalize(req));
// }
