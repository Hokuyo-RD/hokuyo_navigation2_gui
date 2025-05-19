# Makefile for c/urg/samples
# $Id$

# Compile options
SRCDIR = ../../src
INCLUDEDIR = ../../include/c
CC = gcc
CFLAGS = -g -O0 -Wall -Werror -W $(INCLUDES)
INCLUDES = -I$(INCLUDEDIR)
LDFLAGS =
LDLIBS = -lm `/bin/sh ld_wsock.sh` `/bin/sh ld_setupapi.sh`

# Target
TARGET = \
	get_distance_intensity_continuous_safetyCMD \
	get_distance_continuous_safetyCMD \
	get_distance_intensity_safetyCMD \
	get_distance_safetyCMD \
	get_full_step_distance_safetyCMD \
	sensor_parameter_safetyCMD \
	get_area_safetyCMD \
	get_status_safetyCMD \
	get_detection_log_safetyCMD \
	clear_detection_log_safetyCMD \

all : $(TARGET)

clean :
	$(RM) *.o $(TARGET) *.exe

depend :
	makedepend -Y -- $(INCLUDES) -- $(wildcard *.h *.c)

.PHONY : all depend clean

######################################################################
REQUIRE_LIB = $(SRCDIR)/liburg_c.a
$(REQUIRE_LIB) : $(wildcard $(SRCDIR)/*.[ch])
	cd $(@D)/ && $(MAKE) $(@F)

get_distance_intensity_continuous_safetyCMD \
get_distance_continuous_safetyCMD \
get_distance_intensity_safetyCMD \
get_distance_safetyCMD \
get_full_step_distance_safetyCMD \
sensor_parameter_safetyCMD \
get_area_safetyCMD \
get_status_safetyCMD \
get_detection_log_safetyCMD \
clear_detection_log_safetyCMD \ : $(REQUIRE_LIB)

find_port : $(REQUIRE_LIB)
