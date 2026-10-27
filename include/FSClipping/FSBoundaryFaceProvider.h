#ifndef FSBOUNDARYFACEPROVIDER_H
#define FSBOUNDARYFACEPROVIDER_H

#include <FSMesh.h>

#include "FSClipping/FSClippingFace.h"

#include <set>
#include <unordered_map>
#include <vector>

_FS_BEGIN_NAMESPACE

struct BoundaryExtraction {
  std::vector<FSClippingFace> faces;
  std::unordered_map<FS_intT, std::vector<FS_intT> > replacedFaces; // cell ID -> local cell number face on the border

  std::unordered_map<FS_intT, std::set<FS_intT> > volumeCells;
  std::unordered_map<FS_intT, std::set<FS_intT> > surfaceCells;
};

class FSBoundaryFaceProvider
{
public:
  static BoundaryExtraction Extract(FSMesh& mesh, FSMeshFaceExtractor& ex, FS_intT boundaryMarker);
};


_FS_END_NAMESPACE

#endif