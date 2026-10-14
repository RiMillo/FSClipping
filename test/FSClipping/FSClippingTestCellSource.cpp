#include "FSMeshImportParamsHDF5.h"
#include "gtest/gtest.h"
#include <FSMeshCheck.h>
#include <algorithm>
#include <map>
#include <set>

#include "FSClipping/FSCellSource.h"
#include "MockMixedMesh.hpp"
#include "TestUtils.hpp"

_FS_BEGIN_NAMESPACE

namespace {

class FSCellSourceTest : public ::testing::Test
{
protected:
  FSClac clac;
  FSMesh mesh = mock::BuildMixedMesh(clac);
  std::vector<FSCellSource> sources = BuildCellSources(mesh);

  const FSCellSource& Source(FSMeshEnums::CellType t) const
  {
    const auto it = std::find_if(sources.begin(), sources.end(), [t](const FSCellSource& s) { return s.Type() == t; });
    EXPECT_NE(it, sources.end()) << "no source for type " << t;
    return *it;
  }

  FS_intT First(FSMeshEnums::CellType t) const { return mock::Offset(mesh, t); }
};

std::vector<FS_intT> Expected(const std::vector<FS_intT>& row, const std::vector<FS_intT>& positions)
{
  std::vector<FS_intT> nodes;
  for(FS_intT p : positions)
    nodes.push_back(row[p]);
  return nodes;
}

} // namespace

TEST_F(FSCellSourceTest, MockMeshIsValid)
{
  FSMeshOpParams dummy;
  FSMeshCheck meshCheck(&clac);
  FSMeshData* meshData = mesh.GetMeshData();
  EXPECT_TRUE(meshCheck.DoOp(meshData, &dummy));
}

TEST_F(FSCellSourceTest, BuildCellSourcesListsAllUnstructTypes)
{
  const std::multiset<FSMeshEnums::CellType> expected = {FSMeshEnums::CT_Hexa8, FSMeshEnums::CT_Tetra4,
                                                         FSMeshEnums::CT_Poly3D, FSMeshEnums::CT_Quad4,
                                                         FSMeshEnums::CT_Tri3, FSMeshEnums::CT_Poly2D};
  std::multiset<FSMeshEnums::CellType> types;
  for(const auto& s : sources) {
    types.insert(s.Type());
    EXPECT_EQ(s.IsPoly3D(), s.Type() == FSMeshEnums::CT_Poly3D);
    EXPECT_EQ(s.IsPoly2D(), s.Type() == FSMeshEnums::CT_Poly2D);
  }
  EXPECT_EQ(types, expected);
}

TEST_F(FSCellSourceTest, OwnedCellIdsAreUniqueAcrossTypes)
{
  std::set<FS_intT> ids;
  FS_intT nVisited = 0;
  for(const auto& s : sources) {
    s.ForEachOwnedCell([&](FS_intT c) {
      EXPECT_EQ(c, First(s.Type()));
      ids.insert(c);
      nVisited++;
    });
  }
  EXPECT_EQ(nVisited, FS_intT(sources.size()));
  EXPECT_EQ(FS_intT(ids.size()), nVisited);
}

TEST_F(FSCellSourceTest, StandardCellRow)
{
  const FSCellSource& hexa = Source(FSMeshEnums::CT_Hexa8);
  const FS_intT c = First(FSMeshEnums::CT_Hexa8);
  EXPECT_EQ(hexa.NNodes(c), 8);
  EXPECT_EQ(hexa.Row(c), mock::kHexa);
  for(FS_intT n = 0; n < 8; n++)
    EXPECT_EQ(hexa.Node(c, n), mock::kHexa[n]);

  EXPECT_EQ(Source(FSMeshEnums::CT_Tetra4).Row(First(FSMeshEnums::CT_Tetra4)), mock::kTetra);
  EXPECT_EQ(Source(FSMeshEnums::CT_Quad4).Row(First(FSMeshEnums::CT_Quad4)), mock::kQuad);
  EXPECT_EQ(Source(FSMeshEnums::CT_Tri3).Row(First(FSMeshEnums::CT_Tri3)), mock::kTri);
}

