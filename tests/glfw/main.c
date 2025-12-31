#define RVK_LOG_LEVEL RVK_INFO
#define RVK_IMPLEMENTATION
#define PLATFORM_DESKTOP_GLFW
#include "../../rvk.h"

/* must be included after vulkan header */
#include <GLFW/glfw3.h>

#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "../../nob.h"

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 400

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

// bool r_init_lazy_ctx(Rvk_Lazy_Ctx *ctx)
// {
//
// }

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "glfw", NULL, NULL);
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

    VkQueueFlags flags = VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT;
    uint32_t queue_fam_idx = r_find_queue_family(physical_device, surface, flags);
    assert(queue_fam_idx != -1 && "queue family unsatisfactory");

    VkDevice device = VK_NULL_HANDLE;
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_ci = {
        .queueFamilyIndex = queue_fam_idx,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    result = vk_create_device(
        physical_device,
        NULL,
        &device,
        .pQueueCreateInfos = &queue_ci,
        .queueCreateInfoCount = 1,
        .enabledExtensionCount = ARRAY_LEN(device_exts),
        .ppEnabledExtensionNames = device_exts,
        .ppEnabledLayerNames = layers,
        .enabledLayerCount = ARRAY_LEN(layers),
    );
    assert(result && "failed to create device");

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queue_fam_idx, 0, &queue);

    /* create swapchain */
    Rvk_Swapchain swapchain = {
        .format    = r_choose_swapchain_format(physical_device, surface),
        .extent    = r_suggest_swapchain_extent(physical_device, surface, WINDOW_WIDTH, WINDOW_HEIGHT),
        .img_count = r_get_suggested_img_count(physical_device, surface),
    };

    result = vk_create_swapchain_khr(
        device,
        NULL,
        &swapchain.handle,
        .surface = surface,
        .minImageCount = swapchain.img_count,
        .imageFormat = swapchain.format.format,
        .imageColorSpace = swapchain.format.colorSpace,
        .imageExtent = swapchain.extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .clipped = VK_TRUE,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = r_choose_present_mode(physical_device, surface),
        .preTransform = r_get_current_transform(physical_device, surface),
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    );

    assert(result && "failed to create swapchain");
    result = RVK(vkGetSwapchainImagesKHR(device, swapchain.handle, &swapchain.img_count, NULL));
    assert(swapchain.img_count <= RVK_MAX_SWAPCHAIN_IMAGES);
    result = RVK(vkGetSwapchainImagesKHR(device, swapchain.handle, &swapchain.img_count, swapchain.imgs));
    assert(result && "failed to populate swapchain images");

    vkDestroySwapchainKHR(device, swapchain.handle, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    return 0;
}
