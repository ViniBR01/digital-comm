# Create the main library
add_library(sdr_network_lib
    # Network interface layer
    network/tun_tap_interface.cpp
    
    # Future implementations will go here:
    # packet/frame.cpp
    # baseband/modulator.cpp  
    # sdr/device.cpp
    # telemetry/collector.cpp
    # common/logging.cpp
)

target_include_directories(sdr_network_lib
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(sdr_network_lib
    PUBLIC
    ${Boost_LIBRARIES}
    Threads::Threads
)

# Add UHD libraries if available
if(UHD_FOUND)
    target_include_directories(sdr_network_lib PUBLIC ${UHD_INCLUDE_DIRS})
    target_link_libraries(sdr_network_lib PUBLIC ${UHD_LIBRARIES})
endif()

# Main executable
add_executable(sdr_network main.cpp)
target_link_libraries(sdr_network
    PRIVATE
    sdr_network_lib
)

# Installation
install(TARGETS sdr_network sdr_network_lib
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