TEST_F(FSCellSourceTest, StandardCellFaces)
{
  struct Case {
    FSMeshEnums::CellType t;
    std::vector<FS_intT> row;
    FS_intT nFaces, nCorners;
  };
  for(const auto& [t, row, nFaces, nCorners] :
      {Case{FSMeshEnums::CT_Hexa8, mock::kHexa, 6, 4}, Case{FSMeshEnums::CT_Tetra4, mock::kTetra, 4, 3}}) {
    const FSCellSource& src = Source(t);
    const FS_intT c = First(t);
    ASSERT_EQ(src.NFaces(c), nFaces);
    for(FS_intT f = 0; f < nFaces; f++) {
      ASSERT_EQ(src.NFaceCorners(c, f), nCorners);
      for(FS_intT k = 0; k < src.NFaceCorners(c, f); k++)
        EXPECT_EQ(src.FaceCorner(c, f, k), row[FSCellInfo::FaceNode(t, f, k)]) << "type " << t << " face " << f;
    }
  }
}

TEST_F(FSCellSourceTest, Poly3DRowAndFaces)
{
  const FSCellSource& poly = Source(FSMeshEnums::CT_Poly3D);
  const FS_intT c = First(FSMeshEnums::CT_Poly3D);
  EXPECT_EQ(poly.NNodes(c), 5);
  EXPECT_EQ(poly.Row(c), mock::kPyramid);

  ASSERT_EQ(poly.NFaces(c), FS_intT(mock::kPyramidFaces.size()));
  for(FS_intT f = 0; f < poly.NFaces(c); f++) {
    const std::vector<FS_intT> expected = Expected(mock::kPyramid, mock::kPyramidFaces[f]);
    ASSERT_EQ(poly.NFaceCorners(c, f), FS_intT(expected.size()));
    for(FS_intT k = 0; k < poly.NFaceCorners(c, f); k++)
      EXPECT_EQ(poly.FaceCorner(c, f, k), expected[k]) << "face " << f;
  }
}

TEST_F(FSCellSourceTest, Poly2DRow)
{
  const FSCellSource& poly = Source(FSMeshEnums::CT_Poly2D);
  const FS_intT c = First(FSMeshEnums::CT_Poly2D);
  EXPECT_EQ(poly.NNodes(c), 4);
  EXPECT_EQ(poly.Row(c), mock::kPoly2D);
  EXPECT_EQ(poly.NFaces(c), 0);
}

TEST_F(FSCellSourceTest, PolyFacesOnlyForPoly3D)
{
  for(const auto& s : sources) {
    if(s.IsPoly3D()) {
      ASSERT_NE(s.PolyFaces(), nullptr);
      EXPECT_EQ(s.PolyFaces()->GetNumPolys(), 1);
    } else {
      EXPECT_EQ(s.PolyFaces(), nullptr) << "type " << s.Type();
    }
  }
}

TEST(FSCellSource, MultiElementFile)
{
  FSClac clac;
  FSMeshImportParamsHDF5 params;
  params.mMeshFilename = MeshPath("input/cube_hexa_translate.h5");
  FSMesh mesh(&clac);
  ASSERT_TRUE(mesh.ImportMesh(&params));
  mesh.GetMeshData()->GetUnstructCells().CreateLocalNumbering();

  std::map<FSMeshEnums::CellType, FS_intT> nCells;
  for(const auto& s : BuildCellSources(mesh))
    s.ForEachOwnedCell([&](FS_intT) { nCells[s.Type()]++; });

  const std::map<FSMeshEnums::CellType, FS_intT> expected = {
    {FSMeshEnums::CT_Hexa8, 4}, {FSMeshEnums::CT_Poly3D, 4}, {FSMeshEnums::CT_Poly2D, 16}, {FSMeshEnums::CT_Quad4, 20}};
  EXPECT_EQ(nCells, expected);
}

_FS_END_NAMESPACE
