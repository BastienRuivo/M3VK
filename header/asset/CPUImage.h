#pragma once

#include <filesystem>
#pragma once

#include <stb_image.h>
#include <vulkan/vulkan.hpp>
class CPUImage
{
    public:
    CPUImage(const std::filesystem::path& path, int channelFormat);
    ~CPUImage();

    CPUImage(CPUImage&& other) noexcept;
    CPUImage& operator=(CPUImage&& other) noexcept;

    CPUImage(const CPUImage&) = delete;
    CPUImage& operator=(const CPUImage&) = delete;

    vk::Format GetGPUFormat() const;

    inline vk::DeviceSize Size() const { return _width * _height* _channels; }
    inline int Width() const { return _width; };
    inline int Height() const { return _height; };
    inline int Channels() const { return _channels; };
    inline stbi_uc* Data() const { return _data; };

    private:
    int _width = 0;
    int _height = 0;
    int _channels = 0;

    stbi_uc* _data = nullptr;
};
