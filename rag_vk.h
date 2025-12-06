/* 
    rag_vk - v1.0.0 - MIT license - https://github.com/satchelfrost/rag_vk

    **WARNING** Probably don't use this, especially not v0.0.0

    This file is an attempt to have a vulkan stb-style header-only library.
    Currently, the header-only goal isn't being met very well. However, this
    file is still being used in multiple projects, and the versions are starting to diverge.
    Therefore, I've collected it into a repo to have a single source of authority,
    and track different versions.

usage:

    #define RAG_VK_IMPLEMENTATION
    #include "rag_vk.h"

    int main()
    {
        // TODO:
    }

Two main APIs:
    +============+====================================================================+
    | API prefix | basic description                                                  |
    +============+====================================================================+
    | rag_vk_*   | Lazy helper functions that assume a lot.                           |
    |            | Convenient, but not explicit.                                      |
    +------------+--------------------------------------------------------------------+
    | rvk_*      | Thin wrapper over vulkan functions.                                |
    |            | Explicit, but with some default behavior which                     |
    |            | can always be overridden to be more explicit.                      |
    |            |                                                                    |  
    |            | Adheres to the following conventions:                              |
    |            |     1) Name is similar to vulkan proper function:                  |
    |            |        vkCreateInstance --> rvk_create_instance                    |  
    |            |                                                                    |  
    |            |     2) Do not require "sType" fields to be set                     |
    |            |                                                                    |  
    |            |     3) fields of associated Vk*CreateInfos are passed in LAST as   |
    |            |        optional "." parameters.                                    |
    |            |        Examples:                                                   |  
    |            |                                                                    |  
    |            |        rvk_instance(NULL, &inst);                                  |
    |            |        rvk_instance(&allocator, &inst);                            |
    |            |        rvk_instance(NULL, &inst, .pApplicationInfo = &app_info);   |
    |            |        rvk_instance(NULL, &inst, .ppEnabledLayerNames = &names);   |
    |            |                                                                    |
    |            |        Parameters beginning with a "." are optional, and named     |
    |            |        the exact same (case and all) as their vulkan counterpart   |
    |            |                                                                    |
    |            |     4) If no optional parameters are used then defaults are        |
    |            |        assumed when possible. Be careful.                          |
    |            |                                                                    |
    |            |     5) optional parameters that go together with an array count    |
    |            |        can assume a count of 1 if the pointer to array is set      |
    |            |        For example this:                                           |
    |            |                                                                    |  
    |            |        rvk_instance(                                               |
    |            |           NULL,                                                    |
    |            |           &instance,                                               |
    |            |           .ppEnabledLayerNames = &names,                           |
    |            |           .enabledLayerCount = 1,                                  |
    |            |        );                                                          |
    |            |                                                                    |  
    |            |        is equivalent to this:                                      |  
    |            |                                                                    |  
    |            |        rvk_instance(                                               |
    |            |           NULL,                                                    |
    |            |           &instance,                                               |
    |            |           .ppEnabledLayerNames = &names,                           |
    |            |        );                                                          |
    |            |                                                                    |  
    |            |        if it's not 1, then set it accordingly                      |  
    |            |                                                                    |  
    |            |     6) when vulkan handles are optional parameters, and also not   |  
    |            |        set, then the default lazy context is used. For example,    |
    |            |        rvk_cmd_draw expects a command buffer, if one is not        |
    |            |        explicity passed in then the default lazy command buffer    |
    |            |        is used (if possible). PLEASE NOTE this ONLY works when     |
    |            |        the lazy vulkan contex was initialized i.e.                 |
    |            |        rag_vk_init_lazy_ctx_init()                                 |
    |            |                                                                    |
    +------------+--------------------------------------------------------------------+
    vkCmdDraw

This file strictly adheres to conventions for your benefit,

*/

/*
 * Goals for v1.0.0
 * 1) don't add deprecated functions
 * 2) Fat_Vk*CreateInfo macro in use
 * 3) thin vulkan functions should have no dependencies on global context
 * 4) For thin vulkan functions if certain parameters are needed and not passed in, then
 *    a default one may be used. For example, vkCmdBeginRenderPass requires a command buffer
 *    so for the thin vulkan wrapper vk_cmd_begin_render_pass, if a command buffer is not
 *    passed in, then we should assume a default.
 *
 * */

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
#define RVK_LOAD_PFN(pfn) PFN_ ## pfn pfn = (PFN_ ## pfn) vkGetInstanceProcAddr(rvk_ctx.instance, #pfn)
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
void rvk_log(Rvk_Log_Level level, const char *fmt, ...);
const char *rvk_res_to_str(VkResult res);
bool rvk_handle_bad_vk_result(VkResult result, const char* function);
#define RAG_VK(func) rvk_handle_bad_vk_result(func, #func);

/* Basic API */
bool rvk_lazy_vulkan_init(uint32_t width, uint32_t height, VkSurfaceKHR surface);

/* Thin Vulkan wrapper API */
bool rvk_create_instance(VkInstance *instance);

/* callback that waits for frame buffer to resize and sets the width and height parameters on completion */
typedef void (*rvk_glfw_wait_resize_frame_buffer)(uint32_t *width, uint32_t *height);

#endif // RAG_VK_H_

/***********************************************************************************
*
*   rag_vk Implementation
*
************************************************************************************/

#ifdef RAG_VK_IMPLEMENTATION

static Rvk_Context rvk_ctx = {0};

bool rvk_check_result(VkResult result, const char* function)
{
    if (!RVK_SUCCEEDED(result)) {
        rvk_log(RVK_ERROR, "Vulkan Error: %s : %s", function, rvk_res_to_str(result));
        return false;
    }
    return true;
}

void rvk_log(Rvk_Log_Level level, const char *fmt, ...)
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

bool rvk_lazy_vulkan_init(uint32_t width, uint32_t height)
{
    return true;
}

const char *rvk_res_to_str(VkResult res)
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
