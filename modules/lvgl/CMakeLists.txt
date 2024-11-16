zephyr_library_amend()
zephyr_library_link_libraries(LVGL)
target_link_libraries(LVGL INTERFACE zephyr_interface)
set_source_files_properties(
        ${ZEPHYR_BASE}/modules/lvgl/lvgl.c
        TARGET_DIRECTORY ${lib_name}
        PROPERTIES HEADER_FILE_ONLY ON)
zephyr_library_sources(lvgl.c)