// CameraOcclusion.h
//
// Keeps the third-person camera out of buildings, tree crowns and other
// tall things. Plain C++ with NO raylib: the platform layer works out
// where it would LIKE the camera (the orbit), and asks here where it may
// actually go.
//
// The town is treated as columns: each tile blocks between some heights
// (see GameData.h, "Camera occlusion"). A building tile blocks up to its
// roof, the tiles around it only in the eaves band; a tree tile blocks up to
// its crown top, the tiles around it only in the crown band (above head
// height). So a line passing over a low wall is fine, a line through a roof
// or crown is not, and a line passing above everything is fine.
//
// Nothing here moves the player or changes collision.

#pragma once

#include "Vec3.h"
#include "World.h"

namespace game {

// True if `point` is inside something the camera must not enter.
bool cameraBlockedAt(const World& world, Vec3 point);

// True if a camera at `point` has `clearance` tiles of room around it (on
// the ground plane) at its height.
bool cameraClearAt(const World& world, Vec3 point, float clearance);

// Where the camera may go, right now, if it would like to be at `desired`
// while looking at `lookAt` (the player's head):
//   - the line from lookAt to desired is checked every kCameraSampleStep;
//     at the first blocked point the camera is pulled in to just before it;
//   - then it is pulled in further until it has kCameraClearance of room;
//   - it stays at least kCameraMinDistance from lookAt, unless that point
//     is itself inside something (then the free point closest to it);
//   - and never closer than kCameraNearestDistance, so the view never
//     collapses (only possible if lookAt itself is inside something).
// Always returns a finite point on the line from lookAt to desired. If
// nothing is in the way, returns `desired` unchanged.
Vec3 unblockedCameraPosition(const World& world, Vec3 lookAt, Vec3 desired);

// The same, with smoothing over time. Keep one of these per camera and call
// update() once per frame. Only the camera's DISTANCE from lookAt is
// smoothed (the direction always follows the orbit, so mouse turning stays
// immediate): it moves toward the allowed distance at a steady rate, a full
// pull-in taking kCameraPullInSeconds and a full release
// kCameraReleaseSeconds. So the camera never jumps when walking past a
// corner. While it is catching up (at most ~0.1 s) it can briefly be
// further out than the allowed point.
class CameraOcclusion {
public:
    Vec3 update(const World& world, Vec3 lookAt, Vec3 desired, float dt);

    // The distance from lookAt the camera is at now (after the last update).
    float distance() const { return distance_; }

private:
    float distance_ = -1.0f; // < 0 = no update yet: start at the allowed distance
};

} // namespace game
