// #include "vulkan/vulkan_core.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cglm/cglm.h>
#include <cglm/mat2.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>


#include "helper.h"

GLFWwindow* window;
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const char* validation_layers[] = {
    "VK_LAYER_KHRONOS_validation",
};
const uint32_t validation_layer_count = sizeof(validation_layers) / sizeof(validation_layers[0]);

const char* device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
const uint32_t device_extensions_count = sizeof(device_extensions) / sizeof(device_extensions[0]);

#ifdef NDEBUG
    const bool enable_validation_layers = false;
#else
    const bool enable_validation_layers = true;
#endif


VkInstance instance;
VkDebugUtilsMessengerEXT debug_messenger;
VkSurfaceKHR surface;

VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice device;

VkQueue graphics_queue;
VkQueue present_queue;

VkSwapchainKHR swap_chain;

typedef struct {
    VkImage* images;
    uint32_t count;
} SwapChainImages;

SwapChainImages swap_chain_images = {0};
VkFormat swap_chain_image_format;
VkExtent2D swap_chain_extent;

typedef struct {
    VkImageView* views;
    uint32_t count;
} SwapChainImageViews;
SwapChainImageViews swap_chain_image_views = {0};

VkFramebuffer* swap_chain_framebuffers;

VkRenderPass render_pass;
VkPipelineLayout pipeline_layout;
VkPipeline graphics_pipeline;

VkCommandPool command_pool;

VkBuffer vertex_buffer;
VkDeviceMemory vertex_buffer_memory;
VkBuffer index_buffer;
VkDeviceMemory index_buffer_memory;
VkBuffer staging_buffer;
VkDeviceMemory staging_buffer_memory;

#define MAX_FRAMES_IN_FLIGHT 1
VkFence frame_fences[MAX_FRAMES_IN_FLIGHT];
VkSemaphore acquire_semaphores[MAX_FRAMES_IN_FLIGHT];
VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT]; 
uint32_t frame_index = 0; // 0..MAX_FRAMES_IN_FLIGHT - 1

VkSemaphore* submit_semaphores; // SwapChainImages.count 

bool framebuffer_resized = false;


typedef struct Vertex {
    vec2 pos;
    vec3 color;
} Vertex;

#define WHITE 1.0f, 1.0f, 1.0f
Vertex vertices[100] = {
    // {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    // {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    // {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    // {{-0.5f, 0.5f}, {1.0f, 0.0f, 0.5f}},
    // {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}


};


const uint32_t VERTEX_COUNT = sizeof(vertices) / sizeof(vertices[0]);

const uint32_t INDICES_COUNT = 6;
const uint16_t indices[] = {0, 1, 2, 2, 3, 0};


// --- Game stuff 
typedef struct Shape {
    float scale;
    Vertex vertices[10]; // shape in NDC.
    int count;
    
} Shape;

typedef struct Entity {
    vec2 pos; // Pos in NDC
    int shape_index;
    vec2 speed;
    float rotation;
    float rotation_speed;
} Entity;

double delta_time = 0.0f;
const double max_delta_time = 0.1f;
int frame_count = 0;
double last_time;


Shape shapes[] = {
    {
        .scale = 0.1f,
        .vertices = {
            // Ship 
            {{0.0f, -1.0f}, {WHITE}}, // Top
            {{-0.5f, 0.5f}, {WHITE}}, // Bottom lef 
            {{0.0f, 0.25f}, {WHITE}}, // Bottom middle
            {{0.5f, 0.5f}, {WHITE}},  // Bottom right 
            {{0.0f, -1.0f}, {WHITE}}, // Top
        },
        .count = 5
    },

    {
        // Box
        .scale = 0.5,
        .vertices = {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f}, {1.0f, 0.0f, 0.5f}},
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}
        },
        .count = 5
    }
};

Entity entities[100] = {0}; 
int entities_count = 0;

#define ATTRIBUTE_COUNT 2
void get_attribute_descriptions(VkVertexInputAttributeDescription attribute_descriptions[ATTRIBUTE_COUNT]) {
    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(Vertex, pos);

    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(Vertex, color);
}

void get_binding_description(VkVertexInputBindingDescription* binding_description) {
    binding_description->binding = 0;
    binding_description->stride = sizeof(Vertex);
    binding_description->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}


