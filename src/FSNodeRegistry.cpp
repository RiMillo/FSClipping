#include "FSClipping/FSNodeRegistry.h"
#include <FSMeshData.h>

_FS_BEGIN_NAMESPACE

FSNodeRegistry::FSNodeRegistry(FSMesh& mesh, const std::vector<FSCellSource>& sources, const FSCellClipped& matches,
                               FS_floatT tol)
{
  FSFloatArrayT oldCoords;
  FS_intT nodeOffset;
  FSQuantityDescArrayT coordDesc;
  mesh.GetMeshData()->GetUnstructCells().GetCoordinates3D(coordDesc, oldCoords, nodeOffset);

  for(FS_intT n = 0; n < oldCoords.Size(); n++) {
    FS_floatT x = oldCoords(n, 0);
    FS_floatT y = oldCoords(n, 1);
    FS_floatT z = oldCoords(n, 2);
    FSVec3 coord(x, y, z);
    NodeKey key(coord, tol);

    this->globalCoords_.push_back(coord);
    this->coordToLocalNode_.emplace(key, n);
  }

  std::vector<FSVec3> newCoords;
  FS_intT elemOwner = 0;
  // auto lambda = [&newCoords](FS_intT m, const FSFaceMatch& matche) { newCoords.push_back(matche.clippedPoly3D[0]);};
  // matches.ForEachMatch(lambda);
}

_FS_END_NAMESPACE