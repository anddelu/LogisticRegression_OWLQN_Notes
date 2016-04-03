#pragma once

#include <deque>
#include <vector>
#include <cmath>
#include <iostream>

#include "OWLQN.h"

//��Ҫ�����ǰ�����(MatrixMarket��ʽ)��������������
class LogisticRegressionProblem {
	std::deque<size_t> indices; //����i��ά��ֵ��indeces[instance_starts[i]]��indices[instance_starts[i-1] - 1]
	std::deque<float> values;//����i����indices��ά��ֵ��Ӧ������ֵ��values[instance_starts[i]]��values[instance_starts[i-1] - 1]
	std::deque<size_t> instance_starts;//����i��indices��values�е���ʼλ�ã�instance_starts[i]
	std::deque<bool> labels;//����i��label��labels[i]��labelΪbool
	size_t numFeats;//������ά��

public:
	LogisticRegressionProblem(size_t numFeats) : numFeats(numFeats) {
		instance_starts.push_back(0);
	}

	LogisticRegressionProblem(const char* mat, const char* labels);
	void AddInstance(const std::deque<size_t>& inds, const std::deque<float>& vals, bool label);
	void AddInstance(const std::vector<float>& vals, bool label);
	double ScoreOf(size_t i, const std::vector<double>& weights) const;

	bool LabelOf(size_t i) const {
		return labels[i];
	}

	//����������i��i��1-������ȷ�ĸ��ʡ��ݶ�����������ʹ����ʧ�����ķ���������������ݶ�
	void AddMultTo(size_t i, double mult, std::vector<double>& vec) const {
		if (labels[i]) mult *= -1; //���Ը��ı�ǩֵ(-label[i])
		//��������i�ĸ���ά��index���ø�ά�ȶ��ڵ�����ֵ*multȥ�����ݶ�������ά��index
		for (size_t j=instance_starts[i]; j < instance_starts[i+1]; j++) {
			size_t index = (indices.size() > 0) ? indices[j] : j - instance_starts[i];
			vec[index] += mult * values[j];//��������ֵvalues
		}
	}

	//��������
	size_t NumInstances() const {
		return labels.size();
	}

	//����������ά��
	size_t NumFeats() const {
		return numFeats;
	}
};

struct LogisticRegressionObjective : public DifferentiableFunction {
	//�洢����������
	const LogisticRegressionProblem& problem;
	const double l2weight;

	LogisticRegressionObjective(const LogisticRegressionProblem& p, double l2weight = 0) : problem(p), l2weight(l2weight) { }

	//�������㵱ǰ�Ĳ���input����µ���ʧ���ݶ�����
	//�������Ż��������ʧ��������ʧ�������ݶ�
	double Eval(const DblVec& input, DblVec& gradient);

};
