find_path(GUROBI_INCLUDE_DIRS
    NAMES gurobi_c.h
    HINTS ${GUROBI_DIR} $ENV{GUROBI_HOME}
    PATH_SUFFIXES include)

find_library(GUROBI_LIBRARY
    NAMES gurobi gurobi81 gurobi90
    HINTS ${GUROBI_DIR} $ENV{GUROBI_HOME}
    PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GUROBI DEFAULT_MSG GUROBI_LIBRARY)
