#============================================================================
# Copyright (C) 2013 - 2018, OpenJK contributors
#
# This file is part of the OpenJK source code.
#
# OpenJK is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#============================================================================

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

set(MPSharedDefines ${SharedDefines})

#    Add Game Project
if(BuildMPGame)
	add_subdirectory("${MPDir}/game")
endif(BuildMPGame)

#    Common things between Engine and Dedicated Server
if(BuildMPDed)

	#    Botlib
	# the files could arguably just be put into the engine and dedicated projects without having a library for it.
	set(MPBotlibDefines ${SharedDefines} "BOTLIB")
	set(MPBotlibIncludeDirectories
		${SharedDir}
		${MPDir}
		"${GSLIncludeDirectory}"
		)
	set(MPBotlibBotlibFiles
		"${MPDir}/botlib/aasfile.h"
		"${MPDir}/botlib/be_aas.h"
		"${MPDir}/botlib/be_aas_bsp.h"
		"${MPDir}/botlib/be_aas_cluster.h"
		"${MPDir}/botlib/be_aas_debug.h"
		"${MPDir}/botlib/be_aas_def.h"
		"${MPDir}/botlib/be_aas_entity.h"
		"${MPDir}/botlib/be_aas_file.h"
		"${MPDir}/botlib/be_aas_funcs.h"
		"${MPDir}/botlib/be_aas_main.h"
		"${MPDir}/botlib/be_aas_move.h"
		"${MPDir}/botlib/be_aas_optimize.h"
		"${MPDir}/botlib/be_aas_reach.h"
		"${MPDir}/botlib/be_aas_route.h"
		"${MPDir}/botlib/be_aas_routealt.h"
		"${MPDir}/botlib/be_aas_sample.h"
		"${MPDir}/botlib/be_ai_char.h"
		"${MPDir}/botlib/be_ai_chat.h"
		"${MPDir}/botlib/be_ai_gen.h"
		"${MPDir}/botlib/be_ai_goal.h"
		"${MPDir}/botlib/be_ai_move.h"
		"${MPDir}/botlib/be_ai_weap.h"
		"${MPDir}/botlib/be_ai_weight.h"
		"${MPDir}/botlib/be_ea.h"
		"${MPDir}/botlib/be_interface.h"
		"${MPDir}/botlib/botlib.h"
		"${MPDir}/botlib/l_crc.h"
		"${MPDir}/botlib/l_libvar.h"
		"${MPDir}/botlib/l_log.h"
		"${MPDir}/botlib/l_memory.h"
		"${MPDir}/botlib/l_precomp.h"
		"${MPDir}/botlib/l_script.h"
		"${MPDir}/botlib/l_struct.h"
		"${MPDir}/botlib/l_utils.h"

		"${MPDir}/botlib/be_aas_bspq3.cpp"
		"${MPDir}/botlib/be_aas_cluster.cpp"
		"${MPDir}/botlib/be_aas_debug.cpp"
		"${MPDir}/botlib/be_aas_entity.cpp"
		"${MPDir}/botlib/be_aas_file.cpp"
		"${MPDir}/botlib/be_aas_main.cpp"
		"${MPDir}/botlib/be_aas_move.cpp"
		"${MPDir}/botlib/be_aas_optimize.cpp"
		"${MPDir}/botlib/be_aas_reach.cpp"
		"${MPDir}/botlib/be_aas_route.cpp"
		"${MPDir}/botlib/be_aas_routealt.cpp"
		"${MPDir}/botlib/be_aas_sample.cpp"
		"${MPDir}/botlib/be_ai_char.cpp"
		"${MPDir}/botlib/be_ai_chat.cpp"
		"${MPDir}/botlib/be_ai_gen.cpp"
		"${MPDir}/botlib/be_ai_goal.cpp"
		"${MPDir}/botlib/be_ai_move.cpp"
		"${MPDir}/botlib/be_ai_weap.cpp"
		"${MPDir}/botlib/be_ai_weight.cpp"
		"${MPDir}/botlib/be_ea.cpp"
		"${MPDir}/botlib/be_interface.cpp"
		"${MPDir}/botlib/l_crc.cpp"
		"${MPDir}/botlib/l_libvar.cpp"
		"${MPDir}/botlib/l_log.cpp"
		"${MPDir}/botlib/l_memory.cpp"
		"${MPDir}/botlib/l_precomp.cpp"
		"${MPDir}/botlib/l_script.cpp"
		"${MPDir}/botlib/l_struct.cpp"
		)
	source_group("botlib" FILES ${MPBotlibBotlibFiles})
	set(MPBotlibFiles ${MPBotlibFiles} ${MPBotlibBotlibFiles})

	set(MPBotlibGameFiles
		"${MPDir}/game/surfaceflags.h"
		)
	source_group("game" FILES ${MPBotlibGameFiles})
	set(MPBotlibFiles ${MPBotlibFiles} ${MPBotlibGameFiles})

	set(MPBotlibCommonFiles
		"${MPDir}/qcommon/cm_public.h"
		"${MPDir}/qcommon/disablewarnings.h"
		"${MPDir}/qcommon/q_shared.h"
		"${MPDir}/qcommon/qcommon.h"
		"${MPDir}/qcommon/qfiles.h"
		"${MPDir}/qcommon/tags.h"
		
		${SharedCommonFiles}
		)
	source_group("common" FILES ${MPBotlibCommonFiles})
	set(MPBotlibFiles ${MPBotlibFiles} ${MPBotlibCommonFiles})

	set(MPBotlibCommonSafeFiles
		"${SharedDir}/qcommon/safe/gsl.h"
		"${SharedDir}/qcommon/safe/string.cpp"
		"${SharedDir}/qcommon/safe/string.h"
		"${SharedDir}/qcommon/safe/sscanf.h"
		)
	source_group("common/safe" FILES ${MPBotlibCommonSafeFiles})
	set(MPBotlibFiles ${MPBotlibFiles} ${MPBotlibCommonSafeFiles})

	set(MPBotlibServerFiles
		"${MPDir}/server/server.h"
		)
	source_group("server" FILES ${MPBotlibServerFiles})
	set(MPBotlibFiles ${MPBotlibFiles} ${MPBotlibServerFiles})

	add_library(${MPBotLib} STATIC ${MPBotlibFiles})

	# Hide symbols not explicitly marked public.
	set_property(TARGET ${MPBotLib} APPEND PROPERTY COMPILE_OPTIONS ${OPENJK_VISIBILITY_FLAGS})

	set_target_properties(${MPBotLib} PROPERTIES COMPILE_DEFINITIONS "${MPBotlibDefines}")
	set_target_properties(${MPBotLib} PROPERTIES INCLUDE_DIRECTORIES "${MPBotlibIncludeDirectories}")
	set_target_properties(${MPBotLib} PROPERTIES PROJECT_LABEL "Bot Library")

	#    Common files/libraries/defines of both Engine and Dedicated Server

	# libraries: Botlib
	set(MPEngineAndDedLibraries ${MPBotLib})
	# Platform-specific libraries
	if(WIN32)
		set(MPEngineAndDedLibraries ${MPEngineAndDedLibraries} "winmm" "wsock32" "Ws2_32")
	endif(WIN32)

	# Include directories
	set(MPEngineAndDedIncludeDirectories ${MPDir} ${SharedDir} ${GSLIncludeDirectory}) # codemp folder, since includes are not always relative in the files

	# Transparently use our bundled minizip.
	list(APPEND MPEngineAndDedIncludeDirectories ${MINIZIP_INCLUDE_DIRS})
	list(APPEND MPEngineAndDedLibraries          ${MINIZIP_LIBRARIES})

	# Transparently use either bundled or system zlib.
	list(APPEND MPEngineAndDedIncludeDirectories ${ZLIB_INCLUDE_DIR})
	list(APPEND MPEngineAndDedLibraries          ${ZLIB_LIBRARIES})
	
	# libcurl
	if(UseInternalCurl)
		set(MPEngineAndDedLibraries ${MPEngineAndDedLibraries} ${OpenJKLibDir}/libcurl/lib/libcurl.lib)
		set(MPEngineAndDedIncludeDirectories ${MPEngineAndDedIncludeDirectories} ${OpenJKLibDir}/libcurl/include)
	else()
		find_package(CURL REQUIRED)
		set(MPEngineAndDedIncludeDirectories ${MPEngineAndDedIncludeDirectories} ${CURL_INCLUDE_DIRS})
		set(MPEngineAndDedLibraries ${MPEngineAndDedLibraries} ${CURL_LIBRARIES})
	endif()

	set(MPEngineAndDedCgameFiles
		"${MPDir}/cgame/cg_public.h"
		)
	source_group("cgame" FILES ${MPEngineAndDedCgameFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedCgameFiles})

	set(MPEngineAndDedClientFiles
		"${MPDir}/client/client.h"
		"${MPDir}/client/keys.h"
		"${MPDir}/client/snd_public.h"
		)
	source_group("client" FILES ${MPEngineAndDedClientFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedClientFiles})

	set(MPEngineAndDedGameFiles
		"${MPDir}/game/anims.h"
		"${MPDir}/game/bg_public.h"
		"${MPDir}/game/bg_weapons.h"
		"${MPDir}/botlib/botlib.h"
		"${MPDir}/game/g_public.h"
		"${MPDir}/game/surfaceflags.h"
		)
	source_group("game" FILES ${MPEngineAndDedGameFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedGameFiles})

	set(MPEngineAndDedCommonFiles
		"${MPDir}/qcommon/q_shared.h"
		"${SharedDir}/qcommon/q_platform.h"
		"${MPDir}/qcommon/cm_load.cpp"
		"${MPDir}/qcommon/cm_local.h"
		"${MPDir}/qcommon/cm_patch.cpp"
		"${MPDir}/qcommon/cm_patch.h"
		"${MPDir}/qcommon/cm_polylib.cpp"
		"${MPDir}/qcommon/cm_polylib.h"
		"${MPDir}/qcommon/cm_public.h"
		"${MPDir}/qcommon/cm_test.cpp"
		"${MPDir}/qcommon/cm_trace.cpp"
		"${MPDir}/qcommon/cmd.cpp"
		"${MPDir}/qcommon/common.cpp"
		"${MPDir}/qcommon/cvar.cpp"
		"${MPDir}/qcommon/disablewarnings.h"
		"${MPDir}/qcommon/files.cpp"
		"${MPDir}/qcommon/game_version.h"
		"${MPDir}/qcommon/GenericParser2.cpp"
		"${MPDir}/qcommon/GenericParser2.h"
		"${MPDir}/qcommon/huffman.cpp"
		"${MPDir}/qcommon/md4.cpp"
		"${MPDir}/qcommon/md5.cpp"
		"${MPDir}/qcommon/md5.h"
		"${MPDir}/qcommon/MiniHeap.h"
		"${MPDir}/qcommon/msg.cpp"
		"${MPDir}/qcommon/matcomp.h"
		"${MPDir}/qcommon/matcomp.cpp"
		"${MPDir}/qcommon/net_chan.cpp"
		"${MPDir}/qcommon/net_ip.cpp"
		"${MPDir}/qcommon/persistence.cpp"
		"${MPDir}/qcommon/q_shared.cpp"
		"${MPDir}/qcommon/qcommon.h"
		"${MPDir}/qcommon/qfiles.h"
		"${MPDir}/qcommon/RoffSystem.cpp"
		"${MPDir}/qcommon/RoffSystem.h"
		"${MPDir}/qcommon/sstring.h"
		"${MPDir}/qcommon/stringed_ingame.cpp"
		"${MPDir}/qcommon/stringed_ingame.h"
		"${MPDir}/qcommon/stringed_interface.cpp"
		"${MPDir}/qcommon/stringed_interface.h"
		"${MPDir}/qcommon/tags.h"
		"${MPDir}/qcommon/timing.h"
		"${MPDir}/qcommon/vm.cpp"
		"${MPDir}/qcommon/z_memman_pc.cpp"

		${SharedCommonFiles}
		)
	if(WIN32)
		set(MPEngineAndDedCommonFiles ${MPEngineAndDedCommonFiles})
	endif(WIN32)
	source_group("common" FILES ${MPEngineAndDedCommonFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedCommonFiles})
	
	set(MPEngineAndDedCommonSafeFiles
		"${SharedDir}/qcommon/safe/files.cpp"
		"${SharedDir}/qcommon/safe/files.h"
		"${SharedDir}/qcommon/safe/gsl.h"
		)
	source_group("common/safe" FILES ${MPEngineAndDedCommonSafeFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedCommonSafeFiles})

	set(MPEngineAndDedG2Files
		"${MPDir}/ghoul2/G2.h"
		"${MPDir}/ghoul2/G2_gore.h"
		"${MPDir}/ghoul2/ghoul2_shared.h"
		"${MPDir}/ghoul2/g2_local.h"
		)
	source_group("ghoul2" FILES ${MPEngineAndDedG2Files})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedG2Files})

	set(MPEngineAndDedIcarusFiles
		"${MPDir}/icarus/BlockStream.cpp"
		"${MPDir}/icarus/blockstream.h"
		"${MPDir}/icarus/GameInterface.cpp"
		"${MPDir}/icarus/GameInterface.h"
		"${MPDir}/icarus/icarus.h"
		"${MPDir}/icarus/Instance.cpp"
		"${MPDir}/icarus/instance.h"
		"${MPDir}/icarus/Interface.cpp"
		"${MPDir}/icarus/interface.h"
		"${MPDir}/icarus/interpreter.h"
		"${MPDir}/icarus/Memory.cpp"
		"${MPDir}/icarus/Q3_Interface.cpp"
		"${MPDir}/icarus/Q3_Interface.h"
		"${MPDir}/icarus/Q3_Registers.cpp"
		"${MPDir}/icarus/Q3_Registers.h"
		"${MPDir}/icarus/Sequence.cpp"
		"${MPDir}/icarus/sequence.h"
		"${MPDir}/icarus/Sequencer.cpp"
		"${MPDir}/icarus/sequencer.h"
		"${MPDir}/icarus/TaskManager.cpp"
		"${MPDir}/icarus/taskmanager.h"
		"${MPDir}/icarus/tokenizer.h"
		)
	source_group("icarus" FILES ${MPEngineAndDedIcarusFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedIcarusFiles})

	set(MPEngineAndDedServerFiles
		"${MPDir}/server/NPCNav/navigator.cpp"
		"${MPDir}/server/NPCNav/navigator.h"
		"${MPDir}/server/server.h"
		"${MPDir}/server/sv_bot.cpp"
		"${MPDir}/server/sv_ccmds.cpp"
		"${MPDir}/server/sv_challenge.cpp"
		"${MPDir}/server/sv_client.cpp"
		"${MPDir}/server/sv_curl.cpp"
		"${MPDir}/server/sv_game.cpp"
		"${MPDir}/server/sv_init.cpp"
		"${MPDir}/server/sv_main.cpp"
		"${MPDir}/server/sv_net_chan.cpp"
		"${MPDir}/server/sv_snapshot.cpp"
		"${MPDir}/server/sv_world.cpp"
		"${MPDir}/server/sv_gameapi.cpp"
		"${MPDir}/server/sv_gameapi.h"
		"${MPDir}/server/sv_geoip.cpp"
		"${MPDir}/server/maxminddb.c"
		"${MPDir}/server/maxminddb.h"
		"${MPDir}/server/maxminddb_config.h"
		"${MPDir}/server/maxminddb-compat-util.h"
		"${MPDir}/server/sv_database.cpp"
		"${MPDir}/server/sqlite3.c"
		"${MPDir}/server/sqlite3.h"
		"${MPDir}/server/sv_location.cpp"
		"${MPDir}/server/kdtree.c"
		"${MPDir}/server/kdtree.h"
		)
	source_group("server" FILES ${MPEngineAndDedServerFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedServerFiles})

	set(MPEngineAndDedUiFiles
		"${MPDir}/ui/keycodes.h"
		"${MPDir}/ui/ui_public.h"
		)
	source_group("ui" FILES ${MPEngineAndDedUiFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedUiFiles})

	set(MPEngineAndDedSysFiles
		"${SharedDir}/sys/snapvector.cpp"
		)
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedSysFiles})
	source_group("sys" FILES ${MPEngineAndDedSysFiles})
