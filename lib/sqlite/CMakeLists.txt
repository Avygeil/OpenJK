# Build SQLite as a static library from the source code amalgamation

# These settings only apply to this directory.
include_directories(${SQLITE_INCLUDE_DIR})
include_directories(include/)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

add_library(bundled_sqlite STATIC sqlite3.c)

# Let consumers get at our bundled copy in the standard CMake way.  These
# variables are not set in the cache, but instead shadow the variables in the
# cache.
set(SQLITE_LIBRARIES bundled_sqlite PARENT_SCOPE)
set(SQLITE_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/sqlite/include/ PARENT_SCOPE)

mark_as_advanced(SQLITE_LIBRARIES SQLITE_INCLUDE_DIRS)

# We need to build it as position-independent code, because it might get linked
# into dynamic libraries.
set_property(TARGET bundled_sqlite PROPERTY POSITION_INDEPENDENT_CODE True)
