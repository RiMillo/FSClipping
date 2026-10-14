#include "FSClipping/FSCellSource.h"
#include <FSMeshData.h>

_FS_BEGIN_NAMESPACE


FS_intT FSCellSource::NNodes(FS_intT c) const { return FSCellInfo::NNodes(cellType_, cellPool_, c); }

FS_intT FSCellSource::Node(FS_intT c, FS_intT n) const { return FSCellInfo::GetCellNode(cellType_, cellPool_, c, n); }

FS_intT FSCellSource::NFaces(FS_intT c) const { return FSCellInfo::NFaces(cellType_, cellPool_, c); }

FS_intT FSCellSource::NFaceCorners(FS_intT c, FS_intT f) const
{
  return FSCellInfo::NFaceCorners(cellType_, cellPool_, c, f);
};

FS_intT FSCellSource::FaceCorner(FS_intT c, FS_intT f, FS_intT k) const
{
  return FSCellInfo::GetCellFaceCorner(cellType_, cellPool_, c, f, k);
}

std::vector<FS_intT> FSCellSource::Row(FS_intT c) const
{
  FS_intT nNodes = NNodes(c);
  std::vector<FS_intT> nodes;
  nodes.reserve(nNodes);
  for(FS_intT n = 0; n < nNodes; n++) {
    nodes.push_back(Node(c, n));
  }
  return nodes;
}
const FSMeshPolyFaceStorage* FSCellSource::PolyFaces() const
{
  return (isPoly3D_) ? &(static_cast<const FSUnstructPolyCellPool*>(&cellPool_)->GetPolyFaceStorage()) : nullptr;
}

std::vector<FSCellSource> BuildCellSources(const FSMesh& mesh)
{
  std::vector<FSCellSource> cellSource;

  for(const auto& t : mesh.GetCellTypes()) {
    if(FSMeshEnums::IsUnstructVolumeCellType(t) || FSMeshEnums::IsUnstructSurfaceCellType(t)) {
      const auto& cellPool = mesh.GetMeshData()->GetUnstructCells().GetCellPool(t);
      if(cellPool)
        cellSource.emplace_back(t, *cellPool);
    }
  }

  return cellSource;
}
_FS_END_NAMESPACE