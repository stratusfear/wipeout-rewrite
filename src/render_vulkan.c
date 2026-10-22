#define VULKAN_H_STATIC_DEFINE
#define VK_NO_PROTOTYPES

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL.h>
#include <vulkan/vulkan.h>
#include <SDL_vulkan.h>
#include "render.h"
#include "platform.h"
#include "utils.h"
#include "system.h"

/* VK_CHECK: assert that a Vulkan call returned VK_SUCCESS. On failure it
 * reports the VkResult and aborts through the project's fatal path (die,
 * which prints file/line). Use only for non-recoverable failures — never
 * for calls whose OUT_OF_DATE / SUBOPTIMAL / TIMEOUT results are handled
 * as normal control flow (swapchain acquire/present). */
#define VK_CHECK(expr) \
    do { \
        VkResult vk_check_result_ = (expr); \
        if (vk_check_result_ != VK_SUCCESS) { \
            die("Vulkan call failed with result %d", (int)vk_check_result_); \
        } \
    } while (0)

/* ------------------------------------------------------------------ */
/* Vulkan loader                                                        */
/* ------------------------------------------------------------------ */

typedef void (*PFN_vkVoidFunction)(void);

/* Global Vulkan bootstrap function pointer — loaded once at start */
static PFN_vkGetInstanceProcAddr vk_global_get_proc_addr = NULL;

static PFN_vkVoidFunction vk_get_proc(const char *name, VkInstance instance) {
    PFN_vkVoidFunction fn = NULL;
    /* Try NULL first (global functions like vkCreateInstance).
     * Then try the provided instance handle (instance functions).
     * This avoids dlsym(RTLD_DEFAULT) which could resolve to a
     * different libvulkan.so instance than SDL loaded. */
    if (vk_global_get_proc_addr) {
        fn = vk_global_get_proc_addr(NULL, name);
        if (!fn && instance) {
            fn = vk_global_get_proc_addr(instance, name);
        }
    }
    return fn;
}

/* Global typed function pointers — required when VK_NO_PROTOTYPES is defined */
static PFN_vkCreateInstance                         vkCreateInstance_fn;
static PFN_vkDestroyInstance                        vkDestroyInstance_fn;
static PFN_vkEnumeratePhysicalDevices               vkEnumeratePhysicalDevices_fn;
static PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties_fn;
static PFN_vkGetPhysicalDeviceProperties            vkGetPhysicalDeviceProperties_fn;
static PFN_vkGetPhysicalDeviceFeatures2              vkGetPhysicalDeviceFeatures2_fn;
static PFN_vkGetPhysicalDeviceSurfaceSupportKHR     vkGetPhysicalDeviceSurfaceSupportKHR_fn;
static PFN_vkGetPhysicalDeviceSurfaceFormatsKHR     vkGetPhysicalDeviceSurfaceFormatsKHR_fn;
static PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR_fn;
static PFN_vkGetPhysicalDeviceMemoryProperties       vkGetPhysicalDeviceMemoryProperties_fn;
static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR_fn;
static PFN_vkCreateDevice                           vkCreateDevice_fn;
static PFN_vkDestroyDevice                          vkDestroyDevice_fn;
static PFN_vkGetDeviceQueue                         vkGetDeviceQueue_fn;
static PFN_vkDestroySurfaceKHR                      vkDestroySurfaceKHR_fn;
static PFN_vkGetDeviceProcAddr                      vkGetDeviceProcAddr_fn;

static PFN_vkCreateDebugUtilsMessengerEXT           vkCreateDebugUtilsMessengerEXT_fn;
static PFN_vkDestroyDebugUtilsMessengerEXT          vkDestroyDebugUtilsMessengerEXT_fn;

/* Swapchain */
static PFN_vkCreateSwapchainKHR                     vkCreateSwapchainKHR_fn;
static PFN_vkDestroySwapchainKHR                    vkDestroySwapchainKHR_fn;
static PFN_vkGetSwapchainImagesKHR                  vkGetSwapchainImagesKHR_fn;
static PFN_vkAcquireNextImageKHR                    vkAcquireNextImageKHR_fn;
static PFN_vkQueuePresentKHR                        vkQueuePresentKHR_fn;

/* Image views */
static PFN_vkCreateImageView                        vkCreateImageView_fn;
static PFN_vkDestroyImageView                       vkDestroyImageView_fn;

/* Sync objects */
static PFN_vkCreateSemaphore                        vkCreateSemaphore_fn;
static PFN_vkDestroySemaphore                       vkDestroySemaphore_fn;
static PFN_vkCreateFence                            vkCreateFence_fn;
static PFN_vkDestroyFence                           vkDestroyFence_fn;
static PFN_vkWaitForFences                          vkWaitForFences_fn;
static PFN_vkWaitSemaphores                         vkWaitSemaphores_fn;

/* Command buffer */
static PFN_vkCreateCommandPool                      vkCreateCommandPool_fn;
static PFN_vkDestroyCommandPool                     vkDestroyCommandPool_fn;
static PFN_vkAllocateCommandBuffers                 vkAllocateCommandBuffers_fn;
static PFN_vkResetCommandBuffer                     vkResetCommandBuffer_fn;
static PFN_vkResetCommandPool                       vkResetCommandPool_fn;
static PFN_vkCmdBindPipeline                        vkCmdBindPipeline_fn;
static PFN_vkCmdSetViewport                         vkCmdSetViewport_fn;
static PFN_vkCmdSetScissor                          vkCmdSetScissor_fn;
static PFN_vkCmdBindVertexBuffers                   vkCmdBindVertexBuffers_fn;
static PFN_vkCmdDraw                                vkCmdDraw_fn;
static PFN_vkCmdSetDepthBias                        vkCmdSetDepthBias_fn;
static PFN_vkCmdSetDepthTestEnable                  vkCmdSetDepthTestEnable_fn;
static PFN_vkCmdSetDepthWriteEnable                 vkCmdSetDepthWriteEnable_fn;
static PFN_vkCmdSetCullMode                         vkCmdSetCullMode_fn;
static PFN_vkCmdBindDescriptorSets                    vkCmdBindDescriptorSets_fn;
static PFN_vkCmdPushConstants                         vkCmdPushConstants_fn;
static PFN_vkCmdBeginRendering                      vkCmdBeginRendering_fn;
static PFN_vkCmdEndRendering                        vkCmdEndRendering_fn;
static PFN_vkCmdPipelineBarrier                     vkCmdPipelineBarrier_fn;
static PFN_vkCmdCopyBufferToImage                   vkCmdCopyBufferToImage_fn;
static PFN_vkCmdBlitImage                          vkCmdBlitImage_fn;
static PFN_vkCmdClearColorImage                    vkCmdClearColorImage_fn;
static PFN_vkBeginCommandBuffer                     vkBeginCommandBuffer_fn;
static PFN_vkEndCommandBuffer                       vkEndCommandBuffer_fn;
static PFN_vkFreeCommandBuffers                     vkFreeCommandBuffers_fn;
static PFN_vkQueueSubmit                            vkQueueSubmit_fn;
static PFN_vkQueueWaitIdle                          vkQueueWaitIdle_fn;

/* Images */
static PFN_vkCreateImage                            vkCreateImage_fn;
static PFN_vkDestroyImage                           vkDestroyImage_fn;
static PFN_vkGetImageMemoryRequirements             vkGetImageMemoryRequirements_fn;
static PFN_vkBindImageMemory                        vkBindImageMemory_fn;


/* Memory allocation */
static PFN_vkAllocateMemory                         vkAllocateMemory_fn;
static PFN_vkFreeMemory                             vkFreeMemory_fn;
static PFN_vkMapMemory                              vkMapMemory_fn;
static PFN_vkUnmapMemory                            vkUnmapMemory_fn;

/* Pipeline */
static PFN_vkCreateGraphicsPipelines                vkCreateGraphicsPipelines_fn;
static PFN_vkDestroyPipeline                        vkDestroyPipeline_fn;
static PFN_vkCreatePipelineLayout                   vkCreatePipelineLayout_fn;
static PFN_vkDestroyPipelineLayout                  vkDestroyPipelineLayout_fn;

/* Shaders */
static PFN_vkCreateShaderModule                     vkCreateShaderModule_fn;
static PFN_vkDestroyShaderModule                    vkDestroyShaderModule_fn;

/* Buffers */
static PFN_vkCreateBuffer                           vkCreateBuffer_fn;
static PFN_vkDestroyBuffer                          vkDestroyBuffer_fn;
static PFN_vkGetBufferMemoryRequirements            vkGetBufferMemoryRequirements_fn;
static PFN_vkBindBufferMemory                       vkBindBufferMemory_fn;

/* Semaphores (signaled via command buffer or queue submit) */

/* Sampler */
static PFN_vkCreateSampler                          vkCreateSampler_fn;
static PFN_vkDestroySampler                         vkDestroySampler_fn;

/* Descriptors */
static PFN_vkCreateDescriptorSetLayout              vkCreateDescriptorSetLayout_fn;
static PFN_vkDestroyDescriptorSetLayout             vkDestroyDescriptorSetLayout_fn;
static PFN_vkCreateDescriptorPool                   vkCreateDescriptorPool_fn;
static PFN_vkDestroyDescriptorPool                  vkDestroyDescriptorPool_fn;
static PFN_vkAllocateDescriptorSets                 vkAllocateDescriptorSets_fn;
static PFN_vkFreeDescriptorSets                     vkFreeDescriptorSets_fn;
static PFN_vkUpdateDescriptorSets                   vkUpdateDescriptorSets_fn;

#define LOAD_FN(ptr, name, inst) \
    ptr = (typeof(ptr))vk_get_proc(#name, inst); \
    if (!(ptr)) { \
        die("Failed to load Vulkan function %s", #name); \
    }
#define LOAD_FN_NULL(ptr, name) LOAD_FN(ptr, name, VK_NULL_HANDLE)
#define LOAD_FN_INSTANCE(ptr, name) LOAD_FN(ptr, name, vulkan_state.instance)

/* ------------------------------------------------------------------ */
/* Device function loading (via vkGetDeviceProcAddr)                  */
/* ------------------------------------------------------------------ */

/* Load a device command via vkGetDeviceProcAddr, falling back to
 * vkGetInstanceProcAddr when it returns NULL. Some drivers and layers
 * (e.g. the Khronos validation layer) return NULL from
 * vkGetDeviceProcAddr for valid commands they do not intercept; the
 * instance-level lookup returns the loader trampoline which chains
 * through the layer stack correctly. */
static PFN_vkVoidFunction load_device_fn(VkDevice device, VkInstance instance, const char *name) {
    PFN_vkVoidFunction fn = (PFN_vkVoidFunction)vkGetDeviceProcAddr_fn(device, name);
    if (!fn) fn = vk_get_proc(name, instance);
    return fn;
}

static bool load_device_functions(VkDevice device, VkInstance instance) {
    /* Image views */
    vkCreateImageView_fn = (PFN_vkCreateImageView)load_device_fn(device, instance, "vkCreateImageView");
    if (!vkCreateImageView_fn) { fprintf(stderr, "Failed to load vkCreateImageView\n"); return false; }
    vkDestroyImageView_fn = (PFN_vkDestroyImageView)load_device_fn(device, instance, "vkDestroyImageView");
    if (!vkDestroyImageView_fn) { fprintf(stderr, "Failed to load vkDestroyImageView\n"); return false; }

    /* Sync objects */
    vkCreateSemaphore_fn = (PFN_vkCreateSemaphore)load_device_fn(device, instance, "vkCreateSemaphore");
    if (!vkCreateSemaphore_fn) { fprintf(stderr, "Failed to load vkCreateSemaphore\n"); return false; }
    vkDestroySemaphore_fn = (PFN_vkDestroySemaphore)load_device_fn(device, instance, "vkDestroySemaphore");
    if (!vkDestroySemaphore_fn) { fprintf(stderr, "Failed to load vkDestroySemaphore\n"); return false; }
    vkCreateFence_fn = (PFN_vkCreateFence)load_device_fn(device, instance, "vkCreateFence");
    if (!vkCreateFence_fn) { fprintf(stderr, "Failed to load vkCreateFence\n"); return false; }
    vkDestroyFence_fn = (PFN_vkDestroyFence)load_device_fn(device, instance, "vkDestroyFence");
    if (!vkDestroyFence_fn) { fprintf(stderr, "Failed to load vkDestroyFence\n"); return false; }
    vkWaitForFences_fn = (PFN_vkWaitForFences)load_device_fn(device, instance, "vkWaitForFences");
    if (!vkWaitForFences_fn) { fprintf(stderr, "Failed to load vkWaitForFences\n"); return false; }
    vkWaitSemaphores_fn = (PFN_vkWaitSemaphores)load_device_fn(device, instance, "vkWaitSemaphores");
    if (!vkWaitSemaphores_fn) { fprintf(stderr, "Failed to load vkWaitSemaphores\n"); return false; }

    /* Command buffer */
    vkCreateCommandPool_fn = (PFN_vkCreateCommandPool)load_device_fn(device, instance, "vkCreateCommandPool");
    if (!vkCreateCommandPool_fn) { fprintf(stderr, "Failed to load vkCreateCommandPool\n"); return false; }
    vkDestroyCommandPool_fn = (PFN_vkDestroyCommandPool)load_device_fn(device, instance, "vkDestroyCommandPool");
    if (!vkDestroyCommandPool_fn) { fprintf(stderr, "Failed to load vkDestroyCommandPool\n"); return false; }
    vkAllocateCommandBuffers_fn = (PFN_vkAllocateCommandBuffers)load_device_fn(device, instance, "vkAllocateCommandBuffers");
    if (!vkAllocateCommandBuffers_fn) { fprintf(stderr, "Failed to load vkAllocateCommandBuffers\n"); return false; }
    vkResetCommandBuffer_fn = (PFN_vkResetCommandBuffer)load_device_fn(device, instance, "vkResetCommandBuffer");
    if (!vkResetCommandBuffer_fn) { fprintf(stderr, "Failed to load vkResetCommandBuffer\n"); return false; }
    vkResetCommandPool_fn = (PFN_vkResetCommandPool)load_device_fn(device, instance, "vkResetCommandPool");
    if (!vkResetCommandPool_fn) { fprintf(stderr, "Failed to load vkResetCommandPool\n"); return false; }
    vkCmdBindPipeline_fn = (PFN_vkCmdBindPipeline)load_device_fn(device, instance, "vkCmdBindPipeline");
    if (!vkCmdBindPipeline_fn) { fprintf(stderr, "Failed to load vkCmdBindPipeline\n"); return false; }
    vkCmdSetViewport_fn = (PFN_vkCmdSetViewport)load_device_fn(device, instance, "vkCmdSetViewport");
    if (!vkCmdSetViewport_fn) { fprintf(stderr, "Failed to load vkCmdSetViewport\n"); return false; }
    vkCmdSetScissor_fn = (PFN_vkCmdSetScissor)load_device_fn(device, instance, "vkCmdSetScissor");
    if (!vkCmdSetScissor_fn) { fprintf(stderr, "Failed to load vkCmdSetScissor\n"); return false; }
    vkCmdBindVertexBuffers_fn = (PFN_vkCmdBindVertexBuffers)load_device_fn(device, instance, "vkCmdBindVertexBuffers");
    if (!vkCmdBindVertexBuffers_fn) { fprintf(stderr, "Failed to load vkCmdBindVertexBuffers\n"); return false; }
    vkCmdDraw_fn = (PFN_vkCmdDraw)load_device_fn(device, instance, "vkCmdDraw");
    if (!vkCmdDraw_fn) { fprintf(stderr, "Failed to load vkCmdDraw\n"); return false; }
    vkCmdSetDepthBias_fn = (PFN_vkCmdSetDepthBias)load_device_fn(device, instance, "vkCmdSetDepthBias");
    if (!vkCmdSetDepthBias_fn) { fprintf(stderr, "Failed to load vkCmdSetDepthBias\n"); return false; }
    vkCmdSetDepthTestEnable_fn = (PFN_vkCmdSetDepthTestEnable)load_device_fn(device, instance, "vkCmdSetDepthTestEnable");
    if (!vkCmdSetDepthTestEnable_fn) { fprintf(stderr, "Failed to load vkCmdSetDepthTestEnable\n"); return false; }
    vkCmdSetDepthWriteEnable_fn = (PFN_vkCmdSetDepthWriteEnable)load_device_fn(device, instance, "vkCmdSetDepthWriteEnable");
    if (!vkCmdSetDepthWriteEnable_fn) { fprintf(stderr, "Failed to load vkCmdSetDepthWriteEnable\n"); return false; }
    vkCmdSetCullMode_fn = (PFN_vkCmdSetCullMode)load_device_fn(device, instance, "vkCmdSetCullMode");
    if (!vkCmdSetCullMode_fn) { fprintf(stderr, "Failed to load vkCmdSetCullMode\n"); return false; }
    vkCmdBindDescriptorSets_fn = (PFN_vkCmdBindDescriptorSets)load_device_fn(device, instance, "vkCmdBindDescriptorSets");
    if (!vkCmdBindDescriptorSets_fn) { fprintf(stderr, "Failed to load vkCmdBindDescriptorSets\n"); return false; }
    vkCmdPushConstants_fn = (PFN_vkCmdPushConstants)load_device_fn(device, instance, "vkCmdPushConstants");
    if (!vkCmdPushConstants_fn) { fprintf(stderr, "Failed to load vkCmdPushConstants\n"); return false; }
    vkCmdBeginRendering_fn = (PFN_vkCmdBeginRendering)load_device_fn(device, instance, "vkCmdBeginRendering");
    if (!vkCmdBeginRendering_fn) { fprintf(stderr, "Failed to load vkCmdBeginRendering\n"); return false; }
    vkCmdEndRendering_fn = (PFN_vkCmdEndRendering)load_device_fn(device, instance, "vkCmdEndRendering");
    if (!vkCmdEndRendering_fn) { fprintf(stderr, "Failed to load vkCmdEndRendering\n"); return false; }
    vkCmdPipelineBarrier_fn = (PFN_vkCmdPipelineBarrier)load_device_fn(device, instance, "vkCmdPipelineBarrier");
    if (!vkCmdPipelineBarrier_fn) { fprintf(stderr, "Failed to load vkCmdPipelineBarrier\n"); return false; }
    vkCmdCopyBufferToImage_fn = (PFN_vkCmdCopyBufferToImage)load_device_fn(device, instance, "vkCmdCopyBufferToImage");
    vkCmdBlitImage_fn = (PFN_vkCmdBlitImage)load_device_fn(device, instance, "vkCmdBlitImage");
    vkCmdClearColorImage_fn = (PFN_vkCmdClearColorImage)load_device_fn(device, instance, "vkCmdClearColorImage");
    if (!vkCmdCopyBufferToImage_fn) { fprintf(stderr, "Failed to load vkCmdCopyBufferToImage\n"); return false; }
    vkBeginCommandBuffer_fn = (PFN_vkBeginCommandBuffer)load_device_fn(device, instance, "vkBeginCommandBuffer");
    if (!vkBeginCommandBuffer_fn) { fprintf(stderr, "Failed to load vkBeginCommandBuffer\n"); return false; }
    vkEndCommandBuffer_fn = (PFN_vkEndCommandBuffer)load_device_fn(device, instance, "vkEndCommandBuffer");
    if (!vkEndCommandBuffer_fn) { fprintf(stderr, "Failed to load vkEndCommandBuffer\n"); return false; }
    vkFreeCommandBuffers_fn = (PFN_vkFreeCommandBuffers)load_device_fn(device, instance, "vkFreeCommandBuffers");
    if (!vkFreeCommandBuffers_fn) { fprintf(stderr, "Failed to load vkFreeCommandBuffers\n"); return false; }
    vkQueueSubmit_fn = (PFN_vkQueueSubmit)load_device_fn(device, instance, "vkQueueSubmit");
    if (!vkQueueSubmit_fn) { fprintf(stderr, "Failed to load vkQueueSubmit\n"); return false; }
    vkQueueWaitIdle_fn = (PFN_vkQueueWaitIdle)load_device_fn(device, instance, "vkQueueWaitIdle");
    if (!vkQueueWaitIdle_fn) { fprintf(stderr, "Failed to load vkQueueWaitIdle\n"); return false; }

    /* Images */
    vkCreateImage_fn = (PFN_vkCreateImage)load_device_fn(device, instance, "vkCreateImage");
    if (!vkCreateImage_fn) { fprintf(stderr, "Failed to load vkCreateImage\n"); return false; }
    vkDestroyImage_fn = (PFN_vkDestroyImage)load_device_fn(device, instance, "vkDestroyImage");
    if (!vkDestroyImage_fn) { fprintf(stderr, "Failed to load vkDestroyImage\n"); return false; }
    vkGetImageMemoryRequirements_fn = (PFN_vkGetImageMemoryRequirements)load_device_fn(device, instance, "vkGetImageMemoryRequirements");
    if (!vkGetImageMemoryRequirements_fn) { fprintf(stderr, "Failed to load vkGetImageMemoryRequirements\n"); return false; }
    vkBindImageMemory_fn = (PFN_vkBindImageMemory)load_device_fn(device, instance, "vkBindImageMemory");
    if (!vkBindImageMemory_fn) { fprintf(stderr, "Failed to load vkBindImageMemory\n"); return false; }

    /* Memory allocation */
    vkAllocateMemory_fn = (PFN_vkAllocateMemory)load_device_fn(device, instance, "vkAllocateMemory");
    if (!vkAllocateMemory_fn) { fprintf(stderr, "Failed to load vkAllocateMemory\n"); return false; }
    vkFreeMemory_fn = (PFN_vkFreeMemory)load_device_fn(device, instance, "vkFreeMemory");
    if (!vkFreeMemory_fn) { fprintf(stderr, "Failed to load vkFreeMemory\n"); return false; }
    vkMapMemory_fn = (PFN_vkMapMemory)load_device_fn(device, instance, "vkMapMemory");
    if (!vkMapMemory_fn) { fprintf(stderr, "Failed to load vkMapMemory\n"); return false; }
    vkUnmapMemory_fn = (PFN_vkUnmapMemory)load_device_fn(device, instance, "vkUnmapMemory");
    if (!vkUnmapMemory_fn) { fprintf(stderr, "Failed to load vkUnmapMemory\n"); return false; }

    /* Render pass */

    /* Pipeline */
    vkCreateGraphicsPipelines_fn = (PFN_vkCreateGraphicsPipelines)load_device_fn(device, instance, "vkCreateGraphicsPipelines");
    if (!vkCreateGraphicsPipelines_fn) { fprintf(stderr, "Failed to load vkCreateGraphicsPipelines\n"); return false; }
    vkDestroyPipeline_fn = (PFN_vkDestroyPipeline)load_device_fn(device, instance, "vkDestroyPipeline");
    if (!vkDestroyPipeline_fn) { fprintf(stderr, "Failed to load vkDestroyPipeline\n"); return false; }
    vkCreatePipelineLayout_fn = (PFN_vkCreatePipelineLayout)load_device_fn(device, instance, "vkCreatePipelineLayout");
    if (!vkCreatePipelineLayout_fn) { fprintf(stderr, "Failed to load vkCreatePipelineLayout\n"); return false; }
    vkDestroyPipelineLayout_fn = (PFN_vkDestroyPipelineLayout)load_device_fn(device, instance, "vkDestroyPipelineLayout");
    if (!vkDestroyPipelineLayout_fn) { fprintf(stderr, "Failed to load vkDestroyPipelineLayout\n"); return false; }

    /* Shaders */
    vkCreateShaderModule_fn = (PFN_vkCreateShaderModule)load_device_fn(device, instance, "vkCreateShaderModule");
    if (!vkCreateShaderModule_fn) { fprintf(stderr, "Failed to load vkCreateShaderModule\n"); return false; }
    vkDestroyShaderModule_fn = (PFN_vkDestroyShaderModule)load_device_fn(device, instance, "vkDestroyShaderModule");
    if (!vkDestroyShaderModule_fn) { fprintf(stderr, "Failed to load vkDestroyShaderModule\n"); return false; }

    /* Buffers */
    vkCreateBuffer_fn = (PFN_vkCreateBuffer)load_device_fn(device, instance, "vkCreateBuffer");
    if (!vkCreateBuffer_fn) { fprintf(stderr, "Failed to load vkCreateBuffer\n"); return false; }
    vkDestroyBuffer_fn = (PFN_vkDestroyBuffer)load_device_fn(device, instance, "vkDestroyBuffer");
    if (!vkDestroyBuffer_fn) { fprintf(stderr, "Failed to load vkDestroyBuffer\n"); return false; }
    vkGetBufferMemoryRequirements_fn = (PFN_vkGetBufferMemoryRequirements)load_device_fn(device, instance, "vkGetBufferMemoryRequirements");
    if (!vkGetBufferMemoryRequirements_fn) { fprintf(stderr, "Failed to load vkGetBufferMemoryRequirements\n"); return false; }
    vkBindBufferMemory_fn = (PFN_vkBindBufferMemory)load_device_fn(device, instance, "vkBindBufferMemory");
    if (!vkBindBufferMemory_fn) { fprintf(stderr, "Failed to load vkBindBufferMemory\n"); return false; }

    /* Sampler */
    vkCreateSampler_fn = (PFN_vkCreateSampler)load_device_fn(device, instance, "vkCreateSampler");
    if (!vkCreateSampler_fn) { fprintf(stderr, "Failed to load vkCreateSampler\n"); return false; }
    vkDestroySampler_fn = (PFN_vkDestroySampler)load_device_fn(device, instance, "vkDestroySampler");
    if (!vkDestroySampler_fn) { fprintf(stderr, "Failed to load vkDestroySampler\n"); return false; }

    /* Descriptors */
    vkCreateDescriptorSetLayout_fn = (PFN_vkCreateDescriptorSetLayout)load_device_fn(device, instance, "vkCreateDescriptorSetLayout");
    if (!vkCreateDescriptorSetLayout_fn) { fprintf(stderr, "Failed to load vkCreateDescriptorSetLayout\n"); return false; }
    vkDestroyDescriptorSetLayout_fn = (PFN_vkDestroyDescriptorSetLayout)load_device_fn(device, instance, "vkDestroyDescriptorSetLayout");
    if (!vkDestroyDescriptorSetLayout_fn) { fprintf(stderr, "Failed to load vkDestroyDescriptorSetLayout\n"); return false; }
    vkCreateDescriptorPool_fn = (PFN_vkCreateDescriptorPool)load_device_fn(device, instance, "vkCreateDescriptorPool");
    if (!vkCreateDescriptorPool_fn) { fprintf(stderr, "Failed to load vkCreateDescriptorPool\n"); return false; }
    vkDestroyDescriptorPool_fn = (PFN_vkDestroyDescriptorPool)load_device_fn(device, instance, "vkDestroyDescriptorPool");
    if (!vkDestroyDescriptorPool_fn) { fprintf(stderr, "Failed to load vkDestroyDescriptorPool\n"); return false; }
    vkAllocateDescriptorSets_fn = (PFN_vkAllocateDescriptorSets)load_device_fn(device, instance, "vkAllocateDescriptorSets");
    if (!vkAllocateDescriptorSets_fn) { fprintf(stderr, "Failed to load vkAllocateDescriptorSets\n"); return false; }
    vkFreeDescriptorSets_fn = (PFN_vkFreeDescriptorSets)load_device_fn(device, instance, "vkFreeDescriptorSets");
    if (!vkFreeDescriptorSets_fn) { fprintf(stderr, "Failed to load vkFreeDescriptorSets\n"); return false; }
    vkUpdateDescriptorSets_fn = (PFN_vkUpdateDescriptorSets)load_device_fn(device, instance, "vkUpdateDescriptorSets");
    if (!vkUpdateDescriptorSets_fn) { fprintf(stderr, "Failed to load vkUpdateDescriptorSets\n"); return false; }
    return true;
}

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

static VkDebugUtilsMessengerEXT debug_callback = VK_NULL_HANDLE;

/* Forward declarations */
static bool create_swapchain(VkSwapchainKHR old_swapchain);
static void destroy_swapchain(void);
static void destroy_swapchain_resources(VkSwapchainKHR swapchain, VkImage *images,
                                        VkImageView *views, uint32_t count);
static void destroy_backbuffer(void);
static bool create_backbuffer(void);
static bool create_vertex_buffer(void);
static bool create_atlas(void);
static void destroy_atlas(void);
static bool create_pipelines(void);
static void destroy_pipelines(void);
static void upload_vertex_buffer(uint32_t frame_index, uint32_t offset, uint32_t count);
static void render_flush(void);
static uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties);

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback_fn(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data
) {
    (void)message_types;
    (void)user_data;
    const char *sev = severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? "ERROR" :
                      severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ? "WARN" : "INFO";
    const char *prefix = callback_data->pMessageIdName ? callback_data->pMessageIdName : "";
    fprintf(stderr, "[VULKAN %s] %s: %s\n", sev, prefix, callback_data->pMessage);
    return VK_FALSE;
}

/* ------------------------------------------------------------------ */
/* Renderer state                                                       */
/* ------------------------------------------------------------------ */

#define MAX_FRAMES_IN_FLIGHT 2
#define TEXTURES_MAX 1024
/* Fixed-size per-swapchain-image arrays (render_complete_semaphores). A
 * driver may return more images than the requested minImageCount, so
 * create_swapchain fails if the actual count exceeds this. */
#define MAX_SWAPCHAIN_IMAGES 16

typedef struct {
    vec2i_t offset;
    vec2i_t size;
} vulkan_texture_t;

/* Pending upload type for deferred texture uploads */
typedef struct {
    uint32_t width;
    uint32_t height;
    rgba_t *pixels;
    uint32_t grid_x;
    uint32_t grid_y;
    uint32_t texture_index;
} pending_upload_t;

static struct vulkan_state {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue graphics_queue;
    uint32_t graphics_queue_family;
    uint32_t present_queue_family;
    vec2i_t screen_size;

    /* Swapchain */
    VkSwapchainKHR swapchain;
    VkFormat swapchain_format;
    VkColorSpaceKHR surface_color_space;
    VkExtent2D swapchain_extent;
    uint32_t swapchain_image_count;
    VkImage *swapchain_images;
    VkImageView *swapchain_image_views;

    /* Sync objects:
     * - image_acquired_semaphores[] indexed by frame slot (per-frame). Binary —
     *   VUID-VkAcquireNextImageInfoKHR-semaphore-03266 requires
     *   VK_SEMAPHORE_TYPE_BINARY for swapchain acquire.
     * - render_complete_semaphores[] indexed by swapchain image (per-image).
     *   Binary — vkQueuePresentKHR only accepts binary semaphores.
     * - timeline_semaphore replaces the old per-frame fences: each submit
     *   signals an explicit monotonic value (slot_fence_value[slot]) and
     *   prepare() CPU-waits on that value for backpressure. Initial value is
     *   0, so the first wait per slot is a no-op. */
    VkSemaphore image_acquired_semaphores[MAX_FRAMES_IN_FLIGHT]; /* One per frame slot */
    VkSemaphore render_complete_semaphores[MAX_SWAPCHAIN_IMAGES]; /* One per swapchain image */
    VkSemaphore timeline_semaphore; /* TIMELINE type, replaces frame fences */
    uint64_t timeline_next_value; /* Next value to signal on submit (monotonic) */
    uint64_t slot_fence_value[MAX_FRAMES_IN_FLIGHT]; /* Value each slot's last submit will signal */
    VkCommandPool command_pools[MAX_FRAMES_IN_FLIGHT];
    VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];
    VkCommandBuffer command_buffer; /* Working pointer to current frame's CB */
    VkCommandPool temp_command_pool; /* One-shot pool for init-time transitions */
    uint32_t frame_index; /* Wraps 0..MAX_FRAMES_IN_FLIGHT-1 */
    uint32_t current_swapchain_image;

    /* Backbuffer (off-screen rendering target) */
    VkImage backbuffer_image;
    VkDeviceMemory backbuffer_image_memory;
    VkImageView backbuffer_image_view;
    VkImage backbuffer_depth_image;
    VkDeviceMemory backbuffer_depth_image_memory;
    VkImageView backbuffer_depth_image_view;
    vec2i_t backbuffer_size;
    render_resolution_t render_res;

    /* Deferred-resize flags. Set by render_set_screen_size /
     * render_set_resolution (which may be called mid-frame from game_update
     * or the SDL event pump) but only APPLIED in render_frame_prepare, after
     * vkQueueWaitIdle — the only point where destroying the old swapchain /
     * backbuffer is safe (no in-flight command buffer references them). */
    int swapchain_recreate_pending;
    int backbuffer_recreate_pending;

    /* Pipeline state */
    VkPipelineLayout pipeline_layout;
    /* Game-pass pipelines: one per blend mode. Depth test/write and cull mode
     * are dynamic state (extendedDynamicState, Vulkan 1.3 core) — issued
     * per-flush via vkCmdSet* after the pipeline bind in render_flush. Front
     * face is always CLOCKWISE in the game pass (see render_flush), so it is
     * not a dimension at all. */
    VkPipeline game_pipelines[2]; /* [blend_mode] */
    VkPipeline post_pipelines[NUM_RENDER_POST_EFFECTS];
    render_post_effect_t current_post_effect;

    /* Vertex buffer (triangles) — per-frame to avoid CPU-GPU race on the
     * single buffer with double-buffered frames. */
    VkBuffer vertex_buffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory vertex_buffer_memories[MAX_FRAMES_IN_FLIGHT];
    void *vertex_buffer_ptrs[MAX_FRAMES_IN_FLIGHT]; /* persistent map, whole buffer */
    uint32_t vertex_count;
    uint32_t vertex_flushed; /* vertices already uploaded+drawn this frame */
    vertex_t *vertex_data; /* host-side staging */
    /* Per-frame triangle capacity. The buffer is cumulative (a frame's draws
     * all reference one growing region that stays live until the frame's
     * submit completes), so this bounds the WHOLE frame, not a single flush —
     * unlike the GL renderer, whose 2048-triangle limit applies per flush.
     * Sized 8x the old value so a busy frame that draws fine in GL can't
     * overflow it; the drop path below still warns if it ever does. */
    #define RENDER_TRIS_BUFFER_CAPACITY 131072

    /* Reusable staging buffer for texture uploads (avoids per-upload 16MB alloc) */
    VkBuffer upload_staging_buffer;
    VkDeviceMemory upload_staging_memory;
    void *upload_staging_mapped; /* permanently mapped host pointer */
    uint32_t upload_staging_size; /* max atlas size in bytes */

    /* Persistent one-shot command pool for texture uploads (avoids per-upload pool churn) */
    VkCommandPool upload_command_pool;
    VkCommandBuffer upload_command_buffer;

    /* Texture atlas */
    VkImage atlas_image;
    VkDeviceMemory atlas_image_memory;
    VkImageView atlas_image_view;
    VkSampler atlas_sampler;
    VkSampler atlas_nearest_sampler;
    VkImageLayout atlas_layout; /* track atlas image layout for lazy transition */
    float max_anisotropy; /* hardware max sampler anisotropy (sampler field clamps at 16) */
    VkImageLayout backbuffer_layout; /* track backbuffer image layout across frames */
    VkImageLayout backbuffer_depth_layout; /* track backbuffer depth image layout across frames */
    #define ATLAS_SIZE 64

    /* Descriptor infrastructure */
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet atlas_descriptor_set;
    VkDescriptorSet backbuffer_descriptor_set;

    /* Full-screen quad for post-process pass */
    VkBuffer quad_vertex_buffer;
    VkDeviceMemory quad_vertex_buffer_memory;
    #define QUAD_VERTEX_COUNT 6

    /* Sampler for backbuffer (used in post pass) */
    VkSampler backbuffer_sampler;

    /* State tracking */
    render_blend_mode_t current_blend_mode;
    mat4_t current_projection;
    mat4_t current_view_mat;
    mat4_t current_model_mat;
    bool current_depth_test;
    bool current_depth_write;
    float current_depth_bias;
    bool current_cull_backface;
    /* Last EDS values issued to the GPU for the in-flight command buffer.
     * Dynamic state is per-command-buffer and resets when the CB is re-begun
     * in render_frame_prepare, so dyn_state_issued is reset there and the
     * first game-pipeline bind of each frame re-issues all three. */
    bool dyn_state_issued;
    bool dyn_depth_test;
    bool dyn_depth_write;
    bool dyn_cull_backface;
    bool current_is_3d;
    vec2_t current_screen_position;

    /* Projection matrices */
    mat4_t projection_mat_3d;
    mat4_t projection_mat_2d;

    /* Texture management */
    vulkan_texture_t textures[TEXTURES_MAX];
    uint32_t textures_len;
    uint32_t atlas_map[ATLAS_SIZE]; /* skyline allocation (mirrors GL reference) */

    /* Pending texture uploads — deferred until the first frame because the
     * per-frame command buffer is not yet allocated during game_init (it is
     * created lazily in render_frame_prepare). Upload requests issued before
     * that point are queued here and flushed at the start of frame 0. */
    #define PENDING_UPLOADS_MAX TEXTURES_MAX
    pending_upload_t pending_uploads[PENDING_UPLOADS_MAX];
    uint32_t pending_uploads_len;

    #define ATLAS_GRID 32
    #define ATLAS_BORDER 16
    #define ATLAS_MIP_LEVELS 12 /* 2048 -> 1x1 mip chain (mirrors GL glGenerateMipmap) */

} vulkan_state;

