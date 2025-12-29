/* 
    rag_vk - v1.0.0 - MIT license - https://github.com/satchelfrost/rag_vk

    A C99 stb-style header-only library for Vulkan.

Usage:

    #define RAG_VK_IMPLEMENTATION
    #include "rag_vk.h"

    int main()
    {
        // TODO:
    }

rag_* vs vk_*?:

    rag_*:

        the functions beginning with "rag_" are the lazy non-explicit functions for fast prototyping. Their
        implementation calls upon the "vk_*" functions. This also means that one way to understand "vk_*" is to
        look at how rag_* uses them.

    vk_*:

        these are the explicit thin wrapper functions over Vulkan. They mostly work how you would expect since they
        mimic the Vulkan-proper functions, the difference is that they have some (overridable) default behavior.


The basic idea behind the vk_* functions:

    These functions work by having overridable defaults.
    For example, listed below are several valid ways to create a VkInstance:

        1) vk_create_instance(NULL, &inst);
        2) vk_create_instance(&allocator, &inst);
        3) vk_create_instance(NULL, &inst, .pApplicationInfo = &app_info);
        4) vk_create_instance(NULL, &inst, .ppEnabledLayerNames = layers);
        5) vk_create_instance(NULL, &inst, .ppEnabledLayerNames = layers, .enabledLayerCount = 2);

    Here 1) assumes a default application info, while 2) assumes that but also uses an allocator. 3) is explicit
    about the app info. 4) is explicit about the enabled layer names, but assumes an "enabledLayerCount" of 1,
    while 5) is explicit about the count. Note that the optional parameters are "camelCase" to make copy-pasting
    easier.

    The general workflow goes something like this:

        1) I want to use a Vulkan function, so find the "vk_*" counterpart
        2) look at the struct create info that I will need
        3) only pass in the parts I care about (e.g. ".pApplicationInfo = blah")

A few things to watch out for:

    1) If the Vulkan-proper function takes multiple structs as arguments,
       then the `vk_*` counterpart takes those structs as the optional arguments, not their members.
       For example, `vk_create_instance` takes the MEMBERS of the VkInstanceCreateInfo as optional arguments,
       while `vk_create_graphics_pipelines` takes several STRUCTS as the optional arguments.
    2) If you are using the default Vulkan context (e.g. rag_init_default_ctx()) and also manually creating
       anything in that context (e.g. VkInstance), then keep in mind there are now TWO of those things.
       See Example 3 below.


Example default cases:

    Example 1 - No need to set `.sType`:

        vk_create_instance(NULL, &inst);

        is equivalent to,

        vk_create_instance(NULL, &inst, .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);

        Here we don't need to specify the sType because it's implied, so vk_create_instance just sets it internally.

    Example 2 - optional array parameters assume a default count of 1:

        These two function calls are identical:

            vk_create_instance(NULL, &inst, .ppEnabledLayerNames = &names, .enabledLayerCount = 1);
            vk_create_instance(NULL, &instance, .ppEnabledLayerNames = &names);

        If `.ppEnabledLayerNames` is not NULL, vk_create_instance will assume an enabledLayerCount of 1.
        If the count should not be 1, then you must explicity set it e.g. `.enabledLayerCount = X` etc.

    Example 3 - a default Vulkan context can be used as a fallback.

        Suppose we want to bind to a descriptor set using a specific command buffer, then we do the following:

            vk_cmd_bind_descriptor_sets(pl_layout, &set, .commandBuffer = cmd_buff);

        If we don't provide the command buffer explicity, then the function will fail UNLESS we have
        previously called `rag_init_default_ctx()`, in which case a default command buffer was allocated:

            vk_cmd_bind_descriptor_sets(pl_layout, &set); <--- assumes default command buffer

        The internal logic goes something like this:

            VkCommandBuffer final_cmd_buff = VK_NULL_HANDLE;
            if (optional_arg.commandBuffer)    final_cmd_buff = optional_arg.commandBuffer;
            else if (default_context.cmd_buff) final_cmd_buff = default_context.cmd_buff;
            else                               RAG_ASSERT(0 && "No command buffer specified");

        Just be careful when using the default context AND manually specifying things in that context
        because you will have duplicates.

*/

