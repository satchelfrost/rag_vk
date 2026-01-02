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
    /* initialize glfw and window */
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "glfw", NULL, NULL);
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_ci = r_get_debug_messenger_info();

    /* create vulkan instance (w/ or w/o validation layers i.e. VK_VALIDATION = 1/0) */
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

    /* create the vulkan surface */
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    result = RVK(glfwCreateWindowSurface(instance, window, NULL, &surface));
    assert(result && "failed to create window surface");

    /* pick physical device (tries to prefer discrete GPU) */
    VkPhysicalDevice physical_device = r_pick_physical_device(instance);
    assert(physical_device && "failed to find suitable physical device");

    /* find a queue family with graphics & present support.
     * if we don't care about present support set surface = NULL.
     * if we want a queue family with compute and graphics set flags e.g.:
     *     VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT */
    uint32_t queue_fam_idx = r_find_queue_family(physical_device, surface, VK_QUEUE_GRAPHICS_BIT);
    assert(queue_fam_idx != -1 && "queue family unsatisfactory");

    /* create a device with the queue family index and physical device we picked */
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

    /* acquire the queue */
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queue_fam_idx, 0, &queue);

    /* create swapchain */
    Rvk_Swapchain swapchain = {0};
    result = r_create_rvk_swapchain(physical_device, device, surface, WINDOW_WIDTH, WINDOW_HEIGHT, &swapchain);
    assert(result && "failed to create Rvk_Swapchain");

    /* create renderpass */
    VkFormat depth_format =  VK_FORMAT_D32_SFLOAT;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    result = r_create_render_pass(device, depth_format, swapchain.surface_format.format, &render_pass);
    assert(result && "failed to create render pass");

    VkImage depth_image = VK_NULL_HANDLE;
    VkDeviceMemory depth_image_memory = VK_NULL_HANDLE;
    result = r_create_2d_image(
        device,
        depth_format,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        swapchain.extent,
        &depth_image
    );
    assert(result && "failed to create depth image");
    result = r_allocate_and_bind_image_memory(physical_device, device, depth_image, &depth_image_memory);
    assert(result && "failed to allocate and bind image memory");

    /* cleanup (mainly so that validation layers don't yell at us, realistically the OS cleans up anyway) */
    vkDestroyImage(device, depth_image, NULL);
    vkFreeMemory(device, depth_image_memory, NULL);
    vkDestroyRenderPass(device, render_pass, NULL);
    for (size_t i = 0; i < swapchain.image_count; i++)
        vkDestroyImageView(device, swapchain.image_views[i], NULL);
    vkDestroySwapchainKHR(device, swapchain.handle, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    return 0;
}
