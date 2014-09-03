CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

# Project properties
PROJECT(qtspell)
SET(QTSPELL_VERSION 0.1.0)


# Soname, see http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
SET(QTSPELL_LIB_VERSION 0.0.0)
SET(QTSPELL_SO_VERSION 0)


# Variables
SET(CMAKE_INSTALL_LIB lib${LIB_SUFFIX})
SET(CMAKE_INSTALL_INCLUDE include/QtSpell)
SET(ISO_CODES_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "Prefix for the iso-codes package")
SET(DISABLE_QT4 OFF CACHE PATH "Disable Qt4 build")
SET(DISABLE_QT5 OFF CACHE PATH "Disable Qt5 build")


# Macros
FUNCTION(ADD_TRANSLATION _qm_files _lrelease_executable)
    FOREACH(_current_FILE ${ARGN})
        GET_FILENAME_COMPONENT(_abs_FILE ${_current_FILE} ABSOLUTE)
        GET_FILENAME_COMPONENT(qm ${_abs_FILE} NAME_WE)
        FILE(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qm-${QT_VER}")
        SET(qm "${CMAKE_CURRENT_BINARY_DIR}/qm-${QT_VER}/${qm}.qm")
        ADD_CUSTOM_COMMAND(OUTPUT ${qm}
            COMMAND ${_lrelease_executable}
            ARGS ${_abs_FILE} -qm ${qm}
            DEPENDS ${_abs_FILE} VERBATIM
        )
        LIST(APPEND ${_qm_files} ${qm})
    ENDFOREACH()
    SET(${_qm_files} ${${_qm_files}} PARENT_SCOPE)
ENDFUNCTION(ADD_TRANSLATION)


# Dependencies
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(ENCHANT REQUIRED enchant)
INCLUDE_DIRECTORIES(${ENCHANT_INCLUDE_DIRS})

IF(NOT DISABLE_QT4)
    FIND_PACKAGE(Qt4)
ENDIF(NOT DISABLE_QT4)
IF(NOT DISABLE_QT5)
    FIND_PACKAGE(Qt5Widgets)
ENDIF(NOT DISABLE_QT5)

IF(NOT Qt4_FOUND AND NOT Qt5Widgets_FOUND)
    message(FATAL_ERROR "Cannot locate neither qt4 nor qt5 development files." )
ENDIF(NOT Qt4_FOUND AND NOT Qt5Widgets_FOUND)

IF(Qt5Widgets_FOUND)
    FIND_PACKAGE(Qt5LinguistTools REQUIRED)
    GET_TARGET_PROPERTY(QT5_LRELEASE_EXECUTABLE Qt5::lrelease IMPORTED_LOCATION)
ENDIF(Qt5Widgets_FOUND)

FIND_PACKAGE(Doxygen)


# Library
SET(qtspell_SRCS src/QtSpell.cpp src/Codetable.cpp)
SET(qtspell_HDRS src/QtSpell_p.hpp)
SET(qtspell_MOC_HDRS src/QtSpell.hpp)
FILE(GLOB qtspell_TS locale/*.ts)

IF(NOT CMAKE_BUILD_TYPE_TOLOWER MATCHES "debug")
    ADD_DEFINITIONS(-DQT_NO_DEBUG_OUTPUT)
ENDIF(NOT CMAKE_BUILD_TYPE_TOLOWER MATCHES "debug")

IF(Qt4_FOUND)
    SET(QT_VER "qt4")

    ADD_TRANSLATION(qtspell-qt4_QM ${QT_LRELEASE_EXECUTABLE} ${qtspell_TS})

    QT4_WRAP_CPP(qtspell-qt4_MOC ${qtspell_MOC_HDRS})
    ADD_LIBRARY(qtspell-qt4 SHARED ${qtspell_SRCS} ${qtspell-qt4_MOC} ${qtspell_HDRS} ${qtspell-qt4_QM})
    QT4_USE_MODULES(qtspell-qt4 Core Gui)
    SET_TARGET_PROPERTIES(qtspell-qt4 PROPERTIES COMPILE_DEFINITIONS "ISO_CODES_PREFIX=\"${ISO_CODES_PREFIX}\"")
    SET_TARGET_PROPERTIES(qtspell-qt4 PROPERTIES VERSION ${QTSPELL_LIB_VERSION} SOVERSION ${QTSPELL_SO_VERSION})
    TARGET_LINK_LIBRARIES(qtspell-qt4 ${ENCHANT_LDFLAGS})

    CONFIGURE_FILE(QtSpell.pc.in QtSpell-qt4.pc @ONLY)

    INSTALL(TARGETS qtspell-qt4 LIBRARY DESTINATION ${CMAKE_INSTALL_LIB})
    INSTALL(FILES src/QtSpell.hpp DESTINATION ${CMAKE_INSTALL_INCLUDE}-${QT_VER})
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/QtSpell-qt4.pc DESTINATION ${CMAKE_INSTALL_LIB}/pkgconfig)
    INSTALL(FILES ${qtspell-qt4_QM} DESTINATION share/qt4/translations)
ENDIF(Qt4_FOUND)

IF(Qt5Widgets_FOUND)
    SET(QT_VER "qt5")

    ADD_TRANSLATION(qtspell-qt5_QM Qt5::lrelease ${qtspell_TS})

    QT5_WRAP_CPP(qtspell-qt5_MOC ${qtspell_MOC_HDRS})
    ADD_LIBRARY(qtspell-qt5 SHARED ${qtspell_SRCS} ${qtspell-qt5_MOC} ${qtspell_HDRS} ${qtspell-qt5_QM})
    QT5_USE_MODULES(qtspell-qt5 Widgets)
    SET_TARGET_PROPERTIES(qtspell-qt5 PROPERTIES COMPILE_DEFINITIONS "ISO_CODES_PREFIX=\"${ISO_CODES_PREFIX}\"")
    SET_TARGET_PROPERTIES(qtspell-qt5 PROPERTIES VERSION ${QTSPELL_LIB_VERSION} SOVERSION ${QTSPELL_SO_VERSION})
    TARGET_LINK_LIBRARIES(qtspell-qt5 ${ENCHANT_LDFLAGS})

    CONFIGURE_FILE(QtSpell.pc.in QtSpell-qt5.pc @ONLY)


    INSTALL(TARGETS qtspell-qt5 LIBRARY DESTINATION ${CMAKE_INSTALL_LIB})
    INSTALL(FILES src/QtSpell.hpp DESTINATION ${CMAKE_INSTALL_INCLUDE}-${QT_VER})
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/QtSpell-qt5.pc DESTINATION ${CMAKE_INSTALL_LIB}/pkgconfig)
    INSTALL(FILES ${qtspell-qt5_QM} DESTINATION share/qt5/translations)
ENDIF(Qt5Widgets_FOUND)



# Example
IF(Qt4_FOUND)
    QT4_WRAP_CPP(example-qt4_MOC examples/example.hpp)
    ADD_EXECUTABLE(example-qt4 examples/example.cpp ${example-qt4_MOC})
    QT4_USE_MODULES(example-qt4 Core Gui)
    INCLUDE_DIRECTORIES(src/)
    TARGET_LINK_LIBRARIES(example-qt4 qtspell-qt4)
ENDIF(Qt4_FOUND)

IF(Qt5Widgets_FOUND)
    QT5_WRAP_CPP(example-qt5_MOC examples/example.hpp)
    ADD_EXECUTABLE(example-qt5 examples/example.cpp ${example-qt5_MOC})
    QT5_USE_MODULES(example-qt5 Widgets)
    INCLUDE_DIRECTORIES(src/)
    TARGET_LINK_LIBRARIES(example-qt5 qtspell-qt5)
ENDIF(Qt5Widgets_FOUND)


# Documentation
IF(DOXYGEN_FOUND)
CONFIGURE_FILE(doc/Doxyfile.in doc/Doxyfile @ONLY)

add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc
    COMMENT "Generating API documentation with Doxygen" VERBATIM
)

ENDIF(DOXYGEN_FOUND)


# Spec file
set(ENV{LC_ALL} "C")
EXECUTE_PROCESS(COMMAND date +%a\ %b\ %d\ %Y OUTPUT_VARIABLE SPEC_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
CONFIGURE_FILE(qtspell.spec.in qtspell.spec @ONLY)
