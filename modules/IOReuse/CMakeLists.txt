get_filename_component(MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
find_package(Threads REQUIRED)
file(GLOB DEMO_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/demo*")

foreach (DEMO_DIR ${DEMO_DIRS})
    if (IS_DIRECTORY ${DEMO_DIR})
        get_filename_component(DEMO_NAME ${DEMO_DIR} NAME)

        set(TARGET_NAME "${MODULE_NAME}_${DEMO_NAME}")

        file(GLOB SRC_FILES "${DEMO_DIR}/*.cpp")
        file(GLOB HEADER_FILES "${DEMO_DIR}/*.h")

        if (SRC_FILES)
            add_executable(${TARGET_NAME} ${SRC_FILES} ${HEADER_FILES})
            target_link_directories(${TARGET_NAME} PRIVATE ${DEMO_DIR} Threads::Threads)
        endif ()
    endif ()
endforeach ()