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
    {{ 0.0f, -0.5f}, {0.0f, 0.0f, 1.0f}},
};

uint16_t indices[] = {0, 1, 2};

static struct {
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
} triangle = {0};

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

    Rvk_Device_Config device_config = {
        .extension_count = ARRAY_LEN(device_exts),
        .extensions = device_exts,
        .layer_count = ARRAY_LEN(layers),
        .layers = layers,
    };
    Rvk_Device device = r_create_rvk_device(instance, surface, device_config);
    if (!device.logical) return 1;

    /* create swapchain */
    Rvk_Swapchain swapchain = r_create_rvk_swapchain(device, surface, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!swapchain.handle) return 1;

    if (!vk_create_pipeline_layout(device.logical, NULL, &triangle.pipeline_layout)) return 1;

    typedef struct {
        VkPipelineShaderStageCreateInfo vertex;
        VkPipelineShaderStageCreateInfo fragment;
    } Rvk_Standard_Shaders;

    /* TODO: put this in something */
    VkPipelineShaderStageCreateInfo stages[] = {
        {.stage = VK_SHADER_STAGE_VERTEX_BIT,   .pName = "main"},
        {.stage = VK_SHADER_STAGE_FRAGMENT_BIT, .pName = "main"},
    };
    String_Builder sb = {0};
    if (!read_entire_file("shaders/triangle.vert.glsl.spv", &sb)) return 1;
    if (!vk_create_shader_module(device.logical, NULL, &stages[0].module,
                                 .codeSize = sb.count, .pCode = (uint32_t *)sb.items)) return 1;
    sb.count = 0; // reuse memory
    if (!read_entire_file("shaders/triangle.frag.glsl.spv", &sb)) return 1;
    if (!vk_create_shader_module(device.logical, NULL, &stages[1].module,
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

    if (!vk_create_graphics_pipeline(device.logical, NULL, NULL, &triangle.pipeline,
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
                                     .renderPass = swapchain.render_pass)) return 1;
    vkDestroyShaderModule(device.logical, stages[0].module, NULL);
    vkDestroyShaderModule(device.logical, stages[1].module, NULL);

    size_t count = ARRAY_LEN(vertices);
    size_t size = count*sizeof(*vertices);
    Rvk_Buffer vtx = r_create_vertex_buffer(device, size, count, vertices);
    if (!vtx.info.buffer) return 1;
    count = ARRAY_LEN(indices);
    size = count*sizeof(*indices);
    Rvk_Buffer idx = r_create_index_buffer(device, size, count, indices);
    if (!idx.info.buffer) return 1;

    uint32_t img_idx = 0;
    uint32_t current_frame = 0;

    /* game loop */
    int esc = 0;
    do {
        /* wait to begin graphics */
        if (!RVK(vkWaitForFences(device.logical, 1, &device.fences[current_frame], VK_TRUE, UINT64_MAX))) return 1;
        if (!RVK(vkResetFences(device.logical, 1, &device.fences[current_frame]))) return 1;
        if (!RVK(vkAcquireNextImageKHR(device.logical, swapchain.handle, UINT64_MAX,
                                       device.image_available_sems[current_frame], VK_NULL_HANDLE, &img_idx))) return 1;
        // TODO: look for swapchain resize if we set GLFW_RESIZABLE to false
        if (!RVK(vkResetCommandBuffer(device.cmd_buffs[current_frame], 0))) return 1;
        VkCommandBufferBeginInfo begin_info = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, };
        if (!RVK(vkBeginCommandBuffer(device.cmd_buffs[current_frame], &begin_info))) return 1;
            r_cmd_begin_render_pass(device.cmd_buffs[current_frame], swapchain.render_pass, swapchain.framebuffers[img_idx],
                                    swapchain.extent, 1.0f, 1.0f, 1.0f, 1.0f);
                vkCmdBindPipeline(device.cmd_buffs[current_frame], 0, triangle.pipeline);
                r_cmd_set_viewport_scissor(device.cmd_buffs[current_frame], swapchain.extent);
                r_cmd_draw_buffers(device.cmd_buffs[current_frame], vtx.info.buffer, idx.info.buffer, ARRAY_LEN(indices));
            vkCmdEndRenderPass(device.cmd_buffs[current_frame]);
        if (!RVK(vkEndCommandBuffer(device.cmd_buffs[current_frame]))) return 1;

        if (!r_submit(device, current_frame)) return 1;
        if (!r_present(device.queue, device.render_finished_sems[current_frame], img_idx, swapchain.handle)) return 1;

        glfwPollEvents();
        esc = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;

        // current_frame = (current_frame + 1) % RVK_MAX_FRAMES_IN_FLIGHT;

    } while (!esc && !glfwWindowShouldClose(window));

    vkQueueWaitIdle(device.queue);

    /* cleanup (mainly so that validation layers don't yell at us, realistically the OS cleans up anyway) */
    vkDestroyBuffer(device.logical, vtx.info.buffer, NULL);
    vkFreeMemory(device.logical, vtx.memory, NULL);
    vkDestroyBuffer(device.logical, idx.info.buffer, NULL);
    vkFreeMemory(device.logical, idx.memory, NULL);

    vkDestroyPipeline(device.logical, triangle.pipeline, NULL);
    vkDestroyPipelineLayout(device.logical, triangle.pipeline_layout, NULL);

    // TODO: this should be in r_destroy_rvk_device
    for (size_t i = 0; i < RVK_MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(device.logical, device.fences[i], NULL);
        vkDestroySemaphore(device.logical, device.image_available_sems[i], NULL);
        vkDestroySemaphore(device.logical, device.render_finished_sems[i], NULL);
    }
    vkFreeCommandBuffers(device.logical, device.command_pool, RVK_MAX_FRAMES_IN_FLIGHT, device.cmd_buffs);
    vkDestroyCommandPool(device.logical, device.command_pool, NULL);

    for (size_t i = 0; i < swapchain.image_count; i++)
        vkDestroyFramebuffer(device.logical, swapchain.framebuffers[i], NULL);
    vkDestroyImageView(device.logical, swapchain.depth_image_view, NULL);
    vkDestroyImage(device.logical, swapchain.depth_image, NULL);
    vkFreeMemory(device.logical, swapchain.depth_image_memory, NULL);
    vkDestroyRenderPass(device.logical, swapchain.render_pass, NULL);
    for (size_t i = 0; i < swapchain.image_count; i++)
        vkDestroyImageView(device.logical, swapchain.image_views[i], NULL);

    vkDestroySwapchainKHR(device.logical, swapchain.handle, NULL);
    vkDestroyDevice(device.logical, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    return 0;
}
