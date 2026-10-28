#ifndef FSPOLYCELL_H
#define FSPOLYCELL_H

#include "FSClipping/FSNodeRegistry.h"
#include "FSClipping/FSCellClipped.h"
#include <FSConfig.h>

_FS_BEGIN_NAMESPACE

struct FSPolyCell {
  std::vector<std::vector<NodeId> > faces;
};

FSPolyCell BuildClippedCell(const FSCellSource& src, FS_intT cell, const FSCellClipped& matches,
                            const FSNodeRegistry& nodes);

FSVec3 ComputeCellCenter(const FSCellSource& src, FS_intT cell, const FSNodeRegistry& nodes);

FSVec3 ComputeFaceCenter(std::span<const NodeId> nodesId, const FSNodeRegistry& nodes);

FSVec3 ComputeFaceNormal(std::span<const NodeId> coords, const FSNodeRegistry& nodes);

_FS_END_NAMESPACE

#endif