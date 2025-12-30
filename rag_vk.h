/* 
    rvk - v1.0.0 - MIT license - https://github.com/satchelfrost/rvk

    A C99 stb-style header-only library for Vulkan.

Usage:

    #define RVK_IMPLEMENTATION
    #include "rvk.h"

    int main()
    {
        // TODO:
    }

API:
    r_*:

        the functions beginning with "r_" are helper functions.

    vk_*:

        Thin wrapper macros which allow optional arguments for Vulkan functions e.g.:

            vk_create_instance(NULL, &inst);
            vk_create_instance(&allocator, &inst);
            vk_create_instance(NULL, &inst, .pApplicationInfo = &app_info);
            vk_create_instance(NULL, &inst, .ppEnabledLayerNames = layers, .enabledLayerCount = 1);

        are all valid ways to create a vulkan instance.

        Note that if the Vulkan-proper function takes multiple structs as arguments,
        then the `vk_*` counterpart takes those structs as the optional arguments, not their members.
        For example, `vk_create_instance` takes the MEMBERS of the VkInstanceCreateInfo as optional arguments,
        while `vk_create_graphics_pipelines` takes several STRUCTS as the optional arguments.

        sTypes do not need to be specified because they are set internally e.g.:

            vk_create_instance(NULL, &inst);

        is the same as:

            vk_create_instance(NULL, &inst, .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);

    RVK/Rvk:

        defines/macros are prefixed with RVK_, while custom types are prefixed with Rvk_

*/

#ifndef RVK_H_
#define RVK_H_

#define RVK_ASSERT assert
#define RVK_REALLOC realloc
#define RVK_FREE free

#ifndef RVK_EXIT_APP
#define RVK_EXIT_APP
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

// #ifdef PLATFORM_DESKTOP_GLFW
// #include <GLFW/glfw3.h>
// #endif
//
// #ifdef PLATFORM_ANDROID
// #include <android_native_app_glue.h>
// #include <android/log.h>
// #endif

#ifndef APP_NAME
    #define APP_NAME "app"
#endif

#ifndef RVK_LOG_LEVEL
    #define RVK_LOG_LEVEL RVK_INFO
#endif

#define RVK_LOAD_PFN(pfn) PFN_ ## pfn pfn = (PFN_ ## pfn) vkGetInstanceProcAddr(instance, #pfn)
#define RVK_SUCCEEDED(x) ((x) == VK_SUCCESS)
#define CLAMP(val, min, max) ((val) < (min)) ? (min) : (((val) > (max)) ? (max) : (val))
#define RVK_ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))

/***********************************************************************************
*
*  r_* API - Convenience functions
*
************************************************************************************/


/* logging and error handling */
typedef enum { RVK_VERBOSE, RVK_INFO, RVK_WARNING, RVK_ERROR, } Rvk_Log_Level;
void r_log(Rvk_Log_Level level, const char *fmt, ...);
const char *r_vk_res_to_str(VkResult res);
bool r_check_vk_result(VkResult result, const char* function);

#define RVK(func) r_check_vk_result(func, #func);

// TODO: ^^ semi-colon?

VkDebugUtilsMessengerCreateInfoEXT r_get_debug_messenger_info();
bool r_setup_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT *debug_messenger);

bool r_instance_layers_supported(const char **requested_layers, uint32_t requested_layer_count);
bool r_instance_extensions_supported(const char **requested_extensions, uint32_t requested_extension_count);
VkPhysicalDevice r_pick_physical_device(VkInstance instance);

/***********************************************************************************
*
*  vk_* API - Thin wrapper over Vulkan
*
************************************************************************************/
#define vk_create_instance(pAllocator, pInstance, ...) vk_create_instance_(pAllocator, pInstance, (VkInstanceCreateInfo){__VA_ARGS__})
bool vk_create_instance_(const VkAllocationCallbacks *pAllocator, VkInstance* pInstance, VkInstanceCreateInfo optional);

#endif // RVK_H_

/***********************************************************************************
*
*   r_vk Implementation
*
************************************************************************************/

#ifdef RVK_IMPLEMENTATION

