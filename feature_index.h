//
//  CRF++ -- Yet Another CRF toolkit
//
//  $Id: feature_index.h 1588 2007-02-12 09:03:39Z taku $;
//
//  Copyright(C) 2005-2007 Taku Kudo <taku@chasen.org>
//
#ifndef CRFPP_FEATURE_INDEX_H_
#define CRFPP_FEATURE_INDEX_H_

#include <vector>
#include <map>
#include <iostream>
#include "common.h"
#include "scoped_ptr.h"
#include "feature_cache.h"
#include "path.h"
#include "node.h"
#include "freelist.h"
#include "mmap.h"
#include "darts.h"

namespace CRFPP {
class TaggerImpl;

class Allocator {
    // allocator掌握着全部的cache、node和path
    // 并且根据线程id将这些信息均分给每一个线程crf模块，也就是CRFEncoderThread
    // 每个crf线程模块再将所拥有的每一个token分配给对应的tagger
 public:
  explicit Allocator(size_t thread_num);
  Allocator();
  virtual ~Allocator();

  char *strdup(const char *str);        // 复制一个新的字符串
  Path *newPath(size_t thread_id);      // 根据线程id创建新的path
  Node *newNode(size_t thread_id);      // 根据线程id创建新的node
  void clear();                         // 清除内存
  void clear_freelist(size_t thread_id);// 清除某个线程的内存
  FeatureCache *feature_cache() const;  // 特征cache
  size_t thread_num() const;            // 线程数量

 private:
  void init();

  size_t                       thread_num_;
  scoped_ptr<FeatureCache>     feature_cache_;
  scoped_ptr<FreeList<char> >  char_freelist_;
  scoped_array< FreeList<Path> > path_freelist_;
  scoped_array< FreeList<Node> > node_freelist_;
};

class FeatureIndex {        // 维护权重、模板等信息，计算cost
 public:
  static const unsigned int version = MODEL_VERSION;

  size_t size() const  { return maxid_; }
  size_t xsize() const { return xsize_; }
  size_t ysize() const { return y_.size(); }
  const char* y(size_t i) const { return y_[i].c_str(); }
  void   set_alpha(const double *alpha) { alpha_ = alpha; }
  const float *alpha_float() { return alpha_float_; }
  const double *alpha() const { return alpha_; }
  void set_cost_factor(double cost_factor) { cost_factor_ = cost_factor; }
  double cost_factor() const { return cost_factor_; }

  void calcCost(Node *node) const;      // 计算每个边和节点的cost，用于最短路算法的节点权值。
  void calcCost(Path *path) const;      // 在预测阶段可以根据边和点的cost求得最优预测序列

  bool buildFeatures(TaggerImpl *tagger) const;     // 根据模板抽取特征并放到cache中
  void rebuildFeatures(TaggerImpl *tagger) const;

  const char* what() { return what_.str(); }

  explicit FeatureIndex(): maxid_(0), alpha_(0), alpha_float_(0),
                           cost_factor_(1.0), xsize_(0),
                           check_max_xsize_(false), max_xsize_(0) {}
  virtual ~FeatureIndex() {}

  const char *getTemplate() const;

 protected:
  virtual int getID(const char *str) const = 0;
  const char *getIndex(const char *&p,
                       size_t pos,
                       const TaggerImpl &tagger) const;
  bool applyRule(string_buffer *os,
                 const char *pattern,
                 size_t pos, const TaggerImpl &tagger) const;

  mutable unsigned int      maxid_;         // 最大的特征id
  const double             *alpha_;         // 权重数组w
  const float              *alpha_float_;   // float类型的w
  double                    cost_factor_;   // cost的折扣因子，默认为1
  unsigned int              xsize_;         // 每行训练列数，输入+特征数
  bool check_max_xsize_;
  mutable unsigned int      max_xsize_;     // 每行训练数据最大列数
  std::vector<std::string>  unigram_templs_;// unigram模板
  std::vector<std::string>  bigram_templs_; // bigram模板
  std::vector<std::string>  y_;             // 输出
  std::string               templs_;        // unigram和bigram组合为字符串的模板
  whatlog                   what_;          // 日志
};

class EncoderFeatureIndex: public FeatureIndex {
 public:
  bool open(const char *template_filename,
            const char *model_filename);
  bool save(const char *filename, bool emit_textmodelfile);
  bool convert(const char *text_filename,
               const char *binary_filename);
  void shrink(size_t freq, Allocator *allocator);

 private:
  int getID(const char *str) const;
  bool openTemplate(const char *filename);
  bool openTagSet(const char *filename);
  mutable std::map<std::string, std::pair<int, unsigned int> > dic_;
};

class DecoderFeatureIndex: public FeatureIndex {
 public:
  bool open(const char *model_filename);
  bool openFromArray(const char *buf, size_t size);

 private:
  Mmap <char> mmap_;
  Darts::DoubleArray da_;
  int getID(const char *str) const;
};
}
#endif
