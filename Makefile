#
# Copyright (c) Flyover Games, LLC
#

SHELL := /usr/bin/env bash

__default__: help

help:
	@echo done $@

GYP ?= gyp
gyp:
	$(GYP) --depth=. -f xcode -DOS=ios --generator-output=./node-sdl2-ios node-sdl2.gyp
	$(GYP) --depth=. -f xcode -DOS=osx --generator-output=./node-sdl2-osx node-sdl2.gyp
