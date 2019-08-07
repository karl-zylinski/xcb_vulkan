#include <xcb/xcb.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <vulkan/vulkan.h>

int main()
{
    xcb_connection_t* c = xcb_connect(NULL, NULL);
    xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(c)).data;
    xcb_drawable_t win = xcb_generate_id(c);

    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t values[] = {screen->white_pixel,  XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS};
    xcb_create_window(
        c,
        XCB_COPY_FROM_PARENT,
        win,
        screen->root,
        0, 0, 640, 480,
        10,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        mask, values);
    xcb_map_window(c, win);
    xcb_flush(c);

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "VulkanTest";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;

    const char* const extensions[] = {VK_KHR_XCB_SURFACE_EXTENSION_NAME};
    instance_info.ppEnabledExtensionNames = extensions;
    instance_info.enabledExtensionCount = 1;
    VkInstance instance;
    VkResult res;
    res = vkCreateInstance(&instance_info, NULL, &instance);
    assert(res == VK_SUCCESS);

    uint32_t devices_count = 0;
    res = vkEnumeratePhysicalDevices(instance, &devices_count, NULL);
    assert(res == VK_SUCCESS);
    assert(devices_count > 0);
    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * devices_count);
    res = vkEnumeratePhysicalDevices(instance, &devices_count, devices);
    assert(res == VK_SUCCESS);

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(devices[0], &queue_family_count, NULL);
    VkQueueFamilyProperties* queue_props = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(devices[0], &queue_family_count, queue_props);

    uint32_t queue_family_idx = -1;
    for (uint32_t i = 0; i < queue_family_count; ++i)
    {
        if (queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queue_family_idx = i;
            break;
        }
    }

    assert(queue_family_idx != -1);
    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.queueFamilyIndex = queue_family_idx;
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueCount = 1;
    float queue_priorities[] = {0.0};
    queue_info.pQueuePriorities = queue_priorities;

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;

    VkDevice device;
    res = vkCreateDevice(devices[0], &device_info, NULL, &device);
    assert(res == VK_SUCCESS);

    VkCommandPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.queueFamilyIndex = queue_family_idx;

    VkCommandPool cmd_pool;
    res = vkCreateCommandPool(device, &cmd_pool_info, NULL, &cmd_pool);
    assert(res == VK_SUCCESS);

    VkCommandBufferAllocateInfo cmd_info = {};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_info.commandPool = cmd_pool;
    cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_info.commandBufferCount = 1;

    VkCommandBuffer cmd;
    res = vkAllocateCommandBuffers(device, &cmd_info, &cmd);
    assert(res == VK_SUCCESS);

    VkXcbSurfaceCreateInfoKHR xcb_create_info = {};
    xcb_create_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    xcb_create_info.connection = c;
    xcb_create_info.window = win;

    VkSurfaceKHR surface;
    res = vkCreateXcbSurfaceKHR(instance, &xcb_create_info, NULL, &surface);
    assert(res == VK_SUCCESS);

    xcb_generic_event_t* evt;
    int run = 1;
    while (run && (evt = xcb_wait_for_event(c)))
    {
        switch(evt->response_type & ~0x80)
        {
            case XCB_KEY_PRESS:
                run = 0;
                break;
        }
        free(evt);
    }

    VkCommandBuffer bufs_to_free[1] = {cmd};
    vkFreeCommandBuffers(device, cmd_pool, 1, bufs_to_free);
    vkDestroyCommandPool(device, cmd_pool, NULL);
    vkDestroyDevice(device, NULL);
    free(devices);
    free(queue_props);
    vkDestroyInstance(instance, NULL);
    xcb_disconnect(c);

    return 0;
}