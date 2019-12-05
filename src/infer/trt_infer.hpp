

#ifndef TRT_INFER_HPP
#define TRT_INFER_HPP

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <opencv2/opencv.hpp>

//Ϊ�˱����Plugin�����ã���inferenceʱĬ��û�в��������hpp��cpp���ɸɻ���Զ���������꣬��ʹ�ò��ʱ�򿪼���
//�ú궨��ֻ���ڷ����л�ģ��֮ǰ��pluginFactory���˸�ֵ��û�������ط�����������
#define HAS_PLUGIN

namespace TRTInfer {

	enum DataHead {
		DataHead_InGPU,
		DataHead_InCPU
	};

	//Tensor�࣬�������ķ�װ����GPU��CPU�����������˷�װ
	//ʹ����ֻ��Ҫ����������������Լ���δ����������������Ӧ����ô���Ƶ�GPU�Լ���ʱ���Ƶ�CPU
	class Tensor {
	public:
		Tensor();
		Tensor(const Tensor& other);
		Tensor(int n, int c, int h, int w);
		Tensor(int ndims, const int* dims);
		Tensor(const std::vector<int>& dims);
		Tensor& operator = (const Tensor& other);

		inline std::vector<int> dims() { return {num_, channel_, height_, width_}; }
		void copyFrom(const Tensor& other);
		inline cv::Size size() { return cv::Size(width_, height_); }
		void release();
		inline int offset(int n = 0, int c = 0, int h = 0, int w = 0) { return ((n * this->channel_ + c) * this->height_ + h) * this->width_ + w; }
		void setTo(float value);
		bool empty();
		void resize(int n, int c = -1, int h = -1, int w = -1);
		void reshape(int n, int c = -1, int h = -1, int w = -1);
		void reshapeLike(const Tensor& other);
		void resizeLike(const Tensor& other);
		int count(int start_axis = 0) const;
		void print();

		/* ��gpu
		* copyedIfCPU = true�����������cpu�ϣ����Ƶ�gpu�����޸�headΪInGPU
		* copyedIfCPU = false���������ݼ��Ƿ���cpu�ϣ������ӣ����޸�headΪInGPU
		* ���gpuû�з���ռ䣬��������cpu��ʱ����Ϊgpu����ռ�*/
		void to_gpu(bool copyedIfCPU = true);
		void to_cpu(bool copyedIfGPU = true);
		const float* cpu() const;
		const float* gpu() const;
		float* cpu();
		float* gpu();
		inline float* cpu(int n, int c = 0, int h = 0, int w = 0) { return cpu() + offset(n, c, h, w); }
		inline float* gpu(int n, int c = 0, int h = 0, int w = 0) { return gpu() + offset(n, c, h, w); }
		float& at(int n = 0, int c = 0, int h = 0, int w = 0) { return *(cpu() + offset(n, c, h, w)); }
		cv::Mat at(int n = 0, int c = 0) { return cv::Mat(height_, width_, CV_32F, cpu(n, c)); }

		/* ����ͼ�����ͼ���С��һ�»�resize��ͨ��������ߡ�����CV_32FҪ�����ƥ�䣬
		* ���򱨴��������ֵ���κβ����������һ�������ţ�*/
		void setMat(int n, const cv::Mat& image);

		/* ����ͼ��result = (image[c] / 255 - mean[c]) / std[c]
		* Ҫ��ͼ��ͨ��һ�£���Ҫ��������ʽ*/
		void setNormMat(int n, const cv::Mat& image, float mean[3], float std[3]);
		int num() const{ return num_; }
		int channel() const { return channel_; }
		int height() const { return height_; }
		int width() const { return width_; }
		int bytes() const { return bytes_; }
		int bytes(int start_axis) const { return count(start_axis) * sizeof(float); }
		Tensor transpose(int axis0, int axis1, int axis2, int axis3);
		void transposeInplace(int axis0, int axis1, int axis2, int axis3);
		void from(const float* ptr, int n, int c = 1, int h = 1, int w = 1);

	private:
		int num_ = 0, channel_ = 0, height_ = 0, width_ = 0;
		void* device_ = nullptr;
		float* host_ = nullptr;
		size_t capacity_ = 0;
		size_t bytes_ = 0;
		DataHead head_ = DataHead_InCPU;
	};

	//TensorRT��inference��װΪEngine��������ֱ�Ӽ���ģ�ͣ�ͨ��Tensor����������������Ҳ�����input(0)��resize(batch)ʵ�ֶ�batch������
	//forwardʱ���Զ���output��resize��ʹ����numά��һ��
	class Engine {
	public:
		virtual bool load(const std::string& file) = 0;
		virtual void destroy() = 0;
		virtual void forward() = 0;
		virtual int maxBatchSize() = 0;

		virtual std::shared_ptr<Tensor> input(int index = 0) = 0;
		virtual std::shared_ptr<Tensor> output(int index = 0) = 0;

		/* �������ֻ�ȡtensorָ�룬��tensor����������Ҳ����������������ҽ���������ģ��ʱ��input��markedOutputָ����Tensor*/
		virtual std::shared_ptr<Tensor> tensor(const std::string& name) = 0;
	};

	void setDevice(int device_id);
	std::shared_ptr<Engine> loadEngine(const std::string& file);
};	//TRTInfer


#endif //TRT_INFER_HPP