#ifndef RAG_VK_H_
#define RAG_VK_H_

#define RAG_ASSERT assert
#define RAG_REALLOC realloc
#define RAG_FREE free

#ifndef RAG_EXIT_APP
#define RAG_EXIT_APP
#endif

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

#ifdef PLATFORM_DESKTOP_GLFW
#include <GLFW/glfw3.h>
#endif

#ifdef PLATFORM_ANDROID
#include <android_native_app_glue.h>
#include <android/log.h>
#endif // PLATFORM_ANDROID

#ifndef APP_NAME
    #define APP_NAME "app"
#endif
#ifndef RAG_VK_VALIDATION_LOG_LEVEL
    #define RAG_VK_VALIDATION_LOG_LEVEL RAG_WARNING
#endif

#define RAG_LOAD_PFN(pfn) PFN_ ## pfn pfn = (PFN_ ## pfn) vkGetInstanceProcAddr(vk_ctx.instance, #pfn)
#define RAG_SUCCEEDED(x) ((x) == VK_SUCCESS)
#define CLAMP(val, min, max) ((val) < (min)) ? (min) : (((val) > (max)) ? (max) : (val))
#define RAG_ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))

#define RAG_MAX_SWAPCHAIN_IMAGES 5
typedef struct {
    VkSwapchainKHR handle;
    VkImage imgs[RAG_MAX_SWAPCHAIN_IMAGES];
    VkImageView img_views[RAG_MAX_SWAPCHAIN_IMAGES];
    VkFramebuffer frame_buffs[RAG_MAX_SWAPCHAIN_IMAGES];
    uint32_t img_count;
    bool resized;
    VkExtent2D extent;
} Rag_Swapchain;

typedef struct {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkDebugReportCallbackEXT report_callback;
    // VkPhysicalDevice physical_device;
    // VkDevice device;
    // uint32_t queue_idx;
    // VkQueue queue;
    // VkCommandPool cmd_pool;
    // VkCommandBuffer cmd_buff;
    // VkSemaphore image_available_semaphore;
    // VkSemaphore render_finished_semaphore;
    // VkFence fence;
    // VkSurfaceKHR surface;
    // VkSurfaceFormatKHR surface_format;
    // VkExtent2D extent;
    // VkRenderPass render_pass;
    // Rag_Swapchain swapchain;
    // Rag_Image depth_img;
    // VkImageView depth_img_view;
} Rag_Context;

VkDebugUtilsMessengerCreateInfoEXT rag_get_debug_messenger_info();

/* logging and error handling */
typedef enum { RAG_INFO, RAG_WARNING, RAG_ERROR, } Rag_Log_Level;
void rag_log(Rag_Log_Level level, const char *fmt, ...);
const char *rag_vk_res_to_str(VkResult res);
bool rag_check_vk_result(VkResult result, const char* function);
#define RAG_VK(func) rag_check_vk_result(func, #func);

#define vk_create_instance(pAllocator, pInstance, ...) vk_create_instance_(pAllocator, pInstance, (VkInstanceCreateInfo){__VA_ARGS__})
bool vk_create_instance_(const VkAllocationCallbacks *pAllocator, VkInstance* pInstance, VkInstanceCreateInfo optional);

#endif // RAG_VK_H_

/***********************************************************************************
*
*   rag_vk Implementation
*
************************************************************************************/

#ifdef RAG_VK_IMPLEMENTATION

static Rag_Context rag_ctx = {0};

bool rag_check_vk_result(VkResult result, const char* function)
{
    if (!RAG_SUCCEEDED(result)) {
        rag_log(RAG_ERROR, "Vulkan Error: %s : %s", function, rag_vk_res_to_str(result));
        return false;
    }
    return true;
}

