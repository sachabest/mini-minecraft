#include "mygl.h"
#include <la.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <QXmlStreamReader>
#include <QFileDialog>
#include <QTime>


MyGL::MyGL(QWidget *parent)
    : GLWidget277(parent)
{
    setFocusPolicy(Qt::ClickFocus);
}

MyGL::~MyGL()
{
    makeCurrent();

    vao.destroy();
}

void MyGL::initializeGL()
{
    // Create an OpenGL context
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    // Set the size with which points should be rendered
    glPointSize(5);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    vao.create();

    // Create and set up the diffuse shader
    prog_lambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat-color shader
    prog_flat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");

    geom_cube.create();
    test_chunk.create();

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    vao.bind();

    //Test scene data initialization
    scene.CreateTestScene();
}

void MyGL::resizeGL(int w, int h)
{
    gl_camera = Camera(w, h, glm::vec3(scene.dimensions.x/2, scene.dimensions.y/2 + 2, scene.dimensions.z/2),
                       glm::vec3(scene.dimensions.x/2, scene.dimensions.y/2+2, scene.dimensions.z/2+1), glm::vec3(0,1,0));

    glm::mat4 viewproj = gl_camera.getViewProj();

    // Upload the projection matrix
    prog_lambert.setViewProjMatrix(viewproj);
    prog_flat.setViewProjMatrix(viewproj);

    printGLErrorLog();
}

// This function is called by Qt any time your GL window is supposed to update
// For example, when the function updateGL is called, paintGL is called implicitly.
void MyGL::paintGL()
{
    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update the viewproj matrix
    prog_lambert.setViewProjMatrix(gl_camera.getViewProj());
    prog_flat.setViewProjMatrix(gl_camera.getViewProj());
    GLDrawScene();
}

void MyGL::GLDrawScene()
{
    for(int x = 0; x < scene.dimensions.x; x++)
        {
            for(int y = 0; y < scene.dimensions.y; y++)
            {
                for(int z = 0; z < scene.dimensions.z; z++)
                {
                    if(scene.objects[x][y][z])
                    {
                        prog_lambert.setModelMatrix(glm::translate(glm::mat4(), glm::vec3(x, y, z)));
                        prog_lambert.draw(*this, geom_cube);
                    }
                }
            }
        }
//    prog_lambert.setModelMatrix(glm::mat4());
//    prog_lambert.draw(*this, test_chunk);
}

void MyGL::keyPressEvent(QKeyEvent *e)
{
    float amount = 2.0f;
    if(e->modifiers() & Qt::ShiftModifier){
        amount = 10.0f;
    }
    // http://doc.qt.io/qt-5/qt.html#Key-enum
    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_Right) {
        gl_camera.RotateAboutUp(-amount);
    } else if (e->key() == Qt::Key_Left) {
        gl_camera.RotateAboutUp(amount);
    } else if (e->key() == Qt::Key_Up) {
        gl_camera.RotateAboutRight(-amount);
    } else if (e->key() == Qt::Key_Down) {
        gl_camera.RotateAboutRight(amount);
    } else if (e->key() == Qt::Key_1) {
        gl_camera.fovy += amount;
    } else if (e->key() == Qt::Key_2) {
        gl_camera.fovy -= amount;
    } else if (e->key() == Qt::Key_W) {
        gl_camera.TranslateAlongLook(amount);
    } else if (e->key() == Qt::Key_S) {
        gl_camera.TranslateAlongLook(-amount);
    } else if (e->key() == Qt::Key_D) {
        gl_camera.TranslateAlongRight(amount);
    } else if (e->key() == Qt::Key_A) {
        gl_camera.TranslateAlongRight(-amount);
    } else if (e->key() == Qt::Key_Q) {
        gl_camera.TranslateAlongUp(-amount);
    } else if (e->key() == Qt::Key_E) {
        gl_camera.TranslateAlongUp(amount);
    }
    gl_camera.RecomputeAttributes();
    update();  // Calls paintGL, among other things
}
