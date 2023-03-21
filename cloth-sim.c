#include <yaul.h>
#include <stdio.h>
#include <stdlib.h>
#include <mic3d.h>
#include "cloth-sim.h"

/* Simulation stuff */
#define CLOTH_SIZE_W (25)
#define CLOTH_SIZE_H (16)
#define CLOTH_SPACING (131072)

#define INDICES3D(a, b, c, d) .indices.p0 = a, .indices.p1 = c, .indices.p2 = d, .indices.p3 = b
#define FLAGS3D(_sort_type, _plane_type, _use_texture) .flags.sort_type = _sort_type, .flags.plane_type = _plane_type, .flags.use_texture = _use_texture

static fix16_t _fast_length(const fix16_vec3_t * vector);
static void _drop_pickup_cloth();
static void _reset_cloth();
static void _set_attributes(command_type_t drawType);
static void _do_sim();

/** @brief Phys point
 */
typedef struct {
    int PosIndex;
    fix16_vec3_t PrevPos;
    bool Locked;
} PhysPoint;

/** @brief Spring segment
 */
typedef struct
{
    PhysPoint * First;
    PhysPoint * Second;
    fix16_t Length;
} PhysSpring;

/* All points of cloth */
static PhysPoint Points[CLOTH_SIZE_W * CLOTH_SIZE_H];

/* All connecting segments */
static PhysSpring Segments[(CLOTH_SIZE_W * (CLOTH_SIZE_H - 1)) + ((CLOTH_SIZE_W - 1) * CLOTH_SIZE_H)];

/* Gravity direction and strength */
static fix16_vec3_t Gravity;

/* Draw clath as wires */
static bool Wireframe = false;

/* Cloth mesh stuff */
static attribute_t ClothMeshAttributes[(CLOTH_SIZE_H - 1) * (CLOTH_SIZE_W - 1)];
static polygon_t ClothMeshPolygons[(CLOTH_SIZE_H - 1) * (CLOTH_SIZE_W - 1)];
static fix16_vec3_t ClothMeshNormals[(CLOTH_SIZE_H - 1) * (CLOTH_SIZE_W - 1)];
static fix16_vec3_t ClothMeshPoints[CLOTH_SIZE_H * CLOTH_SIZE_W];

/* Cloth mesh data */
static mesh_t ClothMesh =
{
    .points = ClothMeshPoints,
    .points_count = CLOTH_SIZE_H * CLOTH_SIZE_W,
    .normals = ClothMeshNormals,
    .polygons = ClothMeshPolygons,
    .attributes = ClothMeshAttributes,
    .polygons_count = (CLOTH_SIZE_H - 1) * (CLOTH_SIZE_W - 1)
};

/** @brief Initialize cloth simulation
 */
uint32_t
ClothSimInitialize()
{
    Gravity.x = 0;
    Gravity.y = 1700;
    Gravity.z = 0;

    _reset_cloth();
    return (CLOTH_SIZE_H - 1) * (CLOTH_SIZE_W - 1);
}

/** @brief Update simulation
 */
void
ClothSimTick()
{
    _do_sim();
    
    render_enable(RENDER_FLAGS_LIGHTING);
    matrix_push();
    render_mesh_transform(&ClothMesh);
    matrix_pop();

}

/** @brief Use controller to move cloth 
 */
