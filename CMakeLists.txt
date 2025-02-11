cmake_minimum_required(VERSION 3.10)

project(esphome-otgw)

add_library(otgw components/otgw/climate.cpp components/otgw/button.cpp components/otgw/otgw.cpp)

target_include_directories(otgw PUBLIC .esphome/build/opentherm-gateway/src)
