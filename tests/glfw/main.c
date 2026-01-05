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

    VkPipelineShaderStageCreateInfo stages[2];
    String_Builder sb = {0};
    if (!read_entire_file("shaders/triangle.vert.glsl.spv", &sb)) return 1;
    stages[0] = r_create_vertex_stage_ci(device.logical, sb.count, (uint32_t*)sb.items);
    if (!stages[0].module) return 1;
    sb.count = 0; // reuse memory
    if (!read_entire_file("shaders/triangle.frag.glsl.spv", &sb)) return 1;
    stages[1] = r_create_fragment_stage_ci(device.logical, sb.count, (uint32_t*)sb.items);
    if (!stages[0].module) return 1;
    sb.count = 0; // reuse memory

    size_t temp_alloc_save_point = r_temp_save();
    if (!vk_create_graphics_pipeline(device.logical, NULL, NULL, &triangle.pipeline,
                                     .stageCount = ARRAY_LEN(stages),
                                     .pStages = stages,
                                     .pVertexInputState = r_temp_default_simple_2D_vertex_input_state_ci(),
                                     .pInputAssemblyState = r_temp_default_input_assembly_state_ci(),
                                     .pViewportState = r_temp_default_viewport_state_ci(swapchain.extent),
                                     .pRasterizationState = r_temp_default_rasterization_state_ci(),
                                     .pMultisampleState = r_temp_default_multisample_state_ci(),
                                     .pDepthStencilState = r_temp_default_depth_stencil_state_ci(),
                                     .pColorBlendState = r_temp_default_color_blend_state_ci(),
                                     .pDynamicState = r_temp_default_dynamic_state_ci(),
                                     .layout = triangle.pipeline_layout,
                                     .renderPass = swapchain.render_pass)) return 1;
    r_temp_rewind(temp_alloc_save_point);
    vkDestroyShaderModule(device.logical, stages[0].module, NULL);
    vkDestroyShaderModule(device.logical, stages[1].module, NULL);

    /* create vertex/index buffers */
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
        if (!r_wait_reset_fence(device.logical, &device.fences[current_frame])) return 1;
        if (!r_acquire_next_image(device.logical, swapchain.handle,
                                  device.image_available_sems[current_frame], &img_idx)) return 1;
        // TODO: look for swapchain resize if we set GLFW_RESIZABLE to false
        if (!r_reset_begin_cmd_buff(device.cmd_buffs[current_frame])) return 1;
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

        current_frame = (current_frame + 1) % RVK_MAX_FRAMES_IN_FLIGHT;

    } while (!esc && !glfwWindowShouldClose(window));

    /* cleanup  */
    vkQueueWaitIdle(device.queue);
    r_destroy_rvk_buffer(device.logical, vtx);
    r_destroy_rvk_buffer(device.logical, idx);
    vkDestroyPipeline(device.logical, triangle.pipeline, NULL);
    vkDestroyPipelineLayout(device.logical, triangle.pipeline_layout, NULL);
    r_destroy_rvk_swapchain(device.logical, swapchain);
    r_destroy_rvk_device(device);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    return 0;
}