void ClothMove(smpc_peripheral_digital_t *digital)
{
    int movex = 0;
    int movey = 0;
    int movez = 0;

    const uint16_t pressed_state = digital->pressed.raw;
    const uint16_t hold_state = digital->held.raw;

    if ((pressed_state & PERIPHERAL_DIGITAL_START) != 0)
    {
        _reset_cloth();
        return;
    }

    if ((hold_state & PERIPHERAL_DIGITAL_A) != 0)
    {
        _drop_pickup_cloth();
    }

    if ((hold_state & PERIPHERAL_DIGITAL_B) != 0)
    {
        _set_attributes(Wireframe ? COMMAND_TYPE_POLYLINE : COMMAND_TYPE_POLYGON);
        Wireframe = !Wireframe;
    }

    if ((pressed_state & PERIPHERAL_DIGITAL_LEFT) != 0)
    {
        movex--;
    }
    else if ((pressed_state & PERIPHERAL_DIGITAL_RIGHT) != 0)
    {
        movex++;
    }

    if ((pressed_state & PERIPHERAL_DIGITAL_UP) != 0)
    {
        movey--;
    }
    else if ((pressed_state & PERIPHERAL_DIGITAL_DOWN) != 0)
    {
        movey++;
    }

    if ((pressed_state & PERIPHERAL_DIGITAL_L) != 0)
    {
        movez--;
    }
    else if ((pressed_state & PERIPHERAL_DIGITAL_R) != 0)
    {
        movez++;
    }

    if (movex != 0 || movey != 0 || movez != 0)
    {
        for(int pointx = 0; pointx < CLOTH_SIZE_W; pointx++)
        {
            for(int pointy = 0; pointy < CLOTH_SIZE_H; pointy++)
            {
                // Generate point
                int coord = pointx + (CLOTH_SIZE_W * pointy);

                if (Points[coord].Locked)
                {
                    fix16_vec3_t current;
                    current.x = ClothMeshPoints[Points[coord].PosIndex].x + (movex << 16);
                    current.y = ClothMeshPoints[Points[coord].PosIndex].y + (movey << 16);
                    current.z = ClothMeshPoints[Points[coord].PosIndex].z + (movez << 16);

                    ClothMeshPoints[Points[coord].PosIndex].x = current.x;
                    ClothMeshPoints[Points[coord].PosIndex].y = current.y;
                    ClothMeshPoints[Points[coord].PosIndex].z = current.z;
                    Points[coord].PrevPos.x = ClothMeshPoints[Points[coord].PosIndex].x;
                    Points[coord].PrevPos.y = ClothMeshPoints[Points[coord].PosIndex].y;
                    Points[coord].PrevPos.z = ClothMeshPoints[Points[coord].PosIndex].z;
                }
            }
        }
    }
}

/** @brief Run simulation
 */
