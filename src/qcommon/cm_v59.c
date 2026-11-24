/* SPDX-License-Identifier: GPL-3.0-or-later
 * PATCH39: engine boundary for the verified IBSP59 vertical capsule solver.
 * Engine trace_t and core trace records deliberately have separate layouts.
 */
#include "cm_local.h"
#include "cm_patch.h"
#include "../game/bg_cod1_impact.h"

static void CM_LoadV59InlineCurves(void);
static v59_world *codWorld;
static v59_context *codContext;
#include "cm_cod1_path_sight.inc"
#include "cm_cod1_path_world_sight.inc"
#include "cm_cod1_navigation_resource.inc"
static cvar_t *codPlayerClip;
static cvar_t *cod1Diag;
static unsigned nativeQueries, fallbackQueries;
static unsigned pointQueries, shapeFallbacks, tiltedFallbacks;
static unsigned mergedPointQueries, triangleOnlyQueries, triangleSelectedHits;
static unsigned lastDiagReport;
static qboolean diagWasEnabled;
static int warnedRotation;

static void CM_Cod1DiagReport(void) {
    unsigned now;
    if (!codWorld || !cod1Diag || !cod1Diag->integer) {
        diagWasEnabled = qfalse;
        return;
    }
    now = (unsigned)Sys_Milliseconds();
    if (diagWasEnabled && (unsigned)(now - lastDiagReport) < 1000u) return;
    lastDiagReport = now;
    diagWasEnabled = qtrue;
    /* Cumulative counts since CM_LoadV59. Merged point traces run both
     * solvers, so they are not part of the exclusive fallback count. */
    Com_Printf("[BSP59 R4 collision] map=%s native=%u fallback=%u shape=%u tilted=%u pointContents=%u mergedPoints=%u triangleOnly=%u triangleSelectedHits=%u\n",
        cm.name, nativeQueries, fallbackQueries, shapeFallbacks,
        tiltedFallbacks, pointQueries, mergedPointQueries,
        triangleOnlyQueries, triangleSelectedHits);
}

/* Called by CM_Trace only where the legacy triangle solver actually runs. */
void CM_Cod1DiagTriangleQuery(qboolean merged, qboolean selectedHit) {
    if (merged) ++mergedPointQueries;
    else ++triangleOnlyQueries;
    if (selectedHit) ++triangleSelectedHits;
    CM_Cod1DiagReport();
}

void CM_ClearV59(void) {
    CM_Cod1NavigationClear();
    v59_context_free(codContext);
    v59_free(codWorld);
    codContext = NULL;
    codWorld = NULL;
    nativeQueries = fallbackQueries = 0;
    pointQueries = shapeFallbacks = tiltedFallbacks = 0;
    mergedPointQueries = triangleOnlyQueries = triangleSelectedHits = 0;
    lastDiagReport = 0;
    diagWasEnabled = qfalse;
    warnedRotation = 0;
}

/* Private loader-state gate for the port's targeted inline USE query. */
qboolean CM_HasV59(void) {
    return codWorld != NULL;
}

qboolean CM_LoadV59(const void *bytes, int length, char *error, int capacity) {
    CM_ClearV59();
    codWorld = v59_load(bytes, (size_t)length, error, (size_t)capacity);
    if (!codWorld) return qfalse;
    codContext = v59_context_create(codWorld);
    if (!codContext) {
        Q_strncpyz(error, "cannot allocate IBSP59 trace context", capacity);
        CM_ClearV59();
        return qfalse;
    }
    /* Playerclip also contains authored stair treads, not just map boundaries.
     * Keep it enabled for correct movement. Mission gates are a game-layer task.
     * This switch is a diagnostic override, deliberately not archived. */
    codPlayerClip = Cvar_Get("cm_cod1PlayerClip", "1", CVAR_CHEAT);
    cod1Diag = Cvar_Get("cm_cod1Diag", "0", 0);
    Com_Printf("PATCH39 CoD1 native capsule collision active (%d models); playerclip=%d\n",
        v59_model_count(codWorld), codPlayerClip->integer);
    CM_Cod1NavigationCapture(bytes,length);
    return qtrue;
}

int CM_V59SurfaceFlags(unsigned flags) {
    /* CoD stores a material ordinal in bits 20..24. Those are independent
     * RTCW surface bits; copying them would turn stone into glass/snow/etc.
     * Only the shared behavioral bits cross this boundary for now. */
    return flags & (SURF_NODAMAGE | SURF_SLICK | SURF_SKY | SURF_LADDER |
        SURF_NOIMPACT | SURF_NOMARKS | SURF_NODRAW | SURF_NOLIGHTMAP |
        SURF_POINTLIGHT | SURF_NOSTEPS | SURF_NONSOLID);
}

