// FSBoundaryFaceProvider::Extract on the mixed mock: Quad4 + Poly2D on the hexa (marker 1), Tri3 on the tetra (marker 2)

#include "gtest/gtest.h"

#include "FSClipping/FSBoundaryFaceProvider.h"

#include "MockMixedMesh.hpp"

_FS_BEGIN_NAMESPACE

namespace {

const FSString kCADGroupID = FSEnums::AttributeTypeToString(FSEnums::AT_CADGroupID);

class FSClippingTestBoundaryFaceProvider : public ::testing::Test
{
protected:
  void SetUp() override
  {
    mock::AddAttribute(mesh, kCADGroupID, FSMeshEnums::CT_Quad4, {1});
    mock::AddAttribute(mesh, kCADGroupID, FSMeshEnums::CT_Poly2D, {1});
    mock::AddAttribute(mesh, kCADGroupID, FSMeshEnums::CT_Tri3, {2});
  }

  BoundaryExtraction Extract(FS_intT marker)
  {
    FSMeshFaceExtractor ex;
    return FSBoundaryFaceProvider::Extract(mesh, ex, marker);
  }

  FSClac clac;
  FSMesh mesh = mock::BuildMixedMesh(clac);
};

FS_intT NCells(const std::unordered_map<FS_intT, std::set<FS_intT> >& cells, FSMeshEnums::CellType t)
{
  auto it = cells.find(FS_intT(t));
  return it == cells.end() ? 0 : FS_intT(it->second.size());
}

} // namespace

TEST_F(FSClippingTestBoundaryFaceProvider, ComputeGeometry)
{
  for(FS_intT marker : {1, 2})
    for(const auto& f : Extract(marker).faces) {
      EXPECT_NEAR(f.tangentVector1().L2Norm(), 1.0, 1e-12);
      EXPECT_NEAR(f.normal().L2Norm(), 1.0, 1e-12);
    }
}

TEST_F(FSClippingTestBoundaryFaceProvider, QuadAndPoly2DOnHexa)
{
  BoundaryExtraction be = Extract(1);

  EXPECT_EQ(be.faces.size(), 2u);
  EXPECT_EQ(be.volumeCells.size(), 1u);
  EXPECT_EQ(NCells(be.volumeCells, FSMeshEnums::CT_Hexa8), 1);
  EXPECT_EQ(NCells(be.surfaceCells, FSMeshEnums::CT_Quad4), 1);
  EXPECT_EQ(NCells(be.surfaceCells, FSMeshEnums::CT_Poly2D), 1);
  ASSERT_EQ(be.cell2BoundaryFaces.size(), 1u);
  EXPECT_EQ(be.cell2BoundaryFaces.begin()->second.size(), 2u);
}

TEST_F(FSClippingTestBoundaryFaceProvider, TriOnTetra)
{
  BoundaryExtraction be = Extract(2);

  EXPECT_EQ(be.faces.size(), 1u);
  EXPECT_EQ(be.volumeCells.size(), 1u);
  EXPECT_EQ(NCells(be.volumeCells, FSMeshEnums::CT_Tetra4), 1);
  EXPECT_EQ(be.surfaceCells.size(), 1u);
  EXPECT_EQ(NCells(be.surfaceCells, FSMeshEnums::CT_Tri3), 1);
}

TEST_F(FSClippingTestBoundaryFaceProvider, UnknownMarkerGivesNothing)
{
  BoundaryExtraction be = Extract(3);

  EXPECT_TRUE(be.faces.empty());
  EXPECT_TRUE(be.volumeCells.empty());
  EXPECT_TRUE(be.surfaceCells.empty());
}

_FS_END_NAMESPACE
