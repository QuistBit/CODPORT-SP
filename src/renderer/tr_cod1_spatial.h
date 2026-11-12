/* Native IBSP59 spatial tree and PVS bridge. The leaf-surface lump describes
 * collision facets, so visual soups are assigned conservatively by their
 * bounds and the validated BSP split planes. */
typedef struct {
    const cod1BspSpatial_t *spatial;
    const cod1BspRenderCells_t *render;
    const byte *cellSurface;
    int worldFirst, worldCount;
    int currentCell;
    int *counts;
    int *writeAt;
    msurface_t **marks;
    msurface_t *surface;
    int total;
    int matched;
    int fallbackSurfaces;
} cod1RenderMarks_t;

/* Diagnostic-only mapping from rendered BSP leaves to authored v59 cells.
 * Keep the cell ID separate from RTCW's area field, which still drives
 * area-mask visibility. Refreshed whenever a CoD1 world is loaded. */
static int *s_cod1LeafCells;

int R_Cod1ViewCell( const mnode_t *leaf ) {
    int index;
    if ( !tr.world || !tr.world->isCod1 || !s_cod1LeafCells || !leaf ||
         leaf < tr.world->nodes + tr.world->numDecisionNodes ||
         leaf >= tr.world->nodes + tr.world->numnodes ) return -1;
    index = (int)( leaf - tr.world->nodes ) - tr.world->numDecisionNodes;
    return s_cod1LeafCells[index];
}

static void R_Cod1MarkLeaf( void *opaque, int leafIndex ) {
    cod1RenderMarks_t *ctx = (cod1RenderMarks_t *)opaque;
    int fields[9];
    COD1_BSP_SpatialRecord( ctx->spatial->leaves, leafIndex, fields );
    if ( fields[0] < 0 ) return; /* solid leaf has no camera cluster */
    if ( ctx->currentCell >= 0 && fields[6] != ctx->currentCell ) return;
    ctx->matched++;
    if ( ctx->marks ) {
        ctx->marks[ctx->writeAt[leafIndex]++] = ctx->surface;
    } else {
        if ( ctx->total >= INT_MAX / (int)sizeof( msurface_t * ) ) {
            ri.Error( ERR_DROP, "CoD1 BSP has too many visual leaf references" );
        }
        ctx->counts[leafIndex]++;
        ctx->total++;
    }
}

static void R_Cod1ForEachWorldSurface( const cod1BspSpatial_t *spatial,
                                       const cod1BspWorldInfo_t *info,
                                       int staticFirst, int staticCount,
                                       cod1RenderMarks_t *ctx ) {
    int pass, i, first, last, cell, cellCount;
    vec3_t worldMins, worldMaxs;
    VectorCopy( s_worldData.bmodels[0].bounds[0], worldMins );
    VectorCopy( s_worldData.bmodels[0].bounds[1], worldMaxs );
    for ( pass = 0; pass < 2; pass++ ) {
        first = pass ? staticFirst : info->worldFirstSoup;
        last = first + ( pass ? staticCount : info->worldSoupCount );
        for ( i = first; i < last; i++ ) {
            msurface_t *surface = s_worldData.surfaces + i;
            const vec3_t *mins = &worldMins, *maxs = &worldMaxs;
            if ( !surface->data || *surface->data == SF_SKIP ) continue;
            if ( *surface->data == SF_TRIANGLES ) {
                srfTriangles_t *tri = (srfTriangles_t *)surface->data;
                mins = &tri->bounds[0];
                maxs = &tri->bounds[1];
            }
            ctx->surface = surface;
            cellCount = pass ? 1 : ctx->render->cellCount;
            for ( cell = 0; cell < cellCount; cell++ ) {
                int leaf, assigned = pass;
                if ( !pass ) {
                    assigned = ctx->cellSurface[( i - ctx->worldFirst ) * cellCount + cell];
                }
                if ( !assigned ) continue;
                ctx->currentCell = pass ? -1 : cell;
                ctx->matched = 0;
                COD1_BSP_VisitBoxLeaves( spatial, *mins, *maxs, R_Cod1MarkLeaf, ctx );
                if ( !ctx->matched ) {
                    /* Geometry above the collision ceiling still belongs
                     * to its authored cell. Static models have no BSP cell. */
                    if ( !ctx->marks ) ctx->fallbackSurfaces++;
                    for ( leaf = 0; leaf < spatial->leafCount; leaf++ ) {
                        if ( s_worldData.nodes[spatial->nodeCount + leaf].parent ) {
                            R_Cod1MarkLeaf( ctx, leaf );
                        }
                    }
                }
            }
        }
    }
}

