#include "RHIVulkan/VulkanRHI.h"

#include "Core/Log.h"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace Au::RHI
{
    namespace
    {
        class VulkanDevice final : public IRHIDevice
        {
        public:
            VulkanDevice(VkInstance instance, std::string adapterName)
                : m_Instance(instance)
                , m_AdapterName(std::move(adapterName))
            {
            }

            ~VulkanDevice() override
            {
                vkDestroyInstance(m_Instance, nullptr);
            }

            VulkanDevice(const VulkanDevice&)            = delete;
            VulkanDevice& operator=(const VulkanDevice&) = delete;
            VulkanDevice(VulkanDevice&&)                 = delete;
            VulkanDevice& operator=(VulkanDevice&&)      = delete;

            [[nodiscard]] ERHIBackend GetBackend() const override
            {
                return ERHIBackend::Vulkan;
            }

            [[nodiscard]] std::string_view GetAdapterName() const override
            {
                return m_AdapterName;
            }

        private:
            VkInstance  m_Instance = VK_NULL_HANDLE;
            std::string m_AdapterName;
        };

        VkPhysicalDevice PickPhysicalDevice(VkInstance instance)
        {
            u32 count = 0;
            vkEnumeratePhysicalDevices(instance, &count, nullptr);
            std::vector<VkPhysicalDevice> devices(count);
            vkEnumeratePhysicalDevices(instance, &count, devices.data());

            VkPhysicalDevice fallback = VK_NULL_HANDLE;
            for (VkPhysicalDevice device : devices)
            {
                VkPhysicalDeviceProperties properties{};
                vkGetPhysicalDeviceProperties(device, &properties);
                if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    return device;
                }
                if (fallback == VK_NULL_HANDLE)
                {
                    fallback = device;
                }
            }
            return fallback;
        }
    } // namespace

    std::unique_ptr<IRHIDevice> CreateVulkanDevice()
    {
        u32 loaderVersion = VK_API_VERSION_1_0;
        vkEnumerateInstanceVersion(&loaderVersion);
        AU_LOG_INFO("Vulkan: loader version {}.{}.{}", VK_API_VERSION_MAJOR(loaderVersion),
                    VK_API_VERSION_MINOR(loaderVersion), VK_API_VERSION_PATCH(loaderVersion));

        VkApplicationInfo appInfo{};
        appInfo.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "AuEngine";
        appInfo.pEngineName      = "AuEngine";
        appInfo.apiVersion       = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        VkInstance instance = VK_NULL_HANDLE;
        if (const VkResult result = vkCreateInstance(&createInfo, nullptr, &instance); result != VK_SUCCESS)
        {
            AU_LOG_ERROR("Vulkan: vkCreateInstance failed ({})", static_cast<int>(result));
            return nullptr;
        }

        VkPhysicalDevice physicalDevice = PickPhysicalDevice(instance);
        if (physicalDevice == VK_NULL_HANDLE)
        {
            AU_LOG_ERROR("Vulkan: no physical device available");
            vkDestroyInstance(instance, nullptr);
            return nullptr;
        }

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        return std::make_unique<VulkanDevice>(instance, properties.deviceName);
    }
} // namespace Au::RHI