endif(BuildMPDed)

#        Dedicated Server (Engine) (jampded.exe)

if(BuildMPDed)
	set(MPDedLibraries ${MPEngineAndDedLibraries})
	set(MPDedIncludeDirectories ${MPEngineAndDedIncludeDirectories})
	set(MPDedFiles ${MPEngineAndDedFiles})
	set(MPDedDefines ${MPSharedDefines} "_CONSOLE" "DEDICATED")

	# Dedicated renderer is compiled with the server.
	set(MPDedicatedRendererFiles
		"${MPDir}/ghoul2/G2_gore.cpp"
		"${MPDir}/rd-common/mdx_format.h"
		"${MPDir}/rd-common/tr_public.h"
		"${MPDir}/rd-dedicated/tr_local.h"
		"${MPDir}/rd-dedicated/G2_API.cpp"
		"${MPDir}/rd-dedicated/G2_bolts.cpp"
		"${MPDir}/rd-dedicated/G2_bones.cpp"
		"${MPDir}/rd-dedicated/G2_misc.cpp"
		"${MPDir}/rd-dedicated/G2_surfaces.cpp"
		"${MPDir}/rd-dedicated/tr_backend.cpp"
		"${MPDir}/rd-dedicated/tr_ghoul2.cpp"
		"${MPDir}/rd-dedicated/tr_init.cpp"
		"${MPDir}/rd-dedicated/tr_main.cpp"
		"${MPDir}/rd-dedicated/tr_mesh.cpp"
		"${MPDir}/rd-dedicated/tr_model.cpp"
		"${MPDir}/rd-dedicated/tr_shader.cpp"
		"${MPDir}/rd-dedicated/tr_skin.cpp")
	source_group("renderer" FILES ${MPDedicatedRendererFiles})
	set(MPDedFiles ${MPDedFiles} ${MPDedicatedRendererFiles})

	set(MPDedNullFiles
		"${MPDir}/null/null_client.cpp"
		"${MPDir}/null/null_input.cpp"
		"${MPDir}/null/null_renderer.cpp"
		"${MPDir}/null/null_snddma.cpp"
		)
	source_group("null" FILES ${MPDedNullFiles})
	set(MPDedFiles ${MPDedFiles} ${MPDedNullFiles})
	if(WIN32)
		set(MPDedWin32Files
			"${MPDir}/win32/openjkded.rc"
			)
		source_group("win32" FILES ${MPDedWin32Files})
		set(MPDedFiles ${MPDedFiles} ${MPDedWin32Files})

		set(MPDedSysFiles
			"${SharedDir}/sys/sys_loadlib.h"
			"${SharedDir}/sys/sys_local.h"
			"${SharedDir}/sys/sys_main.cpp"
			"${SharedDir}/sys/sys_event.cpp"
			"${SharedDir}/sys/sys_public.h"
			"${SharedDir}/sys/sys_win32.cpp"
			"${SharedDir}/sys/con_local.h"
			"${SharedDir}/sys/con_log.cpp"
			"${SharedDir}/sys/con_win32.cpp"
			)
		set(MPDedLibraries ${MPDedLibraries} ${CMAKE_DL_LIBS})
		source_group("sys" FILES ${MPDedSysFiles})
		set(MPDedFiles ${MPDedFiles} ${MPDedSysFiles})
	else(WIN32)
		set(MPDedSysFiles
			"${SharedDir}/sys/sys_loadlib.h"
			"${SharedDir}/sys/sys_local.h"
			"${SharedDir}/sys/sys_main.cpp"
			"${SharedDir}/sys/sys_event.cpp"
			"${SharedDir}/sys/sys_public.h"
			"${SharedDir}/sys/sys_unix.cpp"
			"${SharedDir}/sys/con_local.h"
			"${SharedDir}/sys/con_log.cpp"
			"${SharedDir}/sys/con_tty.cpp"
			)
		set(MPDedLibraries ${MPDedLibraries} ${CMAKE_DL_LIBS})
		source_group("sys" FILES ${MPDedSysFiles})
		set(MPDedFiles ${MPDedFiles} ${MPDedSysFiles})
	endif(WIN32)

	add_executable(${MPDed} ${MPDedFiles})
	install(TARGETS ${MPDed}
		RUNTIME
		DESTINATION ${JKAInstallDir}
		COMPONENT ${JKAMPServerComponent})

	set_target_properties(${MPDed} PROPERTIES COMPILE_DEFINITIONS "${MPDedDefines}")

	# Hide symbols not explicitly marked public.
	set_property(TARGET ${MPDed} APPEND PROPERTY COMPILE_OPTIONS ${OPENJK_VISIBILITY_FLAGS})

	set_target_properties(${MPDed} PROPERTIES INCLUDE_DIRECTORIES "${MPDedIncludeDirectories}")
	set_target_properties(${MPDed} PROPERTIES PROJECT_LABEL "MP Dedicated Server")
	target_link_libraries(${MPDed} ${MPDedLibraries})
endif(BuildMPDed)

	set(GameLibsBuilt)
	if(BuildMPGame)
		set(GameLibsBuilt ${GameLibsBuilt} ${MPGame})
	endif()

if(WIN32)
set(GameLibFullPaths)
	if(MSVC)
		foreach(GameLib ${GameLibsBuilt})
			set(GameLibFullPaths
				${GameLibFullPaths}
				${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${GameLib}${CMAKE_SHARED_LIBRARY_SUFFIX})
		endforeach(GameLib)
	else()
		foreach(GameLib ${GameLibsBuilt})
			set(GameLibFullPaths
				${GameLibFullPaths}
				${CMAKE_BINARY_DIR}/${GameLib}${CMAKE_SHARED_LIBRARY_SUFFIX})
		endforeach(GameLib)
	endif()

	include(InstallZIP)
	add_zip_command(${AssetsPk3}
		FILES ${GameLibFullPaths}
		DEPENDS "${GameLibsBuilt}")
	add_custom_target(Assets
		ALL
		DEPENDS ${AssetsPk3})
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${AssetsPk3}
		DESTINATION "${JKAInstallDir}/OpenJK"
		COMPONENT ${JKAMPCoreComponent})
endif()