bool check_validation_layer_support() {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);

    VkLayerProperties* available_layers = malloc(layer_count * sizeof(VkLayerProperties));
    if (available_layers == NULL) {
        return false;
    }

    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    for (size_t i = 0; i < validation_layer_count; i++) {
        bool layer_supported = false;

        for (size_t j = 0; j < layer_count; j++) {
            if (strcmp(validation_layers[i], available_layers[j].layerName) == 0) {
                layer_supported = true;
                break;
            }
        }

        if (!layer_supported) {
            return false;
        }
    }

    //TODO(memory leak): This will leak memory in the case a layer is not supported 
    free(available_layers);
    return true;
}

const char** get_required_extensions(uint32_t* extension_count) {
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(extension_count);

    const char** extensions = malloc((*extension_count + (enable_validation_layers ? 1 : 0)) * sizeof(const char*));

    if (!extensions) {
        return NULL;
    }

    memcpy(extensions, glfw_extensions, *extension_count * sizeof(const char*));

    if (enable_validation_layers)  {
        extensions[*extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        (*extension_count)++;
    }

    return extensions;
}

void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
    // printf("Window resized: w: %d, h: %d\n", width, height);
    framebuffer_resized = true;
    // last_time = glfwGetTime();
}

bool key_left = false;
bool key_right = false;
bool key_up = false;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS)  {
        switch (key) {
            case GLFW_KEY_LEFT:
                key_left = true;
                break;
            case GLFW_KEY_RIGHT:
                key_right = true;
                break;
            case GLFW_KEY_UP:
                key_up = true;
                break;
        }
    }
    else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_LEFT:
                key_left = false;
                break;
            case GLFW_KEY_RIGHT:
                key_right = false;
                break;
            case GLFW_KEY_UP:
                key_up = false;
                break;
        }
        
    }
}


void init_window() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Astroids", NULL, NULL);
    glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
    glfwSetKeyCallback(window, key_callback);
}

VkResult create_debug_utils_messenger_EXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info, const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL) {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroy_debug_utils_messenger_EXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* p_allocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL) {
        func(instance, debug_messenger, p_allocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data) {

    fprintf(stderr, "Validation layer: %s\n", p_callback_data->pMessage);
    
    return VK_FALSE;
}


void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT* create_info) {
    create_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info->pfnUserCallback = debug_callback;
    create_info->pUserData = NULL;    
}
void setup_debug_messenger() {
    if (!enable_validation_layers) return;

    VkDebugUtilsMessengerCreateInfoEXT create_info = {0};
    populate_debug_messenger_create_info(&create_info);

    if (create_debug_utils_messenger_EXT(instance, &create_info, NULL, &debug_messenger) != VK_SUCCESS) {
        fprintf(stderr, "Failed to set up debug messenger!\n");
    }
}

int create_instance() {
    if (enable_validation_layers && !check_validation_layer_support()) {
        fprintf(stderr, "Validation layers requested, but not avaliable!\n");
    }
    
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Astroids";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;


    uint32_t extension_count = 0;
    const char** extensions = get_required_extensions(&extension_count);
    if (extensions == NULL) {
        fprintf(stderr, "Faild to get extensions\n");
        return -1;
    }

    for (int i = 0; i < extension_count; i++) {
        printf("%s\n", extensions[i]);
    }

    
    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = extension_count;
    create_info.ppEnabledExtensionNames = extensions;
    create_info.enabledLayerCount = 0;
    

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
    if (enable_validation_layers) {
        create_info.enabledLayerCount = validation_layer_count;
        create_info.ppEnabledLayerNames = validation_layers;        

        populate_debug_messenger_create_info(&debug_create_info);
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_create_info;
    }
    else {
        create_info.enabledLayerCount = 0;
        create_info.pNext = NULL;
    }

    VkResult result = vkCreateInstance(&create_info, NULL, &instance);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create instance!, Error Code: %d\n", result);
        return -1;
    }

    //TODO(memory leak): We might want to free extensions, as of now we are leaking some memmory 
    return 0;
}

typedef struct QueueFamilyIndices {
    uint32_t graphics_family;
    uint32_t present_family;
    bool graphicsFamily_has_value;
    bool presentFamily_has_value;
} QueueFamilyIndices;

bool queue_family_is_complete(QueueFamilyIndices qfi) {
    return qfi.graphicsFamily_has_value && qfi.presentFamily_has_value;
}

QueueFamilyIndices find_queue_families(VkPhysicalDevice device) {
    QueueFamilyIndices indices = {0}; 

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

    VkQueueFamilyProperties* queue_families = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);


    for (size_t i = 0; i < queue_family_count; i++) {
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
        if (present_support) {
            indices.present_family = i;
            indices.presentFamily_has_value = true;
        }

        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
            indices.graphicsFamily_has_value = true;
        }

        if (queue_family_is_complete(indices)) {
            break;
        }
    }
    free(queue_families);
    return indices;
}


