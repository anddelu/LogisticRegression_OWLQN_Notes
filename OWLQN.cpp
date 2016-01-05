#include "OWLQN.h"

#include "TerminationCriterion.h"

#include <vector>
#include <deque>
#include <cmath>
#include <iostream>
#include <iomanip>

using namespace std;

double OptimizerState::dotProduct(const DblVec& a, const DblVec& b) {
	double result = 0;
	for (size_t i=0; i<a.size(); i++) {
		result += a[i] * b[i];
	}
	return result;
}

void OptimizerState::addMult(DblVec& a, const DblVec& b, double c) {
	for (size_t i=0; i<a.size(); i++) {
		a[i] += b[i] * c;
	}
}

void OptimizerState::add(DblVec& a, const DblVec& b) {
	for (size_t i=0; i<a.size(); i++) {
		a[i] += b[i];
	}
}

void OptimizerState::addMultInto(DblVec& a, const DblVec& b, const DblVec& c, double d) {
	for (size_t i=0; i<a.size(); i++) {
		a[i] = b[i] + c[i] * d;
	}
}

void OptimizerState::scale(DblVec& a, double b) {
	for (size_t i=0; i<a.size(); i++) {
		a[i] *= b;
	}
}

void OptimizerState::scaleInto(DblVec& a, const DblVec& b, double c) {
	for (size_t i=0; i<a.size(); i++) {
		a[i] = b[i] * c;
	}
}

//OWLQN
//�����½�����dir��������һ���ݶȣ����ݶȵĸ�����
void OptimizerState::MakeSteepestDescDir() {

	if (l1weight == 0) {
		//l1������ȨֵΪ0ʱ�����ҷ���dirΪ��ʧ�����ݶȵĸ�����
		scaleInto(dir, grad, -1);
	} else {
		//l1������Ȩֵ��Ϊ0ʱ��������ʧ�������ݶȺ�l1������Ȩֵ��ȷ�����ҷ���
		for (size_t i=0; i<dim; i++) {
			if (x[i] < 0) {
				//xi<0ʱ���ҵ�һ��С��0������xi�ķ���Ϊ�������ݶ�ȡ�ҵ����½�����Ϊ���ݶȵķ�����
				dir[i] = -grad[i] + l1weight;
			} else if (x[i] > 0) {
				//xi>0ʱ����һ������0������xi�ķ���Ϊ�������ݶ�ȡ�󵼣��½�����Ϊ���ݶȵķ�����
				dir[i] = -grad[i] - l1weight;
			} else {//xi == 0
				if (grad[i] < -l1weight) {
					//xi == 0���ҵ�<0�����ݶ�ȡ�ҵ����½�����Ϊ���ݶȵķ�����
					dir[i] = -grad[i] - l1weight;
				} else if (grad[i] > l1weight) {
					//xi == 0����>0�����ݶ�ȡ�󵼣��½�����Ϊ���ݶȵķ�����
					dir[i] = -grad[i] + l1weight;
				} else {
					//xi == 0�����ҵ�����Ϊ0���½�����Ϊ0
					dir[i] = 0;
				}
			}
		}
	}

	//��ǰ�������½�����
	steepestDescDir = dir;
}

//lgfgs
//�����½�����dir�������Ķ����ݶȣ�
//lbfgs�е�two loop���ù�ȥm�ε���Ϣ�����Ƽ���Hessian�������(�����õ���ǰ���½�����)
void OptimizerState::MapDirByInverseHessian() {
	int count = (int)sList.size(); //lbfgs����Ĺ�ȥ�ĵ�������ĸ���m

	if (count != 0) {
		//��һ��for loop
		for (int i = count - 1; i >= 0; i--) {
			alphas[i] = -dotProduct(*sList[i], dir) / roList[i]; //��ͬ�������еĵط��ǣ�����ruo�ļ���δȡ���������������ǳ��������⣬�����alphaȡ�˸�ֵ
			addMult(dir, *yList[i], alphas[i]);
		}

		//����lastY��lastRuo scale��һ��dir
		//ΪʲôҪscale�أ�
		const DblVec& lastY = *yList[count - 1];
		double yDotY = dotProduct(lastY, lastY);
		double scalar = roList[count - 1] / yDotY;
		scale(dir, scalar);

		//�ڶ���for loop
		for (int i = 0; i < count; i++) {
			double beta = dotProduct(*yList[i], dir) / roList[i];//��ͬ�������еĵط��ǣ�����ruo�ļ���δȡ���������������ǳ���
			addMult(dir, *sList[i], -alphas[i] - beta);
		}
	}
}

