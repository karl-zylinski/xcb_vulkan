#!/usr/bin/env python3

import os

os.system("clang -Wall -o xcb_vulkan xcb_vulkan.c -lxcb -lvulkan -DVK_USE_PLATFORM_XCB_KHR");
