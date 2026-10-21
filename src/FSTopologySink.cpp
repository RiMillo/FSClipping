#include "FSClipping/FSTopologySink.h"

void FSTopologySink::AddStandard(const FSCellSource& src, FS_intT cell)
{
  // 1 - Add the cells to inner_
  const auto type = src.Type();
  const auto row = src.Row(cell);

  auto [it, inserted] = inner_.try_emplace(type);
  FSFixedrow_s& rows = it->second;
  if(inserted)
    rows.nPerrow_ = src.NNodes(cell);
  assert(rows.nPerrow_ == static_cast<FS_intT>(row.size()));

  for(FS_intT n : row) {
    rows.values.push_back(n);
    used_[n] = true;
  }

  // 2 - complete the parents_[type]
  auto& p = parents_[type];
  p.parent.push_back(cell);
  p.parentType.push_back(type);
}

void FSTopologySink::AddPoly3D(const FSCellSource& src, FS_intT cell) // Poly3D d'origine, recopie
{
  const auto row = src.Row(cell);
  row_.assign(row.begin(), row.end());
  const std::size_t nRow = row_.size();

  AppendPolyhedron(
    src.NFaces(cell), [&](FS_intT f) { return src.NFaceCorners(cell, f); },
    [&](FS_intT f, FS_intT k) { return src.FaceCorner(cell, f, k); });
  assert(poly3D_.counts.back() == static_cast<FS_intT>(nRow));

  auto& p = parents_[FSMeshEnums::CT_Poly3D];
  p.parent.push_back(cell);
  p.parentType.push_back(FSMeshEnums::CT_Poly3D);
}

void FSTopologySink::AddPoly3D(FS_intT parent, FSMeshEnums::CellType parentType, const FSPolyCell& poly)
{
  AppendPolyhedron(
    static_cast<FS_intT>(poly.faces.size()), [&](FS_intT f) { return static_cast<FS_intT>(poly.faces[f].size()); },
    [&](FS_intT f, FS_intT k) { return static_cast<FS_intT>(poly.faces[f][k]); });

  auto& p = parents_[FSMeshEnums::CT_Poly3D];
  p.parent.push_back(parent);
  p.parentType.push_back(parentType);
}

void FSTopologySink::AddPoly2D(FS_intT parent, FSMeshEnums::CellType parentType,
                               std::span<const FS_intT> row /*, FS_intT globalNumber */)
{
  poly2D_.counts.push_back(static_cast<FS_intT>(row.size()));
  for(FS_intT n : row) {
    poly2D_.values.push_back(n);
    used_[n] = true;
  }

  auto& p = parents_[FSMeshEnums::CT_Poly2D];
  p.parent.push_back(parent);
  p.parentType.push_back(parentType);
  // poly2DGlobalNumbers_.push_back(globalNumber);  // cf. point 7
}
