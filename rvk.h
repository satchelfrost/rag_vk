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

API Conventions:
    r_*:

        The functions beginning with "r_" are helper/lazy functions.
        Meant for quick prototyping, but if you need to be more explicit then use vk_*.

    vk_*:

        Thin wrapper macros that allow optional arguments for Vulkan functions which take a Vk*CreateInfo.
        Aside from snake_case, there are only three main differences from the Vulkan-proper functions:

            1) They have optional "." arguments for Vk*CreateInfo struct members,
               e.g.

                   vk_create_instance(NULL, &inst, .ppEnabledLayerNames = layers, .enabledLayerCount = 1);
                                       ^      ^                 ^                         ^
                                       |      |                 |                         |
                                       +--+---+                 +------------+------------+
                                          |                                  |
                                  required arguments      optional arguments from VkInstanceCreateInfo;
                                  are the pAllocator     if not set explicitly, then they will be
                                (NULL is valid), and            implicitly zero initialized.
                                    the pInstance

               Except for Vk*CreateInfo members which must now go last, The order of the parameters are
               preserved from the Vulkan-proper function.

            2) Instead of returning a VkResult, they return true on VK_SUCCESS and false otherwise.

            3) The ".sType" field in Vk*CreateInfo structs do not need to be set
               e.g.

               vk_create_instance(
                   NULL,
                   &inst,
                   .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO <--- unnecessary, can be left out
                );

               ALSO,

               VkApplicationInfo app_info = {
                   .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, <--- unnecessary, can be left out
                   .pApplicationName = APP_NAME,
                   .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
                   .pEngineName = "Cool Vulkan Renderer",
                   .engineVersion = VK_MAKE_VERSION(0, 0, 1),
                   .apiVersion = VK_API_VERSION_1_3,
               };
               vk_create_instance(NULL, &inst, .pApplicationInfo = &app_info);

               ***WARNING***: the exception to this rule is a structure in a pNext chain (i.e. linked list).
    RVK_*:

        defines/macros are prefixed with all caps "RVK_".
        e.g.

        RVK_INFO <--- log level
        RVK_IMPLEMENTATION <--- #define

    Rvk_*:

        typedefed structures/enums are prefixed with "Rvk_"
        e.g.

        Rvk_Swapchain <--- custom structure for swapchain
        Rvk_Log_Level <--- the name of the enum for logging

*/

#ifndef RVK_H_
#define RVK_H_

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

#define RVK_MAX_SWAPCHAIN_IMAGES 5
typedef struct {
    VkSwapchainKHR handle;
    VkImage images[RVK_MAX_SWAPCHAIN_IMAGES];
    VkImageView image_views[RVK_MAX_SWAPCHAIN_IMAGES];
    VkFramebuffer framebuffers[RVK_MAX_SWAPCHAIN_IMAGES];
    uint32_t image_count;
    bool resized;
    VkSurfaceFormatKHR surface_format;
    VkExtent2D extent;
} Rvk_Swapchain;

/***********************************************************************************
*  r_* API declarations
************************************************************************************/

/* logging and error handling */
typedef enum { RVK_VERBOSE, RVK_INFO, RVK_WARNING, RVK_ERROR, } Rvk_Log_Level;
void r_log(Rvk_Log_Level level, const char *fmt, ...);
const char *r_vk_res_to_str(VkResult res);
bool r_check_vk_result(VkResult result, const char* function);
#define RVK(func) r_check_vk_result(func, #func)
VkDebugUtilsMessengerCreateInfoEXT r_get_debug_messenger_info();

bool r_instance_layers_supported(const char **requested_layers, uint32_t requested_layer_count);
bool r_instance_extensions_supported(const char **requested_extensions, uint32_t requested_extension_count);

/* tries to prefer discrete GPUs, set log level to RVK_INFO for more info */
VkPhysicalDevice r_pick_physical_device(VkInstance instance);

/* returns max unt32_t upon error, surface == NULL means we don't care about present support */
uint32_t r_find_queue_family(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkQueueFlags flags);

