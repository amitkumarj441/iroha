addlibrary(simulator
    impl/simulator.cpp
    )

target_link_libraries(simulator
    model
    rxcpp
    optional
    logger
    )
