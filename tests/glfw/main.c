#define RVK_LOG_LEVEL RVK_INFO
#define RVK_IMPLEMENTATION
#define PLATFORM_DESKTOP_GLFW
#include "../../rvk.h"

/* must be included after vulkan header */
#include <GLFW/glfw3.h>

#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "../../nob.h"

#define VK_VALIDATION 1 

static const char *instance_exts[] = {
    "VK_KHR_surface",
    "VK_KHR_xcb_surface",
#if VK_VALIDATION
    "VK_EXT_debug_utils",
#endif
};
static const char *layers[] = {
#if VK_VALIDATION
    "VK_LAYER_KHRONOS_validation",
#endif
};
static const char *device_exts[] = {"VK_KHR_swapchain"};

bool r_init_lazy_ctx(Rvk_Lazy_Ctx *ctx)
{

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
    result = RVK(glfwCreateWindowSurface(instance, window, NULL, &surface));
    assert(result && "failed to create window surface");

    VkPhysicalDevice physical_device = r_pick_physical_device(instance);
    assert(physical_device && "failed to find suitable physical device");

    uint32_t queue_idx = r_find_queue(physical_device, surface, VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT);
    assert(queue_idx != -1 && "queue unsatisfactory");

    VkDevice device = VK_NULL_HANDLE;
    float priority = 1.0f;
    VkDeviceQueueCreateInfo ci = {
        .queueFamilyIndex = queue_idx,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    result = vk_create_device(
        physical_device,
        NULL,
        &device,
        .pQueueCreateInfos = &ci,
        .queueCreateInfoCount = 1,
        .enabledExtensionCount = ARRAY_LEN(device_exts),
        .ppEnabledExtensionNames = device_exts,
    );
    assert(result && "failed to create device");

    // VkQueue queue = VK_NULL_HANDLE;
    // result = RVK(vkGetDeviceQueue(device, queue_idx, 0))

    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    return 0;
}
