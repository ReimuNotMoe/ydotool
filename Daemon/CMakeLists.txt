configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ydotoold.service.in"
    "${PROJECT_BINARY_DIR}/ydotoold.service"
    @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/ydotoold.service" DESTINATION ${SD_UNITDIR})