static void R_Cod1ExpandTreeBounds( mnode_t *node ) {
    int j;
    if ( node->contents != CONTENTS_NODE ) return;
    R_Cod1ExpandTreeBounds( node->children[0] );
    R_Cod1ExpandTreeBounds( node->children[1] );
    for ( j = 0; j < 2; j++ ) {
        AddPointToBounds( node->children[j]->mins, node->mins, node->maxs );
        AddPointToBounds( node->children[j]->maxs, node->mins, node->maxs );
    }
}

static byte *R_Cod1CellSurfaces( const cod1BspRenderCells_t *render,
                                const cod1BspWorldInfo_t *info ) {
    byte *membership;
    int cell, i, first, count, root, groupFirst, groupCount, j;
    if ( info->worldSoupCount && render->cellCount > INT_MAX / info->worldSoupCount ) {
        ri.Error( ERR_DROP, "CoD1 cell membership size overflow" );
    }
    membership = ri.Hunk_Alloc( info->worldSoupCount ?
        info->worldSoupCount * render->cellCount : 1, h_low );
    memset( membership, 0, info->worldSoupCount ?
        info->worldSoupCount * render->cellCount : 1 );
    if ( !info->worldSoupCount ) return membership;
    for ( cell = 0; cell < render->cellCount; cell++ ) {
        COD1_BSP_RenderCell( render, cell, &root, &groupFirst, &groupCount );
        if ( render->treeCount && ( cell == 0 || root ) ) {
            COD1_BSP_RenderTree( render, root, &first, &count );
            for ( j = first; j < first + count; j++ )
                membership[( j - info->worldFirstSoup ) * render->cellCount + cell] = 1;
        }
        for ( i = groupFirst; i < groupFirst + groupCount; i++ ) {
            int group = COD1_BSP_RenderGroupIndex( render, i );
            COD1_BSP_RenderGroup( render, group, &first, &count );
            for ( j = first; j < first + count; j++ )
                membership[( j - info->worldFirstSoup ) * render->cellCount + cell] = 1;
        }
    }
    /* Keep maps with additional, as yet unidentified world soup owners
     * visible rather than silently dropping their surfaces. */
    for ( i = 0; i < info->worldSoupCount; i++ ) {
        for ( cell = 0; cell < render->cellCount; cell++ )
            if ( membership[i * render->cellCount + cell] ) break;
        if ( cell == render->cellCount ) {
            ri.Printf( PRINT_WARNING, "CoD1 world soup %d has no authored cell; using all cells\n",
                info->worldFirstSoup + i );
            memset( membership + i * render->cellCount, 1, render->cellCount );
        }
    }
    return membership;
}

