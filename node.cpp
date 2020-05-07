//
//  CRF++ -- Yet Another CRF toolkit
//
//  $Id: node.cpp 1595 2007-02-24 10:18:32Z taku $;
//
//  Copyright(C) 2005-2007 Taku Kudo <taku@chasen.org>
//
#include <stdlib.h>
#include <cmath>
#include "node.h"
#include "common.h"

namespace CRFPP {

void Node::calcAlpha() {        //前向概率
  alpha = 0.0;
  // paper中定义为：alpha (k, s) = sum(phi (s', s) * alpha (k-1, s')),即能成功转移的概率和
  // 而在log空间中变成：alpha (k, s) = sum (w * phi (s, x) + alpha (k-1, s'))
  // 前半部分即为cost
  for (const_Path_iterator it = lpath.begin(); it != lpath.end(); ++it) {
    alpha = logsumexp(alpha,
                      (*it)->cost + (*it)->lnode->alpha,
                      (it == lpath.begin()));
  }
  alpha += cost;    // 加上节点cost满足上述循环
}

void Node::calcBeta() {         //后向概率
  beta = 0.0;
  for (const_Path_iterator it = rpath.begin(); it != rpath.end(); ++it) {
    beta = logsumexp(beta,
                     (*it)->cost +(*it)->rnode->beta,
                     (it == rpath.begin()));
  }
  beta += cost;
}

void Node::calcExpectation(double *expected, double Z, size_t size) const {
  const double c = std::exp(alpha + beta - cost - Z);   //前后向概率都加了一个cost，所以减去一个
  for (const int *f = fvector; *f != -1; ++f) {         // 节点期望等于alpha*beta/Z
    expected[*f + y] += c;                              //对于当前特征中的当前y标签加上c
  }
  for (const_Path_iterator it = lpath.begin(); it != lpath.end(); ++it) {   // 边期望
    (*it)->calcExpectation(expected, Z, size);
  }
}
}
