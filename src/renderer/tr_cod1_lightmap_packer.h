#ifndef BSP59_LM_PACKER_H
#define BSP59_LM_PACKER_H

/* Host-side reconstruction of the CoDSP BSP v59 lightmap page placement.
 * No renderer or OpenGL dependency. The 128-page limit comes from the
 * observed stack arrays in the original code; errors leave *layout zeroed.
 */
#include <stddef.h>
#include <stdint.h>

#define BSP59_LM_TILE_SIZE 512u
#define BSP59_LM_MAX_PAGES 128u

typedef struct bsp59_lm_soup_s {
    int32_t material;
    int32_t page;
    int32_t vertices;
} bsp59_lm_soup_t;

typedef struct bsp59_lm_group_s {
    uint32_t width_tiles;
    uint32_t height_tiles;
    uint32_t pixel_width;
    uint32_t pixel_height;
    uint32_t first_slot;
    uint32_t slot_count;
} bsp59_lm_group_t;

typedef struct bsp59_lm_page_s {
    uint32_t group;
    uint32_t tile_x;
    uint32_t tile_y;
    float offset_u;
    float offset_v;
    float scale_u;
    float scale_v;
} bsp59_lm_page_t;

typedef struct bsp59_lm_layout_s {
    uint32_t source_rgb_pages;
    uint32_t derived_pages;
    uint32_t group_count;
    uint32_t material_count;
    uint32_t shared_materials;
    uint32_t nonzero_page_pairs;
    size_t unlit_soups;
    size_t lightmapped_soups;
    uint32_t page_order[BSP59_LM_MAX_PAGES];
    bsp59_lm_group_t groups[BSP59_LM_MAX_PAGES];
    bsp59_lm_page_t page[BSP59_LM_MAX_PAGES];
} bsp59_lm_layout_t;

/* max_texture_size is an input to a scenario, not a value measured from the
 * user's CoDSP process. source_rgb_pages may exceed derived_pages when the
 * final RGB pages have no soup references; those pages are not placed.
 * Only page == -1 is accepted as an unlit soup. Material/page/vertices
 * must be values representable by signed 16-bit BSP fields.
 * Returns 1 on success and 0 on invalid input or arithmetic overflow.
 */
int bsp59_lm_plan(const bsp59_lm_soup_t *soups, size_t soup_count,
                 uint32_t material_count, uint32_t source_rgb_pages,
                 int32_t max_texture_size, bsp59_lm_layout_t *layout,
                 char *error, size_t error_size);

#endif