/* Billboard rotation for render_push_sprite: rebuilt from camera angles in
 * render_set_view so sprite quads always face the camera (mirrors the static
 * sprite_mat in render_gl.c). Identity-init so sprites drawn before any 3D
 * view still render, as in GL. */
static mat4_t sprite_mat = mat4_identity();

static render_stats_t running_stats = {0};
// Previous frame's total stats (copy of running_stats in render_frame_end())
static render_stats_t end_stats = {0};

/* Forward declaration */


/* ------------------------------------------------------------------ */
/* Helper: compute MVP = projection * view * model                      */
/* ------------------------------------------------------------------ */
static mat4_t compute_mvp(void) {
    mat4_t mvp = vulkan_state.current_view_mat;
    mat4_t tmp = vulkan_state.current_model_mat;

    /* Multiply: mvp = view * model */
    mat4_t result = mat4_identity();
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            float sum = 0;
            for (int k = 0; k < 4; k++)
                sum += mvp.m[k * 4 + j] * tmp.m[i * 4 + k];
            result.m[i * 4 + j] = sum;
        }

    /* Multiply: mvp = projection * (view * model) */
    mat4_t proj = vulkan_state.current_is_3d ? vulkan_state.projection_mat_3d : vulkan_state.projection_mat_2d;
    mat4_t final = mat4_identity();
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            float sum = 0;
            for (int k = 0; k < 4; k++)
                sum += proj.m[k * 4 + j] * result.m[i * 4 + k];
            final.m[i * 4 + j] = sum;
        }

    return final;
}

/* ------------------------------------------------------------------ */
/* Projection matrix setup (matches GL renderer)                        */
/* ------------------------------------------------------------------ */

static mat4_t render_setup_3d_projection_mat(vec2i_t size) {
    /* Wipeout has a horizontal fov of 90deg, but we want the fov to be fixed
     * for the vertical axis, so that widescreen displays just have a wider
     * view. For the original 4/3 aspect ratio this equates to a vertical fov
     * of 73.75deg. */
    float aspect = (float)size.x / (float)size.y;
    float fov = (73.75f / 180.0f) * 3.14159265358f;
    float f = 1.0f / tanf(fov / 2.0f);
    float near_plane = 16.0f;
    float far_plane = RENDER_FADEOUT_FAR;
    float nf = 1.0f / (near_plane - far_plane);
    return mat4(
        f / aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (far_plane + near_plane) * nf, -1,
        0, 0, 2.0f * far_plane * near_plane * nf, 0
    );
}

static mat4_t render_setup_2d_projection_mat(vec2i_t size) {
    float near = -1.0f;
    float far = 1.0f;
    float left = 0.0f;
    float right = (float)size.x;
    float bottom = (float)size.y;
    float top = 0.0f;
    float lr = 1.0f / (left - right);
    float bt = 1.0f / (bottom - top);
    float nf = 1.0f / (near - far);
    return mat4(
        -2.0f * lr,  0,  0,  0,
        0,  -2.0f * bt,  0,  0,
        0,        0,  2.0f * nf,    0,
        (left + right) * lr, (top + bottom) * bt, (far + near) * nf, 1
    );
}

/* ------------------------------------------------------------------ */
/* Public API (stubs for non-core functions)                            */
/* ------------------------------------------------------------------ */

uint16_t RENDER_NO_TEXTURE;