typedef struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;

    VkSurfaceFormatKHR* formats;
    uint32_t formatCount;

    VkPresentModeKHR* present_modes;
    uint32_t present_mode_count;
} SwapChainSupportDetails;

SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device) {
    SwapChainSupportDetails details = {0};
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, NULL);
    if (details.formatCount != 0) {
        details.formats = malloc(details.formatCount * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount,  details.formats);
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_mode_count, NULL);
    if (details.present_mode_count != 0) {
        details.present_modes = malloc(details.present_mode_count * sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_mode_count, details.present_modes);
    }

    return details;
}

VkSurfaceFormatKHR choose_swap_surface_format(const VkSurfaceFormatKHR* available_formats, uint32_t format_count) {
    for (size_t i = 0; i < format_count; i++) {
        if (available_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_formats[i];
        }
    }
    return available_formats[0];
}

VkPresentModeKHR choose_swap_present_mode(const VkPresentModeKHR* available_present_modes, uint32_t present_modes_count) {
    for (size_t i = 0; i < present_modes_count; i++) {
        if (available_present_modes[i] == /* VK_PRESENT_MODE_IMMEDIATE_KHR */VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_modes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t clamp(uint32_t value, uint32_t min, uint32_t max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}



VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR* capabilities) {
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    }

    int width, height; 
    glfwGetFramebufferSize(window, &width, &height);
    VkExtent2D actual_extent = {
        .width = (uint32_t) width,
        .height = (uint32_t) height,
    };

    actual_extent.width = clamp(actual_extent.width,
                                   capabilities->minImageExtent.width,
                                   capabilities->maxImageExtent.width);
    actual_extent.height = clamp(actual_extent.height,
                                   capabilities->minImageExtent.height,
                                   capabilities->maxImageExtent.height);

    return actual_extent;

}

bool check_device_extension_support(VkPhysicalDevice device) {
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);

    VkExtensionProperties* availableExtensions = malloc(extension_count * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, availableExtensions);

    for (size_t i = 0; i < device_extensions_count; i++) {
        bool extension_supported = false;

        for (size_t j = 0; j < extension_count; j++) {
            if (strcmp(device_extensions[i], availableExtensions[j].extensionName) == 0) {
                extension_supported = true;
                break;
            }
        }

        if (!extension_supported) {
            fprintf(stderr, "Does not support needed extensions\n");
            return false;
        }
    }

    free(availableExtensions);
    return true;
}

bool is_device_sutable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = find_queue_families(device);

    bool extensions_supported = check_device_extension_support(device);

    bool swap_chain_adequate = false;
    if (extensions_supported) {
        SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device);
        swap_chain_adequate = swap_chain_support.formatCount && swap_chain_support.present_modes; 
    }

    //TODO(memory leak): swap_chain_support *format and *present_modes is never freed I think
    return queue_family_is_complete(indices) && extensions_supported && swap_chain_adequate;
}

int pick_pysical_device() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if (device_count == 0) {
        fprintf(stderr, "Faild to find GPUs with Vulkan support!\n");
        return -1;
    }

    VkPhysicalDevice* devices = malloc(device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &device_count, devices);
    for (uint32_t i = 0; i < device_count; i++) {
        if (is_device_sutable(devices[i])) {
            physical_device = devices[i];
            break;
        }
    }

    if (physical_device == VK_NULL_HANDLE) {
        fprintf(stderr, "Faild to find sutiable GPU!\n");
        return -1;
    }
    return 0;
}

void add_to_unique_queue_families(uint32_t family, uint32_t* unique_queue_families, uint32_t* count) {
    for (size_t i = 0; i < *count; i++) {
        if (unique_queue_families[i] == family) {
            return; 
        }
    }
    unique_queue_families[*count] = family;
    (*count)++;
}

