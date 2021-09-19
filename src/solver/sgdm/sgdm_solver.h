/**
 *  Copyright (c) 2021 by exFM Contributors
 */
#pragma once
#include "feature/fea_manager.h"
#include "solver/sgdm/sgdm_param.h"
#include "solver/base_solver.h"
#include "train/train_opt.h"

class SgdmSolver : public BaseSolver {
 public:
  SgdmSolver(const FeaManager &fea_manager)
      : BaseSolver(fea_manager),
        lr(train_opt.sgdm.lr),
        beta1(train_opt.sgdm.beta1),
        l2_reg_w(train_opt.sgdm.l2_reg_w),
        l2_reg_V(train_opt.sgdm.l2_reg_V),
        l1_reg_w(train_opt.sgdm.l1_reg_w),
        l1_reg_V(train_opt.sgdm.l1_reg_V) 
        {}
  virtual ~SgdmSolver() {}

  virtual void update(real_t grad) {
    if (y == 1) grad *= 7.2816; // TODO 正负样本loss均衡。暂时写死

    for (auto param_context : backward_params) {
      SgdmParamUnit *backward_param = (SgdmParamUnit *)param_context.param;
      param_context.mutex->lock();
      real_t xi = param_context.x;
      grad *= xi; //暂时都是离散特征，不支持连续值特征，所以此处关闭

      real_t & w = backward_param->fm_param.w;
      real_t & wm = backward_param->momentum.w;

      wm = beta1 * wm + (1-beta1) * grad;
      w -= lr * (wm  + w * l2_reg_w);

      for (int f = 0; f < DIM; ++f) {
        real_t &vf = backward_param->fm_param.V[f];
        real_t & vmf = backward_param->momentum.V[f];

        real_t vgf = grad * (sum[f]  - vf * xi );

        vmf = beta1 * vmf + (1-beta1) * vgf;

        vf -= lr * (vmf + vf * l2_reg_V);
      }

      param_context.mutex->unlock();
    }
  }

  const real_t l1_reg_w;
  const real_t l1_reg_V;
  const real_t l2_reg_w;
  const real_t l2_reg_V;
  const real_t lr;
  const real_t beta1;
};