VkSurfaceFormatKHR r_choose_swapchain_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
uint32_t r_get_suggested_image_count(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
uint32_t r_get_current_transform(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/* unlikely, but possible to return extent thats not the same as width and height */
VkExtent2D r_suggest_swapchain_extent(VkPhysicalDevice physical_device, VkSurfaceKHR surface, int width, int height);

VkPresentModeKHR r_choose_present_mode(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/* create a basic render pass with color and depth attachments */
bool r_create_render_pass(VkDevice device, VkFormat depth_format, VkFormat color_format, VkRenderPass *render_pass);

/* create a basic swapchain */
bool r_create_rvk_swapchain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, int width, int height, Rvk_Swapchain *swapchain);

/* for depth ,you might try format = VK_FORMAT_D32_SFLOAT and flags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
 * for color you might try format = VK_FORMAT_R8G8B8A8_SRGB and flags =
 * VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
 * ...only suggestions */
bool r_create_2d_image(VkDevice device, VkFormat format, VkImageUsageFlags flags, VkExtent2D extent, VkImage *image);

/* returns max unt32_t upon error */
uint32_t r_find_memory_type_index(VkPhysicalDevice physical_device, uint32_t type, VkMemoryPropertyFlags properties);

bool r_allocate_and_bind_image_memory(VkPhysicalDevice physical_device, VkDevice device, VkImage image, VkDeviceMemory *memory);

/***********************************************************************************
*  vk_* API declarations
************************************************************************************/

#define vk_create_instance(pAllocator, pInstance, ...) vk_create_instance_(pAllocator, pInstance, (VkInstanceCreateInfo){__VA_ARGS__})
bool vk_create_instance_(const VkAllocationCallbacks *pAllocator, VkInstance* pInstance, VkInstanceCreateInfo ci);

#define vk_create_device(physical_device, pAllocator, pDevice, ...) vk_create_device_(physical_device, pAllocator, pDevice, (VkDeviceCreateInfo){__VA_ARGS__})
bool vk_create_device_(VkPhysicalDevice physical_device, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice, VkDeviceCreateInfo ci);

#define vk_create_swapchain_khr(device, pAllocator, pSwapchain, ...) vk_create_swapchain_khr_(device, pAllocator, pSwapchain, (VkSwapchainCreateInfoKHR){__VA_ARGS__})
bool vk_create_swapchain_khr_(VkDevice device, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain, VkSwapchainCreateInfoKHR ci);

#define vk_create_image_view(device, pAllocator, pView, ...) vk_create_image_view_(device, pAllocator, pView, (VkImageViewCreateInfo){__VA_ARGS__})
bool vk_create_image_view_(VkDevice device, const VkAllocationCallbacks *pAllocator, VkImageView *pView, VkImageViewCreateInfo ci);

#define vk_create_render_pass(device, pAllocator, pRenderPass, ...) vk_create_render_pass_(device, pAllocator, pRenderPass, (VkRenderPassCreateInfo){__VA_ARGS__})
bool vk_create_render_pass_(VkDevice device, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass, VkRenderPassCreateInfo ci);

#define vk_create_image(device, pAllocator, pImage, ...) vk_create_image_(device, pAllocator, pImage, (VkImageCreateInfo){__VA_ARGS__})
bool vk_create_image_(VkDevice device, const VkAllocationCallbacks *pAllocator, VkImage *pImage, VkImageCreateInfo ci);

#endif // RVK_H_

#ifdef RVK_IMPLEMENTATION

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
*  r_* API implementation
************************************************************************************/

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
                char *device_type = NULL;
                switch (props.deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: device_type = "Discrete"; break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: device_type = "Integrated"; break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU: device_type = "CPU"; break;
                default: device_type = "N/A";
                }
                r_log(RVK_INFO, "Selected %s-GPU: %s", device_type, props.deviceName);
                return devices[i];
            }
        }
    }

    return VK_NULL_HANDLE;
}

uint32_t r_find_queue_family(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkQueueFlags flags)
{
    uint32_t queue_fam_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_fam_count, NULL);
    VkQueueFamilyProperties queue_fam_props[queue_fam_count];
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_fam_count, queue_fam_props);
    for (uint32_t i = 0; i < queue_fam_count; i++) {
        bool flag_check = (queue_fam_props[i].queueFlags & flags) == flags;
        if (surface) {
            VkBool32 present_support = true;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
            if (flag_check && present_support) return i;
        } else {
            if (flag_check) return i;
        }
    }

    return -1;
}

