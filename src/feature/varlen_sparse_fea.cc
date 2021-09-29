/**
 *  Copyright (c) 2021 by exFM Contributors
 */
#include "feature/varlen_sparse_fea.h"
#include "solver/solver_factory.h"

VarlenSparseFeaConfig::VarlenSparseFeaConfig() { pooling_type = "sum"; }

VarlenSparseFeaConfig::~VarlenSparseFeaConfig() {}

VarlenSparseFeaContext::VarlenSparseFeaContext(const VarlenSparseFeaConfig &cfg)
    : cfg_(cfg) {
  fea_ids.reserve(cfg_.max_len);
}

VarlenSparseFeaContext::~VarlenSparseFeaContext() {}

int VarlenSparseFeaConfig::initParams(map<string, shared_ptr<ParamContainerInterface>> & shared_param_container_map) {
  if (pooling_type == "sum") {
    pooling_type_id = SeqPoolTypeSUM;
  } else if (pooling_type == "avg") {
    pooling_type_id = SeqPoolTypeAVG;
  } else {
    std::cerr << "Not supported.  use sum pooling." << endl;
    pooling_type_id = SeqPoolTypeSUM;
  }

  sparse_cfg.initParams(shared_param_container_map);
  // 保存model会用到。直接引用内部sparse_cfg的param_container
  param_container = sparse_cfg.param_container;

  return 0;
}

void to_json(json &j, const VarlenSparseFeaConfig &p) {
  j = json{{"name", p.name},
           {"max_id", p.sparse_cfg.max_id},
           {"vocab_size", p.sparse_cfg.vocab_size},
           {"mapping_dict_name", p.sparse_cfg.mapping_dict_name},
           {"use_id_mapping", p.sparse_cfg.use_id_mapping},
           {"use_hash", p.sparse_cfg.use_hash},
           {"default_id", p.sparse_cfg.default_id},
           {"max_len", p.max_len},
           {"shared_embedding_name", p.sparse_cfg.shared_embedding_name},
           {"pooling_type", p.pooling_type}};
}

void from_json(const json &j, VarlenSparseFeaConfig &p) {
  j.at("name").get_to(p.name);
  j.at("name").get_to(p.sparse_cfg.name);
  j.at("vocab_size").get_to(p.sparse_cfg.vocab_size);
  j.at("max_len").get_to(p.max_len);
  if (j.find("max_id") != j.end())                       j.at("max_id").get_to(p.sparse_cfg.max_id);
  if (j.find("mapping_dict_name") != j.end())         j.at("mapping_dict_name").get_to(p.sparse_cfg.mapping_dict_name);
  if (j.find("use_hash") != j.end())                     j.at("use_hash").get_to(p.sparse_cfg.use_hash);
  if (j.find("use_id_mapping") != j.end())               j.at("use_id_mapping").get_to(p.sparse_cfg.use_id_mapping);
  if (j.find("default_id") != j.end())                j.at("default_id").get_to(p.sparse_cfg.default_id);
  if (j.find("pooling_type") != j.end())                 j.at("pooling_type").get_to(p.pooling_type);
  if (j.find("shared_embedding_name") != j.end())        j.at("shared_embedding_name").get_to(p.sparse_cfg.shared_embedding_name);
}

int VarlenSparseFeaContext::feedSample(const char *line,
                                        vector<ParamContext> &forward_params,
                                        vector<ParamContext> &backward_params) {
  cfg_.parseFeaIdList(line, orig_fea_ids);
  if (orig_fea_ids.size() > cfg_.max_len) {
    orig_fea_ids.resize(cfg_.max_len);
  }

  fea_params.clear();
  if (cfg_.sparse_cfg.use_id_mapping == 0) {
    fea_ids = orig_fea_ids;
  } else {
    fea_ids.clear();
    for (auto orig_fea_id : orig_fea_ids) {
      feaid_t mapped_id = cfg_.sparse_cfg.fea_id_mapping.get(orig_fea_id);
      fea_ids.push_back(mapped_id);
    }
  }
  if (!valid()) {
    return -1;
  }

  DEBUG_OUT << "feedSample " << cfg_.name << " orig_fea_ids " << orig_fea_ids << " fea_ids " << fea_ids << endl;

  FMParamUnit *forward_param = forward_param_container->get();
  forward_param->clear();

  for (auto id : fea_ids) {
    FMParamUnit *fea_param = cfg_.sparse_cfg.param_container->get(id);
    Mutex_t *param_mutex = cfg_.sparse_cfg.param_container->GetMutexByFeaID(id);

    param_mutex->lock();
    *forward_param += *fea_param;
    param_mutex->unlock();
    
    fea_params.push_back(fea_param);
    backward_params.push_back(ParamContext((ParamContainerInterface*)cfg_.sparse_cfg.param_container.get(), fea_param, param_mutex, 1.0));
  }

  forward_params.push_back(ParamContext((ParamContainerInterface*)cfg_.sparse_cfg.param_container.get(), forward_param, NULL, 1.0));

  return 0;
}

void VarlenSparseFeaContext::forward(vector<ParamContext> &forward_params) {}

void VarlenSparseFeaContext::backward() {
}