void OptimizerState::FixDirSigns() {
	//�������l1������
	if (l1weight > 0) {
		//dim�ǲ�������������ά����
		for (size_t i = 0; i<dim; i++) {
			//dir[i]��ԭ�������ݶȼ�������ķ���ͬ��ά�ȣ�����
			if (dir[i] * steepestDescDir[i] <= 0) {
				dir[i] = 0;
			}
		}
	}
}

void OptimizerState::UpdateDir() {
	MakeSteepestDescDir();
	MapDirByInverseHessian();
	FixDirSigns();

#ifdef _DEBUG
	TestDirDeriv();
#endif
}

void OptimizerState::TestDirDeriv() {
	double dirNorm = sqrt(dotProduct(dir, dir));
	double eps = 1.05e-8 / dirNorm;
	GetNextPoint(eps);
	double val2 = EvalL1();
	double numDeriv = (val2 - value) / eps;
	double deriv = DirDeriv();
	if (!quiet) cout << "  Grad check: " << numDeriv << " vs. " << deriv << "  ";
}

//����������Բ��Ҹ��²�����һ���֣��ж�ֹͣ���ҵ������е��½�����*���ݶ�[δ����alpha]
double OptimizerState::DirDeriv() const {
	if (l1weight == 0) {
		return dotProduct(dir, grad);
	} else {
		double val = 0.0;
		for (size_t i = 0; i < dim; i++) {
			//ͬMakeSteepestDescDir�����ݶȵļ���
			if (dir[i] != 0) { 
				if (x[i] < 0) {
					val += dir[i] * (grad[i] - l1weight);
				} else if (x[i] > 0) {
					val += dir[i] * (grad[i] + l1weight);
				} else if (dir[i] < 0) {
					val += dir[i] * (grad[i] - l1weight);
				} else if (dir[i] > 0) {
					val += dir[i] * (grad[i] + l1weight);
				}
			}
		}

		return val;
	}
}

//����x��dir��alpha����µĲ��ҵ�newX
void OptimizerState::GetNextPoint(double alpha) {
	//����µĲ��ҵ�newX
	addMultInto(newX, x, dir, alpha);
	if (l1weight > 0) {
		for (size_t i=0; i<dim; i++) {
			//������ҵ�������ޣ�����
			if (x[i] * newX[i] < 0.0) {
				newX[i] = 0.0;
			}
		}
	}
}

double OptimizerState::EvalL1() {
	//�����µ�X�����������������µ��ݶ�newGrad���µ���ʧֵloss
	double val = func.Eval(newX, newGrad);
	//���l1������Ĳ���Ϊ������ʧ����l1������Ĳ���
	if (l1weight > 0) {
		for (size_t i=0; i<dim; i++) {
			val += fabs(newX[i]) * l1weight;
		}
	}

	//������ʧֵ
	return val;
}

//���˵����Բ��ң��Ҹ��µĲ�����ѧϰ�ʣ�alpha
void OptimizerState::BackTrackingLineSearch() {
	//����������Բ��Ҹ��²�����һ���֣��ж�ֹͣ���ҵ������е��½�����*���ݶ�[δ����alpha]
	double origDirDeriv = DirDeriv();
	// if a non-descent direction is chosen, the line search will break anyway, so throw here
	// The most likely reason for this is a bug in your function's gradient computation
	if (origDirDeriv >= 0) {
		cerr << "L-BFGS chose a non-descent direction: check your gradient!" << endl;
		exit(1);
	}

	double alpha = 1.0;
	double backoff = 0.5;

	//��һ�ε���ʱ
	if (iter == 1) {
		//alpha = 0.1;
		//backoff = 0.5;
		//����dir�ľ���ֵ
		double normDir = sqrt(dotProduct(dir, dir));
		//��alpha��backoff���ó��µ��ض�ֵ
		alpha = (1 / normDir);
		backoff = 0.1;
	}

	const double c1 = 1e-4;
	double oldValue = value; //��¼֮ǰ����ʧֵ

	while (true) {
		//����x��dir��alpha����µĲ��ҵ�newX
		GetNextPoint(alpha);
		//����newX�����������������µ��ݶ�newGrad���µ���ʧֵvalue
		value = EvalL1();


		//����������Բ��Ҹ��²�����ֹͣ��������
		if (value <= oldValue + c1 * origDirDeriv * alpha) break;

		if (!quiet) cout << "." << flush;

		//����alpha�����������ֹͣ�����������������ˣ���beta^n
		alpha *= backoff;
	}

	if (!quiet) cout << endl;
}