void render_init(vec2i_t screen_size) {
    vulkan_state.screen_size = screen_size;

    /* Load Vulkan entry point from SDL, which is linked against the same
     * libvulkan as the rest of the process. */
    vk_global_get_proc_addr = SDL_Vulkan_GetVkGetInstanceProcAddr();
    if (vk_global_get_proc_addr == NULL) {
        die("Failed to get vkGetInstanceProcAddr (SDL: %s)", SDL_GetError());
    }

    /* -- Phase 1: Load global functions (resolvable with NULL instance) -- */
    LOAD_FN_NULL(vkCreateInstance_fn, vkCreateInstance)

    /* -- Phase 2: Create VkInstance -- */
    uint32_t sdl_ext_count = 0;
    SDL_Vulkan_GetInstanceExtensions((SDL_Window*)platform_get_window_handle(), &sdl_ext_count, NULL);

    const char **sdl_exts = NULL;
    if (sdl_ext_count > 0) {
        sdl_exts = (const char **)calloc(sdl_ext_count, sizeof(const char *));
        if (!sdl_exts) { die("Failed to allocate SDL extension list"); }
        SDL_Vulkan_GetInstanceExtensions((SDL_Window*)platform_get_window_handle(), &sdl_ext_count, sdl_exts);
    }

    uint32_t total_ext_count = sdl_ext_count;
    const char **exts = NULL;
    int want_debug = 0;

#if defined(NDEBUG)
    want_debug = 0;
#else
    want_debug = 1;
#endif

    if (want_debug) {
        total_ext_count++;
    }

    exts = (const char **)calloc(total_ext_count, sizeof(const char *));
    if (!exts) { die("Failed to allocate extension list"); }

    memcpy(exts, sdl_exts, sdl_ext_count * sizeof(const char *));

    VkDebugUtilsMessengerCreateInfoEXT debug_ci = {0};
    if (want_debug) {
        exts[sdl_ext_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        debug_ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        debug_ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_ci.pfnUserCallback = debug_callback_fn;
    }

    VkInstanceCreateInfo instance_ci = {0};
    instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_ci.enabledExtensionCount = total_ext_count;
    instance_ci.ppEnabledExtensionNames = exts;
    instance_ci.pNext = want_debug ? &debug_ci : NULL;

    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.apiVersion = VK_API_VERSION_1_3;
    app_info.pApplicationName = "wipeout-rewrite";
    app_info.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    app_info.pEngineName = "wipeout-rewrite";
    app_info.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    instance_ci.pApplicationInfo = &app_info;

    VkResult result = vkCreateInstance_fn(&instance_ci, NULL, &vulkan_state.instance);
    free(sdl_exts);
    free(exts);

    if (result != VK_SUCCESS) {
        die("vkCreateInstance failed: %d (Vulkan 1.3 required)", result);
    }

    /* -- Phase 3: Load instance functions (need valid VkInstance) -- */
    LOAD_FN_INSTANCE(vkDestroyInstance_fn, vkDestroyInstance)
    LOAD_FN_INSTANCE(vkEnumeratePhysicalDevices_fn, vkEnumeratePhysicalDevices)
    LOAD_FN_INSTANCE(vkGetPhysicalDeviceQueueFamilyProperties_fn, vkGetPhysicalDeviceQueueFamilyProperties)
    LOAD_FN_INSTANCE(vkGetPhysicalDeviceProperties_fn, vkGetPhysicalDeviceProperties)
    LOAD_FN_INSTANCE(vkGetPhysicalDeviceFeatures2_fn, vkGetPhysicalDeviceFeatures2)
    LOAD_FN_INSTANCE(vkGetPhysicalDeviceSurfaceSupportKHR_fn, vkGetPhysicalDeviceSurfaceSupportKHR)
    LOAD_FN_INSTANCE(vkGetPhysicalDeviceSurfaceFormatsKHR_fn, vkGetPhysicalDeviceSurfaceFormatsKHR)
    LOAD_FN_INSTANCE(vkGetPhysicalDeviceSurfacePresentModesKHR_fn, vkGetPhysicalDeviceSurfacePresentModesKHR)
    LOAD_FN_INSTANCE(vkGetPhysicalDeviceMemoryProperties_fn, vkGetPhysicalDeviceMemoryProperties)
    LOAD_FN_INSTANCE(vkGetPhysicalDeviceSurfaceCapabilitiesKHR_fn, vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
    LOAD_FN_INSTANCE(vkCreateDevice_fn, vkCreateDevice)
    LOAD_FN_INSTANCE(vkDestroyDevice_fn, vkDestroyDevice)
    LOAD_FN_INSTANCE(vkGetDeviceQueue_fn, vkGetDeviceQueue)
    LOAD_FN_INSTANCE(vkDestroySurfaceKHR_fn, vkDestroySurfaceKHR)
    LOAD_FN_INSTANCE(vkGetDeviceProcAddr_fn, vkGetDeviceProcAddr)
    /* Debug utils messenger is an optional extension function */
    vkCreateDebugUtilsMessengerEXT_fn = (PFN_vkCreateDebugUtilsMessengerEXT)vk_get_proc("vkCreateDebugUtilsMessengerEXT", vulkan_state.instance);
    vkDestroyDebugUtilsMessengerEXT_fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vk_get_proc("vkDestroyDebugUtilsMessengerEXT", vulkan_state.instance);
    if (want_debug && vkCreateDebugUtilsMessengerEXT_fn) {
        VkResult cb_result = vkCreateDebugUtilsMessengerEXT_fn(vulkan_state.instance, &debug_ci, NULL, &debug_callback);
        if (cb_result != VK_SUCCESS) {
            fprintf(stderr, "vkCreateDebugUtilsMessengerEXT failed: %d\n", cb_result);
            debug_callback = VK_NULL_HANDLE;
        }
    }
    /* Swapchain extension functions (not covered by load_device_functions) */
    LOAD_FN_INSTANCE(vkCreateSwapchainKHR_fn, vkCreateSwapchainKHR)
    LOAD_FN_INSTANCE(vkDestroySwapchainKHR_fn, vkDestroySwapchainKHR)
    LOAD_FN_INSTANCE(vkGetSwapchainImagesKHR_fn, vkGetSwapchainImagesKHR)
    LOAD_FN_INSTANCE(vkAcquireNextImageKHR_fn, vkAcquireNextImageKHR)
    LOAD_FN_INSTANCE(vkQueuePresentKHR_fn, vkQueuePresentKHR)

    /* -- 2. Create VkSurfaceKHR via SDL2 -- */
    void *window_handle = platform_get_window_handle();
    if (!window_handle) {
        die("platform_get_window_handle returned NULL");
    }

    if (SDL_Vulkan_CreateSurface((SDL_Window*)window_handle, vulkan_state.instance, &vulkan_state.surface) != SDL_TRUE) {
        die("SDL_Vulkan_CreateSurface failed");
    }

    /* -- 3. Select physical device -- */
    uint32_t pd_count = 0;
    vkEnumeratePhysicalDevices_fn(vulkan_state.instance, &pd_count, NULL);
    if (pd_count == 0) {
        fprintf(stderr, "No physical devices found\n");
        goto cleanup;
    }

    VkPhysicalDevice *phys_devices = (VkPhysicalDevice *)calloc(pd_count, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices_fn(vulkan_state.instance, &pd_count, phys_devices);

    vulkan_state.physical_device = VK_NULL_HANDLE;
    for (uint32_t i = 0; i < pd_count; i++) {
        VkPhysicalDeviceProperties props = {0};
        vkGetPhysicalDeviceProperties_fn(phys_devices[i], &props);

        /* Prefer discrete GPU */
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            vulkan_state.physical_device = phys_devices[i];
            break;
        }
        if (vulkan_state.physical_device == VK_NULL_HANDLE &&
            props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            vulkan_state.physical_device = phys_devices[i];
        }
    }

    if (vulkan_state.physical_device == VK_NULL_HANDLE) {
        vulkan_state.physical_device = phys_devices[0];
    }

    free(phys_devices);

    if (vulkan_state.physical_device == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to select physical device\n");
        goto cleanup;
    }

    /* Max sampler anisotropy for the atlas sampler (mirrors GL's
     * GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT; the VkSampler field clamps at 16). */
    {
        VkPhysicalDeviceProperties pd_props = {0};
        vkGetPhysicalDeviceProperties_fn(vulkan_state.physical_device, &pd_props);
        vulkan_state.max_anisotropy = pd_props.limits.maxSamplerAnisotropy > 16.0f ?
            16.0f : pd_props.limits.maxSamplerAnisotropy;
    }

    /* -- 4. Find queue families -- */
    uint32_t qf_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties_fn(vulkan_state.physical_device, &qf_count, NULL);
    VkQueueFamilyProperties *queue_families = (VkQueueFamilyProperties *)calloc(qf_count, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties_fn(vulkan_state.physical_device, &qf_count, queue_families);

    vulkan_state.graphics_queue_family = 0;
    vulkan_state.present_queue_family = 0;

    /* Pick a graphics queue family that also supports present: rendering and
     * vkQueuePresentKHR go through the same queue, and the spec requires
     * that queue's family to support presentation for the surface
     * (VUID-vkQueuePresentKHR-pSwapchains-01292). */
    {
        bool found = false;
        for (uint32_t i = 0; i < qf_count && !found; i++) {
            if (!(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) continue;
            VkBool32 support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR_fn(vulkan_state.physical_device, i, vulkan_state.surface, &support);
            if (support) {
                vulkan_state.graphics_queue_family = i;
                found = true;
            }
        }
        if (!found) {
            /* Non-conformant driver: no graphics family supports present.
             * Fall back to the first graphics family; presentation will
             * likely fail at runtime. */
            for (uint32_t i = 0; i < qf_count; i++) {
                if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    vulkan_state.graphics_queue_family = i;
                    break;
                }
            }
            fprintf(stderr, "No queue family supports both graphics and present\n");
        }
    }
    vulkan_state.present_queue_family = vulkan_state.graphics_queue_family;

    free(queue_families);

    /* -- 5. Create logical device -- */
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_ci = {0};
    queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_ci.queueFamilyIndex = vulkan_state.graphics_queue_family;
    queue_ci.queueCount = 1;
    queue_ci.pQueuePriorities = &queue_priority;

    /* Device features, enabled through the single 1.3 path. Timeline
     * semaphores, synchronization2 and dynamic rendering are core (mandatory)
     * in 1.3; extended dynamic state 1/2 are core in 1.4 and on a 1.3 device
     * come from the VK_EXT_extended_dynamic_state{,2} extensions — optional
     * for 1.3 conformance, but mandatory of anything reporting 1.4+ and
     * universally supported by current drivers. All five are queried below
     * and any missing one fails with a named error. Note the EDS feature
     * structs keep their EXT names/sTypes even in 1.4 — only the commands and
     * VkDynamicState values were promoted to core. The v1 feature set is
     * enabled minimally (see below): only samplerAnisotropy, which the atlas
     * sampler's anisotropyEnable requires. */
    VkPhysicalDeviceExtendedDynamicState2FeaturesEXT feat_eds2 = {0};
    feat_eds2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
    VkPhysicalDeviceDynamicRenderingFeatures feat_dyn_render = {0};
    feat_dyn_render.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    feat_eds2.pNext = &feat_dyn_render;
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT feat_eds = {0};
    feat_eds.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    feat_eds.pNext = &feat_eds2;
    VkPhysicalDeviceSynchronization2Features feat_sync2 = {0};
    feat_sync2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    feat_sync2.pNext = &feat_eds;
    VkPhysicalDeviceTimelineSemaphoreFeatures feat_timeline = {0};
    feat_timeline.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    feat_timeline.pNext = &feat_sync2;

    VkPhysicalDeviceFeatures2 features2 = {0};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &feat_timeline;
    vkGetPhysicalDeviceFeatures2_fn(vulkan_state.physical_device, &features2);

    if (!features2.features.samplerAnisotropy) {
        fprintf(stderr, "Vulkan driver missing required feature: samplerAnisotropy\n");
        goto cleanup;
    }

    /* Enable only the v1 features this renderer actually uses. The query
     * above filled features2.features with everything the device supports;
     * passing that through would silently enable robustBufferAccess,
     * depthBiasClamp, textureCompression* and friends. The only v1 feature
     * in use is samplerAnisotropy (the atlas sampler's anisotropyEnable). */
    features2.features = (VkPhysicalDeviceFeatures){0};
    features2.features.samplerAnisotropy = VK_TRUE;

    if (!feat_timeline.timelineSemaphore) {
        fprintf(stderr, "Vulkan 1.3 driver missing required feature: timelineSemaphore\n");
        goto cleanup;
    }
    if (!feat_sync2.synchronization2) {
        fprintf(stderr, "Vulkan 1.3 driver missing required feature: synchronization2\n");
        goto cleanup;
    }
    if (!feat_eds.extendedDynamicState) {
        fprintf(stderr, "Vulkan 1.3 driver missing required feature: extendedDynamicState\n");
        goto cleanup;
    }
    if (!feat_eds2.extendedDynamicState2) {
        fprintf(stderr, "Vulkan 1.3 driver missing required feature: extendedDynamicState2\n");
        goto cleanup;
    }
    if (!feat_dyn_render.dynamicRendering) {
        fprintf(stderr, "Vulkan 1.3 driver missing required feature: dynamicRendering\n");
        goto cleanup;
    }

    /* All five are guaranteed supported by the checks above — enable them. */
    feat_timeline.timelineSemaphore = VK_TRUE;
    feat_sync2.synchronization2 = VK_TRUE;
    feat_eds.extendedDynamicState = VK_TRUE;
    feat_eds2.extendedDynamicState2 = VK_TRUE;
    feat_dyn_render.dynamicRendering = VK_TRUE;

    /* The game-pass pipelines use core 1.0 dynamic states (viewport, scissor,
     * depth bias) plus the extended dynamic states depth test/write enable
     * and cull mode (see game_pipelines / render_flush); front face stays
     * static CLOCKWISE. No device extension is needed for any of it. */
    const char *device_exts[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo device_ci = {0};
    device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_ci.queueCreateInfoCount = 1;
    device_ci.pQueueCreateInfos = &queue_ci;
    device_ci.pNext = &features2;
    device_ci.enabledExtensionCount = 1;
    device_ci.ppEnabledExtensionNames = device_exts;
    device_ci.enabledLayerCount = 0;

    result = vkCreateDevice_fn(vulkan_state.physical_device, &device_ci, NULL, &vulkan_state.device);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreateDevice failed: %d\n", result);
        goto cleanup;
    }

    /* -- 6. Get queue -- */
    vkGetDeviceQueue_fn(vulkan_state.device, vulkan_state.graphics_queue_family, 0, &vulkan_state.graphics_queue);

    /* -- 6.5. Load device-level function pointers -- */
    if (!load_device_functions(vulkan_state.device, vulkan_state.instance)) {
        fprintf(stderr, "Failed to load device functions\n");
        goto cleanup;
    }

    /* -- 7. Create swapchain -- */
    if (!create_swapchain(VK_NULL_HANDLE)) {
        fprintf(stderr, "Failed to create swapchain\n");
        goto cleanup;
    }

    /* -- 9. Command pool deferred to init (just created above) */
    vulkan_state.frame_index = 0;

    /* -- 10. Create sync objects -- */
    {
        VkSemaphoreCreateInfo semaphore_ci = {0};
        semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        /* Per-frame-slot acquire semaphores (indexed by frame slot, not image).
         * Must be binary: VUID-VkAcquireNextImageInfoKHR-semaphore-03266
         * requires VK_SEMAPHORE_TYPE_BINARY for swapchain acquire. */
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore_fn(vulkan_state.device, &semaphore_ci, NULL,
                                     &vulkan_state.image_acquired_semaphores[i]) != VK_SUCCESS) {
                fprintf(stderr, "Failed to create acquire semaphore\n");
                goto cleanup;
            }
        }

        /* Per-swapchain-image render-complete semaphores (indexed by image) */
        for (uint32_t i = 0; i < MAX_SWAPCHAIN_IMAGES; i++) {
            if (vkCreateSemaphore_fn(vulkan_state.device, &semaphore_ci, NULL,
                                     &vulkan_state.render_complete_semaphores[i]) != VK_SUCCESS) {
                fprintf(stderr, "Failed to create render-complete semaphore\n");
                goto cleanup;
            }
        }

        /* Timeline semaphore replacing the old per-frame fences. Initial
         * value is 0 (so the first backpressure wait per slot is a no-op);
         * each frame submit signals an explicit monotonic value and prepare()
         * CPU-waits on it. */
        VkSemaphoreTypeCreateInfo timeline_ci = {0};
        timeline_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        timeline_ci.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        semaphore_ci.pNext = &timeline_ci;
        if (vkCreateSemaphore_fn(vulkan_state.device, &semaphore_ci, NULL,
                                 &vulkan_state.timeline_semaphore) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create timeline semaphore\n");
            goto cleanup;
        }
        vulkan_state.timeline_next_value = 1;
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vulkan_state.slot_fence_value[i] = 0;
        }
    }

    /* -- 11. Create per-frame command pools (one per in-flight frame) */
    {
        VkCommandPoolCreateInfo pool_ci = {0};
        pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_ci.queueFamilyIndex = vulkan_state.graphics_queue_family;
        pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateCommandPool_fn(vulkan_state.device, &pool_ci, NULL,
                                       &vulkan_state.command_pools[i]) != VK_SUCCESS) {
                fprintf(stderr, "Failed to create command pool %u\n", i);
                goto cleanup;
            }
            vulkan_state.command_buffers[i] = VK_NULL_HANDLE;
        }
        vulkan_state.command_buffer = VK_NULL_HANDLE; /* Working pointer */
    }

    /* -- 11.5. Create temp command pool for init-time one-shot transitions
     * (used when command_buffer isn't allocated yet, e.g. atlas creation) */
    {
        VkCommandPoolCreateInfo pool_ci = {0};
        pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_ci.queueFamilyIndex = vulkan_state.graphics_queue_family;
        pool_ci.flags = 0;
        if (vkCreateCommandPool_fn(vulkan_state.device, &pool_ci, NULL,
                                   &vulkan_state.temp_command_pool) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create temp command pool\n");
            goto cleanup;
        }
    }

    /* -- 12. Create descriptor set layout -- */
    {
        VkDescriptorSetLayoutBinding sampler_binding = {0};
        sampler_binding.binding = 0;
        sampler_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_binding.descriptorCount = 1;
        sampler_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        sampler_binding.pImmutableSamplers = NULL;

        VkDescriptorSetLayoutCreateInfo layout_ci = {0};
        layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_ci.bindingCount = 1;
        layout_ci.pBindings = &sampler_binding;

        if (vkCreateDescriptorSetLayout_fn(vulkan_state.device, &layout_ci, NULL,
                                           &vulkan_state.descriptor_set_layout) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create descriptor set layout\n");
            goto cleanup;
        }
    }

    /* -- 13. Create descriptor pool -- */
    {
        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2}
        };

        VkDescriptorPoolCreateInfo pool_ci = {0};
        pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        /* Allow freeing the long-lived descriptor sets individually at
         * shutdown instead of only implicitly via pool destruction. */
        pool_ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_ci.maxSets = 2;
        pool_ci.poolSizeCount = 1;
        pool_ci.pPoolSizes = pool_sizes;

        if (vkCreateDescriptorPool_fn(vulkan_state.device, &pool_ci, NULL,
                                      &vulkan_state.descriptor_pool) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create descriptor pool\n");
            goto cleanup;
        }
    }

    /* -- 14. Allocate descriptor sets -- */
    {
        VkDescriptorSetAllocateInfo alloc_ci = {0};
        alloc_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_ci.descriptorPool = vulkan_state.descriptor_pool;
        alloc_ci.descriptorSetCount = 1;
        alloc_ci.pSetLayouts = &vulkan_state.descriptor_set_layout;

        if (vkAllocateDescriptorSets_fn(vulkan_state.device, &alloc_ci, &vulkan_state.atlas_descriptor_set) != VK_SUCCESS ||
            vkAllocateDescriptorSets_fn(vulkan_state.device, &alloc_ci, &vulkan_state.backbuffer_descriptor_set) != VK_SUCCESS) {
            fprintf(stderr, "Failed to allocate descriptor sets\n");
            goto cleanup;
        }
    }


    /* -- 16. Create backbuffer (off-screen render target) --
     * render_res is NATIVE by default (zero-init), so the backbuffer tracks
     * the (already driver-synced) screen_size. create_backbuffer() uses
     * backbuffer_size as-is and does NOT clobber it. */
    vulkan_state.backbuffer_size = vulkan_state.screen_size;
    if (!create_backbuffer()) {
        fprintf(stderr, "Failed to create backbuffer\n");
        goto cleanup;
    }

    /* -- 17. Create sampler for backbuffer (used in post pass) -- */
    {
        VkSamplerCreateInfo sampler_ci = {0};
        sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_ci.magFilter = VK_FILTER_NEAREST;
        sampler_ci.minFilter = VK_FILTER_NEAREST;
        sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_ci.minLod = 0;
        sampler_ci.maxLod = 1;
        sampler_ci.maxAnisotropy = 1.0f;
        sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_ci.compareOp = VK_COMPARE_OP_NEVER;

        if (vkCreateSampler_fn(vulkan_state.device, &sampler_ci, NULL, &vulkan_state.backbuffer_sampler) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create backbuffer sampler\n");
            goto cleanup;
        }
    }

    /* -- 18. Create vertex buffer -- */
    if (!create_vertex_buffer()) {
        fprintf(stderr, "Failed to create vertex buffer\n");
        goto cleanup;
    }

    /* -- 19. Create texture atlas -- */
    if (!create_atlas()) {
        fprintf(stderr, "Failed to create texture atlas\n");
        goto cleanup;
    }

    /* -- 19b. Create reusable staging buffer for texture uploads -- */
    {
        uint32_t atlas_width = ATLAS_SIZE * ATLAS_GRID;
        uint32_t atlas_height = ATLAS_SIZE * ATLAS_GRID;
        vulkan_state.upload_staging_size = atlas_width * atlas_height * sizeof(rgba_t);

        VkBufferCreateInfo buf_ci = {0};
        buf_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_ci.size = vulkan_state.upload_staging_size;
        buf_ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer_fn(vulkan_state.device, &buf_ci, NULL, &vulkan_state.upload_staging_buffer) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create upload staging buffer\n");
            goto cleanup;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements_fn(vulkan_state.device, vulkan_state.upload_staging_buffer, &mem_reqs);

        VkMemoryAllocateInfo alloc_ci = {0};
        alloc_ci.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_ci.allocationSize = mem_reqs.size;
        alloc_ci.memoryTypeIndex = find_memory_type(vulkan_state.physical_device, mem_reqs.memoryTypeBits,
                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (alloc_ci.memoryTypeIndex == UINT32_MAX ||
            vkAllocateMemory_fn(vulkan_state.device, &alloc_ci, NULL, &vulkan_state.upload_staging_memory) != VK_SUCCESS) {
            vkDestroyBuffer_fn(vulkan_state.device, vulkan_state.upload_staging_buffer, NULL);
            vulkan_state.upload_staging_buffer = VK_NULL_HANDLE;
            goto cleanup;
        }

        if (vkBindBufferMemory_fn(vulkan_state.device, vulkan_state.upload_staging_buffer, vulkan_state.upload_staging_memory, 0) != VK_SUCCESS) {
            vkFreeMemory_fn(vulkan_state.device, vulkan_state.upload_staging_memory, NULL);
            vkDestroyBuffer_fn(vulkan_state.device, vulkan_state.upload_staging_buffer, NULL);
            vulkan_state.upload_staging_buffer = VK_NULL_HANDLE;
            vulkan_state.upload_staging_memory = VK_NULL_HANDLE;
            goto cleanup;
        }

        /* Map once — persists for lifetime */
        if (vkMapMemory_fn(vulkan_state.device, vulkan_state.upload_staging_memory, 0,
                           vulkan_state.upload_staging_size, 0, &vulkan_state.upload_staging_mapped) != VK_SUCCESS) {
            vkFreeMemory_fn(vulkan_state.device, vulkan_state.upload_staging_memory, NULL);
            vkDestroyBuffer_fn(vulkan_state.device, vulkan_state.upload_staging_buffer, NULL);
            vulkan_state.upload_staging_buffer = VK_NULL_HANDLE;
            vulkan_state.upload_staging_memory = VK_NULL_HANDLE;
            vulkan_state.upload_staging_mapped = NULL;
            goto cleanup;
        }
    }

    /* -- 19c. Create persistent one-shot command pool for texture uploads */
    {
        VkCommandPoolCreateInfo pool_ci = {0};
        pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_ci.queueFamilyIndex = vulkan_state.graphics_queue_family;
        if (vkCreateCommandPool_fn(vulkan_state.device, &pool_ci, NULL,
                                    &vulkan_state.upload_command_pool) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create upload command pool\n");
            goto cleanup;
        }

        VkCommandBufferAllocateInfo cb_ai = {0};
        cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cb_ai.commandPool = vulkan_state.upload_command_pool;
        cb_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cb_ai.commandBufferCount = 1;
        if (vkAllocateCommandBuffers_fn(vulkan_state.device, &cb_ai,
                                        &vulkan_state.upload_command_buffer) != VK_SUCCESS) {
            fprintf(stderr, "Failed to allocate upload command buffer\n");
            vkDestroyCommandPool_fn(vulkan_state.device, vulkan_state.upload_command_pool, NULL);
            vulkan_state.upload_command_pool = VK_NULL_HANDLE;
            goto cleanup;
        }
    }

    /* -- 19d. Create default white texture (RENDER_NO_TEXTURE), index 0 --
     * Mirrors render_gl.c: game code (pause dim quad, HUD panels) passes
     * RENDER_NO_TEXTURE and expects a solid-color 2x2 texture at index 0.
     * Must be created before game_init loads the font, otherwise the game's
     * textures start at index 0 and RENDER_NO_TEXTURE silently aliases the
     * font atlas. The GPU upload is deferred (command buffer isn't
     * allocated yet); it flushes on the first render_frame_prepare. */
    {
        rgba_t white_pixels[4] = {
            rgba(128,128,128,255), rgba(128,128,128,255),
            rgba(128,128,128,255), rgba(128,128,128,255)
        };
        RENDER_NO_TEXTURE = render_texture_create(2, 2, white_pixels);
    }

    /* -- 20. Update descriptor sets (atlas + backbuffer) -- */
    {
        VkWriteDescriptorSet writes[2] = {0};

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = vulkan_state.atlas_descriptor_set;
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].pImageInfo = &(VkDescriptorImageInfo){
            .sampler = vulkan_state.atlas_sampler,
            .imageView = vulkan_state.atlas_image_view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = vulkan_state.backbuffer_descriptor_set;
        writes[1].dstBinding = 0;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].pImageInfo = &(VkDescriptorImageInfo){
            .sampler = vulkan_state.backbuffer_sampler,
            .imageView = vulkan_state.backbuffer_image_view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        vkUpdateDescriptorSets_fn(vulkan_state.device, 2, writes, 0, NULL);
    }

    /* -- 21. Create quad vertex buffer (for post pass fullscreen quad) -- */
    {
        float quad_vertices[] = {
            /* pos.x, pos.y, uv.x, uv.y */
            /* Vulkan backbuffer: scene top renders to image row 0 (v=0), so the
             * top of the window must sample v=0 (GL sampled v=1 for its top). */
             1.0f,  1.0f, 1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 0.0f,
            -1.0f, -1.0f, 0.0f, 1.0f,
             1.0f,  1.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 1.0f,
        };

        VkBufferCreateInfo buf_ci = {0};
        buf_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_ci.size = sizeof(quad_vertices);
        buf_ci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer_fn(vulkan_state.device, &buf_ci, NULL, &vulkan_state.quad_vertex_buffer) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create quad vertex buffer\n");
            goto cleanup;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements_fn(vulkan_state.device, vulkan_state.quad_vertex_buffer, &mem_reqs);

        VkMemoryAllocateInfo alloc_ci = {0};
        alloc_ci.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_ci.allocationSize = mem_reqs.size;
        alloc_ci.memoryTypeIndex = find_memory_type(vulkan_state.physical_device, mem_reqs.memoryTypeBits,
                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (alloc_ci.memoryTypeIndex == UINT32_MAX ||
            vkAllocateMemory_fn(vulkan_state.device, &alloc_ci, NULL, &vulkan_state.quad_vertex_buffer_memory) != VK_SUCCESS) {
            vkDestroyBuffer_fn(vulkan_state.device, vulkan_state.quad_vertex_buffer, NULL);
            vulkan_state.quad_vertex_buffer = VK_NULL_HANDLE;
            goto cleanup;
        }

        if (vkBindBufferMemory_fn(vulkan_state.device, vulkan_state.quad_vertex_buffer, vulkan_state.quad_vertex_buffer_memory, 0) != VK_SUCCESS) {
            vkFreeMemory_fn(vulkan_state.device, vulkan_state.quad_vertex_buffer_memory, NULL);
            vkDestroyBuffer_fn(vulkan_state.device, vulkan_state.quad_vertex_buffer, NULL);
            vulkan_state.quad_vertex_buffer = VK_NULL_HANDLE;
            vulkan_state.quad_vertex_buffer_memory = VK_NULL_HANDLE;
            goto cleanup;
        }

        void *data;
        vkMapMemory_fn(vulkan_state.device, vulkan_state.quad_vertex_buffer_memory, 0, sizeof(quad_vertices), 0, &data);
        memcpy(data, quad_vertices, sizeof(quad_vertices));
        vkUnmapMemory_fn(vulkan_state.device, vulkan_state.quad_vertex_buffer_memory);
    }


    /* -- 22. Create pipeline layout + pipelines + shaders -- */
    if (!create_pipelines()) {
        fprintf(stderr, "Failed to create pipelines\n");
        goto cleanup;
    }

    /* Wait for all GPU work from temp command buffers during init to complete */
    vkQueueWaitIdle_fn(vulkan_state.graphics_queue);

    /* Command buffer is allocated lazily in render_frame_prepare (first
     * frame) rather than here, so that init-time GPU work (atlas creation,
     * mip blits) uses the dedicated temp pool and per-frame CBs are only
     * allocated once the render loop actually begins. */
    vulkan_state.command_buffer = VK_NULL_HANDLE;

    printf("Vulkan renderer initialized\n");
    return;

cleanup:
    render_cleanup();
    /* Init failure is fatal: continuing would leave the next render_* call
     * dereferencing NULL function pointers and handles. die() prints the
     * file/line and exits; the per-site message above says what failed. */
    die("Vulkan renderer initialization failed");
}

void render_cleanup(void) {
    if (!vulkan_state.device) {
        /* Initialization failed before the logical device existed — only
         * instance-level objects may be live (surface, debug callback,
         * instance). Tear those down so pre-device failure paths in
         * render_init don't leak them. */
        if (vulkan_state.surface) {
            vkDestroySurfaceKHR_fn(vulkan_state.instance, vulkan_state.surface, NULL);
            vulkan_state.surface = VK_NULL_HANDLE;
        }
        if (vulkan_state.instance) {
            if (debug_callback) {
                vkDestroyDebugUtilsMessengerEXT_fn(vulkan_state.instance, debug_callback, NULL);
                debug_callback = VK_NULL_HANDLE;
            }
            vkDestroyInstance_fn(vulkan_state.instance, NULL);
            vulkan_state.instance = VK_NULL_HANDLE;
        }
        return;
    }

    /* Wait for device idle before cleanup */
    vkQueueWaitIdle_fn(vulkan_state.graphics_queue);

    /* Destroy pipelines */
    destroy_pipelines();

    /* Destroy swapchain */
    destroy_swapchain();

    /* Destroy sync objects */
    /* Acquire semaphores: one per frame slot */
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vulkan_state.image_acquired_semaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore_fn(vulkan_state.device, vulkan_state.image_acquired_semaphores[i], NULL);
            vulkan_state.image_acquired_semaphores[i] = VK_NULL_HANDLE;
        }
    }
    /* Render-complete semaphores: one per swapchain image (16 max) */
    for (uint32_t i = 0; i < 16; i++) {
        if (vulkan_state.render_complete_semaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore_fn(vulkan_state.device, vulkan_state.render_complete_semaphores[i], NULL);
            vulkan_state.render_complete_semaphores[i] = VK_NULL_HANDLE;
        }
    }
    /* Timeline semaphore (queue was waited idle above, so no outstanding
     * signal operations remain) */
    if (vulkan_state.timeline_semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore_fn(vulkan_state.device, vulkan_state.timeline_semaphore, NULL);
        vulkan_state.timeline_semaphore = VK_NULL_HANDLE;
    }

    /* Destroy per-frame command pools */
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vulkan_state.command_pools[i] != VK_NULL_HANDLE) {
            vkDestroyCommandPool_fn(vulkan_state.device, vulkan_state.command_pools[i], NULL);
            vulkan_state.command_pools[i] = VK_NULL_HANDLE;
        }
    }

    /* Destroy temp command pool (init-time one-shot transitions) */
    if (vulkan_state.temp_command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool_fn(vulkan_state.device, vulkan_state.temp_command_pool, NULL);
        vulkan_state.temp_command_pool = VK_NULL_HANDLE;
    }

    /* Destroy per-frame vertex buffers */
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vulkan_state.vertex_buffer_memories[i] != VK_NULL_HANDLE) {
            if (vulkan_state.vertex_buffer_ptrs[i])
                vkUnmapMemory_fn(vulkan_state.device, vulkan_state.vertex_buffer_memories[i]);
            vulkan_state.vertex_buffer_ptrs[i] = NULL;
            vkFreeMemory_fn(vulkan_state.device, vulkan_state.vertex_buffer_memories[i], NULL);
            vulkan_state.vertex_buffer_memories[i] = VK_NULL_HANDLE;
        }
        if (vulkan_state.vertex_buffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer_fn(vulkan_state.device, vulkan_state.vertex_buffers[i], NULL);
            vulkan_state.vertex_buffers[i] = VK_NULL_HANDLE;
        }
    }
    free(vulkan_state.vertex_data);
    vulkan_state.vertex_data = NULL;

    /* Destroy backbuffer */
    destroy_backbuffer();

    /* Destroy texture atlas */
    destroy_atlas();

    /* Destroy reusable upload staging buffer */
    if (vulkan_state.upload_staging_mapped) {
        vulkan_state.upload_staging_mapped = NULL;
    }
    if (vulkan_state.upload_staging_memory != VK_NULL_HANDLE) {
        vkFreeMemory_fn(vulkan_state.device, vulkan_state.upload_staging_memory, NULL);
        vulkan_state.upload_staging_memory = VK_NULL_HANDLE;
    }
    if (vulkan_state.upload_staging_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer_fn(vulkan_state.device, vulkan_state.upload_staging_buffer, NULL);
        vulkan_state.upload_staging_buffer = VK_NULL_HANDLE;
    }

    /* Destroy persistent upload command pool */
    if (vulkan_state.upload_command_buffer != VK_NULL_HANDLE) {
        vkFreeCommandBuffers_fn(vulkan_state.device, vulkan_state.upload_command_pool, 1,
                                &vulkan_state.upload_command_buffer);
        vulkan_state.upload_command_buffer = VK_NULL_HANDLE;
    }
    if (vulkan_state.upload_command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool_fn(vulkan_state.device, vulkan_state.upload_command_pool, NULL);
        vulkan_state.upload_command_pool = VK_NULL_HANDLE;
    }

    /* Destroy quad vertex buffer */
    if (vulkan_state.quad_vertex_buffer_memory != VK_NULL_HANDLE) {
        vkFreeMemory_fn(vulkan_state.device, vulkan_state.quad_vertex_buffer_memory, NULL);
        vulkan_state.quad_vertex_buffer_memory = VK_NULL_HANDLE;
    }
    if (vulkan_state.quad_vertex_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer_fn(vulkan_state.device, vulkan_state.quad_vertex_buffer, NULL);
        vulkan_state.quad_vertex_buffer = VK_NULL_HANDLE;
    }

    /* Destroy backbuffer sampler */
    if (vulkan_state.backbuffer_sampler != VK_NULL_HANDLE) {
        vkDestroySampler_fn(vulkan_state.device, vulkan_state.backbuffer_sampler, NULL);
        vulkan_state.backbuffer_sampler = VK_NULL_HANDLE;
    }

    /* Free descriptor sets before destroying their pool */
    {
        VkDescriptorSet sets[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
        uint32_t set_count = 0;
        if (vulkan_state.atlas_descriptor_set != VK_NULL_HANDLE) {
            sets[set_count++] = vulkan_state.atlas_descriptor_set;
            vulkan_state.atlas_descriptor_set = VK_NULL_HANDLE;
        }
        if (vulkan_state.backbuffer_descriptor_set != VK_NULL_HANDLE) {
            sets[set_count++] = vulkan_state.backbuffer_descriptor_set;
            vulkan_state.backbuffer_descriptor_set = VK_NULL_HANDLE;
        }
        if (set_count > 0) {
            vkFreeDescriptorSets_fn(vulkan_state.device, vulkan_state.descriptor_pool, set_count, sets);
        }
    }

    /* Destroy descriptor pool + layout */
    if (vulkan_state.descriptor_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool_fn(vulkan_state.device, vulkan_state.descriptor_pool, NULL);
        vulkan_state.descriptor_pool = VK_NULL_HANDLE;
    }
    if (vulkan_state.descriptor_set_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout_fn(vulkan_state.device, vulkan_state.descriptor_set_layout, NULL);
        vulkan_state.descriptor_set_layout = VK_NULL_HANDLE;
    }

    /* Device */
    if (vulkan_state.graphics_queue) {
        vulkan_state.graphics_queue = VK_NULL_HANDLE;
    }
    if (vulkan_state.device) {
        vkDestroyDevice_fn(vulkan_state.device, NULL);
        vulkan_state.device = VK_NULL_HANDLE;
    }
    if (vulkan_state.surface) {
        vkDestroySurfaceKHR_fn(vulkan_state.instance, vulkan_state.surface, NULL);
        vulkan_state.surface = VK_NULL_HANDLE;
    }
    if (vulkan_state.instance) {
        if (debug_callback) {
            vkDestroyDebugUtilsMessengerEXT_fn(vulkan_state.instance, debug_callback, NULL);
            debug_callback = VK_NULL_HANDLE;
        }
        vkDestroyInstance_fn(vulkan_state.instance, NULL);
        vulkan_state.instance = VK_NULL_HANDLE;
    }
}

