GIT_REV ?= $(shell [ -r .git ] && git --no-pager log -n 1 --oneline | cut -d " " -f 1 || echo 0)

CFLAGS += -pedantic -Wall -W -Wno-unused-parameter -Os -g3 -std=gnu99 -DGIT_REV=\"$(GIT_REV)\"
#-DHAVE_CONFIG_H

# optinal defines:
# CFLAGS += -static
# CFLAGS += -pg # "-pg" with openWrt causes "gcrt1.o: No such file"! Needs ld -o myprog /lib/gcrt0.o myprog.o utils.o -lc_p, grep: http://www.cs.utah.edu/dept/old/texinfo/as/gprof.html

# paranoid defines (helps bug hunting during development):
# CFLAGS += -DEXTREME_PARANOIA -DEXIT_ON_ERROR -DPROFILING

# Some test cases:
# CFLAGS += -DTEST_LINK_ID_COLLISION_DETECTION
# CFLAGS += -DTEST_DEBUG          # (testing syntax of __VA_ARGS__ dbg...() macros)
# CFLAGS += -DTEST_DEBUG_MALLOC   # allocates a never freed byte which should be reported at bmx6 termination
# CFLAGS += -DAVL_5XLINKED -DAVL_DEBUG -DAVL_TEST

# optional defines (you may disable these features if you dont need them)
# CFLAGS += -DNO_DEBUG_TRACK
# CFLAGS += -DNO_DEBUG_SYS
# CFLAGS += -DLESS_OPTIONS
# CFLAGS += -DNO_DYN_PLUGIN
# CFLAGS += -DNO_TRACE_FUNCTION_CALLS

# CFLAGS += -DDEBUG_ALL
# CFLAGS += -DTRAFFIC_DUMP
# CFLAGS += -DDEBUG_DUMP
CFLAGS += -DDEBUG_MALLOC
# CFLAGS += -DMEMORY_USAGE

# experimental or advanced defines (please dont touch):
# CFLAGS += -DNO_ASSERTIONS       # (disable syntax error checking and error-code creation!)
# CFLAGS += -DEXTREME_PARANOIA    # (check difficult syntax errors)
# CFLAGS += -DEXIT_ON_ERROR       # (exit and return code due to unusual behavior)
# CFLAGS += -DTEST_DEBUG
# CFLAGS += -DWITH_UNUSED         # (includes yet unused stuff and buggy stuff)
# CFLAGS += -DPROFILING           # (no static functions -> better profiling and cores)

#EXTRA_CFLAGS +=
#EXTRA_LDFLAGS +=

#for profiling:
#EXTRA_CFLAGS="-DPROFILING -pg"

#for very poor embedded stuff (reducing binary size and cpu footprint):
#EXTRA_CFLAGS="-DNO_DEBUG_TRACK -DNO_TRACE_FUNCTION_CALLS -DNO_ASSERTIONS"

#for small embedded stuff the defaults are just fine.

#for normal machines (adding features and facilitating debugging):
#EXTRA_CFLAGS="-DDEBUG_ALL -DTRAFFIC_DUMP -DDEBUG_DUMP -DEBUG_MALLOC -DMEMORY_USAGE"

LDFLAGS += -g3

LDFLAGS += $(shell echo "$(CFLAGS) $(EXTRA_CFLAGS)" | grep -q "DNO_DYNPLUGIN" || echo "-Wl,-export-dynamic -ldl" )
LDFLAGS += $(shell echo "$(CFLAGS) $(EXTRA_CFLAGS)" | grep -q "DPROFILING" && echo "-pg -lc" )

SBINDIR = $(INSTALL_PREFIX)/usr/sbin

SRC_C = bmx.c msg.c metrics.c tools.c plugin.c list.c allocate.c avl.c iid.c hna.c control.c schedule.c ip.c cyassl/sha.c cyassl/random.c cyassl/arc4.c
SRC_H = bmx.h msg.h metrics.h tools.h plugin.h list.h allocate.h avl.h iid.h hna.h control.h schedule.h ip.h cyassl/sha.h cyassl/random.h cyassl/arc4.h

SRC_C += $(shell echo "$(CFLAGS) $(EXTRA_CFLAGS)" | grep -q "DTRAFFIC_DUMP" && echo dump.c )
SRC_H += $(shell echo "$(CFLAGS) $(EXTRA_CFLAGS)" | grep -q "DTRAFFIC_DUMP" && echo dump.h )

OBJS = $(SRC_C:.c=.o)

PACKAGE_NAME := bmx6
BINARY_NAME  := bmx6
