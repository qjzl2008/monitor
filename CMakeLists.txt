PROJECT(Monitor)
cmake_minimum_required(VERSION 3.4)
add_definitions("-std=c++0x")

add_executable(test_monitor_api src/monitor_api/main.cpp src/monitor_api/monitor_api.cpp)
target_link_libraries(test_monitor_api pthread)

add_executable(monitor_agent src/monitor_agent/monitor_agent.cpp src/monitor_agent/MonitorCollecter.cpp src/monitor_agent/MonitorReporter.cpp 
libs/tinyxml/tinyxml.cpp libs/tinyxml/tinystr.cpp libs/tinyxml/tinyxmlerror.cpp libs/tinyxml/tinyxmlparser.cpp)
target_link_libraries(monitor_agent pthread mysqlpp)
