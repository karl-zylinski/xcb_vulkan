#include <xcb/xcb.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <math.h>

typedef struct {
    VkImage image;
    VkImageView view;
} swapchain_buffer_t;

typedef struct
{
    float x, y, z, w;
} vec4_t;

typedef struct
{
    vec4_t x, y, z, w;
} mat4_t;

const float pi = 3.1415926535897932;

static mat4_t create_projection_matrix(float bb_width, float bb_height)
{
    float near_plane = 0.01f;
    float far_plane = 1000.0f;
    float fov = 75.0f;
    float aspect = bb_width / bb_height;
    float y_scale = 1.0f / tanf((pi / 180.0f) * fov / 2);
    float x_scale = y_scale / aspect;
    mat4_t proj = {
        {x_scale, 0, 0, 0},
        {0, 0, far_plane/(far_plane-near_plane), 1},
        {0, y_scale, 0, 0},
        {0, 0, (-far_plane * near_plane) / (far_plane - near_plane), 0}
    };
    return proj;
}


mat4_t mat4_identity()
{
    const mat4_t i = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };

    return i;
}



mat4_t mat4_mul(const mat4_t* m1, const mat4_t* m2)
{
    mat4_t product =
    {
        {
            m1->x.x * m2->x.x + m1->x.y * m2->y.x + m1->x.z * m2->z.x + m1->x.w * m2->w.x,
            m1->x.x * m2->x.y + m1->x.y * m2->y.y + m1->x.z * m2->z.y + m1->x.w * m2->w.y,
            m1->x.x * m2->x.z + m1->x.y * m2->y.z + m1->x.z * m2->z.z + m1->x.w * m2->w.z,
            m1->x.x * m2->x.w + m1->x.y * m2->y.w + m1->x.z * m2->z.w + m1->x.w * m2->w.w
        },
        {
            m1->y.x * m2->x.x + m1->y.y * m2->y.x + m1->y.z * m2->z.x + m1->y.w * m2->w.x,
            m1->y.x * m2->x.y + m1->y.y * m2->y.y + m1->y.z * m2->z.y + m1->y.w * m2->w.y,
            m1->y.x * m2->x.z + m1->y.y * m2->y.z + m1->y.z * m2->z.z + m1->y.w * m2->w.z,
            m1->y.x * m2->x.w + m1->y.y * m2->y.w + m1->y.z * m2->z.w + m1->y.w * m2->w.w
        },
        {
            m1->z.x * m2->x.x + m1->z.y * m2->y.x + m1->z.z * m2->z.x + m1->z.w * m2->w.x,
            m1->z.x * m2->x.y + m1->z.y * m2->y.y + m1->z.z * m2->z.y + m1->z.w * m2->w.y,
            m1->z.x * m2->x.z + m1->z.y * m2->y.z + m1->z.z * m2->z.z + m1->z.w * m2->w.z,
            m1->z.x * m2->x.w + m1->z.y * m2->y.w + m1->z.z * m2->z.w + m1->z.w * m2->w.w
        },
        {
            m1->w.x * m2->x.x + m1->w.y * m2->y.x + m1->w.z * m2->z.x + m1->w.w * m2->w.x,
            m1->w.x * m2->x.y + m1->w.y * m2->y.y + m1->w.z * m2->z.y + m1->w.w * m2->w.y,
            m1->w.x * m2->x.z + m1->w.y * m2->y.z + m1->w.z * m2->z.z + m1->w.w * m2->w.z,
            m1->w.x * m2->x.w + m1->w.y * m2->y.w + m1->w.z * m2->z.w + m1->w.w * m2->w.w
        }
    };

    return product;
}


