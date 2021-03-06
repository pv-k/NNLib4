#ifndef ENTROPY_LOSS_COST_MODULE_H
#define ENTROPY_LOSS_COST_MODULE_H

#include "CostModule.h"
#include <algorithm>

template <class T>
class CrossEntropyCostModule : public CostModule<T>
{
	const T eps;
	double log2_;

	double log2(double x)
	{
		return (double)(std::log(x) / log2_);
	}

public:

	CrossEntropyCostModule() : CostModule(), eps(std::numeric_limits<T>::epsilon()), log2_(std::log(2))
	{
	}

	virtual double sub_GetCost(const Tensor<T>& net_output, const Tensor<T>& expected_output, 
		const std::vector<T>& importance_weights, bool normalize_by_importance, double lambda);
	
	virtual void sub_bprop(const Tensor<T>& net_output, const Tensor<T>& expected_output, 
		const std::vector<T>& importance_weights, bool normalize_by_importance, Tensor<T>& output_gradients_buffer, double lambda);
};

template <class T>
double CrossEntropyCostModule<T>::sub_GetCost(const Tensor<T>& net_output, const Tensor<T>& expected_output, 
		const std::vector<T>& importance_weights, bool normalize_by_importance, double lambda)
{
	double importance_sum = 0;
	size_t minibatch_size = net_output.GetDimensionSize(net_output.NumDimensions()-1);
	if (normalize_by_importance)
		for (size_t sample_ind = 0; sample_ind<minibatch_size; sample_ind++)
			importance_sum += importance_weights[sample_ind];
	else
		importance_sum = 1;
	
	double cost = 0;
	size_t num_features = net_output.Numel() / minibatch_size;
	for (size_t sample_ind = 0; sample_ind<minibatch_size; sample_ind++)
	{
		size_t offset = num_features*sample_ind;
		double feature_cost = 0;
		for (size_t feature_ind = 0; feature_ind<num_features; feature_ind++)
		{
			size_t feature_offset = offset+feature_ind;
			double feature_probability = net_output[feature_offset];
			feature_cost -= expected_output[feature_offset]*log2(feature_probability+eps);
		}
		cost += feature_cost*importance_weights[sample_ind];
	}

	return lambda*cost/importance_sum;
}

template <class T>
void CrossEntropyCostModule<T>::sub_bprop(const Tensor<T>& net_output, const Tensor<T>& expected_output, 
		const std::vector<T>& importance_weights, bool normalize_by_importance, Tensor<T>& output_gradients_buffer, double lambda)
{
	double importance_sum = 0;
	size_t minibatch_size = net_output.GetDimensionSize(net_output.NumDimensions()-1);
	assert(minibatch_size == importance_weights.size());
	size_t num_features = net_output.Numel() / minibatch_size;
	if (normalize_by_importance)
		for (size_t sample_ind = 0; sample_ind<minibatch_size; sample_ind++)
			importance_sum += importance_weights[sample_ind];
	else
		importance_sum = 1;
	
	for (size_t sample_ind = 0; sample_ind<minibatch_size; sample_ind++)
	{
		size_t offset = num_features*sample_ind;

		double sample_importance = importance_weights[sample_ind]/importance_sum;
		for (size_t feature_ind = 0; feature_ind<num_features; feature_ind++)
		{
			size_t feature_offset = offset+feature_ind;
			double feature_probability = net_output[feature_offset];
			output_gradients_buffer[feature_offset] = static_cast<T>(-lambda*sample_importance* expected_output[feature_offset] / (feature_probability+eps) / log2_);
		}
	}
}

#endif