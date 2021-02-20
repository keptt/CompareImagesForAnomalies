#pragma once

#include <exception>
#include <string>

namespace cex {
    class Error: public std::exception {
    private:
        std::string message_;
    public:
        explicit Error(const std::string &message="");
        const char* what() const noexcept override {
            return message_.c_str();
        }
    };


    Error::Error(const std::string& message) : message_(message) {
    }


    class ImageLoadingError : public Error {
    public:
        explicit ImageLoadingError(const std::string &message="Could not load image") : Error(message) {};
    };


    class ChannelInconsistency : public Error {
    public:
        explicit ChannelInconsistency(const std::string &message="Channels are of inconsistent dimensions") : Error(message) {};
    };


    class NoImageSavePath : public Error {
    public:
        explicit NoImageSavePath(const std::string &message="No image save path found") : Error(message) {};
    };


    class ImageMemoryAllocationError : public Error {
    public:
        explicit ImageMemoryAllocationError(const std::string &message="Could not allocate memory for the image") : Error(message) {};
    };
}



