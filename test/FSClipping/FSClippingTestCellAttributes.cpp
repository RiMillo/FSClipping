// CopyCellAttributes with Poly cells whose parents have different types (tri + quad boundary, hexa + tetra volume)

#include "gtest/gtest.h"
#include <vector>

#include "FSClipping/FSMeshReconstruction.h"
#include "MockMixedMesh.hpp"

_FS_BEGIN_NAMESPACE

namespace {

const FSString kCADID = "CADID";
const FSString kQuadOnly = "QuadOnly";
const FSString kCADGroupID = FSEnums::AttributeTypeToString(FSEnums::AT_CADGroupID);

void AddAttribute(FSMesh& mesh, const FSString& name, FSMeshEnums::CellType t, const std::vector<FS_intT>& v)
{
  FSIntArrayT values(FS_intT(v.size()));
  for(FS_intT i = 0; i < FS_intT(v.size()); i++)
    values[i] = v[i];
  mesh.GetMeshData()->GetUnstructCells().InitCellAttribute(name, t, values);
}

// Minimal output mesh: only the Poly2D / Poly3D pool sizes matter, not the geometry
FSMesh BuildTarget(FSClac& clac, FS_intT nPoly2D, FS_intT nPoly3D)
{
  FSMesh mesh(&clac);
  mesh.BeginInitialization();
  FSMeshData& md = *mesh.GetMeshData();

  const FS_intT nNodes = FS_intT(mock::kCoords.size());
  FSIntArrayT node2Proc(FSCLAC_NPROCS(&clac) + 1);
  node2Proc.Fill(nNodes);
  node2Proc(0) = 0;
  md.InitUnstructNodes(node2Proc);

  FSIntRegisterT poly2D = mock::RaggedRows(mock::Rows(nPoly2D, mock::kPoly2D));
  md.InitUnstructCells(FSMeshEnums::CT_Poly2D, poly2D);

  FSIntRegisterT poly3D = mock::RaggedRows(mock::Rows(nPoly3D, mock::kPyramid));
  FSMeshPolyFaceStorage polyFaces;
  polyFaces.Init(nPoly3D);
  const FS_intT nFaces = FS_intT(mock::kPyramidFaces.size());
  for(FS_intT c = 0; c < nPoly3D; c++)
    polyFaces.SetNumFacesForPoly(c, nFaces);
  polyFaces.EndNumFacesSetup();
  for(FS_intT c = 0; c < nPoly3D; c++)
    for(FS_intT f = 0; f < nFaces; f++)
      polyFaces.SetNumNodesForFace(c, f, FS_intT(mock::kPyramidFaces[f].size()));
  polyFaces.EndNumNodesSetup();
  for(FS_intT c = 0; c < nPoly3D; c++)
    for(FS_intT f = 0; f < nFaces; f++)
      for(FS_intT n : mock::kPyramidFaces[f])
        polyFaces.AddFaceNode(c, f, n);
  md.InitUnstructCells(FSMeshEnums::CT_Poly3D, poly3D);
  md.InitUnstructCellFaces(FSMeshEnums::CT_Poly3D, polyFaces);

  mesh.EndInitialization();
  return mesh;
}

FSIntArrayT ToArray(const std::vector<FS_intT>& v)
{
  FSIntArrayT a(FS_intT(v.size()));
  for(FS_intT i = 0; i < FS_intT(v.size()); i++)
    a[i] = v[i];
  return a;
}

std::vector<FS_intT> Values(const FSUnstructMeshData& md, const FSString& name, FSMeshEnums::CellType t)
{
  std::vector<FS_intT> v;
  const FSIntArrayT& a = md.GetCellAttribute(name, t);
  for(FS_intT i = 0; i < a.Size(); i++)
    v.push_back(a[i]);
  return v;
}

class FSCopyCellAttributesTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    AddAttribute(source, kCADID, FSMeshEnums::CT_Tri3, {11});
    AddAttribute(source, kCADID, FSMeshEnums::CT_Quad4, {22});
    AddAttribute(source, kCADID, FSMeshEnums::CT_Hexa8, {100});
    AddAttribute(source, kCADID, FSMeshEnums::CT_Tetra4, {200});
    AddAttribute(source, kQuadOnly, FSMeshEnums::CT_Quad4, {7});
    AddAttribute(source, kCADGroupID, FSMeshEnums::CT_Tri3, {3});
    AddAttribute(source, kCADGroupID, FSMeshEnums::CT_Quad4, {4});
  }

  // One output Poly type, cell i has parent parents[i] (pool offset of parentTypes[i])
  FSTopologyData Topology(FSMeshEnums::CellType polyType, const std::vector<FSMeshEnums::CellType>& parentTypes)
  {
    std::vector<FS_intT> parents, types;
    for(auto t : parentTypes) {
      parents.push_back(mock::Offset(source, t));
      types.push_back(FS_intT(t));
    }
    FSTopologyData topo;
    topo.cellParent[polyType] = ToArray(parents);
    topo.cellParentType[polyType] = ToArray(types);
    return topo;
  }

  void Copy(const FSTopologyData& topo, FSMesh& target)
  {
    FSMeshReconstruction(clac).CopyCellAttributes(source.GetMeshData()->GetUnstructCells(), topo,
                                                  target.GetMeshData()->GetUnstructCells());
  }

  FSClac clac;
  FSMesh source = mock::BuildMixedMesh(clac);
};

} // namespace

