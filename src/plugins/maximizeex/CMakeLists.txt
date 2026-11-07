#######################################
# Effect

set(maximizeex_SOURCES
    main.cpp
    maximizeeffect.cpp
)

kwin_add_builtin_effect(maximizeex ${maximizeex_SOURCES})
target_link_libraries(maximizeex PRIVATE
    kwin
    KF6::ConfigGui
)