int create_logical_device() {
    QueueFamilyIndices indices = find_queue_families(physical_device);


    uint32_t unique_queue_families[2]; // Can not be larger than 2
    uint32_t unique_queue_families_count = 0;

    add_to_unique_queue_families(indices.graphics_family, unique_queue_families, &unique_queue_families_count);
    add_to_unique_queue_families(indices.present_family, unique_queue_families, &unique_queue_families_count);
    printf("Count: %d\n", unique_queue_families_count);
    for (int i = 0; i < unique_queue_families_count; i++) {
        printf("---- %d\n", unique_queue_families[i]);
    }
    

    VkDeviceQueueCreateInfo queue_create_infos[2]; // Can not be larger than 2
    float queue_priority = 1.0f;
    for (size_t i = 0; i < unique_queue_families_count; i++) {
        VkDeviceQueueCreateInfo queue_create_info = {0};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = unique_queue_families[i];
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;

        queue_create_infos[i] = queue_create_info;
    }

    VkDeviceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = queue_create_infos;
    create_info.queueCreateInfoCount = unique_queue_families_count;

    VkPhysicalDeviceFeatures device_features = {0};
    device_features.wideLines = VK_TRUE;

    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = device_extensions_count;
    create_info.ppEnabledExtensionNames = device_extensions;

    // NOTE: This is set to be compatible with older versions of Vulkan
    // This is not needed in reality
    if (enable_validation_layers) {
        create_info.enabledLayerCount = validation_layer_count;
        create_info.ppEnabledLayerNames = validation_layers;
    }
    else {
        create_info.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physical_device, &create_info, NULL, &device) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create logical device!\n");
        return -1;
    }
    vkGetDeviceQueue(device, indices.graphics_family, 0, &graphics_queue);
    vkGetDeviceQueue(device, indices.present_family, 0, &present_queue);
    return 0;
}

int create_surface() {
    if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
        fprintf(stderr, "Failed to crate window surface!\n");
        return -1;
    }
}


int create_swap_chain() {
    SwapChainSupportDetails swap_chain_support = query_swap_chain_support(physical_device);

    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.formats, swap_chain_support.formatCount);
    VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.present_modes, swap_chain_support.present_mode_count);
    VkExtent2D extent = choose_swap_extent(&swap_chain_support.capabilities);

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }


    VkSwapchainCreateInfoKHR create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = find_queue_families(physical_device);
    uint32_t queue_family_indices[] = {indices.graphics_family, indices.present_family};
    if (indices.graphics_family != indices.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0; // Optional
        create_info.pQueueFamilyIndices = NULL; // Optional
    }

    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &create_info, NULL, &swap_chain) != VK_SUCCESS) {
        fprintf(stderr, "Faild to create swap chain!\n");
        return -1;
    }

    vkGetSwapchainImagesKHR(device, swap_chain, &swap_chain_images.count, NULL);
    swap_chain_images.images = malloc(swap_chain_images.count * sizeof(VkImage));
    vkGetSwapchainImagesKHR(device, swap_chain, &swap_chain_images.count, swap_chain_images.images);

    swap_chain_image_format = surface_format.format;
    swap_chain_extent = extent;

    return 0;
}

int create_image_views() {
    swap_chain_image_views.views = malloc(swap_chain_images.count * sizeof(VkImageView));
    swap_chain_image_views.count = swap_chain_images.count; 


    for (size_t i = 0; i < swap_chain_images.count; i++) {
        VkImageViewCreateInfo create_info = {0};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swap_chain_images.images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swap_chain_image_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;   

        if (vkCreateImageView(device, &create_info, NULL, &swap_chain_image_views.views[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create image views!\n");
            return -1;
        }
    }

    return 0;
}

VkShaderModule create_shader_module(char* code, size_t size) {
    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = size;
    // This is a safe cast to do since code is the conte of a SPIR-V file, i.e. allways aligned to uint32_t
    create_info.pCode = (const uint32_t*)code; 

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, NULL, &shader_module) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create shader module!\n");
    }

    return shader_module;
}

