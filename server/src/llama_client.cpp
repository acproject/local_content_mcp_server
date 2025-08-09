#include "llama_client.hpp"
#include "config.hpp"
#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>
#include <sstream>
#include <regex>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif
#include <fcntl.h>
#include <mutex>

namespace mcp {

// LlamaRequest 实现
nlohmann::json LlamaRequest::to_json() const {
    nlohmann::json j;
    j["prompt"] = prompt;
    j["max_tokens"] = max_tokens;
    j["temperature"] = temperature;
    j["top_p"] = top_p;
    j["top_k"] = top_k;
    j["stop_sequences"] = stop_sequences;
    j["stream"] = stream;
    return j;
}

void LlamaRequest::from_json(const nlohmann::json& j) {
    prompt = j.value("prompt", "");
    max_tokens = j.value("max_tokens", 512);
    temperature = j.value("temperature", 0.7f);
    top_p = j.value("top_p", 0.9f);
    top_k = j.value("top_k", 40);
    stop_sequences = j.value("stop_sequences", std::vector<std::string>{});
    stream = j.value("stream", false);
}

// LlamaResponse 实现
nlohmann::json LlamaResponse::to_json() const {
    nlohmann::json j;
    j["success"] = success;
    j["text"] = text;
    j["error_message"] = error_message;
    j["tokens_generated"] = tokens_generated;
    j["generation_time"] = generation_time;
    return j;
}

// ModelInfo 实现
nlohmann::json ModelInfo::to_json() const {
    nlohmann::json j;
    j["model_path"] = model_path;
    j["model_name"] = model_name;
    j["is_loaded"] = is_loaded;
    j["context_size"] = context_size;
    j["vocab_size"] = vocab_size;
    j["architecture"] = architecture;
    return j;
}

// LlamaClient::Impl 实现
class LlamaClient::Impl {
public:
    std::string model_path_;
    bool model_loaded_ = false;
    ModelInfo model_info_;
    
    struct Statistics {
        size_t total_requests = 0;
        size_t successful_requests = 0;
        size_t failed_requests = 0;
        double total_generation_time = 0.0;
        size_t total_tokens_generated = 0;
        
        nlohmann::json to_json() const {
            nlohmann::json j;
            j["total_requests"] = total_requests;
            j["successful_requests"] = successful_requests;
            j["failed_requests"] = failed_requests;
            j["total_generation_time"] = total_generation_time;
            j["total_tokens_generated"] = total_tokens_generated;
            if (total_requests > 0) {
                j["average_generation_time"] = total_generation_time / total_requests;
                j["success_rate"] = static_cast<double>(successful_requests) / total_requests;
            }
            return j;
        }
        
        void update(const LlamaResponse& response) {
            total_requests++;
            if (response.success) {
                successful_requests++;
                total_tokens_generated += response.tokens_generated;
            } else {
                failed_requests++;
            }
            total_generation_time += response.generation_time;
        }
        
        void reset() {
            total_requests = 0;
            successful_requests = 0;
            failed_requests = 0;
            total_generation_time = 0.0;
            total_tokens_generated = 0;
        }
    } stats_;
    
    std::mutex mutex_;
};

// LlamaClient 实现
LlamaClient::LlamaClient() : pimpl_(std::make_unique<Impl>()) {}

LlamaClient::~LlamaClient() = default;

bool LlamaClient::initialize() {
    auto& config = Config::instance();
    
    if (!config.is_llama_enabled()) {
        spdlog::info("LLaMA integration is disabled");
        return true;
    }
    
    std::string model_path = config.get_llama_model_path();
    if (!model_path.empty()) {
        return load_model(model_path);
    }
    
    spdlog::info("LLaMA client initialized without model");
    return true;
}

bool LlamaClient::load_model(const std::string& model_path) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto& config = Config::instance();
    if (!config.is_llama_enabled()) {
        spdlog::error("LLaMA integration is disabled");
        return false;
    }
    
    // 检查模型文件是否存在
    #ifdef _WIN32
    if (_access(model_path.c_str(), 0) != 0) {
#else
    if (access(model_path.c_str(), F_OK) != 0) {
#endif
        spdlog::error("Model file not found: {}", model_path);
        return false;
    }
    
    // 检查llama.cpp可执行文件是否存在
    std::string executable_path = config.get_llama_executable_path();
    #ifdef _WIN32
    if (_access(executable_path.c_str(), 0) != 0) {
#else
    if (access(executable_path.c_str(), X_OK) != 0) {
#endif
        spdlog::error("LLaMA executable not found or not executable: {}", executable_path);
        return false;
    }
    
    pimpl_->model_path_ = model_path;
    pimpl_->model_loaded_ = true;
    
    // 更新模型信息
    pimpl_->model_info_.model_path = model_path;
    pimpl_->model_info_.model_name = model_path.substr(model_path.find_last_of('/') + 1);
    pimpl_->model_info_.is_loaded = true;
    pimpl_->model_info_.context_size = config.get_llama_context_size();
    pimpl_->model_info_.vocab_size = 0; // 需要从模型文件中读取
    pimpl_->model_info_.architecture = "unknown";
    
    spdlog::info("Model loaded successfully: {}", model_path);
    return true;
}

bool LlamaClient::unload_model() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    pimpl_->model_loaded_ = false;
    pimpl_->model_path_.clear();
    pimpl_->model_info_ = ModelInfo{};
    
    spdlog::info("Model unloaded");
    return true;
}

bool LlamaClient::is_model_loaded() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->model_loaded_;
}

