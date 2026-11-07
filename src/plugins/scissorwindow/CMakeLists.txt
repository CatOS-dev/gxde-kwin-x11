set(scissor_SOURCES
    main.cpp
    scissorwindow.cpp
    scissor.qrc
)

kwin_add_builtin_effect(scissor ${scissor_SOURCES})
target_link_libraries(scissor PRIVATE
    kwin
    Qt6::Gui
)
