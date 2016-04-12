#ifndef CHUNK_H
#define CHUNK_H

#include <openGL/drawable.h>

class Chunk : public Drawable
{
public:
    Chunk(QList<QList<QList<bool>>> cells);
    Chunk();
    ~Chunk();
    void create();
    QList<QList<QList<bool>>> cells;

private:
    int index_count;
    int vertex_count;
    QVector<glm::vec3> createChunkVertexPositions();
    QVector<glm::vec3> createChunkVertexNormals();
    QVector<GLuint> createChunkIndices();
};

#endif // CHUNK_H