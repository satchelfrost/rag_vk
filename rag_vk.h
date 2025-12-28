/* 
    rag_vk - v1.0.0 - MIT license - https://github.com/satchelfrost/rvk

    A C99 stb-style header-only library for Vulkan.

Usage:

    #define RVK_IMPLEMENTATION
    #include "rvk.h"

    int main()
    {
        // TODO:
    }


The basic idea behind`rvk`:

    Too simple of an API, and you can no longer do complicated things;
    too complicated of an API, and doing simple things becomes complicated.
    `rvk` attempts to solve this by having overridable defaults.
    For example, listed are several ways to create an instance:

        1) vk_create_instance(NULL, &inst);
        2) vk_create_instance(&allocator, &inst);
        3) vk_create_instance(NULL, &inst, .pApplicationInfo = &app_info);
        4) vk_create_instance(NULL, &inst, .pApplicationInfo = &app_info, .ppEnabledLayerNames = layers);
        5) vk_create_instance(NULL, &inst, .pApplicationInfo = &app_info, .ppEnabledLayerNames = layers,
                              .enabledLayerCount = layer_count);

    Here 1) assumes a default application info, while 2) assumes that but also uses an allocator. 3) is explicit
    about the app info. 4) is explicit about the enabled layer names, but assumes a count of 1 while 5) is explicit
    about the layer count.

    Now you have gist of how `rvk` works. The rest is understanding what the default behavior is, and when you
    want to get rid of it.

A few things to watch out for:

    1) Optional "." parameters are "camelCase" for ease of struct member copy-pasting.
    2) If the Vulkan-proper function takes multiple structs as arguments,
       then the `vk_*` counterpart takes those structs as the optional arguments, not their members.
    3) Defaults might cause issues if you are not expecting the default behavior.
       To help you avoid confusion, some below examples are given.

Example default cases:

    Example 1 - No need to set `.sType`:

        vk_create_instance(NULL, &inst);

        is equivalent to,

        vk_create_instance(NULL, &inst, .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);

        Here we don't need to specify the sType because it's implied, so vk_create_instance just sets it internally.

    Example 2 - Unless you disable it, a default Vulkan context MAY get allocated for you:

        vk_cmd_bind_descriptor_sets(pl_layout, &set); <--- assumes default command buffer

        whereas,

        vk_cmd_bind_descriptor_sets(pl_layout, &set, .commandBuffer = cmd_buff); <--- explicit about command buffer

        This means that `vk_cmd_bind_descriptor_sets` checks if the command buffer is NULL, and if it is,
        allocates a default vulkan context, which you can clean up later with `rvk_cleanup_default_ctx()`.

        To disable this feature (for full control), you can define the following:

            #define RVK_NO_DEFAULT_CONTEXT
            #include "rag_vk.h"

        This means calling `vk_cmd_bind_descriptor_sets` without explicity passing a command buffer will assert
        on a NULL vulkan handle. RAG_VK_ASSERT can also be redefined to do nothing, so in that case
        you will likely segfault if the handle is NULL.

    Example 3 - optional array parameters assume a default count of 1:

        vk_create_instance(NULL, &inst, .ppEnabledLayerNames = &names, .enabledLayerCount = 1);

        is the same as,

        vk_create_instance(NULL, &instance, .ppEnabledLayerNames = &names);

        If `.ppEnabledLayerNames` is not NULL, vk_create_instance will assume an enabledLayerCount of 1.
        If the count should not be 1, then you must explicity set it e.g. `.enabledLayerCount = 2` etc.

*/

#ifndef RAG_VK_H_
#define RAG_VK_H_

#define RVK_ASSERT assert
#define RVK_REALLOC realloc
#define RVK_FREE free

#ifdef PLATFORM_DESKTOP_GLFW
#define RVK_EXIT_APP RVK_ASSERT(0)
#else
#define RVK_EXIT_APP
#endif

/* try to use vulkan validation layers by default,
 * though it's still possible that validation is unsupported */
#ifndef NO_VK_VALIDATION
#define VK_VALIDATION
#endif // NO_VK_VALIDATION

#include <stdint.h>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef PLATFORM_ANDROID
#include <android_native_app_glue.h>
#include <android/log.h>
#endif // PLATFORM_ANDROID

#ifndef APP_NAME
    #define APP_NAME "app"
#endif
#ifndef MIN_SEVERITY
    #define MIN_SEVERITY RVK_WARNING
#endif

#define VK_FLAGS_NONE 0
#define RVK_LOAD_PFN(pfn) PFN_ ## pfn pfn = (PFN_ ## pfn) vkGetInstanceProcAddr(vk_ctx.instance, #pfn)
#define RVK_SUCCEEDED(x) ((x) == VK_SUCCESS)
#define CLAMP(val, min, max) ((val) < (min)) ? (min) : (((val) > (max)) ? (max) : (val))
#define RVK_ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))

