
TARGET_WIRING_EX_PATH = $(WIRING_EX_MODULE_PATH)
INCLUDE_DIRS += $(TARGET_WIRING_EX_PATH)/$(PLATFORM_NAME)/inc/
INCLUDE_DIRS += $(TARGET_WIRING_EX_PATH)/$(PLATFORM_NAME)/src/sensors/inc/

LIB_DIRS += $(TARGET_WIRING_EX_PATH)/$(PLATFORM_NAME)/src/lib/
LIBS += PDMFilter_CM4_GCC
