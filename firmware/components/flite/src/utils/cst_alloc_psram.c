/*************************************************************************/
/*  PSRAM-backed flite allocator.                                        */
/*                                                                       */
/*  Selected via CST_USER_MALLOC (defined for this component in          */
/*  CMakeLists.txt), which compiles out the default malloc-based         */
/*  cst_safe_alloc/calloc/realloc/cst_free in cst_alloc.c. This moves    */
/*  all flite synthesis working memory off internal RAM into PSRAM.      */
/*                                                                       */
/*  Semantics mirror the originals: the alloc/calloc paths return        */
/*  zero-initialised memory (flite relies on it), realloc does not zero  */
/*  the grown region, and allocation failure routes through cst_error(). */
/*************************************************************************/
#include "cst_alloc.h"
#include "cst_error.h"
#include <esp_heap_caps.h>

#define FLITE_ALLOC_CAPS (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)

void *cst_safe_alloc(int size)
{
    void *p = NULL;

    if (size < 0)
    {
        cst_errmsg("alloc: asked for negative size %d\n", size);
        cst_error();
    }
    else if (size == 0) /* some mallocs return NULL for this */
        size++;

    /* Zeroed, matching the original calloc-based behaviour flite relies on. */
    p = heap_caps_calloc(1, (size_t)size, FLITE_ALLOC_CAPS);

    if (p == NULL)
    {
        cst_errmsg("alloc: can't alloc %d bytes\n", size);
        cst_error();
    }

    return p;
}

void *cst_safe_calloc(int size)
{
    return cst_safe_alloc(size);
}

void *cst_safe_realloc(void *p, int size)
{
    void *np;

    if (size == 0) /* as some mallocs do strange things with 0 */
        size++;

    if (p == NULL)
        return cst_safe_alloc(size);

    np = heap_caps_realloc(p, (size_t)size, FLITE_ALLOC_CAPS);

    if (np == NULL)
    {
        cst_errmsg("CST_REALLOC failed for %d bytes\n", size);
        cst_error();
    }

    return np;
}

void cst_free(void *p)
{
    if (p != NULL)
        heap_caps_free(p);
}
