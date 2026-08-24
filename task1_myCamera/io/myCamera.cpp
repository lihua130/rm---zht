#include "myCamera.h"
#include <iostream>

myCamera::myCamera(int index, int width, int height)
    : index_(index), width_(width), height_(height), opened_(false)
{
#ifdef _WIN32
    // Windows 平台：使用 DirectShow 后端自适应
    cap_.open(index_, cv::CAP_DSHOW);
#else
    // Ubuntu/Linux 平台：使用 V4L2 后端自适应
    cap_.open(index_, cv::CAP_V4L2);
#endif

    if (cap_.isOpened())
    {
        cap_.set(cv::CAP_PROP_FRAME_WIDTH, width_);
        cap_.set(cv::CAP_PROP_FRAME_HEIGHT, height_);
        opened_ = true;
    }
    else
    {
        std::cerr << "[myCamera] open camera " << index_ << " failed!" << std::endl;
    }
}

myCamera::~myCamera()
{
    if (cap_.isOpened())
    {
        cap_.release();
    }
}

bool myCamera::read(cv::Mat &frame)
{
    if (!opened_)
    {
        return false;
    }
    return cap_.read(frame) && !frame.empty();
}