LlamaResponse LlamaClient::generate(const LlamaRequest& request) {
    LlamaResponse response;
    response.success = false;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        std::lock_guard<std::mutex> lock(pimpl_->mutex_);
        
        if (!pimpl_->model_loaded_) {
            response.error_message = "No model loaded";
            return response;
        }
        
        auto& config = Config::instance();
        if (!config.is_llama_enabled()) {
            response.error_message = "LLaMA integration is disabled";
            return response;
        }
        
        // 构建命令参数
        std::vector<std::string> args = build_command_args(request);
        
        // 执行命令
        std::string command = config.get_llama_executable_path();
        for (const auto& arg : args) {
            command += " " + escape_shell_arg(arg);
        }
        
        spdlog::debug("Executing LLaMA command: {}", command);
        
        // 使用popen执行命令并捕获输出
        #ifdef _WIN32
        FILE* pipe = _popen(command.c_str(), "r");
#else
        FILE* pipe = popen(command.c_str(), "r");
#endif
        if (!pipe) {
            response.error_message = "Failed to execute LLaMA command";
            return response;
        }
        
        std::string output;
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
        
        #ifdef _WIN32
        int exit_code = _pclose(pipe);
#else
        int exit_code = pclose(pipe);
#endif
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        response.generation_time = duration.count() / 1000.0;
        
        if (exit_code == 0) {
            response = parse_output(output, response.generation_time);
        } else {
            response.error_message = "LLaMA execution failed with exit code: " + std::to_string(exit_code);
            response.error_message += "\nOutput: " + output;
        }
        
    } catch (const std::exception& e) {
        response.error_message = "Exception during generation: " + std::string(e.what());
        spdlog::error("LLaMA generation error: {}", e.what());
    }
    
    // 更新统计信息
    pimpl_->stats_.update(response);
    
    return response;
}

std::future<LlamaResponse> LlamaClient::generate_async(const LlamaRequest& request) {
    return std::async(std::launch::async, [this, request]() {
        return generate(request);
    });
}

bool LlamaClient::generate_stream(const LlamaRequest& request, 
                                 std::function<bool(const std::string& token)> callback) {
    // 流式生成的简单实现
    // 在实际应用中，这需要与llama.cpp的流式API集成
    LlamaResponse response = generate(request);
    
    if (response.success) {
        // 模拟流式输出，将文本分割成单词
        std::istringstream iss(response.text);
        std::string token;
        
        while (iss >> token) {
            if (!callback(token + " ")) {
                return false; // 回调返回false表示停止
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 模拟延迟
        }
        
        return true;
    }
    
    return false;
}

ModelInfo LlamaClient::get_model_info() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->model_info_;
}

bool LlamaClient::update_config(const nlohmann::json& config) {
    // 配置更新会通过Config类处理
    // 这里可以添加特定于LLaMA客户端的配置处理
    return true;
}

nlohmann::json LlamaClient::get_config() const {
    auto& config = Config::instance();
    nlohmann::json j;
    
    j["enabled"] = config.is_llama_enabled();
    j["model_path"] = config.get_llama_model_path();
    j["executable_path"] = config.get_llama_executable_path();
    j["context_size"] = config.get_llama_context_size();
    j["threads"] = config.get_llama_threads();
    j["temperature"] = config.get_llama_temperature();
    j["max_tokens"] = config.get_llama_max_tokens();
    
    return j;
}

bool LlamaClient::health_check() {
    auto& config = Config::instance();
    
    if (!config.is_llama_enabled()) {
        return false;
    }
    
    // 检查可执行文件
    std::string executable_path = config.get_llama_executable_path();
#ifdef _WIN32
    if (_access(executable_path.c_str(), 0) != 0) {
#else
    if (access(executable_path.c_str(), X_OK) != 0) {
#endif
        return false;
    }
    
    // 检查模型文件（如果已加载）
    if (pimpl_->model_loaded_) {
#ifdef _WIN32
        if (_access(pimpl_->model_path_.c_str(), 0) != 0) {
#else
        if (access(pimpl_->model_path_.c_str(), F_OK) != 0) {
#endif
            return false;
        }
    }
    
    return true;
}

nlohmann::json LlamaClient::get_statistics() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->stats_.to_json();
}

void LlamaClient::reset_statistics() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->stats_.reset();
}

