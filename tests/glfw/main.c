// #define RAG_VK_VALIDATION_LOG_LEVEL RAG_INFO
#define RAG_VK_IMPLEMENTATION
#define PLATFORM_DESKTOP_GLFW
#include "../../rag_vk.h"

#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "../../nob.h"

GLFWwindow *rag_init_glfw(int width, int height, const char* title, GLFWmonitor* monitor, GLFWwindow* share)
{
    if (!glfwInit()) {
        printf("failed to initialize glfw\n");
        return NULL;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    return glfwCreateWindow(width, height, title, monitor, share);
}

bool rag_create_glfw_surface(VkInstance instance, GLFWwindow *window, const VkAllocationCallbacks *allocator, VkSurfaceKHR *surface)
{
    return RAG_VK(glfwCreateWindowSurface(instance, window, allocator, surface));
}

typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} Strings;

void rag_append_glfw_extensions(Strings *strings)
{
    uint32_t glfw_ext_count = 0;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    for (size_t i = 0; i < glfw_ext_count; i++)
        da_append(strings, glfw_extensions[i]);
}

#define VK_VALIDATION true

int main()
{
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    GLFWwindow *window = rag_init_glfw(400, 400, "glfw", NULL, NULL);
    if (!window) {
        printf("failed to create glfw window\n");
        return 1;
    }

    Strings extensions = {0};
    rag_append_glfw_extensions(&extensions);

    Strings layers = {0};
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger = {0};
    bool validation = VK_VALIDATION;
    if (validation) {
        da_append(&extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        da_append(&layers, "VK_LAYER_KHRONOS_validation");
        debug_messenger = rag_get_debug_messenger_info();
    }

    if (rag_instance_layers_supported(layers.items, layers.count)) return 1;

    bool result = vk_create_instance(
        NULL,
        &instance,
        .pNext = &debug_messenger,
        .ppEnabledLayerNames = layers.items,
        .ppEnabledExtensionNames = extensions.items,
        .enabledExtensionCount = extensions.count,
    );
    if (!result) return 1;

    if (!rag_create_glfw_surface(instance, window, NULL, &surface)) {
        printf("failed to create surface\n");
        return 1;
    }

    return 0;
}
