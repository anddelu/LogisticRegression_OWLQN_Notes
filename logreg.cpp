#include "logreg.h"
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

void skipEmptyAndComment(ifstream& file, string& s) {
	do {
		getline(file, s);
	} while (s.size() == 0 || s[0] == '%');
}

//������������
LogisticRegressionProblem::LogisticRegressionProblem(const char* matFilename, const char* labelFilename) {
	ifstream matfile(matFilename);
	if (!matfile.good()) {
		cerr << "error opening matrix file " << matFilename << endl;
		exit(1);
	}
	string s;
	getline(matfile, s);
	//MatrixMarket��һ���ļ���ʽ(http://math.nist.gov/MatrixMarket/formats.html)���������ָ�ʽ�ļ��ĵ�һ��
	if (!s.compare("%%MatrixMarket matrix coordinate real general")) {
		skipEmptyAndComment(matfile, s);

		stringstream st(s);
		size_t numIns, numNonZero;
		st >> numIns >> numFeats >> numNonZero; //��������������0���ݸ���

		vector<deque<size_t> > rowInds(numIns);
		vector<deque<float> > rowVals(numIns);
		for (size_t i = 0; i < numNonZero; i++) {
			size_t row, col;
			float val;
			matfile >> row >> col >> val; //�кţ��кţ���ֵ
			row--;
			col--;
			rowInds[row].push_back(col);
			rowVals[row].push_back(val);
		}

		matfile.close();

		ifstream labfile(labelFilename);
		getline(labfile, s);
		if (s.compare("%%MatrixMarket matrix array real general")) {
			cerr << "unsupported label file format in " << labelFilename << endl;
			exit(1);
		}

		skipEmptyAndComment(labfile, s);
		stringstream labst(s);
		size_t labNum, labCol;
		labst >> labNum >> labCol;
		if (labNum != numIns) {
			cerr << "number of labels doesn't match number of instances in " << labelFilename << endl;
			exit(1);
		} else if (labCol != 1) {
			cerr << "label matrix may not have more than one column" << endl;
			exit(1);
		}

		instance_starts.push_back(0);

		for (size_t i=0; i<numIns; i++) {
			int label;
			labfile >> label;
			bool bLabel;
			switch (label) {
					case 1:
						bLabel = true;
						break;

					case -1:
						bLabel = false;
						break;

					default:
						cerr << "illegal label: must be 1 or -1" << endl;
						exit(1);
			}

			AddInstance(rowInds[i], rowVals[i], bLabel);//����i������������label
		}

		labfile.close();
	} else if (!s.compare("%%MatrixMarket matrix array real general")) {
		skipEmptyAndComment(matfile, s);
		stringstream st(s);
		size_t numIns;
		st >> numIns >> numFeats;

		vector<vector<float> > rowVals(numIns);

		for (size_t j=0; j<numFeats; j++) {
			for (size_t i=0; i<numIns; i++) {
				float val;
				matfile >> val;
				rowVals[i].push_back(val);
			}
		}

		matfile.close();

		ifstream labfile(labelFilename);
		getline(labfile, s);
		if (s.compare("%%MatrixMarket matrix array real general")) { 
			cerr << "unsupported label file format in " << labelFilename << endl;
			exit(1);
		}

		skipEmptyAndComment(labfile, s);
		stringstream labst(s);
		size_t labNum, labCol;
		labst >> labNum >> labCol;
		if (labNum != numIns) {
			cerr << "number of labels doesn't match number of instances in " << labelFilename << endl;
			exit(1);
		} else if (labCol != 1) {
			cerr << "label matrix may not have more than one column" << endl;
			exit(1);
		}

		instance_starts.push_back(0);
		for (size_t i=0; i<numIns; i++) {
			int label;
			labfile >> label;
			bool bLabel;
			switch (label) {
					case 1:
						bLabel = true;
						break;

					case -1:
						bLabel = false;
						break;

					default:
						cerr << "illegal label: must be 1 or -1" << endl;
						exit(1);
			}

			AddInstance(rowVals[i], bLabel);
		}

		labfile.close();
	} else {
		cerr << "unsupported matrix file format in " << matFilename << endl;
		exit(1);
	}
}

//���һ������������
void LogisticRegressionProblem::AddInstance(const deque<size_t>& inds, const deque<float>& vals, bool label) {
	for (size_t i=0; i<inds.size(); i++) {
		indices.push_back(inds[i]);
		values.push_back(vals[i]);
	}//��ǰ������inds.size()������ά���±꼰���Ӧ������ֵ
	instance_starts.push_back(indices.size());//��һ���������ݵĿ�ʼλ�ã���indices��values�е��±꣩
	labels.push_back(label);//��ǰ������label
}

void LogisticRegressionProblem::AddInstance(const vector<float>& vals, bool label) {
	for (size_t i=0; i<vals.size(); i++) {
		values.push_back(vals[i]);
	}
	instance_starts.push_back(values.size()); 
	labels.push_back(label);
}

//����yi*��W*X + b)
double LogisticRegressionProblem::ScoreOf(size_t i, const vector<double>& weights) const {
	double score = 0;
	for (size_t j=instance_starts[i]; j < instance_starts[i+1]; j++) {
		double value = values[j];
		size_t index = (indices.size() > 0) ? indices[j] : j - instance_starts[i];
		score += weights[index] * value;
	}//��������i�ĸ���ά�ȵ�����ֵ��Ȩ�صĳ˻��ĺͣ���score
	if (!labels[i]) score *= -1; //�������i��label��-1����scoreȡ��������������i��label
	return score;
}

//�������㵱ǰ�Ĳ�������µ���ʧ���ݶ�����
//input�ǲ�������
double LogisticRegressionObjective::Eval(const DblVec& input, DblVec& gradient) {
	double loss = 1.0; //ΪʲôҪ��ʼ��Ϊ1��

	//����ʹ����ʧ��������������������ݶ�
	//������ֵ�loss��gradient
	for (size_t i=0; i<input.size(); i++) {
		loss += 0.5 * input[i] * input[i] * l2weight;//0.5 * C * wi^2�ĺͣ��ۼ���ʧ
		gradient[i] = l2weight * input[i];//C * wi����ǰ���ݶ�(�������)
	}

	for (size_t i =0 ; i<problem.NumInstances(); i++) {
		double score = problem.ScoreOf(i, input);

		//insProb������i����ȷ���ൽyi�ĸ���
		double insLoss, insProb;
		if (score < -30) {
			insLoss = -score;//��ʧȡ-score����-score�Ƚϴ�ʱ��log(1.0 + exp(-score))Լ����-score
			insProb = 0;//��ǰģ�ͷ�����ȷ�ĸ���Ϊ0
		} else if (score > 30) {//score����30ʱ
			insLoss = 0;//��ʧΪ0����score�Ƚϴ�ʱ��log(1.0 + exp(-score))Լ����0
			insProb = 1;//��ǰģ�ͷ�����ȷ�ĸ���Ϊ1
		} else {//��score��-30��30֮��ʱ
			double temp = 1.0 + exp(-score);
			insLoss = log(temp);
			insProb = 1.0/temp;
		}
		loss += insLoss;//�ۼ���ʧ

		//����ʹ����ʧ�����ķ���������������ݶ�
		//����������i��i��1-������ȷ�ĸ��ʡ��ݶ�����
		problem.AddMultTo(i, 1.0 - insProb, gradient);
	}

	return loss;
}
