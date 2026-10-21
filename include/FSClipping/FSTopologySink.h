#ifndef FSTOPOLOGYSINK_H
#define FSTOPOLOGYSINK_H

#include "FSClipping/FSNodeRegistry.h"
#include "FSClipping/FSCellClipped.h"
#include "FSClipping/FSPolyCell.h"
#include "FSClipping/FSTopologyAssembler.h"

_FS_BEGIN_NAMESPACE

struct FSFixedrow_s {
  FS_intT nPerrow_;
  std::vector<FS_intT> values;
  // FSIntArrayT Build(const FSRenumber&) const;
};
struct FSRaggedrow_s {
  std::vector<FS_intT> counts, values;
  // FSIntRegisterT Build(const FSRenumber&) const;
};
struct FSPolyFacerow_s {
  std::vector<FS_intT> nFaces, nFaceNodes, nodes;
  void Build(FSMeshPolyFaceStorage&) const;
};
struct FSParentrow_s {
  std::vector<FS_intT> parent, parentType;
};

class FSTopologySink
{
public:
  explicit FSTopologySink(const FSNodeRegistry& nodes) : nodes_(nodes), used_(nodes.Size(), false){};

  void AddStandard(const FSCellSource& src, FS_intT cell); // inner_[type] + parents_[type]
  void AddPoly2D(FS_intT parent, FSMeshEnums::CellType parentType,
                 std::span<const FS_intT> row); // poly2D_ + parents_[Poly2D]
  void AddPoly3D(FS_intT parent, FSMeshEnums::CellType parentType,
                 const FSPolyCell&);                     // poly3D_ + faces_ + parents_[Poly3D]
  void AddPoly3D(const FSCellSource& src, FS_intT cell); // Poly3D d'origine : ligne et faces recopiées

  FSTopologyData Finalize() &&;

private:
  template<class NFaceNodes, class FaceNode>
  void AppendPolyhedron(FS_intT nFaces, NFaceNodes&& nFaceNodes, FaceNode&& faceNode)
  {
    faces_.nFaces.push_back(nFaces);
    for(FS_intT f = 0; f < nFaces; ++f) {
      const FS_intT nk = nFaceNodes(f);
      faces_.nFaceNodes.push_back(nk);
      for(FS_intT k = 0; k < nk; ++k) {
        const FS_intT node = faceNode(f, k);
        // we look if the nodes was not already added
        auto it = std::find(row_.begin(), row_.end(), node);
        if(it == row_.end()) {
          row_.push_back(node);
          it = row_.end() - 1;
        }
        // we add the local index inter cell for polyFace
        faces_.nodes.push_back(static_cast<FS_intT>(it - row_.begin()));
      }
    }
    for(FS_intT n : row_)
      used_[n] = true;
    poly3D_.counts.push_back(static_cast<FS_intT>(row_.size()));
    poly3D_.values.insert(poly3D_.values.end(), row_.begin(), row_.end());
    row_.clear();
  }

  const FSNodeRegistry& nodes_;
  std::vector<bool> used_;                                 // globalCoords, nodeGlobalNumbers, FSRenumber
  std::map<FSMeshEnums::CellType, FSFixedrow_s> inner_;    // cell2NodeInner
  FSRaggedrow_s poly2D_, poly3D_;                          // cell2NodePoly2D, cell2NodePoly3D
  FSPolyFacerow_s faces_;                                  // polyFaces
  std::map<FSMeshEnums::CellType, FSParentrow_s> parents_; // cellParent, cellParentType
  std::vector<FS_intT> row_;                               // current row of a cell
};

_FS_END_NAMESPACE

#endif