void rag_log(Rag_Log_Level level, const char *fmt, ...)
{
#if defined(PLATFORM_ANDROID)
    va_list args;
    va_start(args, fmt);
    switch (level) {
    case RAG_INFO:
         __android_log_vprint(ANDROID_LOG_INFO,  APP_NAME, fmt, args);
        break;
    case RAG_WARNING:
         __android_log_vprint(ANDROID_LOG_WARN,  APP_NAME, fmt, args);
        break;
    case RAG_ERROR:
         __android_log_vprint(ANDROID_LOG_ERROR,  APP_NAME, fmt, args);
        break;
    }
#else
    switch (level) {
    case RAG_INFO:
        fprintf(stderr, "[RAG][INFO] ");
        break;
    case RAG_WARNING:
        fprintf(stderr, "[RAG][WARNING] ");
        break;
    case RAG_ERROR:
        fprintf(stderr, "[RAG][ERROR] ");
        break;
    default:
        RAG_EXIT_APP;
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
#endif // end of platform defines
}

const char *rag_vk_res_to_str(VkResult res)
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

bool rag_instance_layers_supported(const char **requested_layers, uint32_t requested_layer_count)
{
    uint32_t available_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&available_layer_count, NULL);
    VkLayerProperties available_layers[available_layer_count];
    vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers);

    for (size_t i = 0; i < requested_layer_count; i++) {
        bool found = false;
        for (size_t j = 0; j < available_layer_count; j++) {
            if (strcmp(requested_layers[i], available_layers[j].layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            rag_log(RAG_ERROR, "validation layer `%s` not available", requested_layers[i]);
            return false;
        }
    }
}

bool vk_create_instance_(const VkAllocationCallbacks *pAllocator, VkInstance* pInstance, VkInstanceCreateInfo optional)
{
    // rag_ctx.using_validation = rag_validation_supported();

    VkInstanceCreateInfo instance_ci = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

    VkApplicationInfo default_app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = APP_NAME,
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "rag_vk",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_API_VERSION_1_3,
    };

    instance_ci.pApplicationInfo = (optional.pApplicationInfo) ? optional.pApplicationInfo : &default_app_info;
    instance_ci.pNext = optional.pNext;
    instance_ci.ppEnabledLayerNames = optional.ppEnabledLayerNames;
    instance_ci.enabledLayerCount = optional.enabledLayerCount;
    instance_ci.ppEnabledExtensionNames = optional.ppEnabledExtensionNames;
    instance_ci.enabledExtensionCount = optional.enabledExtensionCount;

    if (optional.ppEnabledLayerNames && !instance_ci.enabledLayerCount)
        instance_ci.enabledLayerCount = 1;
    if (optional.ppEnabledExtensionNames && !instance_ci.enabledExtensionCount)
        instance_ci.enabledExtensionCount = 1;

    // if (!rvk_inst_exts_satisfied()) return false;

    return RAG_VK(vkCreateInstance(&instance_ci, pAllocator, pInstance));
}

static VKAPI_ATTR VkBool32 VKAPI_CALL rag_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
    VkDebugUtilsMessageTypeFlagsEXT msg_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data)
{
    (void)msg_type;
    (void)p_user_data;

    Rag_Log_Level log_lvl = RAG_INFO;

    switch (msg_severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: log_lvl = RAG_INFO;    break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:    log_lvl = RAG_INFO;    break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: log_lvl = RAG_WARNING; break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:   log_lvl = RAG_ERROR;   break;
    default: return VK_FALSE;
    }

    if (log_lvl < RAG_VK_VALIDATION_LOG_LEVEL) return VK_FALSE;

    rag_log(log_lvl, "%s", p_callback_data->pMessage);

    return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT rag_get_debug_messenger_info()
{
    return (VkDebugUtilsMessengerCreateInfoEXT) {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = 0x1110, // error, warning, info
        .messageType = 0x7, // general, validation, performance
        .pfnUserCallback = rag_debug_callback,
    };
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
