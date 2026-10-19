#ifndef FSCELLSOURCE_H
#define FSCELLSOURCE_H


#include <FSCellPool.h>
#include <FSMesh.h>
#include <FSMeshPolyFaceStorage.h>

_FS_BEGIN_NAMESPACE

class FSCellSource
{
public:
  FSCellSource(FSMeshEnums::CellType t, const FSCellPool& cellPool)
    : cellType_(t), cellPool_(cellPool), isPoly3D_(t == FSMeshEnums::CT_Poly3D),
      isPoly2D_((t == FSMeshEnums::CT_Poly2D)){};

  FSMeshEnums::CellType Type() const noexcept { return cellType_; }
  bool IsPoly2D() const noexcept { return isPoly2D_; }
  bool IsPoly3D() const noexcept { return isPoly3D_; }

  FS_intT NNodes(FS_intT c) const;
  FS_intT Node(FS_intT c, FS_intT n) const;

  FS_intT NCorners(FS_intT c) const;

  std::vector<FS_intT> Row(FS_intT c) const;
  FS_intT NFaces(FS_intT c) const;
  FS_intT NFaceCorners(FS_intT c, FS_intT f) const;
  FS_intT FaceCorner(FS_intT c, FS_intT f, FS_intT k) const;
  const FSMeshPolyFaceStorage* PolyFaces() const;

  template<class F> void ForEachOwnedCell(F&& f) const
  {
    FS_intT offSet = cellPool_.GetOffset();
    FS_intT nCells = cellPool_.GetNOwnedCells();

    for(FS_intT c = offSet; c < offSet + nCells; c++) {
      f(c);
    }
  }

private:
  FSMeshEnums::CellType cellType_;
  const FSCellPool& cellPool_;
  bool isPoly3D_ = false;
  bool isPoly2D_ = false;
};

std::vector<FSCellSource> BuildCellSources(const FSMesh& mesh);

_FS_END_NAMESPACE

#endif