static void
_do_sim()
{
    // Update points
    int count = CLOTH_SIZE_W * CLOTH_SIZE_H;

    for(int point = 0; point < count; point++)
    {
        if (!Points[point].Locked)
        {
            int posIndex = Points[point].PosIndex;

            // Do gravity
            fix16_vec3_t prev =
            {
                .x = ClothMeshPoints[posIndex].x,
                .y = ClothMeshPoints[posIndex].y,
                .z = ClothMeshPoints[posIndex].z
            };
            ClothMeshPoints[posIndex].x += (ClothMeshPoints[posIndex].x - Points[point].PrevPos.x) + Gravity.x;
            ClothMeshPoints[posIndex].y += (ClothMeshPoints[posIndex].y - Points[point].PrevPos.y) + Gravity.y;
            ClothMeshPoints[posIndex].z += (ClothMeshPoints[posIndex].z - Points[point].PrevPos.z) + Gravity.z;

            Points[point].PrevPos.x = prev.x;
            Points[point].PrevPos.y = prev.y;
            Points[point].PrevPos.z = prev.z;
        } 
    }

    count = (CLOTH_SIZE_W * (CLOTH_SIZE_H - 1)) + ((CLOTH_SIZE_W - 1) * CLOTH_SIZE_H);

    for (int segment = 0; segment < count; segment++)
    {
        // Update springs
        // Skip segments where both points are locked
        if ((!Segments[segment].First->Locked || !Segments[segment].Second->Locked))
        {
            int firstIndex = Segments[segment].First->PosIndex;
            int secondIndex = Segments[segment].Second->PosIndex;

            fix16_vec3_t dir;
            dir.x = ClothMeshPoints[firstIndex].x - ClothMeshPoints[secondIndex].x;
            dir.y = ClothMeshPoints[firstIndex].y - ClothMeshPoints[secondIndex].y;
            dir.z = ClothMeshPoints[firstIndex].z - ClothMeshPoints[secondIndex].z;
            fix16_t length = _fast_length(&dir);

            // Segment is overstretched
            if (length > Segments[segment].Length)
            {
                // Normalize vector
                dir.x = fix16_div(dir.x, length);
                dir.y = fix16_div(dir.y, length);
                dir.z = fix16_div(dir.z, length);

                // Segment center point
                fix16_vec3_t center;
                center.x = (ClothMeshPoints[firstIndex].x + ClothMeshPoints[secondIndex].x) >> 1;
                center.y = (ClothMeshPoints[firstIndex].y + ClothMeshPoints[secondIndex].y) >> 1;
                center.z = (ClothMeshPoints[firstIndex].z + ClothMeshPoints[secondIndex].z) >> 1;

                fix16_vec3_t segmentDir;
                segmentDir.x = fix16_mul(dir.x, Segments[segment].Length) >> 1;
                segmentDir.y = fix16_mul(dir.y, Segments[segment].Length) >> 1;
                segmentDir.z = fix16_mul(dir.z, Segments[segment].Length) >> 1;

                // Don't move point if locked
                if (!Segments[segment].First->Locked)
                {
                    ClothMeshPoints[firstIndex].x = center.x + segmentDir.x;
                    ClothMeshPoints[firstIndex].y = center.y + segmentDir.y;
                    ClothMeshPoints[firstIndex].z = center.z + segmentDir.z;
                }

                // Don't move point if locked
                if (!Segments[segment].Second->Locked)
                {
                    ClothMeshPoints[secondIndex].x = center.x - segmentDir.x;
                    ClothMeshPoints[secondIndex].y = center.y - segmentDir.y;
                    ClothMeshPoints[secondIndex].z = center.z - segmentDir.z;
                }
            }
        }
    }

    // Update normals
    for (int quad = 0; quad < (CLOTH_SIZE_H - 1) * (CLOTH_SIZE_W - 1); quad++)
    {
        polygon_t * polygon = &ClothMeshPolygons[quad];
        fix16_vec3_t * normal = &ClothMeshNormals[quad];

        fix16_vec3_t a;
        fix16_vec3_t b;

        fix16_vec3_sub(&ClothMeshPoints[polygon->indices.p3], &ClothMeshPoints[polygon->indices.p0], &a);
        fix16_vec3_sub(&ClothMeshPoints[polygon->indices.p2], &ClothMeshPoints[polygon->indices.p1], &b);

        fix16_vec3_cross(&a, &b, normal);
        fix16_vec3_normalize(normal);
    }
}

/** @brief reset cloth
 */
static void
_reset_cloth()
{
    // Set segments and points
    int segment = 0;

    for(int pointx = 0; pointx < CLOTH_SIZE_W; pointx++)
    {
        bool canLock = pointx % 4 == 0;

        for(int pointy = 0; pointy < CLOTH_SIZE_H; pointy++)
        {
            // Generate point
            int x = (pointx * CLOTH_SPACING) - (CLOTH_SPACING * (CLOTH_SIZE_W / 2));
            int y = (pointy * CLOTH_SPACING) - (CLOTH_SPACING * (CLOTH_SIZE_H / 2));
            int coord = pointx + (CLOTH_SIZE_W * pointy);

            ClothMeshPoints[coord].x = x;
            ClothMeshPoints[coord].y = y;
            ClothMeshPoints[coord].z = 0;
            Points[coord].PosIndex = coord;
            Points[coord].PrevPos.x = x;
            Points[coord].PrevPos.y = y;
            Points[coord].PrevPos.z = 0;

            Points[coord].Locked = pointy == 0 && canLock;

            // Create segment
            if (pointx < CLOTH_SIZE_W - 1)
            {
                Segments[segment].First = &(Points[coord]);
                Segments[segment].Second = &(Points[coord + 1]);
                Segments[segment].Length = CLOTH_SPACING;
                segment++;
            }

            if (pointy < CLOTH_SIZE_H - 1)
            {
                Segments[segment].First = &(Points[coord]);
                Segments[segment].Second = &(Points[pointx + (CLOTH_SIZE_W * (pointy + 1))]);
                Segments[segment].Length = CLOTH_SPACING;
                segment++;
            }
        }
    }

    // Set mesh attributes
    _set_attributes(Wireframe ? COMMAND_TYPE_POLYLINE : COMMAND_TYPE_POLYGON);
    
    // Set mesh vertices
    for (int polyx = 0; polyx < CLOTH_SIZE_W - 1; polyx++)
    {
        for (int polyy = 0; polyy < CLOTH_SIZE_H - 1; polyy++)
        {
            int coord = polyx + ((CLOTH_SIZE_W - 1) * polyy);
            int coord1 = polyx + (CLOTH_SIZE_W * polyy);
            int coord2 = polyx + (CLOTH_SIZE_W * polyy) + 1;
            int coord3 = polyx + (CLOTH_SIZE_W * (polyy + 1)) + 1;
            int coord4 = polyx + (CLOTH_SIZE_W * (polyy + 1));

            polygon_t poly =
            {
                FLAGS3D(SORT_TYPE_CENTER, PLANE_TYPE_DOUBLE, false), INDICES3D(coord1, coord4, coord2, coord3)
            };

            ClothMeshPolygons[coord] = poly;

            ClothMeshNormals[coord].x = 0;
            ClothMeshNormals[coord].y = 0;
            ClothMeshNormals[coord].z = 65536;
        }
    }
}

