#include "FSClipping/FSPolyCell.h"

_FS_BEGIN_NAMESPACE

FSPolyCell BuildClippedCell(const FSCellSource& src, FS_intT cell, const FSCellClipped& matches,
                            const FSNodeRegistry& nodes)
{
  FSPolyCell polyCell;
  FS_intT nFaces = src.NFaces(cell);
  std::vector<NodeId> nodeId;
  FSVec3 cellCenter = ComputeCellCenter(src, cell, nodes);

  // 1 - CellSourceFace loop
  for(FS_intT f = 0; f < nFaces; f++) {
    if(matches.IsBoundaryFace(cell, f))
      continue;
    FS_intT nNodes = src.NFaceCorners(cell, f);
    nodeId.reserve(nNodes);
    for(FS_intT n = 0; n < nNodes; n++) {
      nodeId.push_back(src.FaceCorner(cell, f, n));
    }
    polyCell.faces.push_back(nodeId);
    nodeId.clear();
  }

  // 2 - CellClippedFace loop and reorienting if necessary
  auto allMatches = matches.MatchesOf(cell);
  for(const auto& m : allMatches) {
    const auto polyNode = nodes.Polygon(m);
    nodeId.reserve(polyNode.size());
    for(const auto& n : polyNode) {
      nodeId.push_back(n);
    }

    polyCell.faces.push_back(nodeId);
    nodeId.clear();
  }

  for(auto& face : polyCell.faces) {
    const FSVec3 dir = ComputeFaceCenter(face, nodes) - cellCenter;
    if(ComputeFaceNormal(face, nodes).InnerProduct(dir) < 0.0)
      std::reverse(face.begin(), face.end());
  }

  return polyCell;
}

FSVec3 ComputeCellCenter(const FSCellSource& src, FS_intT cell, const FSNodeRegistry& nodes)
{
  FSVec3 cellCenter(0, 0, 0);
  const FS_intT numCorners = src.NCorners(cell);
  for(FS_intT corner = 0; corner < numCorners; ++corner) {
    NodeId nodeId = nodes.Old(src.Node(cell, corner)); // corners are always the first nodes
    cellCenter += nodes.Coord(nodeId);
  }
  cellCenter /= static_cast<FS_floatT>(numCorners);
  return cellCenter;
}

FSVec3 ComputeFaceCenter(std::span<const NodeId> nodesId, const FSNodeRegistry& nodes)
{
  FSVec3 faceCenter(0, 0, 0);
  for(const auto& n : nodesId) {
    faceCenter += nodes.Coord(n);
  }
  return faceCenter / nodesId.size();
}

FSVec3 ComputeFaceNormal(std::span<const NodeId> nodesId, const FSNodeRegistry& nodes)
{
  assert(nodesId.size() >= 3);
  FSVec3 normal(0.0, 0.0, 0.0);
  const FSVec3& p0 = nodes.Coord(nodesId[0]);

  for(size_t i = 1; i < nodesId.size() - 1; ++i) {
    const FSVec3& p1 = nodes.Coord(nodesId[i]);
    const FSVec3& p2 = nodes.Coord(nodesId[i + 1]);
    normal += (p1 - p0).CrossProduct(p2 - p0);
  }
  FS_floatT norm = normal.L2Norm();
  if(norm > 0.0)
    return normal /= norm;
  return normal;
}

_FS_END_NAMESPACE