std::string LlamaClient::escape_shell_arg(const std::string& arg) {
    std::string escaped = "'";
    for (char c : arg) {
        if (c == '\'') {
            escaped += "'\"'\"'";
        } else {
            escaped += c;
        }
    }
    escaped += "'";
    return escaped;
}

std::vector<std::string> LlamaClient::build_command_args(const LlamaRequest& request) {
    auto& config = Config::instance();
    std::vector<std::string> args;
    
    // 模型文件
    args.push_back("-m");
    args.push_back(pimpl_->model_path_);
    
    // 上下文大小
    args.push_back("-c");
    args.push_back(std::to_string(config.get_llama_context_size()));
    
    // 线程数
    args.push_back("-t");
    args.push_back(std::to_string(config.get_llama_threads()));
    
    // 生成参数
    args.push_back("-n");
    args.push_back(std::to_string(request.max_tokens));
    
    args.push_back("--temp");
    args.push_back(std::to_string(request.temperature));
    
    args.push_back("--top-p");
    args.push_back(std::to_string(request.top_p));
    
    args.push_back("--top-k");
    args.push_back(std::to_string(request.top_k));
    
    // 停止序列
    for (const auto& stop : request.stop_sequences) {
        args.push_back("--reverse-prompt");
        args.push_back(stop);
    }
    
    // 提示词
    args.push_back("-p");
    args.push_back(request.prompt);
    
    // 其他选项
    args.push_back("--no-display-prompt");
    
    return args;
}

LlamaResponse LlamaClient::parse_output(const std::string& output, double generation_time) {
    LlamaResponse response;
    response.success = true;
    response.generation_time = generation_time;
    
    // 简单的输出解析
    // 在实际应用中，这需要根据llama.cpp的具体输出格式进行调整
    response.text = output;
    
    // 移除可能的调试信息和提示词
    std::regex prompt_regex(R"(^.*?\n\n)");
    response.text = std::regex_replace(response.text, prompt_regex, "");
    
    // 估算生成的token数量（简单的单词计数）
    std::istringstream iss(response.text);
    std::string word;
    response.tokens_generated = 0;
    while (iss >> word) {
        response.tokens_generated++;
    }
    
    // 清理输出
    response.text = std::regex_replace(response.text, std::regex("\n+$"), "");
    
    return response;
}

// LlamaService 实现
LlamaService& LlamaService::instance() {
    static LlamaService instance;
    return instance;
}

bool LlamaService::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (running_) {
        return true;
    }
    
    client_ = std::make_unique<LlamaClient>();
    if (!client_->initialize()) {
        client_.reset();
        return false;
    }
    
    running_ = true;
    spdlog::info("LLaMA service started");
    return true;
}

bool LlamaService::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!running_) {
        return true;
    }
    
    client_.reset();
    running_ = false;
    
    spdlog::info("LLaMA service stopped");
    return true;
}

bool LlamaService::restart() {
    stop();
    return start();
}

bool LlamaService::is_running() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

LlamaResponse LlamaService::process_request(const LlamaRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!running_ || !client_) {
        LlamaResponse response;
        response.success = false;
        response.error_message = "LLaMA service is not running";
        return response;
    }
    
    auto response = client_->generate(request);
    stats_.update(response);
    
    return response;
}

std::future<LlamaResponse> LlamaService::process_request_async(const LlamaRequest& request) {
    return std::async(std::launch::async, [this, request]() {
        return process_request(request);
    });
}

bool LlamaService::update_config(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (client_) {
        return client_->update_config(config);
    }
    
    return true;
}

nlohmann::json LlamaService::get_status() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json status;
    status["running"] = running_;
    status["statistics"] = stats_.to_json();
    
    if (client_) {
        status["model_info"] = client_->get_model_info().to_json();
        status["config"] = client_->get_config();
        status["health"] = client_->health_check();
    }
    
    return status;
}

// Statistics 实现
nlohmann::json LlamaService::Statistics::to_json() const {
    nlohmann::json j;
    j["total_requests"] = total_requests;
    j["successful_requests"] = successful_requests;
    j["failed_requests"] = failed_requests;
    j["total_generation_time"] = total_generation_time;
    j["total_tokens_generated"] = total_tokens_generated;
    
    if (total_requests > 0) {
        j["average_generation_time"] = total_generation_time / total_requests;
        j["success_rate"] = static_cast<double>(successful_requests) / total_requests;
    }
    
    if (successful_requests > 0) {
        j["average_tokens_per_request"] = static_cast<double>(total_tokens_generated) / successful_requests;
    }
    
    return j;
}

void LlamaService::Statistics::update(const LlamaResponse& response) {
    total_requests++;
    if (response.success) {
        successful_requests++;
        total_tokens_generated += response.tokens_generated;
    } else {
        failed_requests++;
    }
    total_generation_time += response.generation_time;
}

void LlamaService::Statistics::reset() {
    total_requests = 0;
    successful_requests = 0;
    failed_requests = 0;
    total_generation_time = 0.0;
    total_tokens_generated = 0;
}

} // namespace mcp