int create_graphics_pipeline() {
    size_t size = 0;
    char* vert_shader_code = readFile("shaders/vert.spv", &size);
    VkShaderModule vert_shader_module = create_shader_module(vert_shader_code, size);
    
    size = 0;
    char* frag_shader_code = readFile("shaders/frag.spv", &size);
    VkShaderModule frag_shader_module = create_shader_module(frag_shader_code, size);
    
    // size = 0;
    // char* geom_shader_code = readFile("shaders/geom.spv", &size);
    // VkShaderModule geom_shader_module = create_shader_module(geom_shader_code, size);

    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {0};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";
    
    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {0};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";

    // VkPipelineShaderStageCreateInfo geom_shader_stage_info = {0};
    // geom_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // geom_shader_stage_info.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
    // geom_shader_stage_info.module = geom_shader_module;
    // geom_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info, /* geom_shader_stage_info */};
    
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkVertexInputBindingDescription binding_description = {0};
    VkVertexInputAttributeDescription attribute_description[ATTRIBUTE_COUNT] = {0};
    get_binding_description(&binding_description);
    get_attribute_descriptions(attribute_description);

    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.vertexAttributeDescriptionCount = ATTRIBUTE_COUNT;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.pVertexAttributeDescriptions = attribute_description;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {0};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    // input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swap_chain_extent.width;
    viewport.height = (float) swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = swap_chain_extent;
    
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {0};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2; // 2 elements
    dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineViewportStateCreateInfo viewport_state = {0};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 2.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending = {0};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY; // Optional
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f; // Optional
    color_blending.blendConstants[1] = 0.0f; // Optional
    color_blending.blendConstants[2] = 0.0f; // Optional
    color_blending.blendConstants[3] = 0.0f; // Optional
    
    VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(device, &pipeline_layout_info, NULL, &pipeline_layout) != VK_SUCCESS) {
        fprintf(stderr, "failed to create pipeline layout\n");
        return -1;
    }

    VkGraphicsPipelineCreateInfo pipeline_info = {0};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = sizeof(shader_stages) / sizeof(shader_stages[0]);
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &graphics_pipeline) != VK_SUCCESS) {
        fprintf(stderr, "Faild to create graphics pipeline\n");
        return -1;
    }

    vkDestroyShaderModule(device, frag_shader_module, NULL);
    vkDestroyShaderModule(device, vert_shader_module, NULL);
    return 0;
}

int create_render_pass() {
    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = swap_chain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;    

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  

    
    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &render_pass_info, NULL, &render_pass) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render pass!\n");
        return -1;
    }
}

int create_framebuffers() {
    swap_chain_framebuffers = malloc(swap_chain_image_views.count * sizeof(VkFramebuffer));

    for (size_t i = 0; i < swap_chain_image_views.count; i++) {
        VkImageView attachments[] = {
            swap_chain_image_views.views[i]
        };

        VkFramebufferCreateInfo framebuffer_info = {0};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = swap_chain_extent.width;
        framebuffer_info.height = swap_chain_extent.height;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(device, &framebuffer_info, NULL, &swap_chain_framebuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create framebuffer!\n");
            return -1;
        }
    }
}

int create_command_pool() {
    QueueFamilyIndices queue_family_indices = find_queue_families(physical_device);

    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_family_indices.graphics_family;

    if (vkCreateCommandPool(device, &pool_info, NULL, &command_pool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool!\n");
        return -1;
    }
    return 0;
}

int create_command_buffers() {
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate command buffers!\n");
        return -1;
    }

    return 0;
}

int record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index) {
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
        fprintf(stderr, "Failed to begin recording command buffer!\n");
        return -1;
    }

    VkRenderPassBeginInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = swap_chain_framebuffers[image_index];
    render_pass_info.renderArea.offset.x = 0;
    render_pass_info.renderArea.offset.y = 0;
    render_pass_info.renderArea.extent = swap_chain_extent;

    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

        VkBuffer vertex_buffers[] = {vertex_buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

        // vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);
        
        VkViewport viewport = {0};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swap_chain_extent.width;
        viewport.height = (float)swap_chain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor = {0};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent = swap_chain_extent;
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    
        uint32_t start_index = 0;
        for (size_t i = 0; i < entities_count; i++) {
            uint32_t shape_count = shapes[entities[i].shape_index].count;
            vkCmdDraw(command_buffer, shape_count, 1, start_index, 0);
            start_index = shape_count;
        }
        // vkCmdDraw(command_buffer, 2, 1, 0, 0);
        // vkCmdDrawIndexed(command_buffer, INDICES_COUNT, 1, 0, 0, 0);
    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to record command bufffer!\n");
        return -1;
    }
    return 0;
}

int create_sync_objects() {
    submit_semaphores = malloc(swap_chain_images.count * sizeof(VkSemaphore));

    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphore_info, NULL, &acquire_semaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fence_info, NULL, &frame_fences[i]) != VK_SUCCESS) {
            fprintf(stderr, "failed to create synchronization objects for a frame!\n");
        }
    }

    for (size_t i = 0; i < swap_chain_images.count; i++) {
        if (vkCreateSemaphore(device, &semaphore_info, NULL, &submit_semaphores[i]) != VK_SUCCESS) {
            fprintf(stderr, "failed to create synchronization objects for a frame!\n");
        }
    }
    
    return 0;
}

