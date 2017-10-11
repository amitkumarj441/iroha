
addlibrary(stateful_validator
    impl/stateful_validator_impl.cpp
    )
target_link_libraries(stateful_validator
    optional
    ed25519
    rxcpp
    model
    logger
    )

addlibrary(stateless_validator
    impl/stateless_validator_impl.cpp
    )
target_link_libraries(stateless_validator
    model
    logger
    )

addlibrary(chain_validator
    impl/chain_validator_impl.cpp)
target_link_libraries(chain_validator
    optional
    ed25519
    rxcpp
    model
    logger
    )
