Name:
Bullet physics

URLs:
https://pybullet.org

Current version used:
3.24+
https://github.com/bulletphysics/bullet3/commit/2c204c49e56ed15ec5fcfa71d199ab6d6570b3f5

Local modifications:
Unused CMake targets commented out in CMakeLists.txt and src/CMakeLists.txt
BT_USE_SSE_IN_API enabled with #if !defined(BT_USE_SSE_IN_API) && (defined (_WIN32) || defined (__i386__)) instead of #if defined (_WIN32) || defined (__i386__)


Purpose in DEM:
Physics simulation, collision, domains, triggers.
User interaction with objects.