//�Ż���״̬Ǩ�ƣ�����lbfgs�����������б�
void OptimizerState::Shift() {
	DblVec *nextS = NULL, *nextY = NULL;

	//lbfgs�м�����ĸ���
	int listSize = (int)sList.size();

	//�տ�ʼʱ���������m�����������µļ�����ռ�
	if (listSize < m) {
		try {
			nextS = new vector<double>(dim);
			nextY = new vector<double>(dim);
		} catch (bad_alloc) {
			m = listSize; //δ����Ŀ����ڴ治��ʱ����ʹ�õ�ǰ�ܹ�����ļ������������Ϊm
			if (nextS != NULL) { //�����S����ɹ��ˣ���Yδ����ɹ����Ͱ�S�ͷŵ�
				delete nextS;
				nextS = NULL;
			}
		}
	}

	//���δ�����µ�S��Y�����Ѿ���m����������
	if (nextS == NULL) {
		nextS = sList.front();
		sList.pop_front(); //�������ϵ�s
		nextY = yList.front();
		yList.pop_front(); //�������ϵ�y
		roList.pop_front(); //�������ϵ�rou
	}

	//����������ݶȵĲ�ֵ������*nextS��nextY
	addMultInto(*nextS, newX, x, -1);
	addMultInto(*nextY, newGrad, grad, -1);

	//�����µ�ruo����ͬ�������еĵط��ǣ�����δȡ����
	double ro = dotProduct(*nextS, *nextY); 

	//�����µļ�����
	sList.push_back(nextS);
	yList.push_back(nextY);
	roList.push_back(ro);

	//���µĲ��������ݶ���Ϊ��ǰ�Ĳ��������ݶ�
	x.swap(newX);
	grad.swap(newGrad);

	//������������
	iter++;
}

//Ѱ����С��ʧ�Ĺ���
//��������Ϊ���Ż����⡢��ʼ����������ʱ�Ĳ���������Ľ������l1������Ĳ������������limit-memory�м���ĵ�������������
void OWLQN::Minimize(DifferentiableFunction& function, const DblVec& initial, DblVec& minimum, double l1weight, double tol, int m) const {
	//��������Ϊ���Ż����⡢��ʼ������limit-memory�м���ĵ���������������l1������Ĳ������Ƿ������Ĭ
	OptimizerState state(function, initial, m, l1weight, quiet);

	if (!quiet) {
		cout << setprecision(4) << scientific << right;
		cout << endl << "Optimizing function of " << state.dim << " variables with OWL-QN parameters:" << endl;
		cout << "   l1 regularization weight: " << l1weight << "." << endl;
		cout << "   L-BFGS memory parameter (m): " << m << endl;
		cout << "   Convergence tolerance: " << tol << endl;
		cout << endl;
		cout << "Iter    n:  new_value    (conv_crit)   line_search" << endl << flush;
		cout << "Iter    0:  " << setw(10) << state.value << "  (***********) " << flush;
	}

	ostringstream str;
	termCrit->GetValue(state, str);

	while (true) {
		//����search direction
		state.UpdateDir();
		//����step size
		state.BackTrackingLineSearch();

		//�ж��Ƿ�������ֹ����
		ostringstream str;
		//���ٵ���ʧֵ����ڵ�ǰ��ʧ�ı���
		double termCritVal = termCrit->GetValue(state, str);
		if (!quiet) {
			cout << "Iter " << setw(4) << state.iter << ":  " << setw(10) << state.value;
			cout << str.str() << flush;
		}
		//������ٵ���ʧֵ����ڵ�ǰ��ʧ�ı���С��ĳ����ֵ����ֹͣ����
		if (termCritVal < tol) break;

		//����״̬
		state.Shift();
	}

	if (!quiet) cout << endl;

	//�����յõ��Ĳ����浽������������
	minimum = state.newX;
}