void r_log_queue_properties(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    uint32_t queue_fam_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_fam_count, NULL);
    VkQueueFamilyProperties queue_fam_props[queue_fam_count];
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_fam_count, queue_fam_props);
    for (uint32_t i = 0; i < queue_fam_count; i++) {
        if (surface) {
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
            r_log(RVK_INFO, "queue %u, present support: %s", i, (present_support) ? "true" : "false");
        } else {
            r_log(RVK_INFO, "queue %u", i);
        }

        VkQueueFlags flags = queue_fam_props[i].queueFlags;
        if (flags & VK_QUEUE_GRAPHICS_BIT        ) r_log(RVK_INFO, "    VK_QUEUE_GRAPHICS_BIT");
        if (flags & VK_QUEUE_COMPUTE_BIT         ) r_log(RVK_INFO, "    VK_QUEUE_COMPUTE_BIT");
        if (flags & VK_QUEUE_TRANSFER_BIT        ) r_log(RVK_INFO, "    VK_QUEUE_TRANSFER_BIT");
        if (flags & VK_QUEUE_SPARSE_BINDING_BIT  ) r_log(RVK_INFO, "    VK_QUEUE_SPARSE_BINDING_BIT");
        if (flags & VK_QUEUE_PROTECTED_BIT       ) r_log(RVK_INFO, "    VK_QUEUE_PROTECTED_BIT");
        if (flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) r_log(RVK_INFO, "    VK_QUEUE_VIDEO_DECODE_BIT_KHR");
        if (flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) r_log(RVK_INFO, "    VK_QUEUE_VIDEO_ENCODE_BIT_KHR");
        if (flags & VK_QUEUE_OPTICAL_FLOW_BIT_NV ) r_log(RVK_INFO, "    VK_QUEUE_OPTICAL_FLOW_BIT_NV");
    }
}

VkSurfaceFormatKHR r_choose_swapchain_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    uint32_t surface_fmt_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_fmt_count, NULL);
    VkSurfaceFormatKHR fmts[surface_fmt_count];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_fmt_count, fmts);
    for (size_t i = 0; i < surface_fmt_count; i++) {
        if (fmts[i].format == VK_FORMAT_B8G8R8A8_SRGB && fmts[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return fmts[i];
        }
    }

    return fmts[0];
}

VkExtent2D r_suggest_swapchain_extent(VkPhysicalDevice physical_device, VkSurfaceKHR surface, int width, int height)
{
    VkSurfaceCapabilitiesKHR capabilities = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        return (VkExtent2D) {
            .width  = CLAMP(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            .height = CLAMP(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
        };
    }
}

uint32_t r_get_suggested_image_count(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    VkSurfaceCapabilitiesKHR capabilities = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);
    uint32_t img_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && img_count > capabilities.minImageCount)
        img_count = capabilities.maxImageCount;

    return img_count;
}

uint32_t r_get_current_transform(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    uint32_t img_count = 0;
    VkSurfaceCapabilitiesKHR capabilities = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);
    return capabilities.currentTransform;
}

VkPresentModeKHR r_choose_present_mode(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, NULL);
    VkPresentModeKHR present_modes[present_mode_count];
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes);
    for (size_t i = 0; i < present_mode_count; i++) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            return present_modes[i];
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

