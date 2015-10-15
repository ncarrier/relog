LOCAL_PATH := $(call my-dir)

ifdef TARGET_DEPLOY_ROOT
	RELOG_LIBRELOG_PATH := $(TARGET_DEPLOY_ROOT)/usr/lib/librelog.so
else
	RELOG_LIBRELOG_PATH := $(TARGET_OUT_FINAL)/usr/lib/librelog.so
endif
RELOG_FORCE_DEFAULT_OUTPUT_TO ?= /dev/stdout
RELOG_FORCE_DEFAULT_ERROR_TO ?= /dev/stderr

################################################################################
# librelog
################################################################################

include $(CLEAR_VARS)

LOCAL_MODULE := librelog
LOCAL_DESCRIPTION := A small wrapper library for redirecting standard output \
	streams to another log facility, e.g. ulog
LOCAL_CATEGORY_PATH := libs
LOCAL_SRC_FILES := \
	librelog.c \
	popen_noshell.c

LOCAL_CFLAGS := \
	-DRELOG_LIBRELOG_PATH=\"$(RELOG_LIBRELOG_PATH)\"

include $(BUILD_LIBRARY)

################################################################################
# relog
################################################################################

include $(CLEAR_VARS)

LOCAL_MODULE := relog
LOCAL_CATEGORY_PATH := utils
LOCAL_DESCRIPTION := A small executable wrapper for redirecting standard \
	output streams to another log facility, e.g. ulog, with librelog
LOCAL_SRC_FILES := relog.c
LOCAL_DEPENDS_HEADERS := libulog
LOCAL_REQUIRED_MODULES := librelog

LOCAL_CFLAGS := \
	-DRELOG_LIBRELOG_PATH=\"$(RELOG_LIBRELOG_PATH)\" \
	-DRELOG_FORCE_DEFAULT_OUTPUT_TO=\"$(RELOG_FORCE_DEFAULT_ERROR_TO)\" \
	-DRELOG_FORCE_DEFAULT_ERROR_TO=\"$(RELOG_FORCE_DEFAULT_ERROR_TO)\"

include $(BUILD_EXECUTABLE)

