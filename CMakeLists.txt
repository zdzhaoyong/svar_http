cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)
include(PICMake)
pi_add_target(svar_http SHARED src REQUIRED LTO System)
pi_add_target(http_demo BIN demo REQUIRED System LTO)
pi_report_target()

pi_install(TARGETS svar_http)