#define RVK_MAX_SWAPCHAIN_IMAGES 5
typedef struct {
    VkSwapchainKHR handle;
    VkImage imgs[RVK_MAX_SWAPCHAIN_IMAGES];
    VkImageView img_views[RVK_MAX_SWAPCHAIN_IMAGES];
    VkFramebuffer frame_buffs[RVK_MAX_SWAPCHAIN_IMAGES];
    uint32_t img_count;
    bool resized;
    VkExtent2D extent;
} Rvk_Swapchain;

typedef struct {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkDebugReportCallbackEXT report_callback;
    bool validation_supported;

    /* the lazy context will be used in cases where values
     * are not explicity passed into functions */ 
    struct {
        VkPhysicalDevice physical_device;
        // VkDevice device;
        // uint32_t queue_idx;
        // VkQueue queue;
        // VkCommandPool cmd_pool;
        // VkCommandBuffer cmd_buff;
        // VkSemaphore image_available_semaphore;
        // VkSemaphore render_finished_semaphore;
        // VkFence fence;
        VkSurfaceKHR surface;
        // VkSurfaceFormatKHR surface_format;
        // VkExtent2D extent;
        // VkRenderPass render_pass;
        Rvk_Swapchain swapchain;
        // Rvk_Image depth_img;
        // VkImageView depth_img_view;
    } lazy;

} Rvk_Context;

/* logging and error handling */
typedef enum { RVK_INFO, RVK_WARNING, RVK_ERROR, } Rvk_Log_Level;
void vk_log(Rvk_Log_Level level, const char *fmt, ...);
const char *vk_res_to_str(VkResult res);
bool vk_handle_bad_vk_result(VkResult result, const char* function);
#define RAG_VK(func) vk_handle_bad_vk_result(func, #func);

/* Basic API */
bool vk_lazy_vulkan_init(uint32_t width, uint32_t height, VkSurfaceKHR surface);

/* Thin Vulkan wrapper API */
bool vk_create_instance(VkInstance *instance);

/* callback that waits for frame buffer to resize and sets the width and height parameters on completion */
typedef void (*vk_glfw_wait_resize_frame_buffer)(uint32_t *width, uint32_t *height);

#endif // RAG_VK_H_

/***********************************************************************************
*
*   rag_vk Implementation
*
************************************************************************************/

#ifdef RAG_VK_IMPLEMENTATION

static Rvk_Context vk_ctx = {0};

bool vk_check_result(VkResult result, const char* function)
{
    if (!RVK_SUCCEEDED(result)) {
        vk_log(RVK_ERROR, "Vulkan Error: %s : %s", function, vk_res_to_str(result));
        return false;
    }
    return true;
}

void vk_log(Rvk_Log_Level level, const char *fmt, ...)
{
#if defined(PLATFORM_ANDROID)
    va_list args;
    va_start(args, fmt);
    switch (level) {
    case RVK_INFO:
         __android_log_vprint(ANDROID_LOG_INFO,  APP_NAME, fmt, args);
        break;
    case RVK_WARNING:
         __android_log_vprint(ANDROID_LOG_WARN,  APP_NAME, fmt, args);
        break;
    case RVK_ERROR:
         __android_log_vprint(ANDROID_LOG_ERROR,  APP_NAME, fmt, args);
        break;
    }
#else
    switch (level) {
    case RVK_INFO:
        fprintf(stderr, "[RVK][INFO] ");
        break;
    case RVK_WARNING:
        fprintf(stderr, "[RVK][WARNING] ");
        break;
    case RVK_ERROR:
        fprintf(stderr, "[RVK][ERROR] ");
        break;
    default:
        RVK_EXIT_APP;
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
#endif // end of platform defines
}

bool vk_lazy_vulkan_init(uint32_t width, uint32_t height)
{
    return true;
}

const char *vk_res_to_str(VkResult res)
{
    /* these aren't all of the results, but I don't feel like dealing with different vulkan versions */
    switch (res) {
    case VK_SUCCESS:                              return "VK_SUCCESS";
    case VK_NOT_READY:                            return "VK_NOT_READY";
    case VK_TIMEOUT:                              return "VK_TIMEOUT";
    case VK_EVENT_SET:                            return "VK_EVENT_SET";
    case VK_EVENT_RESET:                          return "VK_EVENT_RESET";
    case VK_INCOMPLETE:                           return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:             return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:           return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:          return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:                    return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:              return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:              return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:          return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:            return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:            return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:               return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:           return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:                return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_UNKNOWN:                        return "VK_ERROR_UNKNOWN";
    case VK_ERROR_OUT_OF_POOL_MEMORY:             return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:        return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_FRAGMENTATION:                  return "VK_ERROR_FRAGMENTATION";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case VK_PIPELINE_COMPILE_REQUIRED:            return "VK_PIPELINE_COMPILE_REQUIRED";
    case VK_ERROR_SURFACE_LOST_KHR:               return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:       return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:                       return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:                return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:       return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:          return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:              return "VK_ERROR_INVALID_SHADER_NV";
    default: return "unrecognized vkresult";
    }
}

#endif // RAG_VK_IMPLEMENTATION

/*
    Revision history:

    0.0.0 (2025-12-04) Highly experimental version.

*/

/*
   MIT License
   Copyright (c) 2025 Reese Gallagher
   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is furnished to do
   so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/