static void R_LoadCod1SpatialWorld( const cod1BspSpatial_t *spatial,
                                    const cod1BspRenderCells_t *render,
                                    const cod1BspWorldInfo_t *info,
                                    int staticFirst, int staticCount ) {
    mnode_t *nodes, *leaf;
    cplane_t *plane;
    cod1RenderMarks_t marks;
    int *counts, *writeAt;
    int i, j, fields[9], first = 0;
    const int totalNodes = spatial->nodeCount + spatial->leafCount;

    if ( spatial->planeCount > INT_MAX / (int)sizeof( cplane_t ) ||
         totalNodes > INT_MAX / (int)sizeof( mnode_t ) ||
         spatial->leafCount > INT_MAX / (int)sizeof( int ) ) {
        ri.Error( ERR_DROP, "CoD1 BSP spatial allocation exceeds hunk size" );
    }

    s_worldData.numplanes = spatial->planeCount;
    s_worldData.planes = ri.Hunk_Alloc( spatial->planeCount * sizeof( *s_worldData.planes ), h_low );
    for ( i = 0; i < spatial->planeCount; i++ ) {
        plane = s_worldData.planes + i;
        COD1_BSP_SpatialPlane( spatial, i, plane->normal, &plane->dist );
        plane->type = PlaneTypeForNormal( plane->normal );
        plane->signbits = 0;
        for ( j = 0; j < 3; j++ ) if ( plane->normal[j] < 0 ) plane->signbits |= 1 << j;
    }
    s_worldData.nodes = nodes = ri.Hunk_Alloc( totalNodes * sizeof( *nodes ), h_low );
    s_worldData.numnodes = totalNodes;
    s_worldData.numDecisionNodes = spatial->nodeCount;
    s_cod1LeafCells = ri.Hunk_Alloc( spatial->leafCount * sizeof( *s_cod1LeafCells ), h_low );
    for ( i = 0; i < spatial->nodeCount; i++ ) {
        mnode_t *out = nodes + i;
        COD1_BSP_SpatialRecord( spatial->nodes, i, fields );
        out->contents = CONTENTS_NODE;
        out->cluster = -1;
        out->plane = s_worldData.planes + fields[0];
        for ( j = 0; j < 3; j++ ) {
            out->mins[j] = (float)fields[3 + j];
            out->maxs[j] = (float)fields[6 + j];
        }
        for ( j = 0; j < 2; j++ ) {
            int child = fields[j + 1];
            out->children[j] = nodes + ( child >= 0 ? child : spatial->nodeCount - 1 - child );
        }
    }
    for ( i = 0; i < spatial->leafCount; i++ ) {
        leaf = nodes + spatial->nodeCount + i;
        COD1_BSP_SpatialRecord( spatial->leaves, i, fields );
        leaf->cluster = fields[0];
        leaf->area = fields[1];
        s_cod1LeafCells[i] = fields[0] < 0 ? -1 : fields[6];
        leaf->contents = fields[0] < 0 ? CONTENTS_SOLID : 0;
        /* Leafs have no bounding box in v59. Inherit the nearest node's
         * bounds, then enlarge them for visual geometry outside clip bounds. */
        VectorCopy( s_worldData.bmodels[0].bounds[0], leaf->mins );
        VectorCopy( s_worldData.bmodels[0].bounds[1], leaf->maxs );
    }
    R_SetParent( nodes, NULL );
    for ( i = 0; i < spatial->leafCount; i++ ) {
        leaf = nodes + spatial->nodeCount + i;
        if ( leaf->parent ) {
            VectorCopy( leaf->parent->mins, leaf->mins );
            VectorCopy( leaf->parent->maxs, leaf->maxs );
        }
    }

    counts = ri.Hunk_Alloc( spatial->leafCount * sizeof( *counts ), h_low );
    writeAt = ri.Hunk_Alloc( spatial->leafCount * sizeof( *writeAt ), h_low );
    memset( counts, 0, spatial->leafCount * sizeof( *counts ) );
    memset( writeAt, 0, spatial->leafCount * sizeof( *writeAt ) );
    memset( &marks, 0, sizeof( marks ) );
    marks.spatial = spatial;
    marks.render = render;
    marks.worldFirst = info->worldFirstSoup;
    marks.worldCount = info->worldSoupCount;
    marks.cellSurface = R_Cod1CellSurfaces( render, info );
    marks.counts = counts;
    marks.writeAt = writeAt;
    R_Cod1ForEachWorldSurface( spatial, info, staticFirst, staticCount, &marks );
    s_worldData.nummarksurfaces = marks.total;
    s_worldData.marksurfaces = ri.Hunk_Alloc( ( marks.total ? marks.total : 1 ) *
                                         sizeof( *s_worldData.marksurfaces ), h_low );
    marks.marks = s_worldData.marksurfaces;
    for ( i = 0; i < spatial->leafCount; i++ ) {
        leaf = nodes + spatial->nodeCount + i;
        leaf->firstmarksurface = marks.marks + first;
        leaf->nummarksurfaces = counts[i];
        writeAt[i] = first;
        first += counts[i];
    }
    R_Cod1ForEachWorldSurface( spatial, info, staticFirst, staticCount, &marks );
    for ( i = 0; i < spatial->leafCount; i++ ) {
        int k;
        leaf = nodes + spatial->nodeCount + i;
        for ( k = 0; k < leaf->nummarksurfaces; k++ ) {
            msurface_t *surface = leaf->firstmarksurface[k];
            if ( *surface->data == SF_TRIANGLES ) {
                srfTriangles_t *tri = (srfTriangles_t *)surface->data;
                AddPointToBounds( tri->bounds[0], leaf->mins, leaf->maxs );
                AddPointToBounds( tri->bounds[1], leaf->mins, leaf->maxs );
            }
        }
    }
    R_Cod1ExpandTreeBounds( nodes );

    s_worldData.numClusters = spatial->clusterCount;
    s_worldData.clusterBytes = spatial->clusterBytes;
    s_worldData.novis = ri.Hunk_Alloc( spatial->clusterBytes, h_low );
    memset( s_worldData.novis, 0xff, spatial->clusterBytes );
    if ( spatial->visibility ) {
        int size = spatial->clusterCount * spatial->clusterBytes;
        byte *copy = ri.Hunk_Alloc( size, h_low );
        memcpy( copy, spatial->visibility, size );
        s_worldData.vis = copy;
    }
    ri.Printf( PRINT_ALL, "CoD1 spatial renderer: %d nodes, %d leafs, %d clusters, %d cells, %d cull groups, %d AABB nodes, %d visual references (%d bounds fallback), PVS %s\n",
        spatial->nodeCount, spatial->leafCount, spatial->clusterCount,
        render->cellCount, render->groupCount, render->treeCount,
        marks.total, marks.fallbackSurfaces,
        spatial->visibility ? "loaded" : "unavailable" );
}