qboolean CM_PointContentsV59(const vec3_t point,int model,int *out) {
    uint32_t contents;
    if (!codWorld || !out || model < 0 || model >= BOX_MODEL_HANDLE - 1)
        return qfalse;
    if (v59_point_contents(codWorld,point,model,&contents))
        Com_Error(ERR_DROP,"CoD1 point contents rejected (model %d)",model);
    *out=(int)contents;
    ++pointQueries;
    CM_Cod1DiagReport();
    return qtrue;
}

static qboolean CM_TraceV59Internal(trace_t *out, const vec3_t start, const vec3_t end,
    const vec3_t mins, const vec3_t maxs, int model, int mask,
    int capsule, const sphere_t *sphere, qboolean diagnosticClip) {
    v59_hit hit;
    int i,box=0,status;
    if (!codWorld || model >= BOX_MODEL_HANDLE - 1) return qfalse;
    {
        qboolean point=!model && mins[0]==0 && mins[1]==0 && mins[2]==0 &&
            maxs[0]==0 && maxs[1]==0 && maxs[2]==0;
        box=!model&&!capsule&&!point&&!diagnosticClip;
        if (!point && ((!capsule&&!box) || maxs[0] <= mins[0] || maxs[1] <= mins[1] ||
            maxs[2] <= mins[2] || (!box&&maxs[0]-mins[0] != maxs[1]-mins[1]))) {
            ++fallbackQueries;
            ++shapeFallbacks;
            CM_Cod1DiagReport();
            return qfalse;
        }
    }
    /* Yaw-only transforms keep a capsule vertical and are exact. Pitch/roll
     * need the oriented mesh kernel; do not silently reinterpret the shape. */
    if (sphere && (sphere->offset[0] != 0 || sphere->offset[1] != 0 || sphere->offset[2] < 0)) {
        if (!warnedRotation) { warnedRotation=1; Com_Printf("CoD1 collision: tilted inline capsule uses legacy fallback (oriented mesh pending)\n"); }
        ++fallbackQueries;
        ++tiltedFallbacks;
        CM_Cod1DiagReport();
        return qfalse;
    }
    if (diagnosticClip && !codPlayerClip->integer) mask &= ~CONTENTS_PLAYERCLIP;
    status=box?v59_query_box(codContext,start,end,mins,maxs,(unsigned)mask,model,1):v59_query(codContext,start,end,mins,maxs,(unsigned)mask,model,1);
    if(box&&status)return qfalse;
    if (status) {
        Com_Error(ERR_DROP, "CoD1 native capsule query rejected (model %d)", model);
        return qfalse;
    }
    ++nativeQueries;
    CM_Cod1DiagReport();
    v59_read_hit(codContext, &hit);
    memset(out, 0, sizeof(*out));
    out->fraction = hit.fraction;
    VectorCopy(hit.end, out->endpos);
    VectorCopy(hit.normal, out->plane.normal);
    out->surfaceFlags = CM_V59SurfaceFlags(hit.surface_flags);
    if(hit.fraction<1 && !hit.allsolid)CI_TraceSurface(out,hit.surface_flags);
    out->contents = hit.contents;
    out->startsolid = hit.startsolid;
    out->allsolid = hit.allsolid;
    out->entityNum = hit.fraction < 1 || hit.startsolid || hit.allsolid ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
    out->plane.type = 3;
    for (i=0;i<3;i++) {
        if (hit.normal[i] < 0) out->plane.signbits |= 1 << i;
        if (hit.normal[i] == 1) out->plane.type = (byte)i;
    }
    /* The original CoD trace reports a contact normal, not a brush plane
     * distance. Leave dist=0 as its unused value; movement consumes normal. */
    return qtrue;
}

qboolean CM_TraceV59(trace_t *out, const vec3_t start, const vec3_t end,
    const vec3_t mins, const vec3_t maxs, int model, int mask,
    int capsule, const sphere_t *sphere) {
    return CM_TraceV59Internal(out,start,end,mins,maxs,model,mask,capsule,sphere,qtrue);
}

qboolean CM_TraceV59Raw(trace_t *out, const vec3_t start, const vec3_t end,
    const vec3_t mins, const vec3_t maxs, int model, int mask,
    int capsule, const sphere_t *sphere) {
    return CM_TraceV59Internal(out,start,end,mins,maxs,model,mask,capsule,sphere,qfalse);
}

