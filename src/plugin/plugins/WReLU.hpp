
#ifndef WReLU_HPP
#define WReLU_HPP

#include <plugin/plugin.hpp>

class WReLU : public Plugin::TRTPlugin {
public:
	//���ò��������ͨ���ִ꣬�в������������ͬʱִ��ģʽƥ�����֣�������ÿ�����������ģʽƥ��
	//��ƥ�䷽���õ���ccutil::patternMatch��������������
	SETUP_PLUGIN(WReLU, "WReLU*");

	//������ֻ��һ������������shape��������0��shape����˷���input0��shape
	virtual nvinfer1::Dims outputDims(int index, const nvinfer1::Dims* inputDims, int nbInputDims) {
		return inputDims[0];
	}

	//ִ�й���
	int enqueue(const std::vector<Plugin::GTensor>& inputs, std::vector<Plugin::GTensor>& outputs, cudaStream_t stream);
};

#endif //WReLU_HPP