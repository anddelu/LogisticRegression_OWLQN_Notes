#include "TerminationCriterion.h"

#include "OWLQN.h"

#include <limits>
#include <iomanip>
#include <cmath>

using namespace std;

//���ƽ����߱�׼
double RelativeMeanImprovementCriterion::GetValue(const OptimizerState& state, std::ostream& message) {
	double retVal = numeric_limits<double>::infinity();

	//����Ѿ���¼��5��������ʧֵ��
	if (prevVals.size() > 5) {
		//ȡ���׵�ֵ
		double prevVal = prevVals.front();
		//����Ѿ���10��ֵ�ˣ��ͰѶ��׵�ֵɾ��
		if (prevVals.size() == 10) prevVals.pop_front();
		//�ö��׵�ֵ��ȥ���µ�ֵ�����Զ��еĳ��ȣ��õ����е�ƽ�����
		double averageImprovement = (prevVal - state.GetValue()) / prevVals.size();
		//�ö��е�ƽ����߳��Ե�ǰֵ�õ��������ڵ�ǰֵ�ı���
		double relAvgImpr = averageImprovement / fabs(state.GetValue());
		message << setprecision(4) << scientific << right;
		message << "  (" << setw(10) << relAvgImpr << ") " << flush;
		//����߱������浽retVal��
		retVal = relAvgImpr;
	} else {
		message << "  (wait for five iters) " << flush;
	}

	//����ǰ��������ʧ��ӵ���ʧ�б�β��
	prevVals.push_back(state.GetValue());
	//������ߵı���
	return retVal;
}
