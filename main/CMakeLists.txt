# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/../esp-idf-lib-master/components)

set(COMPONENT_SRCS "NextionMQTTBridge.cpp" "UART.cpp")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
