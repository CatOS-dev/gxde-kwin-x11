set(splitscreen_SOURCES
    main.cpp
    splitprevieweffect.cpp
)

kwin_add_builtin_effect(splitscreen ${splitscreen_SOURCES})
target_link_libraries(splitscreen PRIVATE
    kwin
    Qt6::DBus
    Qt6::GuiPrivate
)

install(DIRECTORY qml/ DESTINATION ${KDE_INSTALL_DATADIR}/${KWIN_DATADIR}/effects/splitscreen/qml)
