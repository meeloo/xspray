project(Noodlz)

include_directories(resources)

add_executable (Noodlz src/Application.cpp src/MainWindow.cpp)

target_link_libraries(Noodlz nui3 ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
