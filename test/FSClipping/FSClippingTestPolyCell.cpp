#include "gtest/gtest.h"
#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include "FSClipping/FSPolyCell.h"
#include "MockMixedMesh.hpp"

_FS_BEGIN_NAMESPACE

namespace {

constexpr FS_floatT kTol = 1e-6;
constexpr FS_floatT kEps = 1e-12;
const FS_intT kNOld = FS_intT(mock::kCoords.size());

using Face = std::vector<NodeId>;
using NodeSet = std::set<NodeId>;

bool Near(const FSVec3& a, const FSVec3& b) { return (a - b).L2Norm() < kEps; }

class FSPolyCellTest : public ::testing::Test
{
protected:
  FSClac clac;
  FSMesh mesh = mock::BuildMixedMesh(clac);
  std::vector<FSCellSource> sources = BuildCellSources(mesh);
  std::vector<FSFaceMatch> matches;
  BoundaryExtraction be;
  std::unique_ptr<FSCellClipped> clipped;
  std::unique_ptr<FSNodeRegistry> nodes;

  const FSCellSource& Source(FSMeshEnums::CellType t) const
  {
    return *std::find_if(sources.begin(), sources.end(), [t](const FSCellSource& s) { return s.Type() == t; });
  }

  FS_intT Cell(FSMeshEnums::CellType t) const { return mock::Offset(mesh, t); }

  // Local face of the cell whose corners are exactly these nodes
  FS_intT LocalFace(FSMeshEnums::CellType t, const NodeSet& corners) const
  {
    const FSCellSource& src = Source(t);
    const FS_intT c = Cell(t);
    for(FS_intT f = 0; f < src.NFaces(c); f++) {
      NodeSet s;
      for(FS_intT k = 0; k < src.NFaceCorners(c, f); k++)
        s.insert(src.FaceCorner(c, f, k));
      if(s == corners)
        return f;
    }
    ADD_FAILURE() << "no such face";
    return -1;
  }

  void Add(FSMeshEnums::CellType t, std::vector<FSVec3> poly, std::vector<FS_intT> ids)
  {
    matches.push_back(mock::MakeMatch(Cell(t), t, std::move(poly), std::move(ids)));
  }

  void Replace(FSMeshEnums::CellType t, const NodeSet& corners) { be.cell2BoundaryFaces[Cell(t)].push_back(LocalFace(t, corners)); }

  void BuildRegistry()
  {
    clipped = std::make_unique<FSCellClipped>(matches, be);
    nodes = std::make_unique<FSNodeRegistry>(mesh, sources, *clipped, kTol);
  }

  FSPolyCell Build(FSMeshEnums::CellType t)
  {
    BuildRegistry();
    return BuildClippedCell(Source(t), Cell(t), *clipped, *nodes);
  }

  // Hexa bottom face {0,1,2,3} split in two by P(.5,0,0) - R(.5,1,0), both polygons given with normal +z (inward)
  void SplitHexaBottom()
  {
    Add(FSMeshEnums::CT_Hexa8, {{0, 0, 0}, {0.5, 0, 0}, {0.5, 1, 0}, {0, 1, 0}}, {10, 100, 101, 13});
    Add(FSMeshEnums::CT_Hexa8, {{0.5, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0.5, 1, 0}}, {100, 11, 12, 101});
    Replace(FSMeshEnums::CT_Hexa8, {0, 1, 2, 3});
  }

  // Non-normalized area vector of a face (fan triangulation)
  FSVec3 AreaVector(const Face& face) const
  {
    FSVec3 a(0, 0, 0);
    const FSVec3& p0 = nodes->Coord(face[0]);
    for(size_t i = 1; i + 1 < face.size(); i++)
      a += (nodes->Coord(face[i]) - p0).CrossProduct(nodes->Coord(face[i + 1]) - p0);
    return a * 0.5;
  }

  void ExpectOutward(const FSPolyCell& cell, FSMeshEnums::CellType t) const
  {
    const FSVec3 center = ComputeCellCenter(Source(t), Cell(t), *nodes);
    for(size_t f = 0; f < cell.faces.size(); f++) {
      const FSVec3 dir = ComputeFaceCenter(cell.faces[f], *nodes) - center;
      EXPECT_GT(ComputeFaceNormal(cell.faces[f], *nodes).InnerProduct(dir), 0.0) << "face " << f;
    }
  }

  // A closed, consistently oriented surface has a zero total area vector
  void ExpectClosed(const FSPolyCell& cell) const
  {
    FSVec3 sum(0, 0, 0);
    for(const auto& face : cell.faces)
      sum += AreaVector(face);
    EXPECT_LT(sum.L2Norm(), kEps);
  }

