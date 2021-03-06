set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

set(SRCS
            camera/still_image_camera.cpp
            config.cpp
            debugger/callstack.cpp
            debugger/disassembler.cpp
            debugger/graphics/graphics.cpp
            debugger/graphics/graphics_breakpoint_observer.cpp
            debugger/graphics/graphics_breakpoints.cpp
            debugger/graphics/graphics_cmdlists.cpp
            debugger/graphics/graphics_surface.cpp
            debugger/graphics/graphics_tracing.cpp
            debugger/graphics/graphics_vertex_shader.cpp
            debugger/profiler.cpp
            debugger/ramview.cpp
            debugger/registers.cpp
            debugger/wait_tree.cpp
            droparea.cpp
            util/spinbox.cpp
            util/util.cpp
            bootmanager.cpp
            cheat_gui.cpp
            configure_audio.cpp
            configure_camera.cpp
            configure_debug.cpp
            configure_dialog.cpp
            configure_general.cpp
            configure_graphics.cpp
            configure_input.cpp
            configure_system.cpp
            game_list.cpp
            hotkeys.cpp
            keybinding_names.cpp
            main.cpp
            ui_settings.cpp
            citra-qt.rc
            Info.plist
            )

set(HEADERS
            camera/still_image_camera.h
            config.h
            debugger/callstack.h
            debugger/disassembler.h
            debugger/graphics/graphics.h
            debugger/graphics/graphics_breakpoint_observer.h
            debugger/graphics/graphics_breakpoints.h
            debugger/graphics/graphics_breakpoints_p.h
            debugger/graphics/graphics_cmdlists.h
            debugger/graphics/graphics_surface.h
            debugger/graphics/graphics_tracing.h
            debugger/graphics/graphics_vertex_shader.h
            debugger/profiler.h
            debugger/ramview.h
            debugger/registers.h
            debugger/wait_tree.h
            droparea.h
            util/spinbox.h
            util/util.h
            bootmanager.h
            cheat_gui.h
            configure_audio.h
            configure_camera.h
            configure_debug.h
            configure_dialog.h
            configure_general.h
            configure_graphics.h
            configure_input.h
            configure_system.h
            game_list.h
            game_list_p.h
            hotkeys.h
            keybinding_names.h
            main.h
            ui_settings.h
            )

set(UIS
            debugger/callstack.ui
            debugger/disassembler.ui
            debugger/profiler.ui
            debugger/registers.ui
            cheat_gui.ui
            configure.ui
            configure_audio.ui
            configure_camera.ui
            configure_debug.ui
            configure_general.ui
            configure_graphics.ui
            configure_system.ui
            configure_input.ui
            configure_system.ui
            hotkeys.ui
            main.ui
            )

create_directory_groups(${SRCS} ${HEADERS} ${UIS})

if (Qt5_FOUND)
    qt5_wrap_ui(UI_HDRS ${UIS})
else()
    qt4_wrap_ui(UI_HDRS ${UIS})
endif()

if (APPLE)
    set(MACOSX_ICON "../../dist/citra.icns")
    set_source_files_properties(${MACOSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    add_executable(citra-qt MACOSX_BUNDLE ${SRCS} ${HEADERS} ${UI_HDRS} ${MACOSX_ICON})
    set_target_properties(citra-qt PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
else()
    add_executable(citra-qt ${SRCS} ${HEADERS} ${UI_HDRS})
endif()
target_link_libraries(citra-qt core video_core audio_core input_core common qhexedit)
target_link_libraries(citra-qt ${OPENGL_gl_LIBRARY} ${CITRA_QT_LIBS})
target_link_libraries(citra-qt ${PLATFORM_LIBRARIES} Threads::Threads)

if(UNIX AND NOT APPLE)
    install(TARGETS citra-qt RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
endif()

if (MSVC)
    include(CopyCitraQt5Deps)
    include(CopyCitraSDLDeps)
    copy_citra_Qt5_deps(citra-qt)
    copy_citra_SDL_deps(citra-qt)
endif()
