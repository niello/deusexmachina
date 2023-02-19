# HACK: see engine\Tools\SceneCommon\CMakeLists.txt
message(WARNING "DEM: Jasper library is forced to be not found to avoid find_package(JPEG)")
SET(JASPER_FOUND "NO")
