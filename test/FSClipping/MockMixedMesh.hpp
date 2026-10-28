// test/FSClipping/MockMixedMesh.hpp

#ifndef MOCKMIXEDMESH_HPP
#define MOCKMIXEDMESH_HPP

#include <FSMesh.h>
#include <FSMeshData.h>
#include <FSMeshPolyFaceStorage.h>
#include <FSVec3.h>
#include <vector>

#include "FSClipping/FSFaceMatcher.h"

_FS_BEGIN_NAMESPACE

// Conforming mixed mesh on one proc: unit hexa, a Poly3D pyramid on its top face, a tetra on a pyramid side,
// plus a Quad4 (hexa bottom), a Tri3 (tetra side) and a Poly2D (hexa face x = 0).
namespace mock {

using Rows = std::vector<std::vector<FS_intT> >;

inline const std::vector<FSVec3> kCoords = {
  {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}, {0.5, 0.5, 1.5}, {1.5, 0.5, 1.5}};

inline const std::vector<FS_intT> kHexa = {0, 1, 2, 3, 4, 5, 6, 7};
inline const std::vector<FS_intT> kPyramid = {4, 5, 6, 7, 8};
inline const Rows kPyramidFaces = {{0, 3, 2, 1}, {0, 1, 4}, {1, 2, 4}, {2, 3, 4}, {3, 0, 4}}; // local positions
inline const std::vector<FS_intT> kTetra = {5, 6, 8, 9};
inline const std::vector<FS_intT> kQuad = {0, 3, 2, 1};
inline const std::vector<FS_intT> kTri = {5, 9, 6};
inline const std::vector<FS_intT> kPoly2D = {0, 4, 7, 3};

inline FSIntArrayT FixedRows(const Rows& rows, FS_intT nNodes)
{
  FSIntArrayT a(FS_intT(rows.size()), nNodes);
  for(FS_intT c = 0; c < FS_intT(rows.size()); c++)
    for(FS_intT n = 0; n < nNodes; n++)
      a(c, n) = rows[c][n];
  return a;
}

inline FSIntRegisterT RaggedRows(const Rows& rows)
{
  FSIntRegisterT r;
  r.Init(FS_intT(rows.size()));
  for(FS_intT c = 0; c < FS_intT(rows.size()); c++)
    r.Count(c, FS_intT(rows[c].size()));
  r.Prepare();
  for(FS_intT c = 0; c < FS_intT(rows.size()); c++)
    for(FS_intT n : rows[c])
      r.Add(c, n);
  return r;
}

inline FSMesh BuildMixedMesh(FSClac& clac)
{
  FSMesh mesh(&clac);
  mesh.BeginInitialization();
  FSMeshData& md = *mesh.GetMeshData();
  FSUnstructMeshData& um = md.GetUnstructCells();

  const FS_intT nNodes = FS_intT(kCoords.size());
  FSIntArrayT node2Proc(FSCLAC_NPROCS(&clac) + 1);
  node2Proc.Fill(nNodes);
  node2Proc(0) = 0;
  md.InitUnstructNodes(node2Proc);

  FSIntArrayT hexa = FixedRows({kHexa}, 8);
  FSIntArrayT tetra = FixedRows({kTetra}, 4);
  FSIntArrayT quad = FixedRows({kQuad}, 4);
  FSIntArrayT tri = FixedRows({kTri}, 3);
  md.InitUnstructCells(FSMeshEnums::CT_Hexa8, hexa);
  md.InitUnstructCells(FSMeshEnums::CT_Tetra4, tetra);
  md.InitUnstructCells(FSMeshEnums::CT_Quad4, quad);
  md.InitUnstructCells(FSMeshEnums::CT_Tri3, tri);

  FSIntRegisterT poly2D = RaggedRows({kPoly2D});
  md.InitUnstructCells(FSMeshEnums::CT_Poly2D, poly2D);

  FSIntRegisterT poly3D = RaggedRows({kPyramid});
  FSMeshPolyFaceStorage polyFaces;
  polyFaces.Init(1);
  polyFaces.SetNumFacesForPoly(0, FS_intT(kPyramidFaces.size()));
  polyFaces.EndNumFacesSetup();
  for(FS_intT f = 0; f < FS_intT(kPyramidFaces.size()); f++)
    polyFaces.SetNumNodesForFace(0, f, FS_intT(kPyramidFaces[f].size()));
  polyFaces.EndNumNodesSetup();
  for(FS_intT f = 0; f < FS_intT(kPyramidFaces.size()); f++)
    for(FS_intT n : kPyramidFaces[f])
      polyFaces.AddFaceNode(0, f, n);
  md.InitUnstructCells(FSMeshEnums::CT_Poly3D, poly3D);
  md.InitUnstructCellFaces(FSMeshEnums::CT_Poly3D, polyFaces);

  FSQuantityDescArrayT coordsDesc(3);
  coordsDesc[0] = FSQuantityDesc(FSDataName::Coordinates(), FSDataName::Coordinate().X());
  coordsDesc[1] = FSQuantityDesc(FSDataName::Coordinates(), FSDataName::Coordinate().Y());
  coordsDesc[2] = FSQuantityDesc(FSDataName::Coordinates(), FSDataName::Coordinate().Z());
  FSFloatArrayT coords(nNodes, 3);
  for(FS_intT n = 0; n < nNodes; n++)
    for(FS_intT d = 0; d < 3; d++)
      coords(n, d) = kCoords[n][d];
  if(!um.SetCoordinates3D(coordsDesc, coords))
    FSError.Print();

  FS_intT offset = 0;
  const FSIntArrayT cellTypes = um.GetCellTypesArray();
  for(FSIntArrayT::ConstIterator t = cellTypes.BeginConst(); t.IsValid(); t.Next()) {
    um.InitGlobalCellNumber(FSMeshEnums::CellType(*t), offset);
    offset += um.GetNCells(FSMeshEnums::CellType(*t));
  }

  mesh.EndInitialization();
  um.CreateLocalNumbering();
  return mesh;
}

inline FS_intT Offset(const FSMesh& mesh, FSMeshEnums::CellType t)
{
  return mesh.GetMeshData()->GetUnstructCells().GetCellPool(t)->GetOffset();
}

inline FSFaceMatch MakeMatch(FS_intT owner, FSMeshEnums::CellType ownerType, std::vector<FSVec3> poly,
                             std::vector<FS_intT> globalIds, FSFaceMatch::MatchType type = FSFaceMatch::INTERSECTING)
{
  FSFaceMatch m;
  m.type = type;
  m.elemOwner1 = owner;
  m.elemOwnerType1 = ownerType;
  m.clippedPoly3D = std::move(poly);
  m.nodeGlobalIds = std::move(globalIds);
  return m;
}

} // namespace mock

_FS_END_NAMESPACE

#endif
