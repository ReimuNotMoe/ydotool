function(man_page section page)
  set(src "${CMAKE_CURRENT_SOURCE_DIR}/${page}.${section}.scd")
  set(bin "${CMAKE_CURRENT_BINARY_DIR}/${page}.${section}")
  add_custom_target(${page}.${section} ALL COMMAND ${SCDOC} < ${src} > ${bin})
  install(FILES ${bin} DESTINATION ${CMAKE_INSTALL_MANDIR}/man${section}/)
endfunction(man_page)

option(BUILD_DOCS "build documentation (requires scdoc)" ON)

find_program(SCDOC scdoc)

if(BUILD_DOCS AND NOT SCDOC)
    message(FATAL_ERROR "build documentation selected, but scdoc could not be found")
endif()

if(BUILD_DOCS)
  man_page(1 ydotool)
  man_page(8 ydotoold)
endif()
