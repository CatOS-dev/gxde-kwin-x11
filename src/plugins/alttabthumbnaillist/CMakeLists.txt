#######################################
# Effect

set(alttabthumbnaillist_SOURCES
    alttabthumbnaillist.cpp
    alttabthumbnaillist.qrc
    main.cpp
)
kwin_add_builtin_effect(alttabthumbnaillist ${alttabthumbnaillist_SOURCES})
target_link_libraries(alttabthumbnaillist PRIVATE
    kwin
)

