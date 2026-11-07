kcoreaddons_add_plugin(gxde_deepin_chameleon
    INSTALL_NAMESPACE "${KWIN_PLUGINDIR}/org.kde.kdecoration3"
    SOURCES
        main.cpp
        chameleon.cpp
        chameleontheme.cpp
        chameleonbutton.cpp
        chameleonshadow.cpp
        chameleonconfig.cpp
        chameleonwindowtheme.cpp
        kwinutils.cpp
        themes.qrc
)

set_target_properties(gxde_deepin_chameleon PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${KWIN_PLUGINDIR}/org.kde.kdecoration3"
    LIBRARY_OUTPUT_NAME "com.gxde.chameleon"
)

target_compile_definitions(gxde_deepin_chameleon PRIVATE
    -DTRANSLATION_DOMAIN="gxde-kwin_x11"
)

target_link_libraries(gxde_deepin_chameleon PRIVATE
    kwin
    KDecoration3::KDecoration
    KF6::CoreAddons
    Qt6::Core
    Qt6::Gui
    Qt6::Svg
)
