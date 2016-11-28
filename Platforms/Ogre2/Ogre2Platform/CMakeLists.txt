set (PROJECTNAME MyGUI.Ogre2Platform)

if(MYGUI_RENDERSYSTEM EQUAL 8)
	include_directories(
	 include
	 ${MYGUI_SOURCE_DIR}/MyGUIEngine/include
	 ${OGRE_INCLUDE_DIR}
	 ${OGRE_SOURCE_DIR}
	)
elseif(MYGUI_RENDERSYSTEM EQUAL 9)
	include_directories(
	 include
	 ${MYGUI_SOURCE_DIR}/MyGUIEngine/include
	 ${OGRE_INCLUDE_DIR}
	 ${OGRE_INCLUDE_DIR_ADDITIONALS}
	 ${OGRE_SOURCE_DIR}
	)
	add_definitions("-DMYGUI_OGRE_21")
else ()
	#2do
	message("unknown render system in MyGUI.Ogre2Platform CMakeLists.txt")
endif()



include(${PROJECTNAME}.list)

add_library(${PROJECTNAME} ${HEADER_FILES} ${SOURCE_FILES})

add_dependencies(${PROJECTNAME} MyGUIEngine)

target_link_libraries(${PROJECTNAME} ${OGRE_LIBRARIES} ${OGRE_HlmsPbs_LIBRARIES} ${OGRE_HlmsUnlit_LIBRARIES} MyGUIEngine)
link_directories(${OGRE_LIB_DIR})

# installation rules
install(FILES ${HEADER_FILES}
	DESTINATION include/MYGUI
)
mygui_install_target(${PROJECTNAME} "")

# platform specific dependencies
if (APPLE)
	include_directories(/usr/local/include /usr/include)
	target_link_libraries(${PROJECTNAME} ${Carbon_LIBRARY})
endif ()