// Control case: a single parent type already works
TEST_F(FSCopyCellAttributesTest, SingleParentTypeIsUnchanged)
{
  FSMesh target = BuildTarget(clac, 2, 0);
  Copy(Topology(FSMeshEnums::CT_Poly2D, {FSMeshEnums::CT_Quad4, FSMeshEnums::CT_Quad4}), target);

  const auto& md = target.GetMeshData()->GetUnstructCells();
  EXPECT_EQ(Values(md, kCADID, FSMeshEnums::CT_Poly2D), (std::vector<FS_intT>{22, 22}));
}

// Each Poly2D must read its own parent pool, not the pool of the first parent type
TEST_F(FSCopyCellAttributesTest, Poly2DWithTri3AndQuad4ParentsCopiesEachParentValue)
{
  FSMesh target = BuildTarget(clac, 2, 0);
  Copy(Topology(FSMeshEnums::CT_Poly2D, {FSMeshEnums::CT_Tri3, FSMeshEnums::CT_Quad4}), target);

  const auto& md = target.GetMeshData()->GetUnstructCells();
  EXPECT_EQ(Values(md, kCADID, FSMeshEnums::CT_Poly2D), (std::vector<FS_intT>{11, 22}));
}

// An attribute carried by only one parent type must still be copied, -1 elsewhere
TEST_F(FSCopyCellAttributesTest, Poly2DAttributeOnOneParentTypeIsCopiedForThatType)
{
  FSMesh target = BuildTarget(clac, 2, 0);
  Copy(Topology(FSMeshEnums::CT_Poly2D, {FSMeshEnums::CT_Tri3, FSMeshEnums::CT_Quad4}), target);

  const auto& md = target.GetMeshData()->GetUnstructCells();
  ASSERT_TRUE(md.HasCellAttribute(kQuadOnly, FSMeshEnums::CT_Poly2D));
  EXPECT_EQ(Values(md, kQuadOnly, FSMeshEnums::CT_Poly2D), (std::vector<FS_intT>{-1, 7}));
}

// A carried CADGroupID wins over the parent value (parent surface cell owned by another proc)
TEST_F(FSCopyCellAttributesTest, CarriedMarkerOverridesParentValue)
{
  FSMesh target = BuildTarget(clac, 2, 0);
  FSTopologyData topo = Topology(FSMeshEnums::CT_Poly2D, {FSMeshEnums::CT_Tri3, FSMeshEnums::CT_Quad4});
  topo.poly2DMarkers = ToArray({9, 8});
  Copy(topo, target);

  const auto& md = target.GetMeshData()->GetUnstructCells();
  EXPECT_EQ(Values(md, kCADGroupID, FSMeshEnums::CT_Poly2D), (std::vector<FS_intT>{9, 8}));
}

// A marker of -1 means none was carried: the parent value is read, as for an original Poly2D
TEST_F(FSCopyCellAttributesTest, MissingCarriedMarkerFallsBackToParent)
{
  FSMesh target = BuildTarget(clac, 2, 0);
  FSTopologyData topo = Topology(FSMeshEnums::CT_Poly2D, {FSMeshEnums::CT_Tri3, FSMeshEnums::CT_Quad4});
  topo.poly2DMarkers = ToArray({-1, 8});
  Copy(topo, target);

  const auto& md = target.GetMeshData()->GetUnstructCells();
  EXPECT_EQ(Values(md, kCADGroupID, FSMeshEnums::CT_Poly2D), (std::vector<FS_intT>{3, 8}));
}

// A parent type without local pool (undefined, or absent from the mesh) gives -1 instead of crashing
TEST_F(FSCopyCellAttributesTest, ParentTypeWithoutPoolGivesMinusOne)
{
  FSMesh target = BuildTarget(clac, 3, 0);
  FSTopologyData topo;
  topo.cellParent[FSMeshEnums::CT_Poly2D] = ToArray({mock::Offset(source, FSMeshEnums::CT_Quad4), -1, 0});
  topo.cellParentType[FSMeshEnums::CT_Poly2D] =
    ToArray({FSMeshEnums::CT_Quad4, FSMeshEnums::CT_Undefined, FSMeshEnums::CT_Prism6});
  Copy(topo, target);

  const auto& md = target.GetMeshData()->GetUnstructCells();
  EXPECT_EQ(Values(md, kCADID, FSMeshEnums::CT_Poly2D), (std::vector<FS_intT>{22, -1, -1}));
}

// GlobalNumber is set by the reconstruction, never copied from the parent
TEST_F(FSCopyCellAttributesTest, GlobalNumberIsNotCopied)
{
  FSMesh target = BuildTarget(clac, 2, 0);
  Copy(Topology(FSMeshEnums::CT_Poly2D, {FSMeshEnums::CT_Tri3, FSMeshEnums::CT_Quad4}), target);

  const FSString globalNumberName = FSMeshEnums::AttributeTypeToString(FSMeshEnums::AT_GlobalNumber);
  ASSERT_TRUE(source.GetMeshData()->GetUnstructCells().HasCellAttribute(globalNumberName, FSMeshEnums::CT_Quad4));
  EXPECT_FALSE(target.GetMeshData()->GetUnstructCells().HasCellAttribute(globalNumberName, FSMeshEnums::CT_Poly2D));
}

// Same limitation for clipped volume cells of different types
TEST_F(FSCopyCellAttributesTest, Poly3DWithHexaAndTetraParentsCopiesEachParentValue)
{
  FSMesh target = BuildTarget(clac, 0, 2);
  Copy(Topology(FSMeshEnums::CT_Poly3D, {FSMeshEnums::CT_Hexa8, FSMeshEnums::CT_Tetra4}), target);

  const auto& md = target.GetMeshData()->GetUnstructCells();
  EXPECT_EQ(Values(md, kCADID, FSMeshEnums::CT_Poly3D), (std::vector<FS_intT>{100, 200}));
}

_FS_END_NAMESPACE
