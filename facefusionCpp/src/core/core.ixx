/**
 ******************************************************************************
 * @file           : core.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-19
 ******************************************************************************
 */

module;
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <condition_variable>

export module core;
import face_analyser;
export import :core_task;
import logger;
import processor_hub;
import core_options;
import progress_observer;
import thread_pool;
import task;

namespace ffc::core {

using namespace ffc::infra;
using namespace ffc::task;
using namespace face_masker;
using namespace std;

export class Core {
public:
    explicit Core(const CoreOptions& options);
    ~Core();

    // Todo: 重置选项
    // void reset_options(const CoreOptions& options);

    [[nodiscard]] bool Run(CoreTask core_task);
    /**
     * @brief 运行任务, 运行任务请通过TaskManager类的submit方法提交任务
     * @param task
     * @return true
     * @return false
     */
    bool run_task(Task task);

private:
    std::shared_ptr<Logger> m_logger;
    std::shared_ptr<Ort::Env> m_env;
    std::shared_ptr<FaceAnalyser> m_face_analyser;
    ProcessorHub m_processor_hub;
    CoreOptions m_core_options;
    std::shared_ptr<IProgressObserver> m_observer{nullptr};
    ThreadPool m_thread_pool;

    bool ProcessImages(CoreTask core_task);
    bool process_images(Task task);
    bool ProcessVideos(const CoreTask& core_task, const bool& autoRemoveTarget = false);
    bool process_videos(Task task, const bool& remove_target_file_after_process = false);
    bool ProcessVideo(CoreTask core_task);
    bool process_video(Task task);
    bool ProcessVideoInSegments(CoreTask core_task);
    bool process_video_by_segments(Task task);
    bool SwapFace(const FaceSwapperInput& face_swapper_input, const std::string& output_path,
                  const FaceSwapperType& type, const model_manager::Model& model);
    bool EnhanceFace(const FaceEnhancerInput& face_enhancer_input, const std::string& output_path,
                     const FaceEnhancerType& type, const model_manager::Model& model);
    bool RestoreExpression(const ExpressionRestorerInput& expression_restorer_input,
                           const std::string& output_path,
                           const ExpressionRestorerType& type);
    bool EnhanceFrame(const FrameEnhancerInput& frame_enhancer_input,
                      const std::string& output_path,
                      const FrameEnhancerType& type, const model_manager::Model& model);
};

} // namespace ffc::core
