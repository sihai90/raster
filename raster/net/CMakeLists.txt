# Copyright 2017 Yeolar

file(GLOB RASTER_NET_SRCS *.cpp)
file(GLOB RASTER_NET_HDRS *.h)

add_library(raster_net OBJECT ${RASTER_NET_SRCS})

install(FILES ${RASTER_NET_HDRS} DESTINATION include/raster/net)