bool r_create_render_pass(VkDevice device, VkFormat depth_format, VkFormat color_format, VkRenderPass *render_pass)
{
    VkAttachmentDescription color = {
        .format = color_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    VkAttachmentReference color_ref = {.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentDescription depth = {
        .format = depth_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkAttachmentReference depth_ref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass = {
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_ref,
        .pDepthStencilAttachment = &depth_ref,
    };
    VkAttachmentDescription attachments[] = {color, depth};
    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    return vk_create_render_pass(
        device,
        NULL,
        render_pass,
        .attachmentCount = RVK_ARRAY_LEN(attachments),
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    );
}

bool r_create_rvk_swapchain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, int width, int height, Rvk_Swapchain *swapchain)
{
    bool result = true;

    swapchain->surface_format = r_choose_swapchain_surface_format(physical_device, surface),
    swapchain->extent         = r_suggest_swapchain_extent(physical_device, surface, width, height),
    swapchain->image_count    = r_get_suggested_image_count(physical_device, surface),

    result = vk_create_swapchain_khr(
        device,
        NULL,
        &swapchain->handle,
        .surface = surface,
        .minImageCount = swapchain->image_count, // only a suggestion not guaranteed
        .imageFormat = swapchain->surface_format.format,
        .imageColorSpace = swapchain->surface_format.colorSpace,
        .imageExtent = swapchain->extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .clipped = VK_TRUE,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = r_choose_present_mode(physical_device, surface),
        .preTransform = r_get_current_transform(physical_device, surface),
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    );
    if (!result) return false;

    /* query ACTUAL image count */
    result = RVK(vkGetSwapchainImagesKHR(device, swapchain->handle, &swapchain->image_count, NULL));
    if (!result) return false;
    if (swapchain->image_count > RVK_MAX_SWAPCHAIN_IMAGES) {
        r_log(RVK_ERROR, "swapchain RVK_MAX_SWAPCHAIN_IMAGES %zu was exceeded", RVK_MAX_SWAPCHAIN_IMAGES);
        return false;
    }
    result = RVK(vkGetSwapchainImagesKHR(device, swapchain->handle, &swapchain->image_count, swapchain->images));
    if (!result) return false;

    /* create the image views for the swapchain */
    for (size_t i = 0; i < swapchain->image_count; i++) {
        result = vk_create_image_view(
            device,
            NULL,
            &swapchain->image_views[i],
            .image = swapchain->images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchain->surface_format.format,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        );
        if (!result) {
            r_log(RVK_ERROR, "failed to create image %zu", i);
            return false;
        }
    }


    return result;
}

bool r_create_2d_image(VkDevice device, VkFormat format, VkImageUsageFlags flags, VkExtent2D extent, VkImage *image)
{
    return vk_create_image(
        device,
        NULL,
        image,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {extent.width, extent.height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    );
}

uint32_t r_find_memory_type_index(VkPhysicalDevice physical_device, uint32_t type, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properites = {0};
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properites);
    for (uint32_t i = 0; i < mem_properites.memoryTypeCount; i++) {
        if (type & (1 << i) && (mem_properites.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return -1;
}

bool r_allocate_and_bind_image_memory(VkPhysicalDevice physical_device, VkDevice device, VkImage image, VkDeviceMemory *memory)
{
    bool result = true;

    VkMemoryRequirements mem_reqs = {0};
    vkGetImageMemoryRequirements(device, image, &mem_reqs);
    VkMemoryAllocateInfo alloc_ci = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs.size,
    };
    alloc_ci.memoryTypeIndex = r_find_memory_type_index(
        physical_device,
        mem_reqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    if (alloc_ci.memoryTypeIndex == -1) {
        r_log(RVK_ERROR, "memory not suitable based on requirements");
        return false;
    }
    result = RVK(vkAllocateMemory(device, &alloc_ci, NULL, memory));
    if (!result) return false;
    result = RVK(vkBindImageMemory(device, image, *memory, 0));
    if (!result) return false;

    return result;
}

/***********************************************************************************
*  vk_* API implementation
************************************************************************************/

bool vk_create_instance_(const VkAllocationCallbacks *pAllocator, VkInstance* pInstance, VkInstanceCreateInfo ci)
{
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    if (ci.pApplicationInfo) {
        /* bypass the const */
        VkBaseOutStructure *base = (VkBaseOutStructure *)ci.pApplicationInfo;
        base->sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    }

    return RVK(vkCreateInstance(&ci, pAllocator, pInstance));
}

bool vk_create_device_(VkPhysicalDevice physical_device, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice, VkDeviceCreateInfo ci)
{
    ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    for (uint32_t i = 0; i < ci.queueCreateInfoCount; i++) {
        if (ci.pQueueCreateInfos[i].sType != VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO) {
            /* bypass the const */
            VkBaseOutStructure *base = (VkBaseOutStructure *)&ci.pQueueCreateInfos[i];
            base->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        }
    }

    return RVK(vkCreateDevice(physical_device, &ci, pAllocator, pDevice));
}

bool vk_create_swapchain_khr_(VkDevice device, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain, VkSwapchainCreateInfoKHR ci)
{
    ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    return RVK(vkCreateSwapchainKHR(device, &ci, pAllocator, pSwapchain));
}

bool vk_create_image_view_(VkDevice device, const VkAllocationCallbacks* pAllocator, VkImageView* pView, VkImageViewCreateInfo ci)
{
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    return RVK(vkCreateImageView(device, &ci, pAllocator, pView));
}

bool vk_create_render_pass_(VkDevice device, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass, VkRenderPassCreateInfo ci)
{
    ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    return RVK(vkCreateRenderPass(device, &ci, pAllocator, pRenderPass));
}

bool vk_create_image_(VkDevice device, const VkAllocationCallbacks *pAllocator, VkImage *pImage, VkImageCreateInfo ci)
{
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    return RVK(vkCreateImage(device, &ci, pAllocator, pImage));
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
