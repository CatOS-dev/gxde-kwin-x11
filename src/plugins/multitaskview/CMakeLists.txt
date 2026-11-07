#######################################
# Effect

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Concurrent LinguistTools)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)

set(multitaskview_SOURCES
    multitaskview.cpp
    qgsettingscompat.cpp
    multitaskview.qrc
    main.cpp
)
kwin_add_builtin_effect(multitaskview ${multitaskview_SOURCES})
target_link_libraries(multitaskview PRIVATE
    kwin
    KF6::GlobalAccel
    Qt6::Concurrent
    Qt6::DBus
    Qt6::GuiPrivate
    PkgConfig::GIO
)

file(GLOB multitaskview_TS CONFIGURE_DEPENDS translations/multitasking*.ts)
qt6_add_lrelease(
    TS_FILES ${multitaskview_TS}
    QM_FILES_OUTPUT_VARIABLE multitaskview_QM
    LRELEASE_TARGET multitaskview_lrelease
)
foreach(qm_file IN LISTS multitaskview_QM)
    get_filename_component(qm_name "${qm_file}" NAME)
    set_source_files_properties("${qm_file}" PROPERTIES QT_RESOURCE_ALIAS "${qm_name}")
endforeach()
qt_add_resources(multitaskview multitaskview_translations
    PREFIX "/effects/multitaskview/translations"
    FILES ${multitaskview_QM}
)

install(DIRECTORY qml6/ DESTINATION ${KDE_INSTALL_DATADIR}/${KWIN_DATADIR}/effects/multitaskview/qml)
