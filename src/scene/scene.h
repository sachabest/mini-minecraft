#pragma once
#include <QList>
#include <scene/camera.h>
#include "terrain/terrain.h"
#include "point3.h"
#include <scene/geometry/chunk.h>
#include "generators/lparser.h"

class Scene {

    static const int tree_length = 8;

public:
    Scene();

    void CreateScene();
    void CreateChunkScene();
    void CreateNewChunks();
    void findNearbyChunks();
    void shift(int dx, int dy, int dz);

    Chunk* getContainingChunk(Point3 p);
    OctNode* getContainingNode(Point3 p);
    Point3 worldToChunk(Point3 p);
    bool isFilled(Point3 p);
    QList<Point3> voxelize(const QVector<LPair_t> &pairs);
    QList<Point3> points;
    glm::ivec3 dimensions;
    glm::vec3 origin;
    Terrain terrain;
    OctNode* octree;

    int num_chunks;
    QList<Point3> chunk_points;
};
