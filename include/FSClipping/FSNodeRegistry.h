#ifndef FSNODEREGISTRY_H
#define FSNODEREGISTRY_H

#include "FSClipping/FSCellClipped.h"
#include "FSClipping/FSCellSource.h"
#include <FSVec3.h>

_FS_BEGIN_NAMESPACE
struct NodeKey {
  FS_intT ix, iy, iz;

  NodeKey(const FSVec3& p, const FS_floatT tol);
  bool operator==(const NodeKey&) const = default;
};

/*
std::hash specialization required for NodeKey to be used as an
unordered_map key. Without it, the default constructor of
unordered_map<NodeKey, ...> is implicitly deleted.
*/
namespace std {
template<> struct hash<NodeKey> {
  size_t operator()(const NodeKey& k) const noexcept
  {
    size_t h1 = std::hash<FS_intT>{}(k.ix);
    size_t h2 = std::hash<FS_intT>{}(k.iy);
    size_t h3 = std::hash<FS_intT>{}(k.iz);

    size_t seed = h1;
    seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};
} // namespace std

class FSNodeRegistry
{
public:
  FSNodeRegistry(FSMesh& mesh, const std::vector<FSCellSource>& sources, const FSCellClipped& matches, FS_floatT tol);

private:
  std::vector<FSVec3> globalCoords_;                   // deduplicated list of all node coordinates
  std::unordered_map<NodeKey, FS_intT> coordToNode_;   // coord → index in globalCoords_
  std::unordered_map<NodeKey, FS_intT> keyToGlobalId_; // coord → clipper-assigned global node ID

  FS_intT nNodeOld;
};

_FS_END_NAMESPACE
#endif