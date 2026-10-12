/* FreeBASIC Newton 3.13 binding qualification
 * File: newton-native.c
 * Purpose: Provide independent C layout and real physics results.
 * Responsibilities: SDK checks, bounded simulation, callbacks and cleanup.
 * This file contains no substitute physics or asynchronous application loop.
 */
/* Newton 3.13 uses bool in its C callbacks without including stdbool itself. */
#include <stdbool.h>
#include <Newton.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

/* One solver thread makes the callback counters safe without atomics. The
 * synchronous NewtonUpdate call completes before the caller reads them. */
static int force_calls;
static int invalid_callback;
static int destroyed_worlds;

static void apply_gravity(const NewtonBody *body, dFloat timestep, int thread_index)
{
    /* Force is mass times acceleration: the test body has mass 2 kg. */
    dFloat force[4] = {0, -19.62f, 0, 0};
    if (!body || timestep <= 0 || thread_index < 0) {
        invalid_callback = 1;
        return;
    }
    force_calls++;
    NewtonBodySetForce(body, force);
}

static void world_destroyed(const NewtonWorld *world)
{
    if (!world) invalid_callback = 1;
    destroyed_worlds++;
}

int main(void)
{
    NewtonWorld *world;
    NewtonCollision *collision;
    NewtonBody *body;
    NewtonCollisionInfoRecord info;
    dFloat matrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    int baseline, status = 1, step;
    printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
           sizeof(int), sizeof(dFloat), sizeof(dFloat64), sizeof(dLong), sizeof(void *),
           sizeof(NewtonCollisionInfoRecord), sizeof(NewtonJointRecord),
           sizeof(NewtonWorldConvexCastReturnInfo), sizeof(NewtonUserMeshCollisionCollideDesc),
           offsetof(NewtonCollisionInfoRecord, m_paramArray),
           offsetof(NewtonWorldConvexCastReturnInfo, m_contactID),
           offsetof(NewtonWorldConvexCastReturnInfo, m_hitBody));
    if (NewtonWorldGetVersion() != 313 || NewtonWorldFloatSize() != sizeof(dFloat)) return 2;
    baseline = NewtonGetMemoryUsed();
    world = NewtonCreate();
    if (!world) return 3;
    NewtonSetThreadsCount(world, 1);
    NewtonWorldSetDestructorCallback(world, world_destroyed);
    collision = NewtonCreateBox(world, 1, 2, 3, 17, NULL);
    if (!collision) goto cleanup;
    NewtonCollisionGetInfo(collision, &info);
    if (info.m_collisionType != SERIALIZE_ID_BOX || info.m_collisionUserID != 17 ||
        fabs(info.m_box.m_x - 1) > 0.0001 || fabs(info.m_box.m_y - 2) > 0.0001 ||
        fabs(info.m_box.m_z - 3) > 0.0001) {
        NewtonDestroyCollision(collision);
        goto cleanup;
    }
    body = NewtonCreateDynamicBody(world, collision, matrix);
    /* The body holds its own collision reference. Release the caller's copy. */
    NewtonDestroyCollision(collision);
    if (!body) goto cleanup;
    NewtonBodySetMassProperties(body, 2, NewtonBodyGetCollision(body));
    NewtonBodySetLinearDamping(body, 0);
    NewtonBodySetForceAndTorqueCallback(body, apply_gravity);
    /* Sixty steps of 1/120 second simulate half a second of free fall. */
    for (step = 0; step < 60; step++) NewtonUpdate(world, 1.0f / 120.0f);
    NewtonBodyGetMatrix(body, matrix);
    if (NewtonWorldGetBodyCount(world) != 1 || force_calls < 60 || invalid_callback ||
        !isfinite(matrix[13]) || matrix[13] > -1.1f || matrix[13] < -1.4f ||
        fabs(matrix[12]) > 0.0001 || fabs(matrix[14]) > 0.0001) goto cleanup;
    printf("FALL %.7f CALLBACKS %d\n", matrix[13], force_calls);
    status = 0;
cleanup:
    /* The world owns the body, and synchronous destruction joins its workers. */
    NewtonDestroy(world);
    if (destroyed_worlds != 1 || NewtonGetMemoryUsed() != baseline) status = 4;
    if (!status) puts("NEWTON_NATIVE=PASS version=313 float=4 cleanup=1");
    return status;
}
/* end of newton-native.c */
