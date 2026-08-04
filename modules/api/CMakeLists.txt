get_filename_component(MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

file(GLOB DEMO_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/demo*")

foreach (DEMO_DIR ${DEMO_DIRS})
    if (IS_DIRECTORY ${DEMO_DIR})
        get_filename_component(DEMO_NAME ${DEMO_DIR} NAME)

        set(TARGET_NAME "${MODULE_NAME}_${DEMO_NAME}")

        file(GLOB SRC_FILES "${DEMO_DIR}/*.cpp")
        file(GLOB HEADER_FILES "${DEMO_DIR}/*.h")

        if (SRC_FILES)
            add_executable(${TARGET_NAME} ${SRC_FILES} ${HEADER_FILES}
                    demo02_listen_backlog/main.cpp
                    demo03_accept/main.cpp
                    demo04_out_of_band_data_client/main.cpp
                    demo04_out_of_band_data_server/main.cpp
                    demo05_sendbuf_client/main.cpp
                    demo05_sendbuf_server/main.cpp
                    demo06_network_info_api/main.cpp
                    demo07_cgi_server/main.cpp
                    demo08_web_server/main.cpp
                    demo09_sendfile_server/main.cpp
                    demo10_reject_serveer/main.cpp
                    demo11_tee/main.cpp
                    ../IOReuse/demo01_select_server/main.cpp
                    ../signal/demo01_unified_event_source/main.cpp)
            target_link_directories(${TARGET_NAME} PRIVATE ${DEMO_DIR})
        endif ()
    endif ()
endforeach ()