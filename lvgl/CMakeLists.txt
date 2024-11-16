zephyr_library()
zephyr_library_link_libraries(LVGL)
target_link_libraries(LVGL INTERFACE zephyr_interface)
set_source_files_properties(
        ${ZEPHYR_BASE}/modules/lvgl/lvgl.c
        TARGET_DIRECTORY modules__lvgl
        PROPERTIES HEADER_FILE_ONLY ON)
zephyr_library_sources(lvgl.c)