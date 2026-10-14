#include "gtest/gtest.h"
#include <vector>

#include "FSClipping/FSCellClipped.h"
#include "MockMixedMesh.hpp"

_FS_BEGIN_NAMESPACE

namespace {

constexpr FS_intT kCorner = 7; // hexa with two clipped faces
constexpr FS_intT kSide = 9;   // tetra with one clipped face
constexpr FS_intT kQuad = 20;
constexpr FS_intT kTri = 30;
constexpr FS_intT kOther = 42;

FSFaceMatch Match(FS_intT owner, FSFaceMatch::MatchType type = FSFaceMatch::INTERSECTING)
{
  return mock::MakeMatch(owner, FSMeshEnums::CT_Hexa8, {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}, {0, 1, 2}, type);
}

// Matches of the corner cell are scattered in the list
std::vector<FSFaceMatch> Matches()
{
  return {Match(kCorner), Match(kSide), Match(kSide, FSFaceMatch::NOT_COVERED), Match(kCorner), Match(kCorner)};
}

BoundaryExtraction Boundary()
{
  BoundaryExtraction be;
  be.replacedFaces = {{kCorner, {0, 3}}, {kSide, {2}}};
  be.volumeCells = {{FSMeshEnums::CT_Hexa8, {kCorner}}, {FSMeshEnums::CT_Tetra4, {kSide}}};
  be.surfaceCells = {{FSMeshEnums::CT_Quad4, {kQuad}}, {FSMeshEnums::CT_Tri3, {kTri}}};
  return be;
}

std::vector<FS_intT> ToVector(std::span<const FS_intT> s) { return {s.begin(), s.end()}; }

class FSCellClippedTest : public ::testing::Test
{
protected:
  const std::vector<FSFaceMatch> matches = Matches();
  const BoundaryExtraction be = Boundary();
  const FSCellClipped clipped{matches, be};
};

} // namespace

TEST_F(FSCellClippedTest, MatchesOfGroupsByOwner)
{
  EXPECT_EQ(ToVector(clipped.MatchesOf(kCorner)), (std::vector<FS_intT>{0, 3, 4}));
  EXPECT_EQ(ToVector(clipped.MatchesOf(kSide)), (std::vector<FS_intT>{1, 2}));
  EXPECT_EQ(&clipped.Match(3), &matches[3]);
}

TEST_F(FSCellClippedTest, Contains)
{
  EXPECT_TRUE(clipped.Contains(kCorner));
  EXPECT_TRUE(clipped.Contains(kSide));
  EXPECT_FALSE(clipped.Contains(kQuad));
  EXPECT_FALSE(clipped.Contains(kOther));
}

TEST_F(FSCellClippedTest, IsReplacedSurfaceAcrossTypes)
{
  EXPECT_TRUE(clipped.IsReplacedSurface(kQuad));
  EXPECT_TRUE(clipped.IsReplacedSurface(kTri));
  EXPECT_FALSE(clipped.IsReplacedSurface(kCorner));
  EXPECT_FALSE(clipped.IsReplacedSurface(kOther));
}

TEST_F(FSCellClippedTest, IsReplacedFace)
{
  EXPECT_TRUE(clipped.IsReplacedFace(kCorner, 0));
  EXPECT_TRUE(clipped.IsReplacedFace(kCorner, 3));
  EXPECT_FALSE(clipped.IsReplacedFace(kCorner, 1));
  EXPECT_TRUE(clipped.IsReplacedFace(kSide, 2));
  EXPECT_FALSE(clipped.IsReplacedFace(kSide, 0));
  EXPECT_FALSE(clipped.IsReplacedFace(kOther, 0));
}

TEST_F(FSCellClippedTest, ForEachMatchVisitsAllInOrder)
{
  std::vector<FS_intT> visited;
  clipped.ForEachMatch([&](FS_intT m, const FSFaceMatch& match) {
    EXPECT_EQ(&match, &matches[m]);
    visited.push_back(m);
  });
  EXPECT_EQ(visited, (std::vector<FS_intT>{0, 1, 2, 3, 4}));
}

TEST(FSCellClipped, EmptyInput)
{
  const std::vector<FSFaceMatch> matches;
  const BoundaryExtraction be;
  const FSCellClipped clipped(matches, be);

  EXPECT_FALSE(clipped.Contains(kCorner));
  EXPECT_FALSE(clipped.IsReplacedSurface(kQuad));
  FS_intT n = 0;
  clipped.ForEachMatch([&](FS_intT, const FSFaceMatch&) { n++; });
  EXPECT_EQ(n, 0);
}

TEST(FSCellClippedDeathTest, InvalidInput)
{
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  const auto be = Boundary();

  auto unknown = Matches();
  unknown[1].type = FSFaceMatch::UNKNOWN;
  EXPECT_DEATH(FSCellClipped(unknown, be), "");

  auto degenerate = Matches();
  degenerate[0].clippedPoly3D.pop_back();
  degenerate[0].nodeGlobalIds.pop_back();
  EXPECT_DEATH(FSCellClipped(degenerate, be), "");

  auto mismatch = Matches();
  mismatch[0].nodeGlobalIds.pop_back();
  EXPECT_DEATH(FSCellClipped(mismatch, be), "");

  const std::vector<FSFaceMatch> onlyCorner = {Match(kCorner)};
  EXPECT_DEATH(FSCellClipped(onlyCorner, be), "");

  const auto matches = Matches();
  const FSCellClipped clipped(matches, be);
  EXPECT_DEATH(clipped.MatchesOf(kOther), "");
}

_FS_END_NAMESPACE