bool r_check_vk_result(VkResult result, const char* function)
{
    if (!RVK_SUCCEEDED(result)) {
        r_log(RVK_ERROR, "Vulkan Error: %s : %s", function, r_vk_res_to_str(result));
        return false;
    }
    return true;
}

void r_log(Rvk_Log_Level level, const char *fmt, ...)
{
    if (level < RVK_LOG_LEVEL) return;
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

const char *r_vk_res_to_str(VkResult res)
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

bool r_instance_layers_supported(const char **requested_layers, uint32_t requested_layer_count)
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
            r_log(RVK_ERROR, "validation layer `%s` not available", requested_layers[i]);
            return false;
        }
    }
}

bool r_instance_extensions_supported(const char **requested_extensions, uint32_t requested_extension_count)
{
    uint32_t available_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&available_layer_count, NULL);
    VkLayerProperties available_layers[available_layer_count];
    vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers);

    for (size_t i = 0; i < requested_extension_count; i++) {
        bool found = false;
        for (size_t j = 0; j < available_layer_count; j++) {
            if (strcmp(requested_extensions[i], available_layers[j].layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            r_log(RVK_ERROR, "validation layer `%s` not available", requested_extensions[i]);
            return false;
        }
    }
}

bool vk_create_instance_(const VkAllocationCallbacks *pAllocator, VkInstance* pInstance, VkInstanceCreateInfo optional)
{
    VkInstanceCreateInfo instance_ci = optional;
    instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    VkApplicationInfo app_info = {0};
    if (optional.pApplicationInfo)
        app_info = *optional.pApplicationInfo;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    instance_ci.pApplicationInfo = (optional.pApplicationInfo) ? &app_info : NULL;

    return RVK(vkCreateInstance(&instance_ci, pAllocator, pInstance));
}

static VKAPI_ATTR VkBool32 VKAPI_CALL r_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
    VkDebugUtilsMessageTypeFlagsEXT msg_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data)
{
    (void)msg_type;
    (void)p_user_data;

    Rvk_Log_Level log_lvl = RVK_INFO;

    switch (msg_severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        log_lvl = RVK_VERBOSE;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        log_lvl = RVK_VERBOSE; // not a mistake, but their idea of info is incredibly verbose
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        log_lvl = RVK_WARNING;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        log_lvl = RVK_ERROR;
        break;
    default: return VK_FALSE;
    }

    if (log_lvl < RVK_LOG_LEVEL) return VK_FALSE;

    r_log(log_lvl, "%s", p_callback_data->pMessage);

    return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT r_get_debug_messenger_info()
{
    return (VkDebugUtilsMessengerCreateInfoEXT) {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = 0x1110, // error, warning, info
        .messageType = 0x7, // general, validation, performance
        .pfnUserCallback = r_debug_callback,
    };
}

bool r_setup_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT *debug_messenger)
{
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_ci = r_get_debug_messenger_info();
    RVK_LOAD_PFN(vkCreateDebugUtilsMessengerEXT);
    if (vkCreateDebugUtilsMessengerEXT) {
        return RVK(vkCreateDebugUtilsMessengerEXT(instance, &debug_messenger_ci, NULL, debug_messenger));
    } else {
        r_log(RVK_ERROR, "failed to load function pointer for vkCreateDebugUtilesMessenger");
        return false;
    }
}

VkPhysicalDevice r_pick_physical_device(VkInstance instance)
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    VkPhysicalDevice devices[device_count];
    vkEnumeratePhysicalDevices(instance, &device_count, devices);
    int rankings[] = {
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
        VK_PHYSICAL_DEVICE_TYPE_CPU,
    };
    for (size_t i = 0; i < RVK_ARRAY_LEN(rankings); i++) {
        for (size_t j = 0; j < device_count; j++) {
            VkPhysicalDeviceProperties props = {0};
            vkGetPhysicalDeviceProperties(devices[j], &props);
            if (props.deviceType == rankings[i]) {
                r_log(RVK_INFO, "GPU selected: %s", props.deviceName);
                return devices[i];
            }
        }
    }

    return VK_NULL_HANDLE;
}


#endif // RVK_IMPLEMENTATION

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
