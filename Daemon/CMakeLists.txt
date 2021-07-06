configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ydotool.service.in"
    "${PROJECT_BINARY_DIR}/ydotool.service"
    @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/ydotool.service" DESTINATION ${SD_UNITDIR})
