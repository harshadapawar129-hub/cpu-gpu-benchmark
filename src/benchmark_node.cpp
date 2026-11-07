#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>

#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <stdexcept>

class BenchmarkNode : public rclcpp::Node
{
public:
    BenchmarkNode() : Node("cpu_gpu_benchmark")
    {
        this->declare_parameter<std::string>("mode", "cpu");
        this->declare_parameter<std::string>("image_topic", "/image");
        this->declare_parameter<int>("warmup_frames", 20);
        this->declare_parameter<int>("benchmark_frames", 100);
        this->declare_parameter<int>("kernel_size", 31);

        mode_ = this->get_parameter("mode").as_string();
        image_topic_ = this->get_parameter("image_topic").as_string();
        warmup_frames_ = this->get_parameter("warmup_frames").as_int();
        benchmark_frames_ =
            this->get_parameter("benchmark_frames").as_int();
        kernel_size_ = this->get_parameter("kernel_size").as_int();

        if (mode_ != "cpu" && mode_ != "opencl") {
            throw std::runtime_error(
                "Mode must be either cpu or opencl");
        }

        if (warmup_frames_ < 0 ||
            benchmark_frames_ <= 0 ||
            kernel_size_ <= 0 ||
            kernel_size_ % 2 == 0) {
            throw std::runtime_error("Invalid benchmark parameters");
        }

        if (mode_ == "opencl") {
            if (!cv::ocl::haveOpenCL()) {
                throw std::runtime_error(
                    "OpenCL is not available in OpenCV");
            }

            cv::ocl::setUseOpenCL(true);

            if (!cv::ocl::useOpenCL()) {
                throw std::runtime_error(
                    "OpenCV could not enable OpenCL");
            }

            RCLCPP_INFO(
                this->get_logger(),
                "OpenCL device: %s",
                cv::ocl::Device::getDefault().name().c_str());
        }

        RCLCPP_INFO(
            this->get_logger(),
            "Benchmark mode: %s",
            mode_.c_str());

        RCLCPP_INFO(
            this->get_logger(),
            "Waiting for images on: %s",
            image_topic_.c_str());

        subscription_ = this->create_subscription<
            sensor_msgs::msg::Image>(
            image_topic_,
            rclcpp::SensorDataQoS(),
            std::bind(
                &BenchmarkNode::image_callback,
                this,
                std::placeholders::_1));
    }

private:

    void image_callback(
        const sensor_msgs::msg::Image::SharedPtr msg)
    {
        if (completed_) {
            return;
        }

        try {
            auto cv_ptr = cv_bridge::toCvShare(msg);

            const cv::Mat &input = cv_ptr->image;

            if (input.empty()) {
                return;
            }

            cv::Mat output;

            auto start = std::chrono::steady_clock::now();

            if (mode_ == "cpu") {

                cv::GaussianBlur(
                    input,
                    output,
                    cv::Size(kernel_size_, kernel_size_),
                    0);

            } else {

                cv::UMat gpu_input;
                cv::UMat gpu_output;

                input.copyTo(gpu_input);

                cv::GaussianBlur(
                    gpu_input,
                    gpu_output,
                    cv::Size(kernel_size_, kernel_size_),
                    0);

                gpu_output.copyTo(output);

                // Wait for OpenCL operations to finish.
                cv::ocl::finish();
            }

            auto end = std::chrono::steady_clock::now();

            double elapsed_ms =
                std::chrono::duration<double, std::milli>(
                    end - start).count();

            if (warmup_count_ < warmup_frames_) {
                warmup_count_++;

                if (warmup_count_ == warmup_frames_) {
                    RCLCPP_INFO(
                        this->get_logger(),
                        "Warm-up completed. Starting benchmark.");
                }

                return;
            }

            timings_.push_back(elapsed_ms);

            if (timings_.size() >=
                static_cast<size_t>(benchmark_frames_)) {

                print_results();
                completed_ = true;
            }

        } catch (const cv_bridge::Exception &e) {

            RCLCPP_ERROR(
                this->get_logger(),
                "Image conversion error: %s",
                e.what());
        }
    }

    void print_results()
    {
        std::vector<double> sorted = timings_;

        std::sort(sorted.begin(), sorted.end());

        double mean = std::accumulate(
            sorted.begin(),
            sorted.end(),
            0.0) / sorted.size();

        double median =
            sorted[sorted.size() / 2];

        size_t p95_index =
            static_cast<size_t>(
                0.95 * (sorted.size() - 1));

        double p95 = sorted[p95_index];

        double min_time = sorted.front();
        double max_time = sorted.back();

        double fps = 1000.0 / mean;

        RCLCPP_INFO(
            this->get_logger(),
            "\n========== BENCHMARK RESULTS ==========");

        RCLCPP_INFO(
            this->get_logger(),
            "Mode: %s",
            mode_.c_str());

        RCLCPP_INFO(
            this->get_logger(),
            "Frames processed: %zu",
            timings_.size());

        RCLCPP_INFO(
            this->get_logger(),
            "Mean latency: %.3f ms",
            mean);

        RCLCPP_INFO(
            this->get_logger(),
            "Median latency: %.3f ms",
            median);

        RCLCPP_INFO(
            this->get_logger(),
            "95th percentile: %.3f ms",
            p95);

        RCLCPP_INFO(
            this->get_logger(),
            "Minimum latency: %.3f ms",
            min_time);

        RCLCPP_INFO(
            this->get_logger(),
            "Maximum latency: %.3f ms",
            max_time);

        RCLCPP_INFO(
            this->get_logger(),
            "Average FPS: %.2f",
            fps);

        RCLCPP_INFO(
            this->get_logger(),
            "=======================================");
    }

    std::string mode_;
    std::string image_topic_;

    int warmup_frames_;
    int benchmark_frames_;
    int kernel_size_;

    int warmup_count_ = 0;

    bool completed_ = false;

    std::vector<double> timings_;

    rclcpp::Subscription<
        sensor_msgs::msg::Image>::SharedPtr subscription_;
};


int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);

    try {
        auto node = std::make_shared<BenchmarkNode>();

        rclcpp::spin(node);

    } catch (const std::exception &e) {

        RCLCPP_ERROR(
            rclcpp::get_logger("cpu_gpu_benchmark"),
            "Error: %s",
            e.what());
    }

    rclcpp::shutdown();

    return 0;
}
