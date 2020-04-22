find_path(GUROBI_INCLUDE_DIRS
    NAMES gurobi_c.h
    HINTS ${GUROBI_DIR} $ENV{GUROBI_HOME}
    PATH_SUFFIXES include)

find_library(GUROBI_LIBRARY
    NAMES gurobi gurobi81 gurobi90
    HINTS ${GUROBI_DIR} $ENV{GUROBI_HOME}
    PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gurobi DEFAULT_MSG GUROBI_LIBRARY GUROBI_INCLUDE_DIRS)

add_library(Gurobi::Gurobi INTERFACE IMPORTED)
target_include_directories(Gurobi::Gurobi INTERFACE ${GUROBI_INCLUDE_DIRS})
target_link_libraries(Gurobi::Gurobi INTERFACE ${GUROBI_LIBRARY})
