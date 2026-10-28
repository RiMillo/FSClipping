#ifndef FSNODEKEY_H
#define FSNODEKEY_H

#include <FSTypes.h>
#include <FSVec3.h>
#include <cmath>
#include <functional>

_FS_BEGIN_NAMESPACE

// Point quantized on a grid of step tol, used to merge coincident nodes
struct NodeKey {
  FS_intT ix, iy, iz;

  NodeKey(const FSVec3& p, const FS_floatT tol)
    : ix(static_cast<FS_intT>(std::llround(p[0] / tol))), iy(static_cast<FS_intT>(std::llround(p[1] / tol))),
      iz(static_cast<FS_intT>(std::llround(p[2] / tol)))
  {
  }
  bool operator==(const NodeKey&) const = default;
};

// std::hash specialization so NodeKey can be an unordered_map key
namespace std {
template<> struct hash<NodeKey> {
  size_t operator()(const NodeKey& k) const noexcept
  {
    size_t seed = std::hash<FS_intT>{}(k.ix);
    seed ^= std::hash<FS_intT>{}(k.iy) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<FS_intT>{}(k.iz) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};
} // namespace std

_FS_END_NAMESPACE

#endif