void cleanup_swap_chain() {
    //NOTE: framebuffers has the same 'count' as SwapChainImagesView.count
    for (size_t i = 0; i < swap_chain_image_views.count; i++) {
        vkDestroyFramebuffer(device, swap_chain_framebuffers[i], NULL);
    }

    for (size_t i = 0; i < swap_chain_image_views.count; i++) {
        vkDestroyImageView(device, swap_chain_image_views.views[i], NULL);
    }

    vkDestroySwapchainKHR(device, swap_chain, NULL);
}

void recreate_swap_chain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    
    vkDeviceWaitIdle(device);

    cleanup_swap_chain();
    
    create_swap_chain();
    create_image_views();
    create_framebuffers();
}


void cleanup() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, acquire_semaphores[i], NULL);
        vkDestroyFence(device, frame_fences[i], NULL);
    }

    for (size_t i = 0; i < swap_chain_images.count; i++) {
        vkDestroySemaphore(device, submit_semaphores[i], NULL);
    }
    
    cleanup_swap_chain();

    // vkDestroyBuffer(device, index_buffer, NULL);
    vkFreeMemory(device, index_buffer_memory, NULL);

    vkDestroyBuffer(device, vertex_buffer, NULL);
    vkFreeMemory(device, vertex_buffer_memory, NULL);
    
    vkDestroyCommandPool(device, command_pool, NULL);

    vkDestroyPipeline(device, graphics_pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    vkDestroyRenderPass(device, render_pass, NULL);
    
    vkDestroyDevice(device, NULL);

    if (enable_validation_layers) {
        destroy_debug_utils_messenger_EXT(instance, debug_messenger, NULL);
    }

    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    
    glfwDestroyWindow(window);
    glfwTerminate();
}

uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properies = {0};
    vkGetPhysicalDeviceMemoryProperties(physical_device,  &mem_properies);

    for (uint32_t i = 0; i < mem_properies.memoryTypeCount; i++) {
        if (type_filter & (1 << i) && (mem_properies.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
}

int create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer *buffer,
                  VkDeviceMemory *buffer_memory) {
  VkBufferCreateInfo buffer_info = {0};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &buffer_info, NULL, buffer) != VK_SUCCESS) {
    fprintf(stderr, "Failed to crate vertex buffer!\n");
    return -1;
  }

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(device, *buffer, &mem_requirements);

  VkMemoryAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &alloc_info, NULL, buffer_memory) != VK_SUCCESS) {
    fprintf(stderr, "Failed to allocate vertex buffer memory!\n");
    return -1;
  }

  vkBindBufferMemory(device, *buffer, *buffer_memory, 0);

  return 0;
}

void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

        VkBufferCopy copy_region = {0};
        copy_region.dstOffset = 0;
        copy_region.dstOffset = 0;
        copy_region.size = size;
        vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}        

void create_vertex_buffer() {
    VkDeviceSize buffer_size = sizeof(vertices[0]) * VERTEX_COUNT;

    // VkBuffer staging_buffer;
    // VkDeviceMemory staging_buffer_memory;

    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags properties_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    create_buffer(buffer_size, usage_flags, properties_flags, &staging_buffer, &staging_buffer_memory);

    void *data;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
        memcpy(data, vertices, buffer_size);
    vkUnmapMemory(device, staging_buffer_memory);
    
    usage_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    properties_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;;
    create_buffer(buffer_size, usage_flags, properties_flags, &vertex_buffer, &vertex_buffer_memory);
    
    copy_buffer(staging_buffer, vertex_buffer, buffer_size);

    
    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);
}

void create_index_buffer() {
    VkDeviceSize buffer_size = sizeof(indices[0]) * INDICES_COUNT;

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags properties_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    create_buffer(buffer_size, usage_flags, properties_flags, &staging_buffer, &staging_buffer_memory);

    void *data;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
        memcpy(data, indices, buffer_size);
    vkUnmapMemory(device, staging_buffer_memory);
    
    usage_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    properties_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    create_buffer(buffer_size, usage_flags, properties_flags, &index_buffer, &index_buffer_memory);
    
    copy_buffer(staging_buffer, index_buffer, buffer_size);
    
    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);
}

int init_vulkan() {
    int res = 0;
    
    res = create_instance();
    if (res < 0) return res;

    setup_debug_messenger();

    res = create_surface();
    if (res < 0) return res;
    
    res = pick_pysical_device();
    if (res < 0) return res; 
    
    res = create_logical_device();
    if (res < 0) return res;

    res = create_swap_chain();
    if (res < 0) return res;

    res = create_image_views();
    if (res < 0) return res;

    res = create_render_pass();
    if (res < 0) return res;

    res = create_graphics_pipeline();
    if (res < 0) return res;

    res = create_framebuffers();
    if (res < 0) return res;

    res = create_command_pool();
    if (res < 0) return res;

    create_vertex_buffer();
    // create_index_buffer();
    
    res = create_command_buffers();
    if (res < 0) return res;

    res = create_sync_objects();
    if (res < 0) return res;
    
    return 0;
}

