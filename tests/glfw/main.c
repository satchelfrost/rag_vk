#define RVK_LOG_LEVEL RVK_INFO
#define RVK_IMPLEMENTATION
#define PLATFORM_DESKTOP_GLFW
#include "../../rag_vk.h"

#include <GLFW/glfw3.h>

#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "../../nob.h"

#define VK_VALIDATION 1 

static const char *instance_exts[] = {
    "VK_KHR_surface",
    "VK_KHR_xcb_surface",
#if VK_VALIDATION_
    "VK_EXT_debug_utils",
#endif
};
static const char *layers[] = {
#if VK_VALIDATION_
    "VK_LAYER_KHRONOS_validation",
#endif
};
static const char *devices_exts[] = {"VK_KHR_swapchain"};

void r_log_queue_properties(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    uint32_t queue_fam_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_fam_count, NULL);
    VkQueueFamilyProperties queue_fam_props[queue_fam_count];
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_fam_count, queue_fam_props);
    for (uint32_t i = 0; i < queue_fam_count; i++) {
        VkBool32 present_support = false;
        if (surface) vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        printf("queue %u, present support: %s\n", i, (present_support) ? "true" : "false");

        VkQueueFlags flags = queue_fam_props[i].queueFlags;
        if (flags & VK_QUEUE_GRAPHICS_BIT        ) printf("    VK_QUEUE_GRAPHICS_BIT\n");
        if (flags & VK_QUEUE_COMPUTE_BIT         ) printf("    VK_QUEUE_COMPUTE_BIT\n");
        if (flags & VK_QUEUE_TRANSFER_BIT        ) printf("    VK_QUEUE_TRANSFER_BIT\n");
        if (flags & VK_QUEUE_SPARSE_BINDING_BIT  ) printf("    VK_QUEUE_SPARSE_BINDING_BIT\n");
        if (flags & VK_QUEUE_PROTECTED_BIT       ) printf("    VK_QUEUE_PROTECTED_BIT\n");
        if (flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) printf("    VK_QUEUE_VIDEO_DECODE_BIT_KHR\n");
        if (flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) printf("    VK_QUEUE_VIDEO_ENCODE_BIT_KHR\n");
        if (flags & VK_QUEUE_OPTICAL_FLOW_BIT_NV ) printf("    VK_QUEUE_OPTICAL_FLOW_BIT_NV\n");
    }
}

/* returns max unt32_t upon error
 * surface == NULL means we don't care about present support */
uint32_t r_find_queue(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkQueueFlags flags)
{
    uint32_t queue_fam_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_fam_count, NULL);
    VkQueueFamilyProperties queue_fam_props[queue_fam_count];
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_fam_count, queue_fam_props);
    for (uint32_t i = 0; i < queue_fam_count; i++) {
        VkBool32 present_support = true;
        bool flag_check = (queue_fam_props[i].queueFlags & flags) == flags;
        if (surface) {
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
            if (flag_check && present_support) return i;
        } else {
            if (flag_check) return i;
        }
    }

    return -1;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(400, 400, "glfw", NULL, NULL);
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_ci = r_get_debug_messenger_info();

    bool result = vk_create_instance(
        NULL,
        &instance,
        .pNext = (VK_VALIDATION) ? &debug_messenger_ci : NULL,
        .ppEnabledLayerNames = layers,
        .enabledLayerCount = ARRAY_LEN(layers),
        .ppEnabledExtensionNames = instance_exts,
        .enabledExtensionCount = ARRAY_LEN(instance_exts),
    );
    assert(result && "failed to create vulkan instance");

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    result = RVK(glfwCreateWindowSurface(instance, window, NULL, &surface))
    assert(result && "failed to create window surface");

    VkPhysicalDevice physical_device = r_pick_physical_device(instance);
    assert(physical_device && "failed to find suitable physical device");

    r_log_queue_properties(physical_device, surface);
    uint32_t queue_idx = r_find_queue(physical_device, surface, VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT);
    printf("queue %s, index = %u\n", (queue_idx == -1) ? "not found" : "found", queue_idx);

    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    return 0;
}
