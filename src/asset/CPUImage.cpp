#include "asset/CPUImage.h"
#include "application/DebugLayer.h"
#include <filesystem>
#include <stdexcept>
#include <vulkan/vulkan.hpp>


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

CPUImage::CPUImage(const std::filesystem::path& path, int channelFormat)
{
    _data = stbi_load(path.c_str(), &_width, &_height, &_channels, channelFormat);

    _channels = channelFormat;


    if(_data == nullptr)
    {
        const char* reason = stbi_failure_reason();
        if (reason)
        {
            DebugLayer::Log(DebugLayer::LogType::ERROR, "Failed to load texture image : " + std::filesystem::current_path().string() + "/" + path.string() + " : " + reason);
        }

        throw std::runtime_error("Failed to load texture image");
    }
}

CPUImage::~CPUImage()
{
    stbi_image_free(_data);
    _width = _height = _channels = 0;
}

CPUImage::CPUImage(CPUImage&& other) noexcept
{
    _width = other._width;
    _height = other._height;
    _channels = other._channels;
    _data = other._data;

    other._data = nullptr;
    _width = _height = _channels = 0;
}

CPUImage& CPUImage::operator=(CPUImage&& other) noexcept
{
    if(this != &other)
    {
        _width = other._width;
        _height = other._height;
        _channels = other._channels;
        _data = other._data;

        other._data = nullptr;
        _width = _height = _channels = 0;
    }
    return *this;
}

vk::Format CPUImage::GetGPUFormat() const
{
    switch (_channels)
    {
        case STBI_rgb_alpha: return vk::Format::eR8G8B8A8Srgb;
        default: throw std::runtime_error("Unimplemented Color Format");
    }
}
