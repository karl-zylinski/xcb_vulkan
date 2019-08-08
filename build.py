#!/usr/bin/env python3

import os
import sys

c = os.system("clang -std=c99 -Wall -Werror xcb_vulkan.c -o xcb_vulkan -g -lm -lxcb -lvulkan -DVK_USE_PLATFORM_XCB_KHR");

if len(sys.argv) > 1 and sys.argv[1] == "run" and c == 0:
    os.system("./xcb_vulkan")
