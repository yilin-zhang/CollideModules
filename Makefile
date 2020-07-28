SOURCES = $(wildcard src/*.cpp)

DISTRIBUTABLES += $(wildcard LICENSE*) res
RACK_DIR ?= ../Rack-v1

include $(RACK_DIR)/plugin.mk