qboolean CM_Cod1NativeWorldTrace(trace_t *out, const vec3_t start, const vec3_t end,
    const vec3_t mins, const vec3_t maxs, int mask, int capsule) {
    trace_t result;
    if (!out || !codWorld || !codContext) return qfalse;
    if (!CM_TraceV59Internal(&result,start,end,mins,maxs,0,mask,capsule,NULL,qfalse))
        return qfalse;
    *out=result;
    return qtrue;
}

/* R56: keep lighting on the already loaded native world. Player movement and
 * the legacy merged point-trace path retain their existing dispatch. */
qboolean CM_Cod1LightTrace(trace_t *out,const vec3_t start,const vec3_t end) {
    const vec3_t zero={0,0,0};
    if(!out || !codWorld || !codContext)return qfalse;
    return CM_TraceV59(out,start,end,zero,zero,0,0x2001,0,NULL);
}

/* STABILIZATION_R9 candidate: missing inline brushes, not a new collision solver.
 * Populate RTCW leaf/brush records from the already validated native tables.
 * CM_TraceThroughBrush / CM_TestInLeaf retain their point/AABB semantics.
 * Only nonworld handles are exported; world/capsule dispatch remains unchanged.
 * Arbitrarily rotated AABBs retain RTCW's documented approximation. Point
 * transforms and translated boxes use the existing CM_TransformedBoxTrace.
 * Curved surfaces reuse their native-subdivided facets. Mesh records remain open.
 */
void CM_LoadV59InlineBrushes(void) {
    int m,first,count,surfaces,b,s,totalBrushes=0,totalSides=0,brushAt=0,sideAt=0;
    v59_brush_info info;v59_brush_side nativeSide;cplane_t *planes;
    if(!codWorld || v59_model_count(codWorld)!=cm.numSubModels)
        Com_Error(ERR_DROP,"CoD1 inline brush export: invalid native world");
    for(m=1;m<cm.numSubModels;m++) {
        if(v59_model_brush_range(codWorld,m,&first,&count,&surfaces))
            Com_Error(ERR_DROP,"CoD1 inline brush export: invalid model %d",m);
        if(count>INT_MAX-totalBrushes)Com_Error(ERR_DROP,"CoD1 inline brush count overflow");
        totalBrushes+=count;
        for(b=0;b<count;b++) {
            if(v59_get_brush(codWorld,first+b,&info) || info.sides<6 || info.sides>INT_MAX-totalSides)
                Com_Error(ERR_DROP,"CoD1 inline brush export: invalid side range");
            totalSides+=info.sides;
        }
        if(surfaces)Com_DPrintf("CoD1 inline model %d: %d collision surfaces to export after brushes\n",m,surfaces);
    }
    if(totalBrushes>INT_MAX/(int)sizeof(cbrush_t)-BOX_BRUSHES ||
       totalBrushes>INT_MAX/(int)sizeof(int)-1 ||
       totalSides>INT_MAX/(int)sizeof(cbrushside_t)-BOX_SIDES ||
       totalSides>INT_MAX/(int)sizeof(cplane_t))
        Com_Error(ERR_DROP,"CoD1 inline brush allocation overflow");
    cm.numBrushes=totalBrushes;cm.numBrushSides=totalSides;cm.numLeafBrushes=totalBrushes;
    cm.brushes=Hunk_Alloc((totalBrushes+BOX_BRUSHES)*sizeof(*cm.brushes),h_high);
    cm.brushsides=Hunk_Alloc((totalSides+BOX_SIDES)*sizeof(*cm.brushsides),h_high);
    cm.leafbrushes=Hunk_Alloc((totalBrushes+1)*sizeof(*cm.leafbrushes),h_high);
    /* Side plane pointers need not belong to cm.planes: that array retains
     * the dummy BSP root and the extra planes reserved by CM_InitBoxHull. */
    planes=Hunk_Alloc((totalSides?totalSides:1)*sizeof(*planes),h_high);
    for(m=1;m<cm.numSubModels;m++) {
        v59_model_brush_range(codWorld,m,&first,&count,&surfaces);
        cm.cmodels[m].leaf.firstLeafBrush=brushAt;cm.cmodels[m].leaf.numLeafBrushes=count;
        for(b=0;b<count;b++,brushAt++) {
            cbrush_t *out=&cm.brushes[brushAt];v59_get_brush(codWorld,first+b,&info);
            memset(out,0,sizeof(*out));out->contents=(int)info.contents;
            VectorCopy(info.mins,out->bounds[0]);VectorCopy(info.maxs,out->bounds[1]);
            out->numsides=info.sides;out->sides=&cm.brushsides[sideAt];
            cm.leafbrushes[brushAt]=brushAt;
            for(s=0;s<info.sides;s++,sideAt++) {
                cbrushside_t *side=&cm.brushsides[sideAt];cplane_t *p=&planes[sideAt];
                v59_get_brush_side(codWorld,first+b,s,&nativeSide);
                memset(side,0,sizeof(*side));memset(p,0,sizeof(*p));
                VectorCopy(nativeSide.normal,p->normal);p->dist=nativeSide.distance;
                p->type=PlaneTypeForNormal(p->normal);SetPlaneSignbits(p);
                side->plane=p;side->surfaceFlags=CM_V59SurfaceFlags(nativeSide.surface_flags);
                side->cod1SurfaceKnown=qtrue;side->cod1SurfaceFlags=nativeSide.surface_flags;
            }
        }
    }
    Com_Printf("CoD1 inline point/box brushes: %d brushes, %d sides\n",totalBrushes,totalSides);
    CM_LoadV59InlineCurves();
}