  // Divergence theorem: V = 1/3 sum(c_f . A_f)
  FS_floatT Volume(const FSPolyCell& cell) const
  {
    FS_floatT v = 0;
    for(const auto& face : cell.faces)
      v += ComputeFaceCenter(face, *nodes).InnerProduct(AreaVector(face));
    return v / 3.0;
  }
};

std::multiset<NodeSet> FaceSets(const FSPolyCell& cell)
{
  std::multiset<NodeSet> sets;
  for(const auto& face : cell.faces)
    sets.insert(NodeSet(face.begin(), face.end()));
  return sets;
}

bool Contains(const FSPolyCell& cell, const Face& face)
{
  return std::find(cell.faces.begin(), cell.faces.end(), face) != cell.faces.end();
}

} // namespace

TEST_F(FSPolyCellTest, CellCenter)
{
  BuildRegistry();
  EXPECT_TRUE(Near(ComputeCellCenter(Source(FSMeshEnums::CT_Hexa8), Cell(FSMeshEnums::CT_Hexa8), *nodes),
                   FSVec3(0.5, 0.5, 0.5)));
  EXPECT_TRUE(Near(ComputeCellCenter(Source(FSMeshEnums::CT_Poly3D), Cell(FSMeshEnums::CT_Poly3D), *nodes),
                   FSVec3(0.5, 0.5, 1.1)));
  EXPECT_TRUE(Near(ComputeCellCenter(Source(FSMeshEnums::CT_Tetra4), Cell(FSMeshEnums::CT_Tetra4), *nodes),
                   FSVec3(1, 0.5, 1.25)));
}

TEST_F(FSPolyCellTest, FaceCenterAndNormal)
{
  BuildRegistry();
  const Face bottom = {0, 1, 2, 3};
  EXPECT_TRUE(Near(ComputeFaceCenter(bottom, *nodes), FSVec3(0.5, 0.5, 0)));
  EXPECT_TRUE(Near(ComputeFaceNormal(bottom, *nodes), FSVec3(0, 0, 1)));
  EXPECT_TRUE(Near(ComputeFaceNormal(Face{3, 2, 1, 0}, *nodes), FSVec3(0, 0, -1)));
  EXPECT_TRUE(Near(ComputeFaceNormal(Face{0, 4, 7}, *nodes), FSVec3(-1, 0, 0)));
}

TEST_F(FSPolyCellTest, NormalIsUnit)
{
  Add(FSMeshEnums::CT_Tetra4, {{1, 0, 1}, {1.5, 0.5, 1.5}, {1, 1, 1}}, {205, 209, 206});
  Replace(FSMeshEnums::CT_Tetra4, {5, 6, 9});
  BuildRegistry();
  EXPECT_NEAR(ComputeFaceNormal(Face{5, 9, 6}, *nodes).L2Norm(), 1.0, kEps);
}

TEST_F(FSPolyCellTest, DegenerateFaceHasZeroNormal)
{
  BuildRegistry();
  // Collinear nodes
  EXPECT_TRUE(Near(ComputeFaceNormal(Face{0, 1, 0}, *nodes), FSVec3(0, 0, 0)));
}

TEST_F(FSPolyCellTest, HexaBoundaryFaceIsDropped)
{
  SplitHexaBottom();
  const FSPolyCell cell = Build(FSMeshEnums::CT_Hexa8);

  ASSERT_EQ(cell.faces.size(), 7u);
  EXPECT_EQ(FaceSets(cell).count(NodeSet{0, 1, 2, 3}), 0u);
}

TEST_F(FSPolyCellTest, HexaKeepsNonBoundarySourceFaces)
{
  SplitHexaBottom();
  const FSPolyCell cell = Build(FSMeshEnums::CT_Hexa8);

  const std::multiset<NodeSet> sets = FaceSets(cell);
  for(const NodeSet& side : {NodeSet{4, 5, 6, 7}, NodeSet{0, 1, 4, 5}, NodeSet{1, 2, 5, 6}, NodeSet{2, 3, 6, 7},
                             NodeSet{0, 3, 4, 7}})
    EXPECT_EQ(sets.count(side), 1u);
}

TEST_F(FSPolyCellTest, HexaClippedPolygonsUseRegistryNodes)
{
  SplitHexaBottom();
  const FSPolyCell cell = Build(FSMeshEnums::CT_Hexa8);

  // Given with normal +z, reversed to point outward (-z)
  EXPECT_TRUE(Contains(cell, Face{3, kNOld + 1, kNOld, 0}));
  EXPECT_TRUE(Contains(cell, Face{kNOld + 1, 2, 1, kNOld}));
}

