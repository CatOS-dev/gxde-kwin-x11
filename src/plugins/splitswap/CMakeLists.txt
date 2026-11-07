set(splitswap_SOURCES
    main.cpp
    splitswap.cpp
)

kwin_add_builtin_effect(splitswap ${splitswap_SOURCES})
target_link_libraries(splitswap PRIVATE kwin)