/* Reuse the CoD-subdivided facets, not RTCW's different control-grid subdivision.
 * Copy checked fields explicitly instead of casting two private ABI layouts. */
static void CM_LoadV59InlineCurves(void) {
    int m,b,c,n,total=0,at=0,curves=0,unsupported=0;
    for(m=1;m<cm.numSubModels;m++) {
        v59_model_brush_range(codWorld,m,&b,&c,&n);
        if(n>INT_MAX-total)Com_Error(ERR_DROP,"CoD1 inline surface count overflow");total+=n;
    }
    if(total>INT_MAX/(int)sizeof(*cm.surfaces)-1||total>INT_MAX/(int)sizeof(int)-1)
        Com_Error(ERR_DROP,"CoD1 inline surface allocation overflow");
    cm.numSurfaces=cm.numLeafSurfaces=total;
    cm.surfaces=Hunk_Alloc((total+1)*sizeof(*cm.surfaces),h_high);
    cm.leafsurfaces=Hunk_Alloc((total+1)*sizeof(*cm.leafsurfaces),h_high);
    for(m=1;m<cm.numSubModels;m++) {
        int i;v59_model_brush_range(codWorld,m,&b,&c,&n);
        cm.cmodels[m].leaf.firstLeafSurface=at;cm.cmodels[m].leaf.numLeafSurfaces=n;
        for(i=0;i<n;i++,at++) {
            int j,k,id=v59_model_surface(codWorld,m,i);v59_surface_info s;cPatch_t *p;patchCollide_t *pc;
            cm.leafsurfaces[at]=at;
            if(v59_get_surface(codWorld,id,&s))Com_Error(ERR_DROP,"CoD1 inline surface export failed");
            if(s.type){unsupported++;continue;}
            if(s.planes<0||s.facets<0||s.planes>INT_MAX/(int)sizeof(patchPlane_t)||s.facets>INT_MAX/(int)sizeof(facet_t))
                Com_Error(ERR_DROP,"CoD1 inline curve allocation overflow");
            cm.surfaces[at]=p=Hunk_Alloc(sizeof(*p),h_high);p->pc=pc=Hunk_Alloc(sizeof(*pc),h_high);
            p->contents=(int)s.contents;p->surfaceFlags=CM_V59SurfaceFlags(s.flags);
            p->cod1SurfaceFlags=s.flags;p->cod1SurfaceKnown=qtrue;
            VectorCopy(s.bounds,pc->bounds[0]);VectorCopy(s.bounds+3,pc->bounds[1]);
            pc->numPlanes=s.planes;pc->numFacets=s.facets;
            pc->planes=Hunk_Alloc((s.planes?s.planes:1)*sizeof(*pc->planes),h_high);
            pc->facets=Hunk_Alloc((s.facets?s.facets:1)*sizeof(*pc->facets),h_high);
            for(j=0;j<s.planes;j++) {
                v59_curve_plane q;v59_get_curve_plane(codWorld,id,j,&q);
                memcpy(pc->planes[j].plane,q.plane,sizeof(q.plane));pc->planes[j].signbits=q.signbits;
            }
            for(j=0;j<s.facets;j++) {
                v59_curve_facet q;facet_t *f=&pc->facets[j];v59_get_curve_facet(codWorld,id,j,&q);
                f->surfacePlane=q.surface;f->numBorders=q.numBorders;
                for(k=0;k<26;k++){f->borderPlanes[k]=q.border[k];f->borderInward[k]=q.inward[k];f->borderNoAdjust[k]=q.noAdjust[k];}
            }
            curves++;
        }
    }
    Com_Printf("CoD1 inline point/box surfaces: %d curves; %d unsupported mesh records\n",curves,unsupported);
}
