LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	bmx.c \
	msg.c \
	metrics.c \
	tools.c \
	plugin.c \
	list.c \
	allocate.c \
	avl.c \
	iid.c \
	hna.c \
	control.c \
	schedule.c \
	ip.c \
	cyassl/sha.c \
	cyassl/random.c \
	cyassl/arc4.c

GIT_REV := $(shell ( [ "$(REVISION_VERSION)" ] && echo "$(REVISION_VERSION)" ) || ( [ -d .git ] && git --no-pager log -n 1 --oneline|cut -d " " -f 1 ) ||  echo 0)
LOCAL_CFLAGS := -Wall -W -Wno-unused-parameter -O0 -g3 -std=gnu99 -DGIT_REV=\"$(GIT_REV)\"

# -pedantic yields a lot of warnings from the NDK includes
LOCAL_CFLAGS := $(filter-out -pedantic,$(CFLAGS))

# Separate, since changing it on its own will break the Android app
LOCAL_MODULE := bmx6

LOCAL_FORCE_STATIC_EXECUTABLE := true

include $(BUILD_EXECUTABLE)