void draw_frame() {
    VkFence frame_fence = frame_fences[frame_index];
    vkWaitForFences(device, 1, &frame_fence, VK_TRUE, UINT64_MAX);

    VkSemaphore acquire_semaphore = acquire_semaphores[frame_index];

    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, acquire_semaphore, VK_NULL_HANDLE, &image_index);
    VkSemaphore submit_semaphore = submit_semaphores[image_index];
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swap_chain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "Failed to aquire swap chain image!\n");
    }
    
    vkResetFences(device, 1, &frame_fence);

    VkCommandBuffer command_buffer = command_buffers[frame_index];
    vkResetCommandBuffer(command_buffer, 0);
    record_command_buffer(command_buffer, image_index);

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &acquire_semaphore;
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &submit_semaphore;

    if (vkQueueSubmit(graphics_queue, 1, &submit_info, frame_fence) != VK_SUCCESS) {
        fprintf(stderr, "Failed to submit draw command buffer!\n");
    }

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &submit_semaphore;

    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swap_chain;
    present_info.pImageIndices = &image_index;

    result = vkQueuePresentKHR(present_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
        framebuffer_resized = false;
        recreate_swap_chain();
    }
    else if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to present swap chain image!\n");
  }

    frame_index = (frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

void uppdate_vertex_buffer() {
    VkDeviceSize buffer_size = sizeof(vertices[0]) * VERTEX_COUNT;
    void *data;
    vkMapMemory(device, vertex_buffer_memory, 0, buffer_size, 0, &data);
        memcpy(data, vertices, buffer_size);
    vkUnmapMemory(device, vertex_buffer_memory);
}



void init_game() {
    entities_count = 0;
    // Ship
    entities[0].shape_index = 0;
    entities[0].pos[0] = 0.0f;
    entities[0].pos[1] = 0.0f;
    entities[0].speed[0] = 0.0f;
    entities[0].speed[1] = 0.0f;
    entities_count++;

    entities[1].shape_index = 1;
    entities[1].pos[0] = 0.0f;
    entities[1].pos[1] = 0.0f;
    entities[1].speed[0] = 0.0f;
    entities[1].speed[1] = 0.0f;
    entities_count++;
}

void fill_vertex_buffer() {
    int start_vertex_index = 0;
    for (size_t i = 0; i < entities_count  && i < 100; i++) {
        int shape_index = entities[i].shape_index;

        int shape_count = shapes[shape_index].count;
        for (size_t k = 0; k < shape_count; k++) {
            // printf("vertex %zu, x: %f, y: %f\n", start_vertex_index + k, shapes[shape_index].vertices[k].pos[0], shapes[shape_index].vertices[k].pos[0]);

            vec2 in_vertex;
            float scale = shapes[shape_index].scale;
            float angle = entities[i].rotation;
            in_vertex[0] = shapes[shape_index].vertices[k].pos[0];
            in_vertex[1] = shapes[shape_index].vertices[k].pos[1];

            mat2 scaling_matrix;
            glm_mat2_identity(scaling_matrix);
            scaling_matrix[0][0] = scale;
            scaling_matrix[1][1] = scale;
            
            mat2 rotation_matrix;
            glm_mat2_identity(rotation_matrix);
            rotation_matrix[0][0] = cosf(angle);
            rotation_matrix[0][1] = -sinf(angle);
            rotation_matrix[1][0] = sinf(angle);
            rotation_matrix[1][1] = cosf(angle);
            
            mat2 transform_matrix;
            glm_mat2_mul(rotation_matrix, scaling_matrix, transform_matrix);

            vec2 transformed_vertex;
            glm_mat2_mulv(transform_matrix, in_vertex, transformed_vertex);

            vertices[start_vertex_index + k].pos[0] = transformed_vertex[0] + entities[i].pos[0];
            vertices[start_vertex_index + k].pos[1] = transformed_vertex[1] + entities[i].pos[1];
            vertices[start_vertex_index + k].color[0] = shapes[shape_index].vertices[k].color[0];
            vertices[start_vertex_index + k].color[1] = shapes[shape_index].vertices[k].color[1];
            vertices[start_vertex_index + k].color[2] = shapes[shape_index].vertices[k].color[2];

        }
        start_vertex_index += shape_count;
    }
}


void update_game() {
    float max_speed = 1.1f;
    float max_rotation_speed = 6.0f;
    float player_impulse = 0.1f;
    float player_rotation_impulse = 0.4f;
    float friction = 0.001f;
    float rotation_friction = 0.01f;


    // Player ---
    float rotation_impulse = 0.0f;
    if (key_left) rotation_impulse = player_rotation_impulse ;
    if (key_right) rotation_impulse = player_rotation_impulse  * -1.0f;

    
    if (key_up) {
        float impulse_x = 0.0f;
        float impulse_y = 0.0f;
        impulse_x = -sinf(entities[0].rotation) * player_impulse;
        impulse_y = -cosf(entities[0].rotation) * player_impulse;

        entities[0].speed[0] += impulse_x;
        entities[0].speed[1] += impulse_y;

    }
    
    entities[0].rotation_speed = entities[0].rotation_speed + rotation_impulse; 
    

    // X/Y speed friction
    if (entities[0].speed[0] >= 0) {
        entities[0].speed[0] = fmax(0, entities[0].speed[0] - friction);
        entities[0].speed[0] = fmin(max_speed, entities[0].speed[0]);
    }
    else if (entities[0].speed[0] <= 0) {
        entities[0].speed[0] = fmin(0, entities[0].speed[0] + friction);
        entities[0].speed[0] = fmax(-max_speed, entities[0].speed[0]);
    }

    if (entities[0].speed[1] >= 0) {
        entities[0].speed[1] = fmax(0, entities[0].speed[1] - friction);
        entities[0].speed[1] = fmin(max_speed, entities[0].speed[1]);
    }
    else if (entities[0].speed[1] <= 0) {
        entities[0].speed[1] = fmin(0, entities[0].speed[1] + friction);
        entities[0].speed[1] = fmax(-max_speed, entities[0].speed[1]);
    }

    // Angular speed friction
    if (entities[0].rotation_speed >= 0) {
        entities[0].rotation_speed = fmax(0, entities[0].rotation_speed - rotation_friction);
        entities[0].rotation_speed = fmin(max_rotation_speed, entities[0].rotation_speed);
    }
    else if (entities[0].rotation_speed <= 0) {
        entities[0].rotation_speed = fmin(0, entities[0].rotation_speed + rotation_friction);
        entities[0].rotation_speed = fmax(-max_rotation_speed, entities[0].rotation_speed);
    }

    

    entities[0].pos[0] += entities[0].speed[0] * delta_time;
    entities[0].pos[1] += entities[0].speed[1] * delta_time;
    entities[0].rotation += entities[0].rotation_speed * delta_time;
    if (entities[0].pos[0] > 1.0f) {
        entities[0].pos[0] = -1.0f;
    }
    else if (entities[0].pos[0] < -1.0f) {
        entities[0].pos[0] = 1.0f;
    }
    if (entities[0].pos[1] > 1.0f) {
        entities[0].pos[1] = -1.0f;
    }
    else if (entities[0].pos[1] < -1.0f) {
        entities[0].pos[1] = 1.0f;
    }
    

    
    // Astroids ---
    for (int i = 1; i < entities_count; i++) {
        entities[i].pos[0] += entities[i].speed[0] * delta_time;
        entities[i].pos[1] += entities[i].speed[1] * delta_time;
        entities[i].rotation += entities[i].rotation_speed * delta_time;
    }
    
}

void main_loop() {
    last_time = glfwGetTime();
    double fps = 0.0f;    
    double last_fps_update = 0.0f;

    bool right = true;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double current_time = glfwGetTime();
        delta_time = current_time - last_time;

        // Clamp delta_time. This is needed beacause when resizing main loop does not get executed.
        delta_time = delta_time > max_delta_time ? max_delta_time : delta_time; 
        last_time = current_time;
        frame_count++;
        last_fps_update += delta_time;
        
        if (last_fps_update >= 1.0f) {
            char title[40];
            sprintf(title, "Astroids FPS: %d", frame_count);
            glfwSetWindowTitle(window, title);

            frame_count = 0;
            last_fps_update = 0.0f;
        }

        update_game();
        
        
        fill_vertex_buffer();

        uppdate_vertex_buffer();

        draw_frame();
    }

    vkDeviceWaitIdle(device);
}

int main() {
    int res = 0;
    init_window();
    res = init_vulkan();
    if (res < 0) goto exit;

    init_game();

    main_loop();

exit:
    cleanup();
    return 0;
}
