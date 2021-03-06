
#ifndef PLUGIN_BASE_HPP
#define PLUGIN_BASE_HPP

#include <memory>
#include <NvInfer.h>
#include <NvCaffeParser.h>
#include <vector>
#include <cc_util.hpp>
#include <cuda_runtime.h>
#include <set>
#include <infer/trt_infer.hpp>

namespace Plugin {

	//定义block内线程数，如果显卡性能较差，需要调整更低一些
#define GPU_BLOCK_THREADS  512
#define KERNEL_POSITION											\
	int position = (blockDim.x * blockIdx.x + threadIdx.x);		\
	if (position >= (edge)) return;

	struct GTensor {
		GTensor() {}
		GTensor(float* ptr, int n, int c, int h, int w);
		int count(int start_axis = 0) const;
		inline int offset(int n = 0, int c = 0, int h = 0, int w = 0) const { return ((n * this->channel_ + c) * this->height_ + h) * this->width_ + w; }
		inline float* ptr() const { return ptr_; }
		inline float* ptr(int n, int c = 0, int h = 0, int w = 0) const { return ptr_ + offset(n, c, h, w); }

		int num_ = 0, channel_ = 0, height_ = 0, width_ = 0;
		float* ptr_ = nullptr;
	};

	struct LayerConfig {

		///////////////////////////////////
		//可以修改的配置
		int nbOutput_ = 1;
		int workspaceSize_ = 0;
		std::set<nvinfer1::DataType> dataType_;
		std::set<nvinfer1::PluginFormat> pluginFormat_;

		//config时请构造权重相关信息和配置，复制权重如果发现count不匹配时会报错
		std::vector<std::shared_ptr<TRTInfer::Tensor>> weights_;

		///////////////////////////////////
		//自动赋值的配置
		std::vector<nvinfer1::Dims> input;
		std::vector<nvinfer1::Dims> output;
		std::string serializeData_;

		LayerConfig();
		void serialCopyTo(void* buffer);
		int serialize();
		void deserialize(const void* ptr, size_t length);
		void loadWeights(const nvinfer1::Weights* weights, int nbWeights);
		virtual void seril(ccutil::BinIO& out) {}
		virtual void deseril(ccutil::BinIO& in) {}
	};

	class TRTPlugin : public nvinfer1::IPluginExt {
	public:

		virtual ~TRTPlugin();
		virtual nvinfer1::Dims outputDims(int index, const nvinfer1::Dims* inputDims, int nbInputDims) = 0;
		virtual int enqueue(const std::vector<GTensor>& inputs, std::vector<GTensor>& outputs, cudaStream_t stream) = 0;

		void pluginInit(const std::string& name, const nvinfer1::Weights* weights, int nbWeights);
		void pluginInit(const std::string& name, const void* serialData, size_t serialLength);

		//如果有权重，请对返回的config实例中的weights做初始化shape
		virtual std::shared_ptr<LayerConfig> config(const std::string& layerName);
		virtual bool supportsFormat(nvinfer1::DataType type, nvinfer1::PluginFormat format) const;
		virtual void configureWithFormat(
			const nvinfer1::Dims* inputDims, int nbInputs, const nvinfer1::Dims* outputDims,
			int nbOutputs, nvinfer1::DataType type, nvinfer1::PluginFormat format, int maxBatchSize);
		virtual int getNbOutputs() const;
		virtual nvinfer1::Dims getOutputDimensions(int index, const nvinfer1::Dims* inputs, int nbInputDims);
		virtual void configure(const nvinfer1::Dims* inputDims, int nbInputs, const nvinfer1::Dims* outputDims, int nbOutputs, int maxBatchSize);
		virtual int initialize();
		virtual void terminate();
		virtual size_t getWorkspaceSize(int maxBatchSize) const;
		virtual int enqueue(int batchSize, const void* const* inputs, void** outputs, void* workspace, cudaStream_t stream);
		virtual size_t getSerializationSize();
		virtual void serialize(void* buffer);

	protected:
		std::shared_ptr<LayerConfig> config_;
		std::vector<GTensor> inputTensors_;
		std::vector<GTensor> outputTensors_;
	};

#define SETUP_PLUGIN(class_, pattern_)									  \
	static std::string pattern() {										  \
		return pattern_;												  \
	}																	  \
																		  \
	static std::shared_ptr<Plugin::TRTPlugin> creator() {					  \
		return std::shared_ptr<Plugin::TRTPlugin>(new class_());			  \
	}

#define AddPlugin(class_)	\
	Manager::addPluginSupport(class_::creator, class_::pattern())

	class Manager {
	public:

		typedef std::shared_ptr<TRTPlugin>(*PluginCreater)();

		struct PluginInfo {
			PluginCreater creater;
			std::string pattern;
		};

		Manager();
		virtual ~Manager();

		void addPluginSupport(PluginCreater creater, const std::string& pattern);

		//如果能找到插件，则说明支持他
		virtual bool support(const std::string& layerName);

		//通过模式匹配的方式，查找支持的插件
		PluginInfo* findPlugin(const std::string& layerName);
		void buildPluginList();

		//根据layerName创建插件
		virtual std::shared_ptr<TRTPlugin> createPlugin(const std::string& layerName);
		virtual nvinfer1::IPlugin* builderCreate(const std::string& layerName, const nvinfer1::Weights* weights, int nbWeights);
		virtual nvinfer1::IPlugin* inferCreate(const std::string& layerName, const void* serialData, size_t serialLength);

	private:
		std::vector<std::shared_ptr<nvinfer1::IPluginExt>> plugins_;
		std::vector<PluginInfo> pluginRegister_;
	};

	class TRTBuilderPluginFactory : public nvcaffeparser1::IPluginFactoryExt, public nvinfer1::IPluginFactory {

	public:
		TRTBuilderPluginFactory();
		virtual ~TRTBuilderPluginFactory();
		virtual bool isPluginExt(const char* layerName);
		virtual bool isPlugin(const char* layerName);
		virtual nvinfer1::IPlugin* createPlugin(const char* layerName, const nvinfer1::Weights* weights, int nbWeights);
		virtual nvinfer1::IPlugin* createPlugin(const char* layerName, const void* serialData, size_t serialLength);

	private:
		std::shared_ptr<Manager> manager_;
	};

	///////////////////////////////////////////////////////////////////////////////////////////
	std::shared_ptr<nvcaffeparser1::IPluginFactoryExt> createPluginFactoryForBuildPhase();
	std::shared_ptr<nvinfer1::IPluginFactory> createPluginFactoryForInferPhase();

	dim3 gridDims(int numJobs);
	dim3 blockDims(int numJobs);
}; //namespace Plugin

#endif //PLUGIN_BASE_HPP