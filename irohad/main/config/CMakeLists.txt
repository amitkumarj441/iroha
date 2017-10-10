add_library(irohad-flags
    gflags_impl/flags.cpp
    flags.hpp
    )
target_link_libraries(irohad-flags
    gflags
    logger
    )

add_library(irohad-config
    gflags_impl/gflags_config.hpp
    gflags_impl/gflags_config.cpp
    config.hpp
    )
target_link_libraries(irohad-config
    irohad-flags
    )