TEST_F(FSPolyCellTest, HexaIsClosedAndOutward)
{
  SplitHexaBottom();
  const FSPolyCell cell = Build(FSMeshEnums::CT_Hexa8);

  ExpectOutward(cell, FSMeshEnums::CT_Hexa8);
  ExpectClosed(cell);
  EXPECT_NEAR(Volume(cell), 1.0, kEps);
}

TEST_F(FSPolyCellTest, HexaTwoBoundaryFaces)
{
  SplitHexaBottom();
  Add(FSMeshEnums::CT_Hexa8, {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}, {10, 14, 17, 13});
  Replace(FSMeshEnums::CT_Hexa8, {0, 3, 4, 7});
  const FSPolyCell cell = Build(FSMeshEnums::CT_Hexa8);

  ASSERT_EQ(cell.faces.size(), 7u);
  EXPECT_EQ(FaceSets(cell).count(NodeSet{0, 3, 4, 7}), 1u); // the clipped polygon, not the source face
  ExpectOutward(cell, FSMeshEnums::CT_Hexa8);
  ExpectClosed(cell);
  EXPECT_NEAR(Volume(cell), 1.0, kEps);
}

TEST_F(FSPolyCellTest, OutwardPolygonKeepsItsOrder)
{
  // Pyramid base given with normal -z, already outward
  Add(FSMeshEnums::CT_Poly3D, {{0, 0, 1}, {0, 1, 1}, {1, 1, 1}, {1, 0, 1}}, {304, 307, 306, 305});
  Replace(FSMeshEnums::CT_Poly3D, {4, 5, 6, 7});
  const FSPolyCell cell = Build(FSMeshEnums::CT_Poly3D);

  ASSERT_EQ(cell.faces.size(), mock::kPyramidFaces.size());
  EXPECT_TRUE(Contains(cell, Face{4, 7, 6, 5}));
  ExpectOutward(cell, FSMeshEnums::CT_Poly3D);
  ExpectClosed(cell);
  EXPECT_NEAR(Volume(cell), 0.5 / 3.0, kEps);
}

TEST_F(FSPolyCellTest, Poly3DSourceFacesAreReoriented)
{
  // Pyramid side faces all reoriented outward whatever the storage order
  Add(FSMeshEnums::CT_Poly3D, {{0, 0, 1}, {1, 0, 1}, {0.5, 0.5, 1.5}}, {304, 305, 308});
  Replace(FSMeshEnums::CT_Poly3D, {4, 5, 8});
  const FSPolyCell cell = Build(FSMeshEnums::CT_Poly3D);

  ASSERT_EQ(cell.faces.size(), mock::kPyramidFaces.size());
  ExpectOutward(cell, FSMeshEnums::CT_Poly3D);
  ExpectClosed(cell);
}

TEST_F(FSPolyCellTest, TetraOwner)
{
  Add(FSMeshEnums::CT_Tetra4, {{1, 0, 1}, {1.5, 0.5, 1.5}, {1, 1, 1}}, {205, 209, 206});
  Replace(FSMeshEnums::CT_Tetra4, {5, 6, 9});
  const FSPolyCell cell = Build(FSMeshEnums::CT_Tetra4);

  ASSERT_EQ(cell.faces.size(), 4u);
  EXPECT_TRUE(Contains(cell, Face{6, 9, 5})); // given inward, reversed
  ExpectOutward(cell, FSMeshEnums::CT_Tetra4);
  ExpectClosed(cell);
  EXPECT_NEAR(Volume(cell), 1.0 / 12.0, kEps);
}

TEST_F(FSPolyCellTest, PolygonWithNewVertexOnly)
{
  // Triangle on the hexa top face with an interior vertex: the replaced face is only partly covered
  Add(FSMeshEnums::CT_Hexa8, {{0, 0, 1}, {1, 0, 1}, {0.5, 0.5, 1}}, {404, 405, 450});
  Replace(FSMeshEnums::CT_Hexa8, {4, 5, 6, 7});
  const FSPolyCell cell = Build(FSMeshEnums::CT_Hexa8);

  ASSERT_EQ(cell.faces.size(), 6u);
  EXPECT_EQ(FaceSets(cell).count(NodeSet{4, 5, kNOld}), 1u);
  ExpectOutward(cell, FSMeshEnums::CT_Hexa8);
}

_FS_END_NAMESPACE
