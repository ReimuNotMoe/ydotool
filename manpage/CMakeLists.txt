function(man_page section page)
  set(src "${CMAKE_CURRENT_SOURCE_DIR}/${page}.${section}.scd")
  set(bin "${CMAKE_CURRENT_BINARY_DIR}/${page}.${section}")
  add_custom_target(${page}.${section} ALL COMMAND scdoc < ${src} > ${bin})
  install(FILES ${bin} DESTINATION ${CMAKE_INSTALL_MANDIR}/man${section}/)
endfunction(man_page)

man_page(1 ydotool)
man_page(8 ydotoold)