int main()
{
    xcb_connection_t* c = xcb_connect(NULL, NULL);
    xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(c)).data;
    xcb_drawable_t win = xcb_generate_id(c);

    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t values[] = {screen->black_pixel,  XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS};
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

    const char* const extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_XCB_SURFACE_EXTENSION_NAME};
    instance_info.ppEnabledExtensionNames = extensions;
    instance_info.enabledExtensionCount = 2;
    VkInstance instance;
    VkResult res;
    res = vkCreateInstance(&instance_info, NULL, &instance);
    assert(res == VK_SUCCESS);



    uint32_t gpus_count = 0;
    res = vkEnumeratePhysicalDevices(instance, &gpus_count, NULL);
    assert(res == VK_SUCCESS);
    assert(gpus_count == 1);
    VkPhysicalDevice* gpus = malloc(sizeof(VkPhysicalDevice) * gpus_count);
    res = vkEnumeratePhysicalDevices(instance, &gpus_count, gpus);
    assert(res == VK_SUCCESS);

    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceProperties gpu_properties;
    vkGetPhysicalDeviceMemoryProperties(gpus[0], &memory_properties);
    vkGetPhysicalDeviceProperties(gpus[0], &gpu_properties);

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpus[0], &queue_family_count, NULL);
    VkQueueFamilyProperties* queue_props = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpus[0], &queue_family_count, queue_props);

    VkXcbSurfaceCreateInfoKHR xcb_create_info = {};
    xcb_create_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    xcb_create_info.connection = c;
    xcb_create_info.window = win;

    VkSurfaceKHR surface;
    res = vkCreateXcbSurfaceKHR(instance, &xcb_create_info, NULL, &surface);
    assert(res == VK_SUCCESS);

    VkBool32* queue_present_support = malloc(queue_family_count * sizeof(VkBool32));
    for (uint32_t i = 0; i < queue_family_count; ++i)
    {
        VkResult rr = vkGetPhysicalDeviceSurfaceSupportKHR(gpus[0], i, surface, &queue_present_support[i]);
        assert(rr == VK_SUCCESS);
    }

    uint32_t graphics_queue_idx = -1;
    uint32_t present_queue_idx = -1;
    for (uint32_t i = 0; i < queue_family_count; ++i)
    {
        if (queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphics_queue_idx = i;

            if (queue_present_support[i] == VK_TRUE)
            {
                present_queue_idx = i;
                break;
            }
        }
    }

    assert(graphics_queue_idx != -1);

    if (present_queue_idx == -1)
    {
        for (uint32_t i = 0; i < queue_family_count; ++i)
        {
            if (queue_present_support[i] == VK_TRUE)
            {
                present_queue_idx = i;
                break;
            }
        }
    }

    assert(present_queue_idx != -1);
    free(queue_present_support);

    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.queueFamilyIndex = graphics_queue_idx;
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueCount = 1;
    float queue_priorities[] = {0.0};
    queue_info.pQueuePriorities = queue_priorities;

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    const char * const device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    device_info.ppEnabledExtensionNames = device_extensions;
    device_info.enabledExtensionCount = 1;

    VkDevice device;
    res = vkCreateDevice(gpus[0], &device_info, NULL, &device);
    assert(res == VK_SUCCESS);

    uint32_t supported_surface_formats_count;
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(gpus[0], surface, &supported_surface_formats_count, NULL);
    assert(res == VK_SUCCESS);
    VkSurfaceFormatKHR* supported_surface_formats = malloc(supported_surface_formats_count * sizeof(VkSurfaceFormatKHR));
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(gpus[0], surface, &supported_surface_formats_count, supported_surface_formats);
    assert(res == VK_SUCCESS);

    VkFormat format;
    if (supported_surface_formats_count == 1 && supported_surface_formats[0].format == VK_FORMAT_UNDEFINED)
    {
        format = VK_FORMAT_R8G8B8A8_UNORM;
    }
    else
    {
        assert(supported_surface_formats_count >= 1);
        format = supported_surface_formats[0].format;
    }
    free(supported_surface_formats);

    VkSurfaceCapabilitiesKHR surface_capabilities;
    res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpus[0], surface, &surface_capabilities);
    assert(res == VK_SUCCESS);

    VkExtent2D swapchain_extent = surface_capabilities.currentExtent; // check so this isn't 0xFFFFFFFF
    assert(swapchain_extent.width != 0xFFFFFFFF);

    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

    uint32_t desired_num_swapchain_images = surface_capabilities.minImageCount;

    VkSurfaceTransformFlagBitsKHR pre_transform;

    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    else
        pre_transform = surface_capabilities.currentTransform;

    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR composite_alpha_flags[4] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };

    for (uint32_t i = 0; i < sizeof(composite_alpha_flags)/sizeof(composite_alpha_flags[0]); ++i)
    {
        if (surface_capabilities.supportedCompositeAlpha & composite_alpha_flags[i])
        {
            composite_alpha = composite_alpha_flags[i];
            break;
        }
    }

    /*uint32_t present_mode_count;
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(gpus[0], surface, &present_mode_count, NULL);
    assert(res == VK_SUCCESS);
    VkPresentModeKHR* present_modes = malloc(present_mode_count * sizeof(VkPresentModeKHR));
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(gpus[0], surface, &present_mode_count, present_modes);
    assert(res == VK_SUCCESS);*/

    VkSwapchainCreateInfoKHR si = {};
    si.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    si.surface = surface;
    si.minImageCount = desired_num_swapchain_images;
    si.imageFormat = format;
    si.imageExtent = swapchain_extent;
    si.preTransform = pre_transform;
    si.compositeAlpha = composite_alpha;
    si.imageArrayLayers = 1;
    si.presentMode = swapchain_present_mode;
    si.oldSwapchain = VK_NULL_HANDLE;
    si.clipped = 1;
    si.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    si.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    si.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    uint32_t queue_family_indicies[] = {graphics_queue_idx, present_queue_idx};

    if (queue_family_indicies[0] != queue_family_indicies[1])
    {
        si.pQueueFamilyIndices = queue_family_indicies;
        si.queueFamilyIndexCount = 2;
        si.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    }

    VkSwapchainKHR swapchain;
    res = vkCreateSwapchainKHR(device, &si, NULL, &swapchain);
    assert(res == VK_SUCCESS);

    uint32_t swapchain_image_count;
    res = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, NULL);
    assert(swapchain_image_count > 0);
    assert(res == VK_SUCCESS);
    VkImage* swapchain_images = malloc(swapchain_image_count * sizeof(VkImage));
    res = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images);
    assert(res == VK_SUCCESS);

    uint32_t num_swapchain_buffers = swapchain_image_count;
    swapchain_buffer_t* swapchain_buffers = malloc(sizeof(swapchain_buffer_t) * num_swapchain_buffers);

    for (uint32_t i = 0; i < num_swapchain_buffers; ++i)
    {
        swapchain_buffers[i].image = swapchain_images[i];

        VkImageViewCreateInfo vci = {};
        vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image = swapchain_buffers[i].image;
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format = format;
        vci.components.r = VK_COMPONENT_SWIZZLE_R;
        vci.components.g = VK_COMPONENT_SWIZZLE_G;
        vci.components.b = VK_COMPONENT_SWIZZLE_B;
        vci.components.a = VK_COMPONENT_SWIZZLE_A;
        vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vci.subresourceRange.baseMipLevel = 0;
        vci.subresourceRange.levelCount = 1;
        vci.subresourceRange.baseArrayLayer = 0;
        vci.subresourceRange.layerCount = 1;

        res = vkCreateImageView(device, &vci, NULL, &swapchain_buffers[i].view);
        assert(res == VK_SUCCESS);
    }

    free(swapchain_images);

    VkCommandPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.queueFamilyIndex = graphics_queue_idx;

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

    VkImageCreateInfo depth_ici = {};
    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    VkFormatProperties depth_format_props;
    vkGetPhysicalDeviceFormatProperties(gpus[0], depth_format, &depth_format_props);
    if (depth_format_props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        depth_ici.tiling = VK_IMAGE_TILING_LINEAR;
    else if (depth_format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        depth_ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    else
        assert(0 && "VK_FORMAT_D16_UNORM unsupported");

    depth_ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_ici.imageType = VK_IMAGE_TYPE_2D;
    depth_ici.format = depth_format;
    depth_ici.extent.width = swapchain_extent.width;
    depth_ici.extent.height = swapchain_extent.height;
    depth_ici.extent.depth = 1;
    depth_ici.mipLevels = 1;
    depth_ici.arrayLayers = 1;
    
    #define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT

    depth_ici.samples = NUM_SAMPLES;
    depth_ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_ici.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth_ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkMemoryAllocateInfo depth_mem_alloc = {};
    depth_mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    VkImageViewCreateInfo depth_ivci = {};
    depth_ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_ivci.image = VK_NULL_HANDLE;
    depth_ivci.format = depth_format;
    depth_ivci.components.r = VK_COMPONENT_SWIZZLE_R;
    depth_ivci.components.g = VK_COMPONENT_SWIZZLE_G;
    depth_ivci.components.b = VK_COMPONENT_SWIZZLE_B;
    depth_ivci.components.a = VK_COMPONENT_SWIZZLE_A;
    depth_ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_ivci.subresourceRange.levelCount = 1;
    depth_ivci.subresourceRange.layerCount = 1;
    depth_ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depth_ivci.flags = 0;

    VkMemoryRequirements depth_mem_reqs;

    VkImage depth_image;
    res = vkCreateImage(device, &depth_ici, NULL, &depth_image);
    assert(res == VK_SUCCESS);

    vkGetImageMemoryRequirements(device, depth_image, &depth_mem_reqs);
    depth_mem_alloc.allocationSize = depth_mem_reqs.size;

    depth_mem_alloc.memoryTypeIndex = -1;
    uint32_t mem_type_bits = depth_mem_reqs.memoryTypeBits;
    VkMemoryPropertyFlags mem_req_mask = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        if ((mem_type_bits & 1) == 1)
        {
            if ((memory_properties.memoryTypes[i].propertyFlags & mem_req_mask) == mem_req_mask)
            {
                depth_mem_alloc.memoryTypeIndex = i;
                break;
            }
        }

        mem_type_bits >>= 1;
    }

    assert(depth_mem_alloc.memoryTypeIndex != -1);
    VkDeviceMemory depth_mem;
    res = vkAllocateMemory(device, &depth_mem_alloc, NULL, &depth_mem);
    assert(res == VK_SUCCESS);
    res = vkBindImageMemory(device, depth_image, depth_mem, 0);
    assert(res == VK_SUCCESS);
    depth_ivci.image = depth_image;
    VkImageView depth_view;
    res = vkCreateImageView(device, &depth_ivci, NULL, &depth_view);
    assert(res == VK_SUCCESS);

    mat4_t proj_matrix = create_projection_matrix((float)swapchain_extent.width, (float)swapchain_extent.height);
    mat4_t view_matrix = {
        {1, 0, 0, 0},
        {0, 1, 0, -2},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };

    mat4_t model_matrix = mat4_identity();
    mat4_t proj_view_matrix = mat4_mul(&proj_matrix, &view_matrix);
    mat4_t mvp_matrix = mat4_mul(&proj_view_matrix, &model_matrix);

    VkBufferCreateInfo uniform_ci = {};
    uniform_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uniform_ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    uniform_ci.size = sizeof(mvp_matrix);
    uniform_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer uniform_buffer;
    res = vkCreateBuffer(device, &uniform_ci, NULL, &uniform_buffer);
    assert(res == VK_SUCCESS);

    xcb_generic_event_t* evt;
    uint32_t run = 1;
    while (run && (evt = xcb_wait_for_event(c)))
    {
        switch(evt->response_type & ~0x80)
        {
            case XCB_KEY_PRESS: {
                if (((xcb_key_press_event_t*)evt)->detail == 9)
                    run = 0;
            } break;
        }
        free(evt);
    }

    vkDestroyImageView(device, depth_view, NULL);
    vkDestroyImage(device, depth_image, NULL);
    vkFreeMemory(device, depth_mem, NULL);
    VkCommandBuffer bufs_to_free[1] = {cmd};
    vkFreeCommandBuffers(device, cmd_pool, 1, bufs_to_free);
    vkDestroyCommandPool(device, cmd_pool, NULL);
    for (uint32_t i = 0; i < swapchain_image_count; i++) {
        vkDestroyImageView(device, swapchain_buffers[i].view, NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDevice(device, NULL);
    free(gpus);
    free(queue_props);
    vkDestroyInstance(instance, NULL);
    xcb_disconnect(c);

    return 0;
}