/** @brief Set quad draw type
 *  @param drawType Quad draw type
 */
static void
_set_attributes(command_type_t drawType)
{
    for (uint32_t attribute = 0; attribute < ClothMesh.polygons_count; attribute++)
    {
        attribute_t attr = 
        {
            .draw_mode.raw = 0x00C4,
            .control.command = drawType,
            .control.link_type = LINK_TYPE_JUMP_ASSIGN,
            .palette_data.base_color = RGB1555(1, 4,  20,  1),
            .shading_slot = attribute
        };

        ClothMeshAttributes[attribute] = attr;
    }
}

/** @brief pickup or drop cloth
 */
static void
_drop_pickup_cloth()
{
    for(int pointx = 0; pointx < CLOTH_SIZE_W; pointx++)
    {
        bool canLock = pointx % 4 == 0;

        for(int pointy = 0; pointy < CLOTH_SIZE_H; pointy++)
        {
            int coord = pointx + (CLOTH_SIZE_W * pointy);
            Points[coord].Locked = !Points[coord].Locked && pointy == CLOTH_SIZE_H - 1 && canLock;
        }
    }
}

/** @brief Fast length of 3D vector (Thx GValiente for solution).
 *  For more info about how it works see: https://math.stackexchange.com/questions/1282435/alpha-max-plus-beta-min-algorithm-for-three-numbers
 *  And also: https://en.wikipedia.org/wiki/Alpha_max_plus_beta_min_algorithm
 *  @param vector Vector to measure
 *  @return Approximation of the vector length
 */
static fix16_t
_fast_length(const fix16_vec3_t * vector)
{
        // Alpha is 0.9398086351723256
        // Beta is 0.38928148272372454
        // Gama is 0.2987061876143797

        // Get absolute values of the vector components
        fix16_t x = fix16_abs(vector->x);
        fix16_t y = fix16_abs(vector->y);
        fix16_t z = fix16_abs(vector->z);

        // Get min, mid, max
        fix16_t minYZ = fix16_min(y, z);
        fix16_t maxYZ = fix16_max(y, z);
        fix16_t min = fix16_min(x, minYZ);
        fix16_t max = fix16_max(x, maxYZ);
        fix16_t mid = (y < x) ? ((y < z) ? ((z < x) ? z : x) : y) : ((x < z) ? ((z < y) ? z : y) : x);

        // Aproximate vector length (alpha * max + beta * mid + gama * min)
        fix16_t approximation = fix16_mul(61591, max) + fix16_mul(25512, mid) + fix16_mul(19576, min);
        return fix16_max(max, approximation);
}