#include "tr_cod1_lightmap_packer.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(char *error, size_t cap, const char *format, ...)
{
    va_list args;
    if (error && cap) {
        va_start(args, format);
        vsnprintf(error, cap, format, args);
        va_end(args);
    }
    return 0;
}

static int soup_cmp(const void *left, const void *right)
{
    const bsp59_lm_soup_t *a = (const bsp59_lm_soup_t *)left;
    const bsp59_lm_soup_t *b = (const bsp59_lm_soup_t *)right;
    if (a->material != b->material) return a->material < b->material ? -1 : 1;
    if (a->page != b->page) return a->page < b->page ? -1 : 1;
    return 0;
}

int bsp59_lm_plan(const bsp59_lm_soup_t *soups, size_t soup_count,
                 uint32_t material_count, uint32_t source_rgb_pages,
                 int32_t max_texture_size, bsp59_lm_layout_t *layout,
                 char *error, size_t error_size)
{
    bsp59_lm_layout_t result;
    int32_t weights[BSP59_LM_MAX_PAGES][BSP59_LM_MAX_PAGES];
    int32_t counts[BSP59_LM_MAX_PAGES];
    unsigned char used[BSP59_LM_MAX_PAGES];
    bsp59_lm_soup_t *sorted = NULL;
    size_t pos, begin, end, k;
    uint32_t pages = 0, width, height, placed, i, j, p;

#define REJECT(...) do { fail(error, error_size, __VA_ARGS__); goto rejected; } while (0)
    if (!layout) return fail(error, error_size, "null layout");
    memset(layout, 0, sizeof(*layout));
    if (error && error_size) error[0] = '\0';
    if (soup_count && !soups) REJECT("null soup array");
    if (max_texture_size < (int32_t)BSP59_LM_TILE_SIZE)
        REJECT("maxTextureSize must be at least 512");
    width = height = (uint32_t)max_texture_size / BSP59_LM_TILE_SIZE;
    if ((uint64_t)width * height > INT32_MAX)
        REJECT("initial tile product exceeds signed 32-bit range");
    if (soup_count > SIZE_MAX / sizeof(*sorted))
        REJECT("soup allocation size overflow");
    if (soup_count) {
        sorted = (bsp59_lm_soup_t *)malloc(soup_count * sizeof(*sorted));
        if (!sorted) REJECT("cannot allocate soup working copy");
        memcpy(sorted, soups, soup_count * sizeof(*sorted));
    }
    memset(&result, 0, sizeof(result));
    memset(weights, 0, sizeof(weights));
    memset(used, 0, sizeof(used));
    result.source_rgb_pages = source_rgb_pages;
    result.material_count = material_count;

    for (pos = 0; pos < soup_count; ++pos) {
        const bsp59_lm_soup_t *s = &sorted[pos];
        if (s->material < 0 || (uint32_t)s->material >= material_count ||
            s->material > INT16_MAX)
            REJECT("soup %zu has invalid material %d", pos, s->material);
        if (s->page < -1 || s->page > INT16_MAX ||
            (s->page >= 0 && (uint32_t)s->page >= source_rgb_pages))
            REJECT("soup %zu has invalid RGB page %d", pos, s->page);
        if (s->vertices < 0 || s->vertices > INT16_MAX)
            REJECT("soup %zu has invalid vertex count %d", pos, s->vertices);
        if (s->page < 0) ++result.unlit_soups;
        else {
            ++result.lightmapped_soups;
            if ((uint32_t)s->page >= pages) pages = (uint32_t)s->page + 1u;
        }
    }
    result.derived_pages = pages;
    if (pages > BSP59_LM_MAX_PAGES)
        REJECT("referenced RGB page range %u exceeds 128", pages);
    if (soup_count) qsort(sorted, soup_count, sizeof(*sorted), soup_cmp);
    for (begin = 0; begin < soup_count; begin = end) {
        int active = 0;
        memset(counts, 0, sizeof(counts));
        for (end = begin; end < soup_count &&
             sorted[end].material == sorted[begin].material; ++end) {
            const bsp59_lm_soup_t *s = &sorted[end];
            if (s->page < 0) continue;
            if (counts[s->page] > INT32_MAX - s->vertices)
                REJECT("per-material page vertex count overflows signed int");
            counts[s->page] += s->vertices;
            /* The original iterates pages with nonzero counters. A page
             * with only zero-vertex soups therefore contributes no edge. */
        }
        for (i = 0; i < pages; ++i) if (counts[i]) {
            ++active;
            for (j = i + 1; j < pages; ++j) if (counts[j]) {
                uint64_t next = (uint64_t)weights[i][j] +
                                (uint32_t)counts[i] + (uint32_t)counts[j];
                /* 0x4CFCA2-0x4CFCAE clamps a negative signed result
                 * to INT32_MAX. Reject modulo-2^32 wraparound that
                 * could turn the result positive again. */
                if (next > UINT32_MAX)
                    REJECT("page pair weight exceeds 32-bit accumulation");
                weights[i][j] = next > INT32_MAX ? INT32_MAX : (int32_t)next;
                weights[j][i] = weights[i][j];
            }
        }
        if (active > 1) ++result.shared_materials;
    }
    for (i = 0; i < pages; ++i)
        for (j = i + 1; j < pages; ++j)
            if (weights[i][j]) ++result.nonzero_page_pairs;

    placed = 0;
    while (placed < pages) {
        bsp59_lm_group_t *group;
        uint32_t cap, first, a = 0, b = 0;
        int32_t best;
        while ((uint64_t)width * height > pages - placed) {
            if (width >= height) width /= 2;
            else height /= 2;
        }
        if (!width || !height) REJECT("tile halving reached zero");
        cap = width * height;
        if (result.group_count == BSP59_LM_MAX_PAGES)
            REJECT("group count exceeds 128");
        group = &result.groups[result.group_count];
        group->width_tiles = width;
        group->height_tiles = height;
        group->pixel_width = width * BSP59_LM_TILE_SIZE;
        group->pixel_height = height * BSP59_LM_TILE_SIZE;
        group->first_slot = placed;
        group->slot_count = cap;
        first = placed;
        if (cap == 1) {
            for (i = 0; i < pages && used[i]; ++i) {}
            if (i == pages) REJECT("no remaining page for single tile");
            used[i] = 1;
            result.page_order[placed++] = i;
        } else {
            best = -1;
            for (i = 0; i < pages; ++i) if (!used[i])
                for (j = i + 1; j < pages; ++j) if (!used[j])
                    if (weights[i][j] > best) {
                        best = weights[i][j]; a = i; b = j;
                    }
            if (best < 0) REJECT("no available pair");
            used[a] = used[b] = 1;
            result.page_order[placed++] = a;
            result.page_order[placed++] = b;
            while (placed - first < cap) {
                uint32_t choice = pages;
                int64_t best_score = -1;
                for (i = 0; i < pages; ++i) if (!used[i]) {
                    int64_t score = 0;
                    for (k = first; k < placed; ++k) {
                        score += weights[result.page_order[k]][i];
                        if (score > INT32_MAX)
                            REJECT("cluster score overflows signed int");
                    }
                    if (score > best_score) {
                        best_score = score; choice = i;
                    }
                }
                if (choice == pages) REJECT("no available page for cluster");
                used[choice] = 1;
                result.page_order[placed++] = choice;
            }
        }
        for (p = 0; p < cap; ++p) {
            uint32_t source = result.page_order[first + p];
            bsp59_lm_page_t *placement = &result.page[source];
            placement->group = result.group_count;
            placement->tile_x = p % width;
            placement->tile_y = p / width;
            /* Candidate-image constant 0x55B580 is assumed to be 1.0f.
             * This numerical value has not been captured live. */
            placement->offset_u = (float)placement->tile_x / (float)width;
            placement->offset_v = (float)placement->tile_y / (float)height;
            placement->scale_u = 1.0f / (float)width;
            placement->scale_v = 1.0f / (float)height;
        }
        ++result.group_count;
    }
    free(sorted);
    *layout = result;
    return 1;

rejected:
    free(sorted);
    return 0;
#undef REJECT
}
