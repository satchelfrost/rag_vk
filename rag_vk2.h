/* rag_vk - v1.0.0 - MIT license - https://github.com/satchelfrost/rag_vk

   **WARNING** Probably don't use this, especially not v0.0.0

   This file is an attempt to have a vulkan stb-style header-only library.
   Currently, the header-only goal isn't being met very well. However, this
   file is still being used in multiple projects, and the versions are starting to diverge.
   Therefore, I've collected it into a repo to have a single source of authority,
   and track different versions.

   TODO: usage exaplanation and conventions

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
#ifndef MIN_SEVERITY
    #define MIN_SEVERITY RVK_WARNING
#endif

#define VK_FLAGS_NONE 0
#define RVK_LOAD_PFN(pfn) PFN_ ## pfn pfn = (PFN_ ## pfn) vkGetInstanceProcAddr(rvk_ctx.instance, #pfn)
#define RVK_SUCCEEDED(x) ((x) == VK_SUCCESS)
#define CLAMP(val, min, max) ((val) < (min)) ? (min) : (((val) > (max)) ? (max) : (val))
#define RVK_ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))

typedef struct {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkDebugReportCallbackEXT report_callback;
    bool validation_supported;

    /* the standard context will be used in cases where values
     * are not explicity passed into functions */ 
    struct {
        VkPhysicalDevice physical_device;
        VkDevice device;
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
        // Rvk_Swapchain swapchain;
        // Rvk_Image depth_img;
        // VkImageView depth_img_view;
    } standard;

} Rvk_Context;

typedef struct {
    int width;
    int height;
    const char *title;
} Rvk_Config;
#define rvk_init(...) rvk_init_((Rvk_Config){__VA_ARGS__})
bool rvk_init_(Rvk_Config cfg);

#endif // RAG_VK_H_

/***********************************************************************************
*
*   rag_vk Implementation
*
************************************************************************************/

#ifdef RAG_VK_IMPLEMENTATION

#include <vulkan/vulkan.h>

static Rvk_Context rvk_ctx = {0};

bool rvk_init_(Rvk_Config cfg)
{
    return true;
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
