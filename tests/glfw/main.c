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

Rvk_Simple_2D_Vertex vertices[] = {
    {{-0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.0f,  0.5f}, {0.0f, 0.0f, 1.0f}},
};

uint16_t indices[] = {0, 1, 2};

typedef struct {
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
} Pipeline;

// TODO: create a lazy context
int main()
{
    /* initialize glfw and window */
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "glfw", NULL, NULL);
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_ci = r_get_debug_messenger_info();

    /* create vulkan instance (w/ or w/o validation layers i.e. VK_VALIDATION = 1/0) */
    if (!vk_create_instance(NULL, &instance,
                            .pNext = (VK_VALIDATION) ? &debug_messenger_ci : NULL,
                            .ppEnabledLayerNames = layers,
                            .enabledLayerCount = ARRAY_LEN(layers),
                            .ppEnabledExtensionNames = instance_exts,
                            .enabledExtensionCount = ARRAY_LEN(instance_exts))) return 1;

    /* create the vulkan surface */
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!RVK(glfwCreateWindowSurface(instance, window, NULL, &surface))) return 1;

    /* pick physical device (tries to prefer discrete GPU) */
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    if (!(physical_device = r_pick_physical_device(instance))) return 1;

    /* find a queue family with graphics & present support.
     * if we don't care about present support set surface = NULL.
     * if we want a queue family with compute and graphics set flags e.g.:
     *     VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT */
    uint32_t queue_fam_idx = r_find_queue_family(physical_device, surface, VK_QUEUE_GRAPHICS_BIT);
    if (queue_fam_idx == -1) {
        r_log(RVK_ERROR, "failed to find sufficient queue family");
        return 1;
    }

    /* create a device with the queue family index and physical device we picked */
    VkDevice device = VK_NULL_HANDLE;
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_ci = {
        .queueFamilyIndex = queue_fam_idx,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    if (!vk_create_device(physical_device, NULL, &device,
                          .pQueueCreateInfos = &queue_ci,
                          .queueCreateInfoCount = 1,
                          .enabledExtensionCount = ARRAY_LEN(device_exts),
                          .ppEnabledExtensionNames = device_exts,
                          .ppEnabledLayerNames = layers,
                          .enabledLayerCount = ARRAY_LEN(layers))) return 1;

    /* acquire the queue */
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queue_fam_idx, 0, &queue);
    if (!queue) return 1;

    /* create swapchain */
    Rvk_Swapchain swapchain = {0};
    if (!r_create_rvk_swapchain(physical_device, device, surface, WINDOW_WIDTH, WINDOW_HEIGHT, &swapchain)) return 1;

    /* create renderpass */
    VkFormat depth_format =  VK_FORMAT_D32_SFLOAT;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    if (!r_create_render_pass(device, depth_format, swapchain.surface_format.format, &render_pass)) return 1;

    /* TODO_BEGIN: this should probably go in r_create_rvk_swapchain */
    if (!r_create_2D_image(device, depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                           swapchain.extent, &swapchain.depth_image)) return 1;
    if (!r_allocate_and_bind_image_memory(physical_device, device, swapchain.depth_image,
                                          &swapchain.depth_image_memory)) return 1;
    if (!vk_create_image_view(device, NULL, &swapchain.depth_image_view,
                              .image = swapchain.depth_image,
                              .viewType = VK_IMAGE_VIEW_TYPE_2D,
                              .format = depth_format,
                              .subresourceRange = {
                                  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                                  .levelCount = 1, .layerCount = 1,
                              })) return 1;

    if (!r_init_framebuffers(device, &swapchain, render_pass)) return 1;
    /* TODO_END */

    VkCommandPool command_pool = VK_NULL_HANDLE;
    if (!vk_create_command_pool(device, NULL, &command_pool,
                                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                .queueFamilyIndex = queue_fam_idx)) return 1;

    VkCommandBuffer cmd_buff = VK_NULL_HANDLE;
    if (!vk_allocate_command_buffers(device, &cmd_buff,
                                     .commandPool = command_pool,
                                     .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                     .commandBufferCount = 1)) return 1;

    VkSemaphore image_available = VK_NULL_HANDLE;
    VkSemaphore render_finished = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    if (!vk_create_semaphore(device, NULL, &image_available)) return 1;
    if (!vk_create_semaphore(device, NULL, &render_finished)) return 1;
    if (!vk_create_fence(device, NULL, &fence, .flags = VK_FENCE_CREATE_SIGNALED_BIT)) return 1;

    Pipeline triangle = {0};
    if (!vk_create_pipeline_layout(device, NULL, &triangle.pipeline_layout)) return 1;

    VkPipelineShaderStageCreateInfo stages[] = {
        {.stage = VK_SHADER_STAGE_VERTEX_BIT,   .pName = "main"},
        {.stage = VK_SHADER_STAGE_FRAGMENT_BIT, .pName = "main"},
    };

    String_Builder sb = {0};
    if (!read_entire_file("shaders/triangle.vert.glsl.spv", &sb)) return 1;
    if (!vk_create_shader_module(device, NULL, &stages[0].module,
                                 .codeSize = sb.count, .pCode = (uint32_t *)sb.items)) return 1;
    sb.count = 0; // reuse memory
    if (!read_entire_file("shaders/triangle.frag.glsl.spv", &sb)) return 1;
    if (!vk_create_shader_module(device, NULL, &stages[1].module,
                                 .codeSize = sb.count, .pCode = (uint32_t *)sb.items)) return 1;
    sb.count = 0; // reuse memory

    VkPipelineVertexInputStateCreateInfo vertex_input_state_ci = r_default_simple_2D_vertex_input_state_ci();
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_ci = r_default_input_assembly_state_ci();
    VkPipelineViewportStateCreateInfo viewport_state_ci = r_default_viewport_state_ci(swapchain.extent);
    VkPipelineRasterizationStateCreateInfo rasterization_state_ci = r_default_rasterization_state_ci();
    VkPipelineMultisampleStateCreateInfo multisampling_state_ci = r_default_multisample_state_ci();
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_ci = r_default_depth_stencil_state_ci();
    VkPipelineColorBlendStateCreateInfo color_blend_state_ci = r_default_color_blend_state_ci();
    VkPipelineDynamicStateCreateInfo dynamic_state_ci = r_default_dynamic_state_ci();

    if (!vk_create_graphics_pipeline(device, NULL, NULL, &triangle.pipeline,
                                     .stageCount = ARRAY_LEN(stages),
                                     .pStages = stages,
                                     .pVertexInputState = &vertex_input_state_ci,
                                     .pInputAssemblyState = &input_assembly_state_ci,
                                     .pViewportState = &viewport_state_ci,
                                     .pRasterizationState = &rasterization_state_ci,
                                     .pMultisampleState = &multisampling_state_ci,
                                     .pDepthStencilState = &depth_stencil_state_ci,
                                     .pColorBlendState = &color_blend_state_ci,
                                     .pDynamicState = &dynamic_state_ci,
                                     .layout = triangle.pipeline_layout,
                                     .renderPass = render_pass)) return 1;

    vkDestroyShaderModule(device, stages[0].module, NULL);
    vkDestroyShaderModule(device, stages[1].module, NULL);

    /* game loop */
    int esc = 0;
    do {
        vkCmdBindPipeline(cmd_buff, 0, triangle.pipeline);
        r_cmd_set_viewport_scissor(cmd_buff, swapchain.extent);

        glfwPollEvents();
        esc = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    } while (!esc && !glfwWindowShouldClose(window));

    /* cleanup (mainly so that validation layers don't yell at us, realistically the OS cleans up anyway) */
    vkDestroyPipeline(device, triangle.pipeline, NULL);
    vkDestroyPipelineLayout(device, triangle.pipeline_layout, NULL);
    vkDestroyFence(device, fence, NULL);
    vkDestroySemaphore(device, image_available, NULL);
    vkDestroySemaphore(device, render_finished, NULL);
    vkFreeCommandBuffers(device, command_pool, 1, &cmd_buff);
    vkDestroyCommandPool(device, command_pool, NULL);
    for (size_t i = 0; i < swapchain.image_count; i++)
        vkDestroyFramebuffer(device, swapchain.framebuffers[i], NULL);
    vkDestroyImageView(device, swapchain.depth_image_view, NULL);
    vkDestroyImage(device, swapchain.depth_image, NULL);
    vkFreeMemory(device, swapchain.depth_image_memory, NULL);
    vkDestroyRenderPass(device, render_pass, NULL);
    for (size_t i = 0; i < swapchain.image_count; i++)
        vkDestroyImageView(device, swapchain.image_views[i], NULL);
    vkDestroySwapchainKHR(device, swapchain.handle, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    return 0;
}
