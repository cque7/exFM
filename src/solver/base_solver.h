/**
 *  Copyright (c) 2021 by exFM Contributors
 */
#pragma once
#include "feature/feat_manager.h"
#include "solver/parammeter_container.h"
#include "utils/base.h"
#include "train/train_opt.h"

class Sample {
 public:
  Sample() {}
  ~Sample() {}

  real_t forward();

  void backward();

  vector<FmLayerNode> fm_layer_nodes;
  
  real_t logit;
  real_t loss;
  real_t grad;
  real_t sum[DIM];
  real_t sum_sqr[DIM];
  int y;
};

class BaseSolver {
 public:
  BaseSolver(const FeatManager &feat_manager);

  virtual ~BaseSolver() {}

  real_t forward(const char *line) {
    procOneLine(line);
    return batch_samples[sample_idx].forward();
  }

  void train(const char *line, int &y, real_t &logit, real_t & loss, real_t & grad) {
    forward(line);

    batch_samples[sample_idx].backward();
    
    y = batch_samples[sample_idx].y;
    logit = batch_samples[sample_idx].logit;
    loss = batch_samples[sample_idx].loss;
    grad = batch_samples[sample_idx].grad;

    rotateSampleIdx();

    DEBUG_OUT << "BaseSolver::train "
              << " y " << y << " logit " << logit << endl;
  }

  void test(const char *line, int &y, real_t &logit) {
    forward(line);
    y = batch_samples[sample_idx].y;
    logit = batch_samples[sample_idx].logit;
  }

protected:
  virtual void update() = 0;

  int procOneLine(const char *line);

  void rotateSampleIdx() {
    ++sample_idx;
    if (sample_idx == batch_size) {
      // merge gradient
      batch_params.clear();
      for (size_t i = 0; i < batch_size; i++) {
        const Sample &sample = batch_samples[i];

        for (const auto & fm_node : sample.fm_layer_nodes) {
          for (const auto & param_node : fm_node.backward_nodes) {
            auto ins_ret = batch_params.insert({param_node.param, param_node});
            if (!ins_ret.second) {
              ins_ret.first->second.fm_grad += param_node.fm_grad;
              ins_ret.first->second.count += 1;
            }
          }
        }
      }
      DEBUG_OUT << "batch update :" << batch_params.size() << endl;
      update();
      sample_idx = 0;
    }
  }

  void batchReduce(FMParamUnit &grad, int count) {
    switch (train_opt.batch_grad_reduce_type) {
      case TrainOption::BatchGradReduceType_Sum:
        break;
      case TrainOption::BatchGradReduceType_AvgByOccurrences:
        grad /= count;
        break;
      case TrainOption::BatchGradReduceType_AvgByOccurrencesSqrt:
        grad /= std::sqrt(count);
        break;
      default: 
      // BatchGradReduceType_AvgByBatchSize by default
        grad /= train_opt.batch_size;
        break;
    }
  }

protected:
  const FeatManager &feat_manager_;
  vector<DenseFeatContext> dense_feas;
  vector<SparseFeatContext> sparse_feas;
  vector<VarlenSparseFeatContext> varlen_feas;

  const size_t batch_size;
  size_t sample_idx;
  vector<Sample> batch_samples;

  std::map<FMParamUnit *, ParamNode> batch_params;
};