/* ------------------------------------------------------------------ */
/* Memory type helper                                                  */
/* ------------------------------------------------------------------ */

static uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties_fn(physical_device, &mem_props);
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((type_filter & (1u << i)) && (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    fprintf(stderr, "Failed to find suitable memory type\n");
    /* UINT32_MAX is never a valid memoryTypeIndex; callers must check for it
     * and fail rather than bind/allocate with an out-of-range index. */
    return UINT32_MAX;
}

/* ------------------------------------------------------------------ */
/* Swapchain creation                                                  */
/* ------------------------------------------------------------------ */

static VkSurfaceFormatKHR choose_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR_fn(physical_device, surface, &format_count, NULL);
    if (format_count == 0) return (VkSurfaceFormatKHR){.format = VK_FORMAT_B8G8R8A8_UNORM, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *)calloc(format_count, sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR_fn(physical_device, surface, &format_count, formats);

    /* Prefer B8G8R8A8 or A8B8G8R8 with SRGB */
    for (uint32_t i = 0; i < format_count; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            VkSurfaceFormatKHR result = formats[i];
            free(formats);
            return result;
        }
    }
    /* Fallback: first format */
    VkSurfaceFormatKHR chosen = formats[0];
    free(formats);
    return chosen;
}

static VkPresentModeKHR choose_present_mode(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    uint32_t mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR_fn(physical_device, surface, &mode_count, NULL);
    if (mode_count == 0) return VK_PRESENT_MODE_FIFO_KHR;

    VkPresentModeKHR *modes = (VkPresentModeKHR *)calloc(mode_count, sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR_fn(physical_device, surface, &mode_count, modes);

    /* Prefer MAILBOX when the surface offers it: the presentation engine
     * replaces its single pending present with each new one instead of
     * queueing behind earlier presents, so a frame rendered before vsync is
     * not delayed by frames queued ahead of it (FIFO can waste a present).
     *
     * The classic MAILBOX acquire deadlock does not apply here:
     *  - acquire happens while we hold no swapchain image — the timeline
     *    wait above guarantees this slot's previous submit completed, so
     *    image_acquired_semaphores[f] has no pending wait and is free to be
     *    re-signaled (VUID-vkAcquireNextImageKHR-semaphore-01779);
     *  - the acquire timeout is finite (see render_frame_prepare), so a
     *    surface that cannot make forward progress (e.g. minimized) yields
     *    VK_TIMEOUT and a skipped frame instead of a hang, per
     *    VUID-vkAcquireNextImageKHR-surface-07783.
     * Fall back to FIFO — the only mode guaranteed to be offered. */
    for (uint32_t i = 0; i < mode_count; i++) {
        if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            free(modes);
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }
    free(modes);
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D choose_extent(VkSurfaceCapabilitiesKHR *caps, vec2i_t screen_size) {
    if (caps->currentExtent.width != UINT32_MAX) {
        /* currentExtent is valid: the swapchain imageExtent must equal it
         * exactly (spec), so a window larger than 4K in either dimension
         * (5K/8K displays) must not be clamped down. The min/maxImageExtent
         * clamp below is a no-op for conformant drivers (currentExtent is
         * always within those bounds) but guards against non-conformant
         * reports where currentExtent exceeds maxImageExtent. */
        VkExtent2D extent = caps->currentExtent;
        if (extent.width > caps->maxImageExtent.width) {
            extent.width = caps->maxImageExtent.width;
        }
        if (extent.height > caps->maxImageExtent.height) {
            extent.height = caps->maxImageExtent.height;
        }
        if (extent.width < caps->minImageExtent.width) {
            extent.width = caps->minImageExtent.width;
        }
        if (extent.height < caps->minImageExtent.height) {
            extent.height = caps->minImageExtent.height;
        }
        return extent;
    }
    /* currentExtent is UINT32_MAX — driver accepts any size.
     * Use screen_size clamped to min/max, with sensible defaults
     * if screen_size is 0 (e.g. Wayland before window is drawn). */
    int32_t width = screen_size.x;
    int32_t height = screen_size.y;
    if (width <= 0) width = 1280;
    if (height <= 0) height = 720;

    VkExtent2D extent = {0};
    extent.width = (uint32_t)width;
    extent.height = (uint32_t)height;
    /* Clamp to [minExtent, maxExtent] using signed comparison */
    if ((int32_t)extent.width < (int32_t)caps->minImageExtent.width) {
        extent.width = caps->minImageExtent.width;
    }
    if ((int32_t)extent.width > (int32_t)caps->maxImageExtent.width) {
        extent.width = caps->maxImageExtent.width;
    }
    if ((int32_t)extent.height < (int32_t)caps->minImageExtent.height) {
        extent.height = caps->minImageExtent.height;
    }
    if ((int32_t)extent.height > (int32_t)caps->maxImageExtent.height) {
        extent.height = caps->maxImageExtent.height;
    }
    return extent;
}

static VkExtent2D choose_backbuffer_extent(vec2i_t backbuffer_size) {
    VkExtent2D extent = {0};
    extent.width = (uint32_t)backbuffer_size.x;
    extent.height = (uint32_t)backbuffer_size.y;
    /* Clamp to 1×1 minimum — Vulkan rejects 0-dimension images */
    if (extent.width < 1) extent.width = 1;
    if (extent.height < 1) extent.height = 1;
    return extent;
}

static bool create_swapchain(VkSwapchainKHR old_swapchain) {
    VkSurfaceCapabilitiesKHR caps = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR_fn(vulkan_state.physical_device, vulkan_state.surface, &caps);

    {
        VkSurfaceFormatKHR surface_format = choose_surface_format(vulkan_state.physical_device, vulkan_state.surface);
        vulkan_state.swapchain_format = surface_format.format;
        /* Use the color space the surface actually reports for this format —
         * hardcoding SRGB mismatches imageColorSpace with imageFormat
         * (VUID-VkSwapchainCreateInfoKHR-imageFormat-01273). */
        vulkan_state.surface_color_space = surface_format.colorSpace;
    }
    VkPresentModeKHR present_mode = choose_present_mode(vulkan_state.physical_device, vulkan_state.surface);
    printf("Vulkan: swapchain present mode: %s\n",
           present_mode == VK_PRESENT_MODE_MAILBOX_KHR ? "MAILBOX" : "FIFO");
    vulkan_state.swapchain_extent = choose_extent(&caps, vulkan_state.screen_size);

    /* Sync screen_size to swapchain_extent — on Wayland screen_size may be
     * 0×0 from SDL_GetDrawableSize() before the window is mapped, but the
     * compositor always provides a valid currentExtent. */
    vulkan_state.screen_size = vec2i((int)vulkan_state.swapchain_extent.width,
                                     (int)vulkan_state.swapchain_extent.height);

    uint32_t image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) {
        image_count = caps.maxImageCount;
    }
    /* Per-swapchain-image sync arrays are fixed at 16 entries; keep the
     * requested count below that so image indices stay in range. */
    if (image_count > 15) {
        image_count = 15;
    }

    /* Build swapchain create info with correct sharing mode */
    VkSwapchainCreateInfoKHR swapchain_ci = {0};
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.surface = vulkan_state.surface;
    swapchain_ci.minImageCount = image_count;
    swapchain_ci.imageFormat = vulkan_state.swapchain_format;
    swapchain_ci.imageColorSpace = vulkan_state.surface_color_space;
    swapchain_ci.imageExtent = vulkan_state.swapchain_extent;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    /* Determine sharing mode */
    if (vulkan_state.graphics_queue_family != vulkan_state.present_queue_family) {
        uint32_t indices[] = {vulkan_state.graphics_queue_family, vulkan_state.present_queue_family};
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_ci.queueFamilyIndexCount = 2;
        swapchain_ci.pQueueFamilyIndices = indices;
    } else {
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapchain_ci.preTransform = caps.currentTransform;
    swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_ci.presentMode = present_mode;
    swapchain_ci.clipped = VK_TRUE;
    /* old_swapchain retires the previous swapchain: its displayed image keeps
     * presenting through the driver until this one has a frame to show, so
     * the surface never lacks a presentation source mid-recreation. */
    swapchain_ci.oldSwapchain = old_swapchain;

    if (vkCreateSwapchainKHR_fn(vulkan_state.device, &swapchain_ci, NULL, &vulkan_state.swapchain) != VK_SUCCESS) {
        return false;
    }

    /* Get swapchain images */
    uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR_fn(vulkan_state.device, vulkan_state.swapchain, &swapchain_image_count, NULL);
    vulkan_state.swapchain_image_count = swapchain_image_count;

    /* A driver may return more images than the requested minImageCount. The
     * per-image sync arrays are fixed at MAX_SWAPCHAIN_IMAGES, so fail with a
     * clear error rather than index out of bounds. */
    if (swapchain_image_count > MAX_SWAPCHAIN_IMAGES) {
        fprintf(stderr, "Vulkan: swapchain returned %u images, exceeds the "
                        "supported maximum of %d\n",
                swapchain_image_count, MAX_SWAPCHAIN_IMAGES);
        vkDestroySwapchainKHR_fn(vulkan_state.device, vulkan_state.swapchain, NULL);
        vulkan_state.swapchain = VK_NULL_HANDLE;
        vulkan_state.swapchain_image_count = 0;
        return false;
    }

    vulkan_state.swapchain_images = (VkImage *)calloc(swapchain_image_count, sizeof(VkImage));
    vkGetSwapchainImagesKHR_fn(vulkan_state.device, vulkan_state.swapchain, &swapchain_image_count, vulkan_state.swapchain_images);

    vulkan_state.swapchain_image_views = (VkImageView *)calloc(swapchain_image_count, sizeof(VkImageView));
    for (uint32_t i = 0; i < swapchain_image_count; i++) {
        VkImageViewCreateInfo view_ci = {0};
        view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_ci.image = vulkan_state.swapchain_images[i];
        view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_ci.format = vulkan_state.swapchain_format;
        view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_ci.subresourceRange.levelCount = 1;
        view_ci.subresourceRange.layerCount = 1;
        if (vkCreateImageView_fn(vulkan_state.device, &view_ci, NULL, &vulkan_state.swapchain_image_views[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

/* Destroy swapchain resources in dependency order: image views belong to
 * the swapchain's images — so views first, then the swapchain. (The
 * object-lifetime rules require destroying referencing objects before the
 * referenced ones.) */
static void destroy_swapchain_resources(VkSwapchainKHR swapchain, VkImage *images,
                                        VkImageView *views, uint32_t count) {
    if (views) {
        for (uint32_t i = 0; i < count; i++) {
            if (views[i] != VK_NULL_HANDLE) {
                vkDestroyImageView_fn(vulkan_state.device, views[i], NULL);
            }
        }
        free(views);
    }
    free(images);
    if (swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR_fn(vulkan_state.device, swapchain, NULL);
    }
}

static void destroy_swapchain(void) {
    destroy_swapchain_resources(vulkan_state.swapchain, vulkan_state.swapchain_images,
                                vulkan_state.swapchain_image_views,
                                vulkan_state.swapchain_image_count);
    vulkan_state.swapchain = VK_NULL_HANDLE;
    vulkan_state.swapchain_images = NULL;
    vulkan_state.swapchain_image_views = NULL;
    vulkan_state.swapchain_image_count = 0;
}

/* ------------------------------------------------------------------ */
/* Image helper: create + allocate image                               */
/* ------------------------------------------------------------------ */

static bool create_image(uint32_t width, uint32_t height, uint32_t mip_levels,
                         VkFormat format,
                         VkImageTiling tiling, VkImageUsageFlags usage,
                         VkMemoryPropertyFlags mem_props,
                         VkImage *out_image, VkDeviceMemory *out_memory) {
    VkImageCreateInfo image_ci = {0};
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.extent.width = width;
    image_ci.extent.height = height;
    image_ci.extent.depth = 1;
    image_ci.mipLevels = mip_levels;
    image_ci.arrayLayers = 1;
    image_ci.format = format;
    image_ci.tiling = tiling;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_ci.usage = usage;
    image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
    image_ci.flags = 0;

    if (vkCreateImage_fn(vulkan_state.device, &image_ci, NULL, out_image) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements_fn(vulkan_state.device, *out_image, &mem_reqs);

    VkMemoryAllocateInfo alloc_ci = {0};
    alloc_ci.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_ci.allocationSize = mem_reqs.size;
    alloc_ci.memoryTypeIndex = find_memory_type(vulkan_state.physical_device, mem_reqs.memoryTypeBits, mem_props);

    if (alloc_ci.memoryTypeIndex == UINT32_MAX ||
        vkAllocateMemory_fn(vulkan_state.device, &alloc_ci, NULL, out_memory) != VK_SUCCESS) {
        vkDestroyImage_fn(vulkan_state.device, *out_image, NULL);
        *out_image = VK_NULL_HANDLE;
        return false;
    }

    if (vkBindImageMemory_fn(vulkan_state.device, *out_image, *out_memory, 0) != VK_SUCCESS) {
        vkFreeMemory_fn(vulkan_state.device, *out_memory, NULL);
        vkDestroyImage_fn(vulkan_state.device, *out_image, NULL);
        *out_image = VK_NULL_HANDLE;
        *out_memory = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* Command-buffer-level image layout transition (records into current  */
/* command buffer instead of creating a separate one)                  */
/* ------------------------------------------------------------------ */

static void cmd_transition_image_layout_fallback(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);
static void transition_image_layout_with_pool(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkCommandPool pool);

/* Build an image layout transition barrier and the pipeline stages for the
 * transition. Shared by the current-CB and pool-based transition paths so the
 * two can't drift apart. */
static void make_image_layout_barrier(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout,
                                     VkImageMemoryBarrier *out_barrier,
                                     VkPipelineStageFlags *out_src_stage,
                                     VkPipelineStageFlags *out_dst_stage) {
    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    /* Determine aspect mask from layout transition */
    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
        old_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        /* Shader read → color attachment write: the image was read by a
         * fragment (or color-attachment) stage in the previous render pass.
         * We must wait for that read to complete before writing. */
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    } else if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        /* Transition to depth/stencil: no access before (top-of-pipe),
         * depth/stencil write after. */
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        /* Depth/stencil → shader read (e.g. after MSAA resolve). */
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    VkPipelineStageFlags src_stage = old_layout == VK_IMAGE_LAYOUT_UNDEFINED ?
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkPipelineStageFlags dst_stage = new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ||
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ?
        VK_PIPELINE_STAGE_TRANSFER_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    /* COLOR_ATTACHMENT uses COLOR_ATTACHMENT_OUTPUT stage.
     * src_stage: COLOR_ATTACHMENT → COLOR_ATTACHMENT needs COLOR_ATTACHMENT_OUTPUT;
     * SHADER_READ_ONLY → COLOR_ATTACHMENT needs FRAGMENT_SHADER. */
    if (new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        } else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
    }

    /* Special case: DEPTH_STENCIL uses EARLY/LATE_FRAGMENT_TESTS stage */
    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
        old_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        src_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }

    *out_barrier = barrier;
    *out_src_stage = src_stage;
    *out_dst_stage = dst_stage;
}

static void cmd_transition_image_layout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout) {
    /* During init (before deferred CB allocation), use temp CB fallback */
    if (vulkan_state.command_buffer == VK_NULL_HANDLE) {
        cmd_transition_image_layout_fallback(image, old_layout, new_layout);
        return;
    }
    VkImageMemoryBarrier barrier;
    VkPipelineStageFlags src_stage, dst_stage;
    make_image_layout_barrier(image, old_layout, new_layout, &barrier, &src_stage, &dst_stage);
    vkCmdPipelineBarrier_fn(vulkan_state.command_buffer, src_stage, dst_stage, 0,
                            0, NULL, 0, NULL, 1, &barrier);
}

/* Fallback: use a temporary command buffer when the persistent CB isn't
 * available (e.g. during atlas creation in init). */
static void cmd_transition_image_layout_fallback(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout) {
    transition_image_layout_with_pool(image, old_layout, new_layout, vulkan_state.temp_command_pool);
}

static void transition_image_layout_with_pool(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkCommandPool pool) {

    VkCommandBufferAllocateInfo alloc_ci = {0};
    alloc_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_ci.commandPool = pool;
    alloc_ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_ci.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    VK_CHECK(vkAllocateCommandBuffers_fn(vulkan_state.device, &alloc_ci, &command_buffer));

    VkCommandBufferBeginInfo begin_ci = {0};
    begin_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkBeginCommandBuffer_fn(command_buffer, &begin_ci));

    VkImageMemoryBarrier barrier;
    VkPipelineStageFlags src_stage, dst_stage;
    make_image_layout_barrier(image, old_layout, new_layout, &barrier, &src_stage, &dst_stage);

    vkCmdPipelineBarrier_fn(command_buffer, src_stage, dst_stage, 0,
                            0, NULL, 0, NULL, 1, &barrier);

    VK_CHECK(vkEndCommandBuffer_fn(command_buffer));

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    VkFence fence;
    VkFenceCreateInfo fence_ci = {0};
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VK_CHECK(vkCreateFence_fn(vulkan_state.device, &fence_ci, NULL, &fence));
    VK_CHECK(vkQueueSubmit_fn(vulkan_state.graphics_queue, 1, &submit_info, fence));
    VK_CHECK(vkWaitForFences_fn(vulkan_state.device, 1, &fence, VK_TRUE, UINT64_MAX));
    vkDestroyFence_fn(vulkan_state.device, fence, NULL);

    vkFreeCommandBuffers_fn(vulkan_state.device, pool, 1, &command_buffer);
}

/* ------------------------------------------------------------------ */
/* Backbuffer creation (off-screen render target)                      */
/* ------------------------------------------------------------------ */

/* Derive the backbuffer size from the current resolution and screen size.
 * Mirrors GL render_set_resolution (render_gl.c:520-534): NATIVE tracks the
 * window, 240P/480P keep the window aspect at 240/480 scanlines. */
static vec2i_t choose_backbuffer_size(render_resolution_t res, vec2i_t screen_size) {
    if (res == RENDER_RES_NATIVE) {
        return screen_size;
    }
    float aspect = (float)screen_size.x / (float)screen_size.y;
    if (res == RENDER_RES_240P) {
        return vec2i(240.0f * aspect, 240);
    }
    return vec2i(480.0f * aspect, 480); /* RENDER_RES_480P */
}

static bool create_backbuffer(void) {
    /* Uses the caller-set backbuffer_size as-is. It is set by render_init
     * (NATIVE -> screen_size) and by the deferred-resize block in
     * render_frame_prepare (recomputed from render_res + screen_size).
     * This function must NOT clobber render_res/backbuffer_size — doing so
     * silently reverted every resolution change to NATIVE. */
    VkExtent2D bb_ext = choose_backbuffer_extent(vulkan_state.backbuffer_size);

    /* Color image */
    if (!create_image(bb_ext.width, bb_ext.height, 1, vulkan_state.swapchain_format,
                      VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      &vulkan_state.backbuffer_image, &vulkan_state.backbuffer_image_memory)) {
        return false;
    }

    /* Depth image */
    if (!create_image(bb_ext.width, bb_ext.height, 1, VK_FORMAT_D32_SFLOAT,
                      VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      &vulkan_state.backbuffer_depth_image, &vulkan_state.backbuffer_depth_image_memory)) {
        return false;
    }

    /* Color image view */
    {
        VkImageViewCreateInfo view_ci = {0};
        view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_ci.image = vulkan_state.backbuffer_image;
        view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_ci.format = vulkan_state.swapchain_format;
        view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_ci.subresourceRange.levelCount = 1;
        view_ci.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView_fn(vulkan_state.device, &view_ci, NULL, &vulkan_state.backbuffer_image_view));
    }

    /* Depth image view */
    {
        VkImageViewCreateInfo view_ci = {0};
        view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_ci.image = vulkan_state.backbuffer_depth_image;
        view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_ci.format = VK_FORMAT_D32_SFLOAT;
        view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        view_ci.subresourceRange.levelCount = 1;
        view_ci.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView_fn(vulkan_state.device, &view_ci, NULL, &vulkan_state.backbuffer_depth_image_view));
    }

    /* Backbuffer starts undefined — lazily transitioned on first frame */
    vulkan_state.backbuffer_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    vulkan_state.backbuffer_depth_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    return true;
}

static void destroy_backbuffer(void) {
    if (vulkan_state.backbuffer_image_view) {
        vkDestroyImageView_fn(vulkan_state.device, vulkan_state.backbuffer_image_view, NULL);
        vulkan_state.backbuffer_image_view = VK_NULL_HANDLE;
    }
    if (vulkan_state.backbuffer_depth_image_view) {
        vkDestroyImageView_fn(vulkan_state.device, vulkan_state.backbuffer_depth_image_view, NULL);
        vulkan_state.backbuffer_depth_image_view = VK_NULL_HANDLE;
    }
    if (vulkan_state.backbuffer_image) {
        vkFreeMemory_fn(vulkan_state.device, vulkan_state.backbuffer_image_memory, NULL);
        vkDestroyImage_fn(vulkan_state.device, vulkan_state.backbuffer_image, NULL);
        vulkan_state.backbuffer_image = VK_NULL_HANDLE;
    }
    if (vulkan_state.backbuffer_depth_image) {
        vkFreeMemory_fn(vulkan_state.device, vulkan_state.backbuffer_depth_image_memory, NULL);
        vkDestroyImage_fn(vulkan_state.device, vulkan_state.backbuffer_depth_image, NULL);
        vulkan_state.backbuffer_depth_image = VK_NULL_HANDLE;
    }
}

/* Recreate the backbuffer sampler and repoint the post-pass descriptor set
 * at the new backbuffer image view. Called after create_backbuffer() in the
 * deferred-resize path (and at init the sampler/descriptor are created
 * separately). Without repointing the descriptor, the post-pass shader keeps
 * sampling a destroyed image view and the GPU hangs on the next draw. */
/* Rebind the backbuffer descriptor to the (recreated) image view. The
 * sampler itself is size-independent and is created once in render_init, so
 * a resize only needs the host-side descriptor update. */
static bool update_backbuffer_descriptor(void) {
    VkWriteDescriptorSet write = {0};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vulkan_state.backbuffer_descriptor_set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &(VkDescriptorImageInfo){
        .sampler = vulkan_state.backbuffer_sampler,
        .imageView = vulkan_state.backbuffer_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    vkUpdateDescriptorSets_fn(vulkan_state.device, 1, &write, 0, NULL);
    return true;
}

/* ------------------------------------------------------------------ */
/* Vertex buffer creation                                              */
/* ------------------------------------------------------------------ */

static bool create_vertex_buffer(void) {
    vulkan_state.vertex_data = (vertex_t *)calloc(RENDER_TRIS_BUFFER_CAPACITY * 3, sizeof(vertex_t));
    if (!vulkan_state.vertex_data) {
        return false;
    }

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkBufferCreateInfo buffer_ci = {0};
        buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_ci.size = sizeof(vertex_t) * RENDER_TRIS_BUFFER_CAPACITY * 3;
        buffer_ci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_ci.flags = 0;

        if (vkCreateBuffer_fn(vulkan_state.device, &buffer_ci, NULL,
                              &vulkan_state.vertex_buffers[i]) != VK_SUCCESS) {
            return false;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements_fn(vulkan_state.device, vulkan_state.vertex_buffers[i], &mem_reqs);

        VkMemoryAllocateInfo alloc_ci = {0};
        alloc_ci.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_ci.allocationSize = mem_reqs.size;
        alloc_ci.memoryTypeIndex = find_memory_type(vulkan_state.physical_device, mem_reqs.memoryTypeBits,
                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (alloc_ci.memoryTypeIndex == UINT32_MAX ||
            vkAllocateMemory_fn(vulkan_state.device, &alloc_ci, NULL,
                                &vulkan_state.vertex_buffer_memories[i]) != VK_SUCCESS) {
            vkDestroyBuffer_fn(vulkan_state.device, vulkan_state.vertex_buffers[i], NULL);
            vulkan_state.vertex_buffers[i] = VK_NULL_HANDLE;
            return false;
        }

        if (vkBindBufferMemory_fn(vulkan_state.device, vulkan_state.vertex_buffers[i],
                                  vulkan_state.vertex_buffer_memories[i], 0) != VK_SUCCESS) {
            vkFreeMemory_fn(vulkan_state.device, vulkan_state.vertex_buffer_memories[i], NULL);
            vulkan_state.vertex_buffer_memories[i] = VK_NULL_HANDLE;
            vkDestroyBuffer_fn(vulkan_state.device, vulkan_state.vertex_buffers[i], NULL);
            vulkan_state.vertex_buffers[i] = VK_NULL_HANDLE;
            return false;
        }

        /* Memory is HOST_COHERENT and the GPU only reads these buffers, so map
         * each one once for the lifetime of the renderer and memcpy into the
         * persistent pointer on flush (no per-flush map/unmap). */
        void *data;
        if (vkMapMemory_fn(vulkan_state.device, vulkan_state.vertex_buffer_memories[i], 0,
                           buffer_ci.size, 0, &data) != VK_SUCCESS) {
            vkFreeMemory_fn(vulkan_state.device, vulkan_state.vertex_buffer_memories[i], NULL);
            vulkan_state.vertex_buffer_memories[i] = VK_NULL_HANDLE;
            vkDestroyBuffer_fn(vulkan_state.device, vulkan_state.vertex_buffers[i], NULL);
            vulkan_state.vertex_buffers[i] = VK_NULL_HANDLE;
            return false;
        }
        vulkan_state.vertex_buffer_ptrs[i] = data;
    }

    return true;
}

static void upload_vertex_buffer(uint32_t frame_index, uint32_t offset, uint32_t vertex_count) {
    if (vertex_count == 0) return;
    /* The buffer memory is mapped once at creation (see create_vertex_buffer);
     * just memcpy into the persistent mapping. */
    memcpy((char *)vulkan_state.vertex_buffer_ptrs[frame_index] + offset * sizeof(vertex_t),
           vulkan_state.vertex_data + offset, sizeof(vertex_t) * vertex_count);
}

/* Fill a (tw+2B) x (th+2B) border-padded block at the top-left of `dst`,
 * writing each row at stride `dst_stride` pixels. `dst_stride` must be the
 * full-atlas row stride (atlas_width) because `dst` points into the full-atlas
 * staging raster at the block's atlas position (x, y); the block copy reads it
 * back at the same stride. Replicates edge pixels into the ATLAS_BORDER border
 * on all sides. Mirrors the GL reference border fill. */
static void atlas_fill_block(rgba_t *dst, uint32_t dst_stride, uint32_t bw, uint32_t bh, uint32_t tw, uint32_t th, rgba_t *pixels) {
    if (!tw || !th) return;

    /* Top border: replicate row 0 */
    for (uint32_t y = 0; y < ATLAS_BORDER; y++) {
        memcpy(dst + dst_stride * y + ATLAS_BORDER, pixels, tw * sizeof(rgba_t));
    }

    /* Bottom border: replicate last row */
    for (uint32_t y = 0; y < ATLAS_BORDER; y++) {
        memcpy(dst + dst_stride * (bh - ATLAS_BORDER + y) + ATLAS_BORDER, pixels + tw * (th - 1), tw * sizeof(rgba_t));
    }

    /* Left/right borders: clamp row to [0, th-1], replicate edge pixels */
    for (uint32_t y = 0; y < bh; y++) {
        uint32_t row = clamp((int32_t)y - ATLAS_BORDER, 0, (int32_t)th - 1);
        for (uint32_t x = 0; x < ATLAS_BORDER; x++) {
            dst[y * dst_stride + x] = pixels[row * tw];
            dst[y * dst_stride + x + bw - ATLAS_BORDER] = pixels[row * tw + tw - 1];
        }
    }

    /* Texture body */
    for (uint32_t y = 0; y < th; y++) {
        memcpy(dst + dst_stride * (y + ATLAS_BORDER) + ATLAS_BORDER, pixels + tw * y, tw * sizeof(rgba_t));
    }
}

/* ------------------------------------------------------------------ */
/* Texture atlas creation                                              */
/* ------------------------------------------------------------------ */

static bool create_atlas(void) {
    uint32_t atlas_width = ATLAS_SIZE * ATLAS_GRID;
    uint32_t atlas_height = ATLAS_SIZE * ATLAS_GRID;

    /* Create atlas image. R8G8B8A8 matches the CPU rgba_t (R,G,B,A) byte
     * order written by atlas_fill_block, mirroring the GL GL_RGBA atlas.
     * The full mip chain mirrors GL's glGenerateMipmap; the clear value
     * matches GL's zero-initialized atlas. */
    if (!create_image(atlas_width, atlas_height, ATLAS_MIP_LEVELS,
                      VK_FORMAT_R8G8B8A8_UNORM,
                      VK_IMAGE_TILING_OPTIMAL,
                      /* TRANSFER_SRC: mip generation blits the atlas onto its
                       * own higher mip levels (src and dst). */
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      &vulkan_state.atlas_image, &vulkan_state.atlas_image_memory)) {
        return false;
    }

    /* Create sampler. Mirrors the GL atlas parameters: NEAREST magnification
     * (pixels stay crisp when magnified), trilinear minification over the
     * mip chain (stable on minified/edge-on surfaces), CLAMP_TO_EDGE, and
     * hardware-max anisotropy for oblique surfaces. */
    VkSamplerCreateInfo sampler_ci = {0};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.magFilter = VK_FILTER_NEAREST;
    sampler_ci.minFilter = VK_FILTER_LINEAR;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_ci.minLod = 0;
    sampler_ci.maxLod = (float)(ATLAS_MIP_LEVELS - 1);
    /* Without anisotropyEnable the maxAnisotropy field is ignored and the
     * sampler runs at 1x — thin edge-on surfaces (e.g. wing tips) would
     * shimmer as the sampled texel jumps. GL sets GL_TEXTURE_MAX_ANISOTROPY_EXT,
     * which implies enabled. */
    sampler_ci.anisotropyEnable = VK_TRUE;
    sampler_ci.maxAnisotropy = vulkan_state.max_anisotropy;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.compareOp = VK_COMPARE_OP_NEVER;

    if (vkCreateSampler_fn(vulkan_state.device, &sampler_ci, NULL, &vulkan_state.atlas_sampler) != VK_SUCCESS) {
        return false;
    }

    /* Second atlas sampler for 240P/480P: GL switches the atlas min filter to
     * NEAREST at low resolutions (render_gl.c:561-568) so the upscaled
     * backbuffer keeps crisp pixels instead of trilinear blur. GL_NEAREST
     * never samples mips, so clamp the LOD to level 0. */
    VkSamplerCreateInfo nearest_ci = sampler_ci;
    nearest_ci.minFilter = VK_FILTER_NEAREST;
    nearest_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    nearest_ci.maxLod = 0.0f;
    if (vkCreateSampler_fn(vulkan_state.device, &nearest_ci, NULL, &vulkan_state.atlas_nearest_sampler) != VK_SUCCESS) {
        return false;
    }

    /* Create image view */
    {
        VkImageViewCreateInfo view_ci = {0};
        view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_ci.image = vulkan_state.atlas_image;
        view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_ci.format = VK_FORMAT_R8G8B8A8_UNORM;
        view_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_ci.subresourceRange.levelCount = ATLAS_MIP_LEVELS;
        view_ci.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView_fn(vulkan_state.device, &view_ci, NULL, &vulkan_state.atlas_image_view));
    }

    /* Atlas starts undefined — will be lazily transitioned on first frame */
    vulkan_state.atlas_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    return true;
}

static void destroy_atlas(void) {
    if (vulkan_state.atlas_image_view) {
        vkDestroyImageView_fn(vulkan_state.device, vulkan_state.atlas_image_view, NULL);
        vulkan_state.atlas_image_view = VK_NULL_HANDLE;
    }
    if (vulkan_state.atlas_sampler) {
        vkDestroySampler_fn(vulkan_state.device, vulkan_state.atlas_sampler, NULL);
        vulkan_state.atlas_sampler = VK_NULL_HANDLE;
    }
    if (vulkan_state.atlas_nearest_sampler) {
        vkDestroySampler_fn(vulkan_state.device, vulkan_state.atlas_nearest_sampler, NULL);
        vulkan_state.atlas_nearest_sampler = VK_NULL_HANDLE;
    }
    if (vulkan_state.atlas_image) {
        vkFreeMemory_fn(vulkan_state.device, vulkan_state.atlas_image_memory, NULL);
        vkDestroyImage_fn(vulkan_state.device, vulkan_state.atlas_image, NULL);
        vulkan_state.atlas_image = VK_NULL_HANDLE;
    }
}

/* ------------------------------------------------------------------ */
/* Shader module loading (SPIR-V)                                      */
/* ------------------------------------------------------------------ */

static VkShaderModule load_shader_module(const char *filename) {
    char path[600];
    /* Resolve relative to the executable instead of the CWD so the game
     * works from any directory: the build places wipeout and shaders/
     * side by side under build/. */
    char exe_dir[512];
    ssize_t len = readlink("/proc/self/exe", exe_dir, sizeof(exe_dir) - 1);
    if (len <= 0 || len >= (ssize_t)sizeof(exe_dir)) {
        die("Failed to resolve executable path for shader lookup");
    }
    exe_dir[len] = '\0';
    char *slash = strrchr(exe_dir, '/');
    if (!slash) {
        die("Unexpected executable path: %s", exe_dir);
    }
    *slash = '\0';
    int path_len = snprintf(path, sizeof(path), "%s/%s", exe_dir, filename);
    if (path_len < 0 || (size_t)path_len >= sizeof(path)) {
        die("Shader path too long: %s/%s", exe_dir, filename);
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open shader file: %s\n", path);
        return VK_NULL_HANDLE;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "Failed to seek in shader file: %s\n", path);
        fclose(f);
        return VK_NULL_HANDLE;
    }
    long size = ftell(f);
    if (size <= 0) {
        fprintf(stderr, "Failed to determine shader file size: %s\n", path);
        fclose(f);
        return VK_NULL_HANDLE;
    }
    fseek(f, 0, SEEK_SET);

    unsigned char *spv = (unsigned char *)malloc((size_t)size);
    if (!spv) {
        fprintf(stderr, "Failed to allocate %ld bytes for shader: %s\n", size, path);
        fclose(f);
        return VK_NULL_HANDLE;
    }
    if (fread(spv, 1, (size_t)size, f) != (size_t)size) {
        fprintf(stderr, "Failed to read shader file: %s\n", path);
        free(spv);
        fclose(f);
        return VK_NULL_HANDLE;
    }
    fclose(f);

    VkShaderModule shader_module;
    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = size;
    create_info.pCode = (const uint32_t *)spv;

    if (vkCreateShaderModule_fn(vulkan_state.device, &create_info, NULL, &shader_module) != VK_SUCCESS) {
        free(spv);
        return VK_NULL_HANDLE;
    }

    free(spv);
    return shader_module;
}

/* Create one post-process pipeline (fullscreen quad, no depth, no blend)
 * from the shared post.vert and the given fragment shader. The caller
 * owns and passes the vert module; the frag module is owned here. */
static VkPipeline create_post_pipeline(VkShaderModule vert, const char *frag_spv) {
    VkShaderModule frag = load_shader_module(frag_spv);
    if (vert == VK_NULL_HANDLE || frag == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to load post shaders (%s)\n", frag_spv);
        if (frag != VK_NULL_HANDLE) {
            vkDestroyShaderModule_fn(vulkan_state.device, frag, NULL);
        }
        return VK_NULL_HANDLE;
    }

    VkPipelineShaderStageCreateInfo stages[2] = {0};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName = "main";

    /* Post shader vertex format: vec2 pos + vec2 uv */
    VkVertexInputBindingDescription vb = {0};
    vb.binding = 0;
    vb.stride = sizeof(float) * 4;
    vb.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2] = {0};
    attrs[0].location = 0;
    attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[0].offset = 0;
    attrs[1].location = 1;
    attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[1].offset = sizeof(float) * 2;

    VkPipelineVertexInputStateCreateInfo vi = {0};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &vb;
    vi.vertexAttributeDescriptionCount = 2;
    vi.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo ia = {0};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = {0};
    viewport.x = 0; viewport.y = 0;
    viewport.width = (float)vulkan_state.swapchain_extent.width;
    viewport.height = (float)vulkan_state.swapchain_extent.height;
    viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.offset.x = 0; scissor.offset.y = 0;
    scissor.extent.width = vulkan_state.swapchain_extent.width;
    scissor.extent.height = vulkan_state.swapchain_extent.height;

    VkPipelineViewportStateCreateInfo vp_state = {0};
    vp_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_state.viewportCount = 1;
    vp_state.pViewports = &viewport;
    vp_state.scissorCount = 1;
    vp_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rast = {0};
    rast.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast.depthClampEnable = VK_FALSE;
    rast.rasterizerDiscardEnable = VK_FALSE;
    rast.polygonMode = VK_POLYGON_MODE_FILL;
    rast.cullMode = VK_CULL_MODE_NONE;
    rast.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rast.depthBiasEnable = VK_FALSE;
    rast.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState color_att = {0};
    color_att.blendEnable = VK_FALSE;
    color_att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cb = {0};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.logicOpEnable = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = &color_att;

    VkPipelineDynamicStateCreateInfo dyn = {0};
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = 2;
    dyn.pDynamicStates = (VkDynamicState[]){VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    /* Dynamic rendering (Vulkan 1.3 core): the pipeline is not tied to a
     * render pass — declare the attachment formats here instead. The post
     * pass renders to a swapchain image (swapchain_format), no depth. */
    VkFormat post_color_formats[1] = {vulkan_state.swapchain_format};
    VkPipelineRenderingCreateInfo post_rendering_ci = {0};
    post_rendering_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    post_rendering_ci.viewMask = 0;
    post_rendering_ci.colorAttachmentCount = 1;
    post_rendering_ci.pColorAttachmentFormats = post_color_formats;
    post_rendering_ci.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    post_rendering_ci.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo pi = {0};
    pi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pi.pNext = &post_rendering_ci;
    pi.stageCount = 2;
    pi.pStages = stages;
    pi.pVertexInputState = &vi;
    pi.pInputAssemblyState = &ia;
    pi.pViewportState = &vp_state;
    pi.pRasterizationState = &rast;
    pi.pColorBlendState = &cb;
    pi.pDynamicState = &dyn;
    pi.layout = vulkan_state.pipeline_layout;
    pi.subpass = 0;

    VkPipelineMultisampleStateCreateInfo ms = {0};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pi.pMultisampleState = &ms;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkResult result = vkCreateGraphicsPipelines_fn(vulkan_state.device, VK_NULL_HANDLE, 1, &pi, NULL, &pipeline);
    vkDestroyShaderModule_fn(vulkan_state.device, frag, NULL);

    if (result != VK_SUCCESS || pipeline == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to create post pipeline (%s)\n", frag_spv);
        return VK_NULL_HANDLE;
    }
    return pipeline;
}

/* ------------------------------------------------------------------ */
/* Pipeline creation                                                   */
/* ------------------------------------------------------------------ */

/* Create one game-pass pipeline (one per blend mode). Depth test/write and
 * cull mode are dynamic state (extendedDynamicState, Vulkan 1.3 core), so
 * the static values below are ignored and render_flush issues vkCmdSet*
 * after the bind; only front face and depth compare op stay static. Front
 * face is always CLOCKWISE in the game pass: the backbuffer is vertically
 * flipped relative to the presented image, which reverses triangle winding,
 * so VK_CULL_MODE_BACK_BIT culls true back-faces (mirrors GL
 * glFrontFace(GL_CCW) + glCullFace(GL_BACK)). Depth bias stays dynamic too
 * (vkCmdSetDepthBias). */
static VkPipeline create_game_pipeline(VkShaderModule vert, VkShaderModule frag,
                                       bool alpha_blend) {
    VkPipelineShaderStageCreateInfo stages[2] = {0};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName = "main";

    VkVertexInputBindingDescription vb = {0};
    vb.binding = 0;
    vb.stride = sizeof(vertex_t);
    vb.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[3] = {0};
    attrs[0].location = 0;
    attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT; /* pos: 3 floats */
    attrs[0].offset = offsetof(vertex_t, pos);
    attrs[1].location = 1;
    attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32_SFLOAT; /* uv: 2 floats */
    attrs[1].offset = offsetof(vertex_t, uv);
    attrs[2].location = 2;
    attrs[2].binding = 0;
    attrs[2].format = VK_FORMAT_R8G8B8A8_UNORM; /* color */
    attrs[2].offset = offsetof(vertex_t, color);

    VkPipelineVertexInputStateCreateInfo vi = {0};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &vb;
    vi.vertexAttributeDescriptionCount = 3;
    vi.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo ia = {0};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = {0};
    viewport.x = 0; viewport.y = 0;
    viewport.width = (float)vulkan_state.backbuffer_size.x;
    viewport.height = (float)vulkan_state.backbuffer_size.y;
    viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.offset.x = 0; scissor.offset.y = 0;
    scissor.extent.width = vulkan_state.backbuffer_size.x;
    scissor.extent.height = vulkan_state.backbuffer_size.y;

    VkPipelineViewportStateCreateInfo vp_state = {0};
    vp_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_state.viewportCount = 1;
    vp_state.pViewports = &viewport;
    vp_state.scissorCount = 1;
    vp_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rast = {0};
    rast.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast.depthClampEnable = VK_FALSE;
    rast.rasterizerDiscardEnable = VK_FALSE;
    rast.polygonMode = VK_POLYGON_MODE_FILL;
    /* Dynamic (see below) — the static value is ignored, keep it valid. */
    rast.cullMode = VK_CULL_MODE_NONE;
    rast.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rast.depthBiasEnable = VK_TRUE;
    rast.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState color_att = {0};
    color_att.blendEnable = VK_TRUE;
    if (alpha_blend) {
        /* NORMAL: alpha blend */
        color_att.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_att.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_att.colorBlendOp = VK_BLEND_OP_ADD;
        color_att.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_att.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_att.alphaBlendOp = VK_BLEND_OP_ADD;
    } else {
        /* LIGHTER: additive blend */
        color_att.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_att.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        color_att.colorBlendOp = VK_BLEND_OP_ADD;
        color_att.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_att.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_att.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    color_att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cb = {0};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.logicOpEnable = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = &color_att;

    /* Core 1.0 dynamic states plus the extended dynamic states for depth
     * test/write and cull mode (extendedDynamicState, Vulkan 1.3 core).
     * Front face stays static CLOCKWISE (see above). */
    VkPipelineDynamicStateCreateInfo dyn = {0};
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = 6;
    dyn.pDynamicStates = (VkDynamicState[]){
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_BIAS,
        VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
        VK_DYNAMIC_STATE_CULL_MODE,
    };

    VkPipelineDepthStencilStateCreateInfo ds = {0};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    /* Dynamic (see above) — the static values are ignored, keep them valid. */
    ds.depthTestEnable = VK_FALSE;
    ds.depthWriteEnable = VK_FALSE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.stencilTestEnable = VK_FALSE;

    /* Dynamic rendering (Vulkan 1.3 core): the pipeline is not tied to a
     * render pass — declare the attachment formats here instead. The game
     * pass renders to the backbuffer (swapchain_format color, D32 depth). */
    VkFormat game_color_formats[1] = {vulkan_state.swapchain_format};
    VkPipelineRenderingCreateInfo game_rendering_ci = {0};
    game_rendering_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    game_rendering_ci.viewMask = 0;
    game_rendering_ci.colorAttachmentCount = 1;
    game_rendering_ci.pColorAttachmentFormats = game_color_formats;
    game_rendering_ci.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
    game_rendering_ci.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo pi = {0};
    pi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pi.pNext = &game_rendering_ci;
    pi.stageCount = 2;
    pi.pStages = stages;
    pi.pVertexInputState = &vi;
    pi.pInputAssemblyState = &ia;
    pi.pViewportState = &vp_state;
    pi.pRasterizationState = &rast;
    pi.pDepthStencilState = &ds;
    pi.pColorBlendState = &cb;
    pi.pDynamicState = &dyn;
    pi.layout = vulkan_state.pipeline_layout;
    pi.subpass = 0;

    VkPipelineMultisampleStateCreateInfo ms = {0};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pi.pMultisampleState = &ms;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkResult result = vkCreateGraphicsPipelines_fn(vulkan_state.device, VK_NULL_HANDLE, 1, &pi, NULL, &pipeline);
    if (result != VK_SUCCESS || pipeline == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }
    return pipeline;
}

static bool create_pipelines(void) {
    /* Create pipeline layout */
    VkPipelineLayoutCreateInfo layout_ci = {0};
    layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_ci.setLayoutCount = 1;
    layout_ci.pSetLayouts = &vulkan_state.descriptor_set_layout;

    /* Single push constant range covering all shader stages.
     * Game pass push block: mat4 u_mvp (64) + vec2 u_screen (8) = 72 bytes.
     * Post pass push block: vec2 u_resolution (8) + float u_time (4) = 12 bytes.
     * Both fit within this single 72-byte range. */
    VkPushConstantRange push_range = {0};
    push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    push_range.offset = 0;
    push_range.size = 72; /* 64 (MVP) + 8 (screen) */

    VkPushConstantRange push_ranges[1] = {push_range};
    layout_ci.pushConstantRangeCount = 1;
    layout_ci.pPushConstantRanges = push_ranges;

    if (vkCreatePipelineLayout_fn(vulkan_state.device, &layout_ci, NULL, &vulkan_state.pipeline_layout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline layout\n");
        return false;
    }

    /* --- Game pipelines: one per blend mode. Depth test/write and cull mode
     * are dynamic state (see game_pipelines); render_flush issues the
     * matching vkCmdSet* after each bind. --- */
    {
        VkShaderModule vert = load_shader_module("shaders/normal.vert.spvasrc.spv");
        VkShaderModule frag = load_shader_module("shaders/normal.frag.spvasrc.spv");
        if (vert == VK_NULL_HANDLE || frag == VK_NULL_HANDLE) {
            fprintf(stderr, "Failed to load normal shaders\n");
            vkDestroyShaderModule_fn(vulkan_state.device, vert, NULL);
            vkDestroyShaderModule_fn(vulkan_state.device, frag, NULL);
            return false;
        }
        vulkan_state.game_pipelines[RENDER_BLEND_NORMAL] = create_game_pipeline(vert, frag, true);
        vkDestroyShaderModule_fn(vulkan_state.device, vert, NULL);
        vkDestroyShaderModule_fn(vulkan_state.device, frag, NULL);
        if (vulkan_state.game_pipelines[RENDER_BLEND_NORMAL] == VK_NULL_HANDLE) {
            fprintf(stderr, "Failed to create normal pipeline\n");
            return false;
        }
    }

    {
        VkShaderModule vert = load_shader_module("shaders/lighter.vert.spvasrc.spv");
        VkShaderModule frag = load_shader_module("shaders/lighter.frag.spvasrc.spv");
        if (vert == VK_NULL_HANDLE || frag == VK_NULL_HANDLE) {
            fprintf(stderr, "Failed to load lighter shaders\n");
            vkDestroyShaderModule_fn(vulkan_state.device, vert, NULL);
            vkDestroyShaderModule_fn(vulkan_state.device, frag, NULL);
            return false;
        }
        vulkan_state.game_pipelines[RENDER_BLEND_LIGHTER] = create_game_pipeline(vert, frag, false);
        vkDestroyShaderModule_fn(vulkan_state.device, vert, NULL);
        vkDestroyShaderModule_fn(vulkan_state.device, frag, NULL);
        if (vulkan_state.game_pipelines[RENDER_BLEND_LIGHTER] == VK_NULL_HANDLE) {
            fprintf(stderr, "Failed to create lighter pipeline\n");
            return false;
        }
    }

    /* --- Post pipelines (fullscreen quad, no depth, no blend).
     * post_pipelines[RENDER_POST_NONE] = plain copy (GL default),
     * post_pipelines[RENDER_POST_CRT]   = CRT effect. --- */
    {
        VkShaderModule vert = load_shader_module("shaders/post.vert.spvasrc.spv");
        if (vert == VK_NULL_HANDLE) {
            fprintf(stderr, "Failed to load post shaders\n");
            return false;
        }

        const char *post_frag_spvs[NUM_RENDER_POST_EFFECTS] = {
            "shaders/post.frag.spvasrc.spv",
            "shaders/post_crt.frag.spvasrc.spv",
        };
        for (int i = 0; i < NUM_RENDER_POST_EFFECTS; i++) {
            vulkan_state.post_pipelines[i] = create_post_pipeline(vert, post_frag_spvs[i]);
            if (vulkan_state.post_pipelines[i] == VK_NULL_HANDLE) {
                vkDestroyShaderModule_fn(vulkan_state.device, vert, NULL);
                return false;
            }
        }
        vkDestroyShaderModule_fn(vulkan_state.device, vert, NULL);
    }

    printf("Vulkan: 4 pipelines created (2 game blend modes, 2 post)\n");

    return true;
}

static void destroy_pipelines(void) {
    if (vulkan_state.pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout_fn(vulkan_state.device, vulkan_state.pipeline_layout, NULL);
        vulkan_state.pipeline_layout = VK_NULL_HANDLE;
    }
    for (int b = 0; b < 2; b++) {
        if (vulkan_state.game_pipelines[b] != VK_NULL_HANDLE) {
            vkDestroyPipeline_fn(vulkan_state.device, vulkan_state.game_pipelines[b], NULL);
            vulkan_state.game_pipelines[b] = VK_NULL_HANDLE;
        }
    }
    for (int i = 0; i < NUM_RENDER_POST_EFFECTS; i++) {
        if (vulkan_state.post_pipelines[i] != VK_NULL_HANDLE) {
            vkDestroyPipeline_fn(vulkan_state.device, vulkan_state.post_pipelines[i], NULL);
            vulkan_state.post_pipelines[i] = VK_NULL_HANDLE;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Frame prepare / end                                                 */
/* ------------------------------------------------------------------ */

void render_set_resolution(render_resolution_t res) {
    if (!vulkan_state.device) {
        fprintf(stderr, "[VK] render_set_resolution: device is NULL (renderer not initialized)\n");
        return;
    }
    vulkan_state.render_res = res;
    /* Flag-only: the backbuffer (and its sampler/descriptor/projections) are
     * actually recreated in render_frame_prepare, after vkQueueWaitIdle.
     * Recreating here — mid-frame, with an in-flight command buffer that still
     * references the old backbuffer — is the UB that froze the game. */
    vulkan_state.backbuffer_recreate_pending = true;
}

void render_flush_pending_textures(void);

void render_frame_prepare(void) {
    /* Deferred resize: render_set_screen_size / render_set_resolution may be
     * called mid-frame (game_update, SDL event pump). They only set flags.
     * The actual swapchain / backbuffer recreation happens HERE, at the top
     * of prepare, after vkQueueWaitIdle — the only point where destroying the
     * old resources is safe (every in-flight command buffer that referenced
     * them has completed). On failure we leave the flag set and skip this
     * frame (command_buffer is NULL, cleared at the end of the previous
     * render_frame_end, so render_frame_end's guard skips it); the next
     * prepare retries. Uploads stay pending across the skip. */
    if (vulkan_state.swapchain_recreate_pending || vulkan_state.backbuffer_recreate_pending) {
        vkQueueWaitIdle_fn(vulkan_state.graphics_queue);

        /* Step 1: swapchain (if pending). create_swapchain() re-syncs
         * screen_size to the extent the driver actually reports, so the
         * backbuffer (NATIVE) tracks the new window size. Must succeed
         * before we touch the backbuffer; on failure retry next frame.
         *
         * The replacement is created FIRST, passing the old swapchain as
         * oldSwapchain (which retires it), and only then is the old one
         * destroyed. Destroy-then-create left a window where the surface
         * had no valid presentation source — the old swapchain's displayed
         * image was gone before the new swapchain had presented anything —
         * which deadlocked the presentation engine on fullscreen toggles. */
        if (vulkan_state.swapchain_recreate_pending) {
            VkSwapchainKHR old_swapchain = vulkan_state.swapchain;
            VkImage *old_images = vulkan_state.swapchain_images;
            VkImageView *old_views = vulkan_state.swapchain_image_views;
            uint32_t old_count = vulkan_state.swapchain_image_count;
            vulkan_state.swapchain = VK_NULL_HANDLE;
            vulkan_state.swapchain_images = NULL;
            vulkan_state.swapchain_image_views = NULL;
            vulkan_state.swapchain_image_count = 0;

            if (!create_swapchain(old_swapchain)) {
                /* Creation failed: old_swapchain is already retired (no more
                 * acquires from it), so destroy it and any partially-created
                 * new resources; retry next frame. */
                destroy_swapchain();
                destroy_swapchain_resources(old_swapchain, old_images, old_views,
                                            old_count);
                return;
            }

            /* New swapchain is live: destroy the retired old one. Its image
             * memory is freed by the driver only once the presentation
             * engine stops using it (spec-guaranteed). */
            destroy_swapchain_resources(old_swapchain, old_images, old_views,
                                        old_count);
            vulkan_state.swapchain_recreate_pending = false;
        }

        /* Step 2: backbuffer (if pending). screen_size is now fresh. */
        if (vulkan_state.backbuffer_recreate_pending) {
            vulkan_state.backbuffer_size = choose_backbuffer_size(vulkan_state.render_res, vulkan_state.screen_size);
            destroy_backbuffer();
            if (!create_backbuffer()) {
                return;
            }
            if (!update_backbuffer_descriptor()) {
                return;
            }
            /* Atlas min filter follows the resolution (GL parity:
             * render_gl.c:561-568): trilinear at NATIVE, NEAREST at 240P/480P.
             * Re-write the existing atlas descriptor set — host-side update,
             * visible to this frame's (not yet recorded) game pass. */
            {
                VkWriteDescriptorSet atlas_write = {0};
                atlas_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                atlas_write.dstSet = vulkan_state.atlas_descriptor_set;
                atlas_write.dstBinding = 0;
                atlas_write.descriptorCount = 1;
                atlas_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                atlas_write.pImageInfo = &(VkDescriptorImageInfo){
                    .sampler = vulkan_state.render_res == RENDER_RES_NATIVE ?
                               vulkan_state.atlas_sampler : vulkan_state.atlas_nearest_sampler,
                    .imageView = vulkan_state.atlas_image_view,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                };
                vkUpdateDescriptorSets_fn(vulkan_state.device, 1, &atlas_write, 0, NULL);
            }
            /* Projection matrices from the backbuffer size (GL parity:
             * render_gl.c:557-558). The 2D HUD is drawn in backbuffer space,
             * so it must use backbuffer_size, not screen_size — identical for
             * NATIVE, divergent (and wrong) for 240P/480P otherwise. */
            if (vulkan_state.backbuffer_size.x > 0 && vulkan_state.backbuffer_size.y > 0) {
                vulkan_state.projection_mat_3d = render_setup_3d_projection_mat(vulkan_state.backbuffer_size);
                vulkan_state.projection_mat_2d = render_setup_2d_projection_mat(vulkan_state.backbuffer_size);
            }
            vulkan_state.backbuffer_recreate_pending = false;
        }
    }

    /* Flush any pending texture uploads that accumulated between frames.
     * This must happen before game_update() so newly-loaded textures are
     * available in the atlas for rendering. */
    if (vulkan_state.pending_uploads_len > 0) {
        render_flush_pending_textures();
    }

    /* Determine which per-frame resources to use. */
    uint32_t f = vulkan_state.frame_index;

    /* Wait for GPU to finish with previous use of this frame's resources
     * BEFORE re-acquiring this frame slot's semaphore or resetting the
     * command pool. The previous submit for this slot is the only consumer
     * of image_acquired_semaphores[f]; until that submit completes, the
     * semaphore still has a pending wait and re-acquiring it violates
     * VUID-vkAcquireNextImageKHR-semaphore-01779. Waiting here also
     * prevents the "command buffer in use" cascade that leads to
     * VK_ERROR_DEVICE_LOST.
     *
     * The previous submit for this slot signaled timeline_semaphore at
     * slot_fence_value[f] (recorded when that submit was issued). Waiting on
     * that value blocks until the GPU finished it — the same backpressure
     * the old per-frame fence provided, with no reset to manage. A fresh
     * slot has slot_fence_value[f] == 0 and the semaphore starts at 0, so
     * the first wait per slot returns immediately. */
    VkSemaphoreWaitInfo timeline_wait = {0};
    timeline_wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    timeline_wait.semaphoreCount = 1;
    timeline_wait.pSemaphores = &vulkan_state.timeline_semaphore;
    timeline_wait.pValues = &vulkan_state.slot_fence_value[f];
    VK_CHECK(vkWaitSemaphores_fn(vulkan_state.device, &timeline_wait, UINT64_MAX));

    /* Acquire next swapchain image, signaling an image-available
     * semaphore.  The semaphore is waited on during submit so the GPU
     * does not start rendering until the presentation engine has
     * released the image.  This fixes the VUID-01780 spec violation
     * (both semaphore/fence NULL) that caused the driver to
     * deadlock on FIFO present mode.
     *
     * The timeout is finite, not UINT64_MAX: when the window is minimized
     * the compositor may stop presenting this surface, in which case
     * forward progress for acquire is not guaranteed and a blocking call
     * would hang the game (VUID-vkAcquireNextImageKHR-surface-07783
     * forbids UINT64_MAX in that case). 100ms covers several vsync
     * intervals at any common refresh rate, so a visible window never
     * times out spuriously; a non-presenting one yields VK_TIMEOUT and we
     * skip the frame (command_buffer stays NULL, render_frame_end's guard
     * drops it) and retry next frame — the same path as OUT_OF_DATE. */
    VkResult acquire_result = vkAcquireNextImageKHR_fn(vulkan_state.device, vulkan_state.swapchain,
                             100ull * 1000 * 1000,
                             vulkan_state.image_acquired_semaphores[f],
                             VK_NULL_HANDLE,
                             &vulkan_state.current_swapchain_image);
    if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
        /* No image was acquired. The surface changed (resized, recomposited),
         * so recreate the swapchain at the top of the next prepare; the screen
         * size may have changed too, so recreate the backbuffer as well.
         * command_buffer is still NULL here, so skipping this frame is safe
         * (render_frame_end's guard drops it). */
        vulkan_state.swapchain_recreate_pending = true;
        vulkan_state.backbuffer_recreate_pending = true;
        return;
    } else if (acquire_result == VK_SUBOPTIMAL_KHR) {
        /* SUBOPTIMAL is a SUCCESS code: an image HAS been acquired and the
         * acquire semaphore is signaled, so render this frame normally. The
         * surface is no longer optimal (e.g. display scale changed), so flag
         * the swapchain for recreation before the next frame. */
        vulkan_state.swapchain_recreate_pending = true;
    } else if (acquire_result == VK_TIMEOUT || acquire_result == VK_NOT_READY) {
        /* No image available within the timeout (e.g. window minimized and
         * the presentation engine stopped releasing images). Skip this
         * frame and retry next frame — never block indefinitely. */
        return;
    } else if (acquire_result == VK_ERROR_SURFACE_LOST_KHR ||
               acquire_result == VK_ERROR_DEVICE_LOST) {
        /* The surface or the device is gone; redrawing cannot recover. Log
         * loudly and stop rendering this frame rather than silently looping
         * on a dead surface. */
        fprintf(stderr, "Vulkan: fatal acquire error (%d); renderer stopping\n",
                (int)acquire_result);
        return;
    }

    /* Reset per-frame command pool, then reset the command buffer
     * so it is in recording state. */
    vkResetCommandPool_fn(vulkan_state.device, vulkan_state.command_pools[f], 0);
    if (vulkan_state.command_buffers[f] != VK_NULL_HANDLE) {
        vkResetCommandBuffer_fn(vulkan_state.command_buffers[f], 0);
    }

    /* Allocate command buffer from per-frame pool. */
    if (vulkan_state.command_buffers[f] == VK_NULL_HANDLE) {
        VkCommandBufferAllocateInfo alloc_ci = {0};
        alloc_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_ci.commandPool = vulkan_state.command_pools[f];
        alloc_ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_ci.commandBufferCount = 1;
        if (vkAllocateCommandBuffers_fn(vulkan_state.device, &alloc_ci,
                                         &vulkan_state.command_buffers[f]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to allocate command buffer %u\n", f);
            vulkan_state.command_buffer = VK_NULL_HANDLE;
            return;
        }
    }

    /* Use the per-frame command buffer. */
    vulkan_state.command_buffer = vulkan_state.command_buffers[f];

    /* Begin command buffer */
    VkCommandBufferBeginInfo begin_ci = {0};
    begin_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_ci.flags = 0;
    VkResult begin_result = vkBeginCommandBuffer_fn(vulkan_state.command_buffer, &begin_ci);
    (void)begin_result;

    /* Re-beginning the command buffer invalidates its dynamic state, so the
     * EDS values must be re-issued before the first game-pipeline draw of
     * this frame (render_flush handles it via dyn_state_issued). */
    vulkan_state.dyn_state_issued = false;

    /* Transition backbuffer from its actual previous layout to color attachment.
     * After game pass, backbuffer is in SHADER_READ_ONLY_OPTIMAL (render pass finalLayout).
     * On first frame, it's UNDEFINED. */
    cmd_transition_image_layout(vulkan_state.backbuffer_image,
                                vulkan_state.backbuffer_layout,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vulkan_state.backbuffer_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    cmd_transition_image_layout(vulkan_state.backbuffer_depth_image,
                                vulkan_state.backbuffer_depth_layout,
                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    vulkan_state.backbuffer_depth_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /* Transition atlas from UNDEFINED to shader-read (first frame). The atlas
     * is mipmapped, so the barrier must cover every mip level. */
    if (vulkan_state.atlas_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
        VkImageMemoryBarrier barrier = {0};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = vulkan_state.atlas_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = ATLAS_MIP_LEVELS;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier_fn(vulkan_state.command_buffer,
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                0, 0, NULL, 0, NULL, 1, &barrier);
        vulkan_state.atlas_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkClearValue clear_colors[2];
    memset(&clear_colors, 0, sizeof(clear_colors));
    clear_colors[0].color.float32[0] = 0.0f;
    clear_colors[0].color.float32[1] = 0.0f;
    clear_colors[0].color.float32[2] = 0.0f;
    clear_colors[0].color.float32[3] = 1.0f;
    clear_colors[1].depthStencil.depth = 1.0f;

    /* Dynamic rendering (Vulkan 1.3 core): no render pass / framebuffer.
     * The backbuffer images are already in the declared layouts (transitioned
     * above) — BeginRendering performs no implicit layout transitions, and
     * the images remain in these layouts after EndRendering. */
    VkRenderingAttachmentInfo color_attachment = {0};
    color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_attachment.imageView = vulkan_state.backbuffer_image_view;
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue = clear_colors[0];

    VkRenderingAttachmentInfo depth_attachment = {0};
    depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depth_attachment.imageView = vulkan_state.backbuffer_depth_image_view;
    depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.clearValue = clear_colors[1];

    VkRenderingInfo rendering_info = {0};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea.offset.x = 0;
    rendering_info.renderArea.offset.y = 0;
    rendering_info.renderArea.extent.width = (uint32_t)vulkan_state.backbuffer_size.x;
    rendering_info.renderArea.extent.height = (uint32_t)vulkan_state.backbuffer_size.y;
    rendering_info.layerCount = 1;
    rendering_info.viewMask = 0;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &color_attachment;
    rendering_info.pDepthAttachment = &depth_attachment;
    vkCmdBeginRendering_fn(vulkan_state.command_buffer, &rendering_info);

    /* Viewport & scissor (dynamic) */
    VkViewport vp = {0};
    vp.width  = (float)vulkan_state.backbuffer_size.x;
    vp.height = (float)vulkan_state.backbuffer_size.y;
    vp.maxDepth = 1.0f;
    vkCmdSetViewport_fn(vulkan_state.command_buffer, 0, 1, &vp);

    VkRect2D scissor = {0};
    scissor.extent.width = (uint32_t)vulkan_state.backbuffer_size.x;
    scissor.extent.height = (uint32_t)vulkan_state.backbuffer_size.y;
    vkCmdSetScissor_fn(vulkan_state.command_buffer, 0, 1, &scissor);

    /* Clear vertex counters */
    vulkan_state.vertex_count = 0;
    vulkan_state.vertex_flushed = 0;

    /* Clear frame stats (mirrors GL reference render_frame_prepare) */
    running_stats.num_tris = 0;
    running_stats.num_draw_calls = 0;

    /* Set initial state */
    vulkan_state.current_blend_mode = RENDER_BLEND_NORMAL;
    vulkan_state.current_depth_test = true;
    vulkan_state.current_depth_write = true;
    vulkan_state.current_depth_bias = 0.0f;
    vulkan_state.current_cull_backface = true;
    vulkan_state.current_is_3d = true;
    vulkan_state.current_screen_position = vec2(0, 0);
}

void render_frame_end(void) {
    /* If no command buffer (out-of-date or error), skip rendering this frame */
    if (vulkan_state.command_buffer == VK_NULL_HANDLE) {
        return;
    }

    /* Flush any remaining vertices */
    render_flush();

    /* End game rendering. Dynamic rendering performs no implicit layout
     * transition, so explicitly transition the backbuffer from
     * COLOR_ATTACHMENT_OPTIMAL to SHADER_READ_ONLY_OPTIMAL. This barrier
     * also provides visibility between the game-pass color writes and the
     * post-pass fragment shader reads — srcStage must be
     * COLOR_ATTACHMENT_OUTPUT (not TRANSFER) to order the rasterization
     * writes. */
    vkCmdEndRendering_fn(vulkan_state.command_buffer);

    VkImageMemoryBarrier bb_layout_barrier = {0};
    bb_layout_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    bb_layout_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    bb_layout_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    bb_layout_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bb_layout_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bb_layout_barrier.image = vulkan_state.backbuffer_image;
    bb_layout_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bb_layout_barrier.subresourceRange.levelCount = 1;
    bb_layout_barrier.subresourceRange.layerCount = 1;
    bb_layout_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    bb_layout_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier_fn(vulkan_state.command_buffer,
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                            0, 0, NULL, 0, NULL, 1, &bb_layout_barrier);
    vulkan_state.backbuffer_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    /* ---- Post-process pass ---- */
    /* Dynamic rendering performs no implicit layout transitions, so
     * explicitly transition the acquired swapchain image (UNDEFINED after
     * vkAcquireNextImageKHR) to COLOR_ATTACHMENT_OPTIMAL. */
    cmd_transition_image_layout(vulkan_state.swapchain_images[vulkan_state.current_swapchain_image],
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkClearValue clear_color;
    memset(&clear_color, 0, sizeof(clear_color));
    clear_color.color.float32[0] = 0.0f;
    clear_color.color.float32[1] = 0.0f;
    clear_color.color.float32[2] = 0.0f;
    clear_color.color.float32[3] = 1.0f;

    VkRenderingAttachmentInfo post_color_attachment = {0};
    post_color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    post_color_attachment.imageView = vulkan_state.swapchain_image_views[vulkan_state.current_swapchain_image];
    post_color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    post_color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    post_color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    post_color_attachment.clearValue = clear_color;

    VkRenderingInfo post_rendering_info = {0};
    post_rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    post_rendering_info.renderArea.offset.x = 0;
    post_rendering_info.renderArea.offset.y = 0;
    post_rendering_info.renderArea.extent.width = vulkan_state.swapchain_extent.width;
    post_rendering_info.renderArea.extent.height = vulkan_state.swapchain_extent.height;
    post_rendering_info.layerCount = 1;
    post_rendering_info.viewMask = 0;
    post_rendering_info.colorAttachmentCount = 1;
    post_rendering_info.pColorAttachments = &post_color_attachment;
    vkCmdBeginRendering_fn(vulkan_state.command_buffer, &post_rendering_info);

    /* Viewport & scissor for screen — use swapchain_extent (always valid) */
    VkViewport post_vp = {0};
    post_vp.width  = (float)vulkan_state.swapchain_extent.width;
    post_vp.height = (float)vulkan_state.swapchain_extent.height;
    post_vp.maxDepth = 1.0f;
    vkCmdSetViewport_fn(vulkan_state.command_buffer, 0, 1, &post_vp);

    VkRect2D post_scissor = {0};
    post_scissor.extent = vulkan_state.swapchain_extent;
    vkCmdSetScissor_fn(vulkan_state.command_buffer, 0, 1, &post_scissor);

    /* Bind quad vertex buffer */
    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers_fn(vulkan_state.command_buffer, 0, 1,
                              &vulkan_state.quad_vertex_buffer, offsets);

    /* Bind post descriptor set */
    VkDescriptorSet post_desc_sets[1] = {vulkan_state.backbuffer_descriptor_set};
    vkCmdBindDescriptorSets_fn(vulkan_state.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               vulkan_state.pipeline_layout, 0, 1, post_desc_sets, 0, NULL);

    /* Push post shader constants: vec2 resolution + float time.
     * Time is in seconds, matching GL's `time = system_cycle_time()`. */
    float push_constants[4] = {
        (float)vulkan_state.screen_size.x,
        (float)vulkan_state.screen_size.y,
        (float)system_cycle_time(), /* time in seconds */
        0.0f
    };
    vkCmdPushConstants_fn(vulkan_state.command_buffer, vulkan_state.pipeline_layout,
                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                          0, 16, push_constants);

    /* Bind post pipeline (selected by current_post_effect) */
    vkCmdBindPipeline_fn(vulkan_state.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                         vulkan_state.post_pipelines[vulkan_state.current_post_effect]);

    /* Draw fullscreen quad */
    vkCmdDraw_fn(vulkan_state.command_buffer, 6, 1, 0, 0);

    /* Count the post quad (2 tris, 1 draw call) — mirrors GL reference, which
     * pushes the fullscreen quad through the same tris buffer and counts it
     * in render_flush. */
    running_stats.num_tris += 2;
    running_stats.num_draw_calls++;

    /* End post rendering, then explicitly transition the swapchain image
     * to PRESENT_SRC_KHR. Dynamic rendering leaves the image in the layout
     * declared at BeginRendering (COLOR_ATTACHMENT_OPTIMAL), and the
     * presentation engine requires PRESENT_SRC_KHR. srcStage must be
     * COLOR_ATTACHMENT_OUTPUT to order the rasterization color writes. */
    vkCmdEndRendering_fn(vulkan_state.command_buffer);

    VkImageMemoryBarrier present_barrier = {0};
    present_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    present_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    present_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    present_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    present_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    present_barrier.image = vulkan_state.swapchain_images[vulkan_state.current_swapchain_image];
    present_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    present_barrier.subresourceRange.levelCount = 1;
    present_barrier.subresourceRange.layerCount = 1;
    present_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    present_barrier.dstAccessMask = 0;
    vkCmdPipelineBarrier_fn(vulkan_state.command_buffer,
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                            0, 0, NULL, 0, NULL, 1, &present_barrier);
    /* End command buffer recording */
    VK_CHECK(vkEndCommandBuffer_fn(vulkan_state.command_buffer));
    /* Submit to queue:
     * - wait_semaphore = image_acquired_semaphores[f] (ensures the
     *   presentation engine has finished reading from the image).
     * - signal semaphores: render_complete_semaphores[img] (ensures present
     *   waits for GPU to finish rendering) and timeline_semaphore
     *   (CPU-side backpressure — replaces the old per-frame fence). */
    uint32_t f = vulkan_state.frame_index;
    uint32_t img = vulkan_state.current_swapchain_image;

    /* Assign this submit its explicit timeline value. Values increase
     * monotonically in submit order (queue is FIFO, so signal operations
     * execute in the same order); prepare() CPU-waits on them for
     * backpressure. No reset needed — that's the point of a timeline. */
    uint64_t timeline_value = vulkan_state.timeline_next_value++;
    vulkan_state.slot_fence_value[f] = timeline_value;

    VkSemaphore signal_semaphores[] = {vulkan_state.render_complete_semaphores[img],
                                       vulkan_state.timeline_semaphore};
    /* Parallel to signal_semaphores (VUID-VkSubmitInfo-pNext-03241): the
     * binary semaphore's value is ignored, the timeline gets the real one. */
    uint64_t signal_values[] = {0, timeline_value};

    VkTimelineSemaphoreSubmitInfo timeline_submit = {0};
    timeline_submit.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timeline_submit.signalSemaphoreValueCount = 2; /* == signalSemaphoreCount */
    timeline_submit.pSignalSemaphoreValues = signal_values;

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = &timeline_submit;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &vulkan_state.image_acquired_semaphores[f];
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vulkan_state.command_buffer;
    submit_info.signalSemaphoreCount = 2;
    submit_info.pSignalSemaphores = signal_semaphores;

    VK_CHECK(vkQueueSubmit_fn(vulkan_state.graphics_queue, 1, &submit_info, VK_NULL_HANDLE));

    /* Present — wait only on the binary render-complete semaphore
     * (vkQueuePresentKHR does not accept timeline semaphores). */
    VkSemaphore present_semaphores[] = {vulkan_state.render_complete_semaphores[img]};
    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = present_semaphores;
    present_info.swapchainCount = 1;

    VkSwapchainKHR swapchains[] = {vulkan_state.swapchain};
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &vulkan_state.current_swapchain_image;
    present_info.pResults = NULL;

    VkResult present_result = vkQueuePresentKHR_fn(vulkan_state.graphics_queue, &present_info);

    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
        /* The surface is out of date or no longer optimal; recreate the
         * swapchain (and backbuffer, since the size may have changed) at the
         * top of the next prepare. */
        vulkan_state.swapchain_recreate_pending = true;
        vulkan_state.backbuffer_recreate_pending = true;
    } else if (present_result == VK_ERROR_SURFACE_LOST_KHR ||
               present_result == VK_ERROR_DEVICE_LOST) {
        /* The surface or the device is gone; redrawing cannot recover. Log
         * loudly rather than silently looping on a dead surface. */
        fprintf(stderr, "Vulkan: fatal present error (%d); renderer stopping\n",
                (int)present_result);
    } else if (present_result != VK_SUCCESS) {
        fprintf(stderr, "vkQueuePresentKHR failed: %d\n", present_result);
    }

    /* Only here do we have the complete stats (mirrors GL reference) */
    memcpy(&end_stats, &running_stats, sizeof(render_stats_t));

    /* Advance frame counter (wraps 0..MAX_FRAMES_IN_FLIGHT-1) */
    vulkan_state.frame_index = (f + 1) % MAX_FRAMES_IN_FLIGHT;

    /* Clear working pointer — no command buffer is recording after this
     * frame. Texture uploads queued between frames are flushed at the next
     * prepare (or before the first draw of a frame that created textures
     * mid-frame), so no upload path depends on a non-recording CB here. */
    vulkan_state.command_buffer = VK_NULL_HANDLE;
}

/* ------------------------------------------------------------------ */
/* Render flush — upload vertices, bind pipeline, draw                  */
/* ------------------------------------------------------------------ */

void render_flush(void) {
    if (vulkan_state.vertex_count == 0) return;
    if (vulkan_state.command_buffer == VK_NULL_HANDLE) {
        /* Can't render — drop accumulated vertices to prevent overflow */
        vulkan_state.vertex_count = 0;
        vulkan_state.vertex_flushed = 0;
        return;
    }

    /* Texture uploads queued mid-frame (e.g. a level load during
     * game_update) must reach the atlas before the first draw that could
     * sample them. Transfers cannot be recorded inside the active rendering
     * scope, so end it, flush on a separate command buffer (submit + wait),
     * then re-open with LOAD_OP_LOAD to preserve what is already drawn. The
     * viewport/scissor are dynamic and re-issued here; pipeline/descriptor
     * bindings persist across End/BeginRendering. */
    if (vulkan_state.pending_uploads_len > 0) {
        vkCmdEndRendering_fn(vulkan_state.command_buffer);
        render_flush_pending_textures();

        VkRenderingAttachmentInfo color_attachment = {0};
        color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment.imageView = vulkan_state.backbuffer_image_view;
        color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingAttachmentInfo depth_attachment = {0};
        depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth_attachment.imageView = vulkan_state.backbuffer_depth_image_view;
        depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        VkRenderingInfo rendering_info = {0};
        rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        rendering_info.renderArea.extent.width = (uint32_t)vulkan_state.backbuffer_size.x;
        rendering_info.renderArea.extent.height = (uint32_t)vulkan_state.backbuffer_size.y;
        rendering_info.layerCount = 1;
        rendering_info.viewMask = 0;
        rendering_info.colorAttachmentCount = 1;
        rendering_info.pColorAttachments = &color_attachment;
        rendering_info.pDepthAttachment = &depth_attachment;
        vkCmdBeginRendering_fn(vulkan_state.command_buffer, &rendering_info);

        VkViewport vp = {0};
        vp.width  = (float)vulkan_state.backbuffer_size.x;
        vp.height = (float)vulkan_state.backbuffer_size.y;
        vp.maxDepth = 1.0f;
        vkCmdSetViewport_fn(vulkan_state.command_buffer, 0, 1, &vp);

        VkRect2D scissor = {0};
        scissor.extent.width = (uint32_t)vulkan_state.backbuffer_size.x;
        scissor.extent.height = (uint32_t)vulkan_state.backbuffer_size.y;
        vkCmdSetScissor_fn(vulkan_state.command_buffer, 0, 1, &scissor);
    }

    uint32_t f = vulkan_state.frame_index;

    /* Nothing new since the last flush (cumulative semantics: flushes don't
     * free space) — skip the redundant full-buffer re-upload. */
    if (vulkan_state.vertex_flushed == vulkan_state.vertex_count) return;

    /* Upload vertex data to GPU (per-frame buffer to avoid CPU-GPU race).
     * Only the new slice [vertex_flushed, vertex_count) is uploaded, at its
     * absolute offset in the device buffer. Earlier slices are never
     * touched, so every vkCmdDraw recorded this frame reads valid data when
     * the queue executes at frame end. Total copy work per frame is
     * O(frame size). */
    uint32_t flushed_count = vulkan_state.vertex_count - vulkan_state.vertex_flushed;
    upload_vertex_buffer(f, vulkan_state.vertex_flushed, flushed_count);

    /* Count stats (mirrors GL reference render_flush). Vertices are pushed 3
     * at a time (one per triangle), so the new slice is flushed_count/3
     * triangles, drawn in the single vkCmdDraw below. */
    running_stats.num_tris += flushed_count / 3;
    running_stats.num_draw_calls++;

    /* Compute MVP matrix */
    mat4_t mvp = compute_mvp();

    /* Push MVP + screen (72 bytes)
     * Shader layout: mat4(64) + vec2(8) = 72 */
    float push_constants[18] = {0};
    for (int i = 0; i < 16; i++) push_constants[i] = mvp.m[i];
    /* u_screen is a clip-space position OFFSET (see GL reference: screen uniform
     * defaults to (0,0), set via render_set_screen_position). y is negated to
     * match GL's bottom-left origin convention (both APIs share NDC y+1 = top). */
    push_constants[16] = (float)vulkan_state.current_screen_position.x;
    push_constants[17] = -(float)vulkan_state.current_screen_position.y;

    vkCmdPushConstants_fn(vulkan_state.command_buffer, vulkan_state.pipeline_layout,
                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                          0, 72, push_constants);

    /* Bind atlas descriptor set */
    VkDescriptorSet desc_sets[1] = {vulkan_state.atlas_descriptor_set};
    vkCmdBindDescriptorSets_fn(vulkan_state.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               vulkan_state.pipeline_layout, 0, 1, desc_sets, 0, NULL);

    /* Bind per-frame vertex buffer */
    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers_fn(vulkan_state.command_buffer, 0, 1,
                              &vulkan_state.vertex_buffers[f], offsets);

    /* Bind pipeline: blend mode selects the shader/blend variant. Depth
     * test/write and cull mode are dynamic state (see game_pipelines) and
     * are issued right after the bind. Front face is always CLOCKWISE
     * (baked into both variants). Mirrors the GL reference's
     * glEnable/glDisable(GL_DEPTH_TEST), glDepthMask and
     * glEnable/glDisable(GL_CULL_FACE). */
    vkCmdBindPipeline_fn(vulkan_state.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        vulkan_state.game_pipelines[vulkan_state.current_blend_mode]);
    /* Extended dynamic state (Vulkan 1.3 core) is per-command-buffer: it
     * resets when the CB is re-begun in render_frame_prepare, so
     * dyn_state_issued is reset there and the first game-pipeline bind of
     * each frame re-issues all three; within a frame, re-issue only when a
     * value differs from what was last issued. */
    if (!vulkan_state.dyn_state_issued ||
        vulkan_state.dyn_depth_test != vulkan_state.current_depth_test ||
        vulkan_state.dyn_depth_write != vulkan_state.current_depth_write ||
        vulkan_state.dyn_cull_backface != vulkan_state.current_cull_backface) {
        vkCmdSetDepthTestEnable_fn(vulkan_state.command_buffer,
                                   vulkan_state.current_depth_test ? VK_TRUE : VK_FALSE);
        vkCmdSetDepthWriteEnable_fn(vulkan_state.command_buffer,
                                    vulkan_state.current_depth_write ? VK_TRUE : VK_FALSE);
        vkCmdSetCullMode_fn(vulkan_state.command_buffer,
                            vulkan_state.current_cull_backface ? VK_CULL_MODE_BACK_BIT
                                                               : VK_CULL_MODE_NONE);
        vulkan_state.dyn_depth_test = vulkan_state.current_depth_test;
        vulkan_state.dyn_depth_write = vulkan_state.current_depth_write;
        vulkan_state.dyn_cull_backface = vulkan_state.current_cull_backface;
        vulkan_state.dyn_state_issued = true;
    }
    /* GL glPolygonOffset(factor, units) -> Vulkan vkCmdSetDepthBias(constant,
     * clamp, slope):
     *   factor  -> depthBiasSlopeFactor    (scales the polygon max depth slope m)
     *   units   -> depthBiasConstantFactor (scales r, the min resolvable step)
     * The parameter roles differ between the APIs: in GL the FIRST arg is the
     * SLOPE scale and the SECOND the CONSTANT; in Vulkan the order is
     * (constant, clamp, slope). The ship shadow is drawn with
     * glPolygonOffset(-32, 1), so factor=-32 scales the slope, pushing the
     * shadow in front of the track (toward the camera) proportional to its
     * depth slope; a positive offset would push it behind the track and fail
     * the LESS depth test. depthBiasClamp=0.0 means "no clamping" (with the
     * depthBiasClamp feature disabled, 0 is the only legal value and the bias
     * is unclamped), matching GL's unclamped glPolygonOffset; a nonzero clamp
     * would require the feature and clamps the bias to [-clamp, +clamp].
     * bias==0 mirrors glDisable(GL_POLYGON_OFFSET_FILL) (both terms zero). */
    vkCmdSetDepthBias_fn(vulkan_state.command_buffer,
                        vulkan_state.current_depth_bias != 0.0f ? 1.0f : 0.0f,
                        0.0f,
                        vulkan_state.current_depth_bias);
    /* The game pass renders into the backbuffer, which is vertically flipped
     * relative to the final (presented) image. The projection matrices are
     * identical to the GL renderer (no y flip in the matrices or the vertex
     * shaders); the flip is inherent to Vulkan's viewport (NDC y=+1 maps to
     * the bottom of the image, vs GL's top). The post pass un-flips it.
     *
     * A vertical flip reverses triangle winding, so a triangle that is
     * front-facing (CCW) in the GL reference appears clockwise in the
     * backbuffer. Every game-pipeline variant therefore bakes in
     * VK_FRONT_FACE_CLOCKWISE so that VK_CULL_MODE_BACK_BIT culls true
     * back-faces, matching GL's glFrontFace(GL_CCW) + glCullFace(GL_BACK). */

    /* Draw only the new slice (cumulative buffer holds the whole frame so far) */
    vkCmdDraw_fn(vulkan_state.command_buffer, flushed_count, 1, vulkan_state.vertex_flushed, 0);

    /* Mark all vertices as flushed (counters reset in render_frame_prepare) */
    vulkan_state.vertex_flushed = vulkan_state.vertex_count;
}

/* ------------------------------------------------------------------ */
/* Render API implementations                                           */
/* ------------------------------------------------------------------ */

void render_set_screen_size(vec2i_t size) {
    vulkan_state.screen_size = size;
    if (!vulkan_state.device || size.x <= 0 || size.y <= 0) {
        return;
    }
    /* Flag-only: the actual swapchain/backbuffer recreation is deferred to
     * render_frame_prepare, after vkQueueWaitIdle. Recreating mid-frame with
     * in-flight command buffers still referencing the old swapchain images
     * is the UB that froze the game.
     *
     * A window size change (e.g. windowed -> fullscreen) needs both:
     *  - the swapchain, to track the new window extent (in GL the SDL
     *    context resizes implicitly; the Vulkan surface must be recreated),
     *  - the backbuffer, because NATIVE tracks the window and the 240P/480P
     *    aspect depends on the window aspect.
     * create_swapchain() re-syncs screen_size to the extent the driver
     * actually reports, so the backbuffer (NATIVE) tracks the real window.
     * Projections are recomputed from the backbuffer size in the same
     * deferred block (GL parity: render_gl.c:557-558). */
    if (size.x != (int)vulkan_state.swapchain_extent.width ||
        size.y != (int)vulkan_state.swapchain_extent.height) {
        vulkan_state.swapchain_recreate_pending = true;
        vulkan_state.backbuffer_recreate_pending = true;
    }
}

void render_set_post_effect(render_post_effect_t post) {
    error_if(post < 0 || post >= NUM_RENDER_POST_EFFECTS, "Invalid post effect %d", post);
    vulkan_state.current_post_effect = post;
}

vec2i_t render_size(void) { return vulkan_state.backbuffer_size; }

const render_stats_t* render_frame_get_stats(void) {
    return &end_stats;
}

void render_set_view(vec3_t pos, vec3_t angles) {
    render_flush();
    render_set_depth_write(true);
    render_set_depth_test(true);

    /* This is a 3D view: force the 3D perspective projection in the MVP.
     * render_set_view_2d() flips this to false, and menu 3D objects are drawn
     * *after* a render_set_view_2d() call (menu_update sets up the 2D view
     * before the page draw func). Without this, the object would be
     * transformed with the 2D orthographic projection, landing at an
     * out-of-range depth (~z/1.0 far past [0,1]) that fails the LESS depth
     * test — i.e. invisible. The GL renderer selects projection_mat_3d
     * directly in this setter; this is the VK equivalent. */
    vulkan_state.current_is_3d = true;

    /* Build view matrix: roll-pitch-yaw rotation + translate to camera pos */
    vulkan_state.current_view_mat = mat4_identity();
    mat4_set_roll_pitch_yaw(&vulkan_state.current_view_mat, vec3(angles.x, -angles.y + M_PI, angles.z + M_PI));
    mat4_translate(&vulkan_state.current_view_mat, vec3_inv(pos));

    /* Billboard matrix for render_push_sprite (mirrors render_gl.c): pure
     * rotation so effect/sprite quads always face the camera. */
    mat4_set_yaw_pitch_roll(&sprite_mat, vec3(-angles.x, angles.y - M_PI, 0));

    /* Reset model to identity */
    vulkan_state.current_model_mat = mat4_identity();
}

void render_set_view_2d(void) {
    render_flush();
    render_set_depth_test(false);
    render_set_depth_write(false);

    /* 2D: view is identity, mode is 2D */
    vulkan_state.current_view_mat = mat4_identity();
    vulkan_state.current_model_mat = mat4_identity();
    vulkan_state.current_is_3d = false;
}

void render_set_model_mat(mat4_t *m) {
    render_flush();
    if (m) {
        vulkan_state.current_model_mat = *m;
    }
}

void render_set_depth_write(bool enabled) {
    render_flush();
    vulkan_state.current_depth_write = enabled;
}

void render_set_depth_test(bool enabled) {
    render_flush();
    vulkan_state.current_depth_test = enabled;
}

void render_set_depth_offset(float offset) {
    render_flush();
    vulkan_state.current_depth_bias = offset;
}

void render_set_screen_position(vec2_t pos) {
    render_flush();
    vulkan_state.current_screen_position = pos;
}

void render_set_blend_mode(render_blend_mode_t mode) {
    if (mode == vulkan_state.current_blend_mode) return;
    render_flush();
    vulkan_state.current_blend_mode = mode;
}

void render_set_cull_backface(bool enabled) {
    render_flush();
    vulkan_state.current_cull_backface = enabled;
}

vec3_t render_transform(vec3_t pos) {
    /* Project 3D world position to NDC (used by HUD for target reticle).
     * Matches the GL renderer's implementation. */
    return vec4_perspective_divide(
        vec3_transform_perspective(
            vec3_transform(pos, &vulkan_state.current_view_mat),
            &vulkan_state.projection_mat_3d));
}

void render_push_tris(tris_t tris, uint16_t texture_index) {
    if (vulkan_state.textures_len == 0) return;
    error_if(texture_index >= vulkan_state.textures_len, "Invalid texture %d", texture_index);

    if (vulkan_state.vertex_count + 3 > RENDER_TRIS_BUFFER_CAPACITY * 3) {
        render_flush();
        /* Flush is cumulative (it never frees space); drop overflow triangles
         * so we never write past the end of vertex_data. Warn once so an
         * oversized frame is visible instead of silently losing geometry. */
        if (vulkan_state.vertex_count + 3 > RENDER_TRIS_BUFFER_CAPACITY * 3) {
            static bool vertex_overflow_warned = false;
            if (!vertex_overflow_warned) {
                vertex_overflow_warned = true;
                fprintf(stderr, "vertex buffer overflow: dropping triangles "
                                "(%u triangles this frame, capacity %d)\n",
                        vulkan_state.vertex_count / 3, RENDER_TRIS_BUFFER_CAPACITY);
            }
            return;
        }
    }

    /* UVs arrive as texture-local pixel coordinates (the shared contract with
     * render_gl.c). Add the texture's atlas offset and normalize to [0,1] by
     * the atlas size here, so the vertex shader passes UVs through verbatim. */
    vulkan_texture_t *t = &vulkan_state.textures[texture_index];
    const float atlas_size = (float)(ATLAS_SIZE * ATLAS_GRID);
    for (int i = 0; i < 3; i++) {
        vertex_t v = tris.vertices[i];
        v.uv.x = (v.uv.x + (float)t->offset.x) / atlas_size;
        v.uv.y = (v.uv.y + (float)t->offset.y) / atlas_size;
        vulkan_state.vertex_data[vulkan_state.vertex_count++] = v;
    }
}

void render_push_sprite(vec3_t pos, vec2i_t size, rgba_t color, uint16_t texture_index) {
    if (vulkan_state.textures_len == 0) return;
    error_if(texture_index >= vulkan_state.textures_len, "Invalid texture %d", texture_index);

    vulkan_texture_t *t = &vulkan_state.textures[texture_index];
    /* UVs are texture-local pixel-space (mirrors render_gl.c); render_push_tris
     * adds the atlas offset and normalizes by the atlas size. */
    const float sw = (float)t->size.x;
    const float sh = (float)t->size.y;

    /* Billboard: quad corners are local XY offsets rotated to face the
     * camera, then translated to pos (mirrors render_gl.c). */
    vec3_t p1 = vec3_add(pos, vec3_transform(vec3(-size.x * 0.5f, -size.y * 0.5f, 0), &sprite_mat));
    vec3_t p2 = vec3_add(pos, vec3_transform(vec3( size.x * 0.5f, -size.y * 0.5f, 0), &sprite_mat));
    vec3_t p3 = vec3_add(pos, vec3_transform(vec3(-size.x * 0.5f,  size.y * 0.5f, 0), &sprite_mat));
    vec3_t p4 = vec3_add(pos, vec3_transform(vec3( size.x * 0.5f,  size.y * 0.5f, 0), &sprite_mat));

    render_push_tris((tris_t){
        .vertices = {
            {.pos = p1,
             .uv = vec2(0, 0), .color = color},
            {.pos = p2,
             .uv = vec2(sw, 0), .color = color},
            {.pos = p3,
             .uv = vec2(0, sh), .color = color},
        }
    }, texture_index);
    render_push_tris((tris_t){
        .vertices = {
            {.pos = p3,
             .uv = vec2(0, sh), .color = color},
            {.pos = p2,
             .uv = vec2(sw, 0), .color = color},
            {.pos = p4,
             .uv = vec2(sw, sh), .color = color},
        }
    }, texture_index);
}

void render_push_2d(vec2i_t pos, vec2i_t size, rgba_t color, uint16_t texture_index) {
    render_push_2d_tile(pos, vec2i(0, 0), render_texture_size(texture_index), size, color, texture_index);
}

void render_push_2d_tile(vec2i_t pos, vec2i_t uv_offset, vec2i_t uv_size,
                         vec2i_t size, rgba_t color, uint16_t texture_index) {
    if (vulkan_state.textures_len == 0) return;
    error_if(texture_index >= vulkan_state.textures_len, "Invalid texture %d", texture_index);

    /* UVs are in pixel-space within the texture cell (mirrors render_gl.c);
     * render_push_tris adds the atlas offset and normalizes by the atlas size. */
    const float uw = (float)uv_size.x;
    const float uh = (float)uv_size.y;
    const float uox = (float)uv_offset.x;
    const float uoy = (float)uv_offset.y;

    /* Bottom-left, top-right, bottom-right (triangle 1) */
    render_push_tris((tris_t){
        .vertices = {
            {.pos = vec3((float)pos.x, (float)(pos.y + size.y), 0),
             .uv = vec2(uox, uoy + uh),
             .color = color},
            {.pos = vec3((float)(pos.x + size.x), (float)pos.y, 0),
             .uv = vec2(uox + uw, uoy),
             .color = color},
            {.pos = vec3((float)pos.x, (float)pos.y, 0),
             .uv = vec2(uox, uoy),
             .color = color},
        }
    }, texture_index);

    /* Top-left, top-right, bottom-right (triangle 2) */
    render_push_tris((tris_t){
        .vertices = {
            {.pos = vec3((float)(pos.x + size.x), (float)(pos.y + size.y), 0),
             .uv = vec2(uox + uw, uoy + uh),
             .color = color},
            {.pos = vec3((float)(pos.x + size.x), (float)pos.y, 0),
             .uv = vec2(uox + uw, uoy),
             .color = color},
            {.pos = vec3((float)pos.x, (float)(pos.y + size.y), 0),
             .uv = vec2(uox, uoy + uh),
             .color = color},
        }
    }, texture_index);
}

/* ------------------------------------------------------------------ */
/* Atlas mip helpers (mirrors GL's glGenerateMipmap + zero-init atlas) */
/* ------------------------------------------------------------------ */

/* Transition the whole mipmapped atlas to TRANSFER_DST_OPTIMAL. On the
 * first upload the atlas contents are undefined, so clear it to black to
 * mirror the GL renderer's zero-initialized atlas — mip generation must
 * never read undefined texels. */
static void atlas_to_transfer_dst(VkCommandBuffer cb) {
    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = vulkan_state.atlas_layout;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vulkan_state.atlas_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = ATLAS_MIP_LEVELS;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    VkPipelineStageFlags src_stage = vulkan_state.atlas_layout == VK_IMAGE_LAYOUT_UNDEFINED ?
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    vkCmdPipelineBarrier_fn(cb, src_stage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                            0, NULL, 0, NULL, 1, &barrier);

    if (vulkan_state.atlas_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
        static const VkClearColorValue black = { .float32 = {0.0f, 0.0f, 0.0f, 0.0f} };
        VkImageSubresourceRange ranges[ATLAS_MIP_LEVELS];
        for (uint32_t i = 0; i < ATLAS_MIP_LEVELS; i++) {
            ranges[i] = (VkImageSubresourceRange){
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = i,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            };
        }
        /* A single pColor applies to every range in pRanges ("Each specified
         * range in pRanges is cleared to the value specified by pColor"). */
        vkCmdClearColorImage_fn(cb, vulkan_state.atlas_image,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                &black, ATLAS_MIP_LEVELS, ranges);
    }
}

/* Generate mip levels 1..ATLAS_MIP_LEVELS-1 from level 0 with linear blits
 * (mirrors the GL renderer's glGenerateMipmap) and transition every level
 * to SHADER_READ_ONLY_OPTIMAL. Level 0 must have just been written while in
 * TRANSFER_DST_OPTIMAL layout. Each level stays in TRANSFER_DST_OPTIMAL
 * until it is used as a blit source, where it transitions to
 * TRANSFER_SRC_OPTIMAL (a valid blit source layout). */
static void atlas_generate_mips_and_finish(VkCommandBuffer cb,
                                           uint32_t atlas_width, uint32_t atlas_height) {
    for (uint32_t i = 1; i < ATLAS_MIP_LEVELS; i++) {
        /* Level i-1 was just written (or blitted) while in
         * TRANSFER_DST_OPTIMAL; move it to TRANSFER_SRC_OPTIMAL for the
         * blit below (also provides the write -> read dependency). */
        VkImageMemoryBarrier b = {0};
        b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = vulkan_state.atlas_image;
        b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel = i - 1;
        b.subresourceRange.levelCount = 1;
        b.subresourceRange.layerCount = 1;
        b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier_fn(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0, 0, NULL, 0, NULL, 1, &b);

        uint32_t w = atlas_width >> (i - 1);
        uint32_t h = atlas_height >> (i - 1);
        VkImageBlit blit = {0};
        blit.srcSubresource = (VkImageSubresourceLayers){VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1};
        blit.srcOffsets[0] = (VkOffset3D){0, 0, 0};
        blit.srcOffsets[1] = (VkOffset3D){(int32_t)w, (int32_t)h, 1};
        blit.dstSubresource = (VkImageSubresourceLayers){VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1};
        blit.dstOffsets[0] = (VkOffset3D){0, 0, 0};
        blit.dstOffsets[1] = (VkOffset3D){(int32_t)(w / 2), (int32_t)(h / 2), 1};
        vkCmdBlitImage_fn(cb, vulkan_state.atlas_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          vulkan_state.atlas_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          1, &blit, VK_FILTER_LINEAR);
    }

    /* Every level to shader-read. After the loop, levels 0..N-2 sit in
     * TRANSFER_SRC_OPTIMAL (blit sources) and the top level in
     * TRANSFER_DST_OPTIMAL (last blit destination). */
    for (uint32_t i = 0; i < ATLAS_MIP_LEVELS; i++) {
        VkImageMemoryBarrier b = {0};
        b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout = (i < ATLAS_MIP_LEVELS - 1) ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL :
                                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = vulkan_state.atlas_image;
        b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel = i;
        b.subresourceRange.levelCount = 1;
        b.subresourceRange.layerCount = 1;
        b.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier_fn(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                0, 0, NULL, 0, NULL, 1, &b);
    }
    vulkan_state.atlas_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

/* ------------------------------------------------------------------ */
/* Flush pending texture uploads (batched)                              */
/* ------------------------------------------------------------------ */
void render_flush_pending_textures(void) {
    if (vulkan_state.pending_uploads_len == 0) return;

    uint32_t atlas_width = ATLAS_SIZE * ATLAS_GRID;
    uint32_t atlas_height = ATLAS_SIZE * ATLAS_GRID;
    /* Fill all pending textures into the persistent staging buffer at their
     * atlas positions (row stride atlas_width) so the whole-block copies
     * below read them back at the same stride. Each fill writes the entire
     * border-padded block, so no copied pixel is ever left unwritten. */
    for (uint32_t i = 0; i < vulkan_state.pending_uploads_len; i++) {
        pending_upload_t *upload = &vulkan_state.pending_uploads[i];
        uint32_t x = upload->grid_x * ATLAS_GRID;
        uint32_t y = upload->grid_y * ATLAS_GRID;
        uint32_t bw = upload->width + ATLAS_BORDER * 2;
        uint32_t bh = upload->height + ATLAS_BORDER * 2;

        rgba_t *block = (rgba_t *)vulkan_state.upload_staging_mapped + (size_t)y * atlas_width + x;
        atlas_fill_block(block, atlas_width, bw, bh, upload->width, upload->height, upload->pixels);
    }

    /* Record the batch on the persistent upload command buffer (reset +
     * reuse). It is a separate command buffer submitted to the same queue,
     * so it executes ahead of the frame's command buffer and its writes are
     * visible to every draw recorded after this point. */
    VkResult reset_r = vkResetCommandPool_fn(vulkan_state.device, vulkan_state.upload_command_pool, 0);
    (void)reset_r;

    VK_CHECK(vkBeginCommandBuffer_fn(vulkan_state.upload_command_buffer,
                                     &(VkCommandBufferBeginInfo){
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    }));

    /* All mip levels -> TRANSFER_DST_OPTIMAL (clears to black on first upload) */
    atlas_to_transfer_dst(vulkan_state.upload_command_buffer);

    /* Copy all textures to the atlas (whole border-padded blocks) */
    for (uint32_t i = 0; i < vulkan_state.pending_uploads_len; i++) {
        pending_upload_t *upload = &vulkan_state.pending_uploads[i];
        uint32_t x = upload->grid_x * ATLAS_GRID;
        uint32_t y = upload->grid_y * ATLAS_GRID;
        uint32_t bw = upload->width + ATLAS_BORDER * 2;
        uint32_t bh = upload->height + ATLAS_BORDER * 2;

        VkBufferImageCopy region = {0};
        region.bufferOffset = (uint32_t)((size_t)(y * atlas_width + x) * sizeof(rgba_t));
        region.bufferRowLength = atlas_width;    /* rows in pixels, not bytes */
        region.bufferImageHeight = 0;             /* 0 = same as image extent height */
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = (VkExtent3D){bw, bh, 1};
        region.imageOffset = (VkOffset3D){(int32_t)x, (int32_t)y, 0};
        vkCmdCopyBufferToImage_fn(vulkan_state.upload_command_buffer, vulkan_state.upload_staging_buffer,
                                  vulkan_state.atlas_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    /* Generate mip levels 1..N-1 and transition all levels to SHADER_READ_ONLY_OPTIMAL */
    atlas_generate_mips_and_finish(vulkan_state.upload_command_buffer, atlas_width, atlas_height);

    VK_CHECK(vkEndCommandBuffer_fn(vulkan_state.upload_command_buffer));

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vulkan_state.upload_command_buffer;
    VK_CHECK(vkQueueSubmit_fn(vulkan_state.graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
    vkQueueWaitIdle_fn(vulkan_state.graphics_queue);

    /* Free the queued pixel copies (allocated in render_texture_create) */
    for (uint32_t i = 0; i < vulkan_state.pending_uploads_len; i++) {
        free(vulkan_state.pending_uploads[i].pixels);
    }
    vulkan_state.pending_uploads_len = 0;
}

/* ------------------------------------------------------------------ */
/* Texture management                                                   */
/* ------------------------------------------------------------------ */

uint16_t render_texture_create(uint32_t width, uint32_t height, rgba_t *pixels) {
    error_if(vulkan_state.textures_len >= TEXTURES_MAX, "TEXTURES_MAX reached");

    /* Find a position in the atlas for this texture (with added border),
     * mirroring the GL reference skyline allocation. */
    uint32_t bw = width + ATLAS_BORDER * 2;
    uint32_t bh = height + ATLAS_BORDER * 2;

    uint32_t grid_width = (bw + ATLAS_GRID - 1) / ATLAS_GRID;
    uint32_t grid_height = (bh + ATLAS_GRID - 1) / ATLAS_GRID;
    uint32_t grid_x = 0;
    uint32_t grid_y = ATLAS_SIZE - grid_height + 1;

    for (uint32_t cx = 0; cx < ATLAS_SIZE - grid_width; cx++) {
        if (vulkan_state.atlas_map[cx] >= grid_y) {
            continue;
        }

        uint32_t cy = vulkan_state.atlas_map[cx];
        bool is_best = true;

        for (uint32_t bx = cx; bx < cx + grid_width; bx++) {
            if (vulkan_state.atlas_map[bx] >= grid_y) {
                is_best = false;
                cx = bx;
                break;
            }
            if (vulkan_state.atlas_map[bx] > cy) {
                cy = vulkan_state.atlas_map[bx];
            }
        }
        if (is_best) {
            grid_y = cy;
            grid_x = cx;
        }
    }

    error_if(grid_y + grid_height > ATLAS_SIZE, "Render atlas ran out of space");

    for (uint32_t cx = grid_x; cx < grid_x + grid_width; cx++) {
        vulkan_state.atlas_map[cx] = grid_y + grid_height;
    }

    uint32_t x = grid_x * ATLAS_GRID;
    uint32_t y = grid_y * ATLAS_GRID;

    /* Queue the pixel upload to pending_uploads regardless of frame state;
     * the batch is flushed at a safe point (top of render_frame_prepare, or
     * before the first draw of a frame that created textures mid-frame).
     * The caller's pixel buffer lives in mem_temp and is freed right after
     * render_texture_create returns (see image.c), so copy the pixels into
     * persistent storage; the flush frees the copies. If the queue is full,
     * flush what is queued first, then queue this upload. */
    if (pixels) {
        if (vulkan_state.pending_uploads_len >= PENDING_UPLOADS_MAX) {
            render_flush_pending_textures();
        }
        uint16_t idx = vulkan_state.textures_len;
        pending_upload_t *upload = &vulkan_state.pending_uploads[vulkan_state.pending_uploads_len++];
        upload->width = width;
        upload->height = height;
        {
            size_t pixel_bytes = (size_t)width * height * sizeof(rgba_t);
            upload->pixels = (rgba_t *)malloc(pixel_bytes);
            memcpy(upload->pixels, pixels, pixel_bytes);
        }
        upload->grid_x = grid_x;
        upload->grid_y = grid_y;
        upload->texture_index = idx;
    }

    /* Record texture metadata */
    uint16_t idx = vulkan_state.textures_len++;
    vulkan_state.textures[idx] = (vulkan_texture_t){
        .offset = vec2i((int32_t)x + ATLAS_BORDER, (int32_t)y + ATLAS_BORDER),
        .size   = vec2i((int32_t)width, (int32_t)height),
    };
    return idx;
}

vec2i_t render_texture_size(uint16_t texture_index) {
    error_if(texture_index >= vulkan_state.textures_len, "Invalid texture %d", texture_index);
    return vulkan_state.textures[texture_index].size;
}

void render_texture_replace_pixels(int16_t texture_index, rgba_t *pixels) {
    error_if((uint16_t)texture_index >= vulkan_state.textures_len, "Invalid texture %d", texture_index);

    /* Flush any queued uploads first: a pending upload for this texture
     * (created with pixels, not yet flushed) would otherwise overwrite the
     * replacement pixels staged below when the batch runs. */
    if (vulkan_state.pending_uploads_len > 0) {
        render_flush_pending_textures();
    }

    vulkan_texture_t *t = &vulkan_state.textures[texture_index];
    uint32_t atlas_width = ATLAS_SIZE * ATLAS_GRID;

    /* Stage the new pixels into the persistent upload buffer at the
     * texture's body offset (row stride atlas_width), mirroring GL's
     * glTexSubImage2D on the same region. */
    {
        rgba_t *dst = (rgba_t *)vulkan_state.upload_staging_mapped + (size_t)t->offset.y * atlas_width + t->offset.x;
        for (uint32_t row = 0; row < (uint32_t)t->size.y; row++) {
            memcpy(dst + (size_t)row * atlas_width,
                   pixels + (size_t)row * (size_t)t->size.x,
                   sizeof(rgba_t) * (size_t)t->size.x);
        }
    }

    /* One-shot submit-and-wait: level 0 -> TRANSFER_DST_OPTIMAL, copy the
     * body region back, then restore SHADER_READ_ONLY_OPTIMAL. Like the GL
     * reference, mip levels are left untouched. */
    {
        VkImageMemoryBarrier barrier = {0};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = vulkan_state.atlas_layout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = vulkan_state.atlas_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        VkBufferImageCopy region = {0};
        region.bufferOffset = (uint32_t)((size_t)(t->offset.y * atlas_width + t->offset.x) * sizeof(rgba_t));
        region.bufferRowLength = atlas_width;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = (VkExtent3D){(uint32_t)t->size.x, (uint32_t)t->size.y, 1};
        region.imageOffset = (VkOffset3D){t->offset.x, t->offset.y, 0};

        VkResult reset_r = vkResetCommandPool_fn(vulkan_state.device, vulkan_state.upload_command_pool, 0);
        (void)reset_r;

        VK_CHECK(vkBeginCommandBuffer_fn(vulkan_state.upload_command_buffer,
                                         &(VkCommandBufferBeginInfo){
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        }));

        vkCmdPipelineBarrier_fn(vulkan_state.upload_command_buffer,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                0, NULL, 0, NULL, 1, &barrier);

        vkCmdCopyBufferToImage_fn(vulkan_state.upload_command_buffer,
                                  vulkan_state.upload_staging_buffer, vulkan_state.atlas_image,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier_fn(vulkan_state.upload_command_buffer,
                                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                0, NULL, 0, NULL, 1, &barrier);

        VK_CHECK(vkEndCommandBuffer_fn(vulkan_state.upload_command_buffer));

        VkSubmitInfo submit = {0};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &vulkan_state.upload_command_buffer;
        VK_CHECK(vkQueueSubmit_fn(vulkan_state.graphics_queue, 1, &submit, VK_NULL_HANDLE));
        vkQueueWaitIdle_fn(vulkan_state.graphics_queue);

        /* Level 0 round-tripped through TRANSFER_DST_OPTIMAL and is back in
         * the tracked layout; the other levels never left it. */
        vulkan_state.atlas_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
}

uint16_t render_textures_len(void) { return vulkan_state.textures_len; }

void render_textures_reset(uint16_t len) {
    error_if(len > vulkan_state.textures_len, "Invalid texture reset len %d >= %d", len, vulkan_state.textures_len);
    render_flush();
    vulkan_state.textures_len = len;
    memset(vulkan_state.atlas_map, 0, sizeof(vulkan_state.atlas_map));

    /* Recreate default white texture */
    if (len == 0) {
        rgba_t white_pixels[4] = {
            rgba(128,128,128,255), rgba(128,128,128,255),
            rgba(128,128,128,255), rgba(128,128,128,255)
        };
        RENDER_NO_TEXTURE = render_texture_create(2, 2, white_pixels);
        return;
    }

    /* Replay skyline footprints for retained textures (mirrors GL reference)
     * so newly allocated textures don't overlap them. */
    for (uint32_t i = 0; i < len; i++) {
        uint32_t grid_x = (vulkan_state.textures[i].offset.x - ATLAS_BORDER) / ATLAS_GRID;
        uint32_t grid_y = (vulkan_state.textures[i].offset.y - ATLAS_BORDER) / ATLAS_GRID;
        uint32_t grid_width = (vulkan_state.textures[i].size.x + ATLAS_BORDER * 2 + ATLAS_GRID - 1) / ATLAS_GRID;
        uint32_t grid_height = (vulkan_state.textures[i].size.y + ATLAS_BORDER * 2 + ATLAS_GRID - 1) / ATLAS_GRID;
        for (uint32_t cx = grid_x; cx < grid_x + grid_width; cx++) {
            vulkan_state.atlas_map[cx] = grid_y + grid_height;
        }
    }
}

void render_textures_dump(const char *path) { (void)path; }
