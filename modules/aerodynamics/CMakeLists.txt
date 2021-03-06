cmake_minimum_required(VERSION 3.0)
set(the_description "The Airdynamics Module")

include_directories(./include
        ../../include/
        ../../3rdparty/eigen3
        ../../3rdparty/rapidjson/include
        /usr/local/include/eigen3
        /usr/local/include
        ../../3rdparty/websocketpp
        ../network_protocol/include
        ../common/include
        )

set(AIRDYNAMICS_MODULE_SOURCES
        src/airdynamics_parser.cpp
        src/base_component.cpp
        src/joints/base_joint.cpp
        src/joints/fixed_joint.cpp
        src/nodes/bodys/aircraft_node.cpp
        src/nodes/base_node.cpp
        src/aerodynamics_configurer.cpp
        include/RapidFDM/aerodynamics/flying_data_defines.h
        include/RapidFDM/aerodynamics/base_component.h
        include/RapidFDM/aerodynamics/aerodynamics.h
        include/RapidFDM/aerodynamics/airdynamics_parser.h
        include/RapidFDM/aerodynamics/joints/base_joint.h
        include/RapidFDM/aerodynamics/joints/joints.h
        include/RapidFDM/aerodynamics/nodes/base_node.h
        include/RapidFDM/aerodynamics/nodes/nodes.h
        include/RapidFDM/aerodynamics/nodes/bodys/aircraft_node.h
        include/RapidFDM/aerodynamics/nodes/node_helper.h
        include/RapidFDM/aerodynamics/nodes/wings/wing_node.h
        include/RapidFDM/aerodynamics/nodes/engines/easy_propeller.h
        include/RapidFDM/aerodynamics/nodes/engines/base_engine.h
        include/RapidFDM/aerodynamics/geometrys/wing_geometrys.h
        include/RapidFDM/aerodynamics/geometrys/base_geometry.h
        include/RapidFDM/aerodynamics/geometrys/Geometrys.h
        include/RapidFDM/aerodynamics/geometrys/geometry_helper.h
        include/RapidFDM/aerodynamics/geometrys/standard_geometrys.h
        include/RapidFDM/aerodynamics/joints/fixed_joint.h
        include/RapidFDM/aerodynamics/joints/joint_helper.h
        src/geometrys/wing_geometrys.cpp
        src/geometrys/base_geometry.cpp
        include/RapidFDM/aerodynamics/blade_element/wing_blade_element.h
        include/RapidFDM/aerodynamics/blade_element/blade_element_manager.h
        src/blade_element/wing_blade_element.cpp
        src/blade_element/blade_element_manger.cpp
        src/nodes/engines/easy_propeller.cpp
        include/RapidFDM/aerodynamics/blade_element/engine_element.h
        include/RapidFDM/aerodynamics/blade_element/base_blade_element.h
        src/blade_element/base_blade_element.cpp
        include/RapidFDM/aerodynamics/discrete_data_loader.h
        src/discrete_data_loader.cpp src/nodes/wing_node.cpp
        )

SET(AERODYNAMICS_CONFIGURER_SOURCES
        src/aerodynamics_configurer.cpp
        src/aerodynamics_configure_server.cpp
        include/RapidFDM/aerodynamics/aerodynamics_configurer.h
        )

set(AIRDYNAMICS_TEST_SOURCES tests/main.cpp)


add_library(rapidfdm_aerodynamics STATIC ${AIRDYNAMICS_MODULE_SOURCES})

target_link_libraries(rapidfdm_aerodynamics rapidfdm_common)

add_executable(rapidfdm_aerodynamics_configure_server ${AERODYNAMICS_CONFIGURER_SOURCES})
target_link_libraries(rapidfdm_aerodynamics_configure_server rapidfdm_aerodynamics rapidfdm_network_protocol)

install(TARGETS  rapidfdm_aerodynamics rapidfdm_aerodynamics_configure_server
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)

INSTALL (DIRECTORY ./include/
        DESTINATION include/
        FILES_MATCHING PATTERN "*.h*")
