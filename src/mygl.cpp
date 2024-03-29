#include "mygl.h"
#include <la.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <QXmlStreamReader>
#include <QFileDialog>
#include <QTime>

int MyGL::frame = 0;
int MyGL::time = 0;

#define SHIFT_DISTANCE 16
MyGL::MyGL(QWidget *parent)
    : GLWidget277(parent), filename("")
{
    setFocusPolicy(Qt::ClickFocus);
}

MyGL::~MyGL()
{
    makeCurrent();
    vao.destroy();
    delete scene.octree;
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
    gltexture = new QOpenGLTexture(QImage(":/minecraft_textures_all.png"));

    // Create and set up the diffuse shader
    prog_lambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat-color shader
    prog_flat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");

    prog_lambert.setUVImage(gltexture);
    prog_flat.setUVImage(gltexture);

    geom_cube.create();

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    vao.bind();

    cross.create();

    //timer = QTimer(this);
    connect(&timer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
    timer.start(17);

    //Test scene data initialization
    scene.CreateNewChunks();
}

void MyGL::resizeGL(int w, int h)
{
    gl_camera = Camera(w, h, glm::vec3(scene.dimensions[0]/2, 20, scene.dimensions[2]/2),
            glm::vec3(10, 2, 10), glm::vec3(0,1,0));

    //gl_camera = Camera(w, h, glm::vec3(0, 20, 0),
    //                   glm::vec3(0, 2, 1), glm::vec3(0,1,0));

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

//    std::cout << glm::to_string(gl_camera.look) << std::endl;

    // Update the viewproj matrix
    prog_lambert.setViewProjMatrix(gl_camera.getViewProj());
    prog_flat.setViewProjMatrix(gl_camera.getViewProj());
    GLDrawScene();

    //draw the center of the gl lines
    glDisable(GL_DEPTH_TEST);
    //MODEL matrix is always identity so we are at the untransformed space of the cross
    prog_flat.setViewProjMatrix(glm::mat4(1.0f));
    prog_flat.setModelMatrix(glm::mat4(1.0f));
    prog_flat.draw(*this, cross);
    glEnable(GL_DEPTH_TEST);
}

// Used to determine whether a chunk is too far away to be rendered
// This can definitely be more efficient, it still checks every node right now
float MyGL::distanceToEye(Point3 p)
{
    return glm::distance(p.toVec3(), gl_camera.eye);
}

void MyGL::drawChunks(OctNode* node)
{
    if (node->is_leaf && node->chunk && distanceToEye(node->base) <= 512) {
        prog_lambert.setModelMatrix(glm::translate(glm::mat4(), glm::vec3(node->base.x*16, node->base.y*16, node->base.z*16)));
        prog_lambert.draw(*this, *(node->chunk));
    } else {    // Draw its children
        for (OctNode* child : node->children) {
            drawChunks(child);
        }
    }
}

void MyGL::GLDrawScene()
{
    drawChunks(scene.octree);
}

// Given the current camera position, which chunk am I located on?
Point3 MyGL::getChunkPosition()
{
    return Point3(glm::floor(gl_camera.eye.x/16.0f), glm::floor(gl_camera.eye.y/160.f), glm::floor(gl_camera.eye.z/16.0f));
}

void MyGL::slot_loadImage() {
    QString fn = QFileDialog::getOpenFileName(0, QString("Load Image"), QString("../../../../cis277final/"), tr("*.*"));
    QImage image = QImage(fn);
    if (image.isNull()) {
        qDebug() << "Invalid image";
        filename = "";
    } else {
        qDebug() << "Loaded image";
        this->filename = fn;
        update();
    }
}

void MyGL::keyPressEvent(QKeyEvent *e)
{
    float amount = 2.0f;
    if(e->modifiers() & Qt::ShiftModifier){
        amount = 10.0f;
    }
    Point3 old_pos = getChunkPosition();
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
    } else if (e->key() == Qt::Key_C) {
        if (filename != "") {
            QImage image = QImage(filename);
            scene.parseImage(image, gl_camera.eye);
            update();
        }
    }

    //z direction
    else if (e->key() == Qt::Key_W) {
        if (!isGravity) {
            gl_camera.TranslateAlongLook(amount);
        }
        else {
            inz = true;
        }

    } else if (e->key() == Qt::Key_S) {
        if (!isGravity) {
            gl_camera.TranslateAlongLook(-amount);
        }
        else {
            outz = true;
        }
    }
    //x direction
    else if (e->key() == Qt::Key_D) {
        if (!isGravity) {
            gl_camera.TranslateAlongRight(amount);
        }
        else {
            rightx = true;
        }

    } else if (e->key() == Qt::Key_A) {
        if (!isGravity) {
            gl_camera.TranslateAlongRight(-amount);
        }
        else {
            leftx = true;
        }
    }

    //enable gravity
    else if (e->key() == Qt::Key_G) {
        isGravity = true;
    }

    //disable gravity
    else if (e->key() == Qt::Key_H) {
        isGravity = false;
    }

    //y direction
    else if (e->key() == Qt::Key_Q) {
        if (!isGravity) {
            gl_camera.TranslateAlongUp(-amount);
        }
    }

    else if (e->key() == Qt::Key_E) {
        if (!isGravity) {
            gl_camera.TranslateAlongUp(amount);
        }
        else {
            upy = true;
        }

    } else if (e->key() == Qt::Key_8) {
        // make plant
        QVector<LPair_t> tree = LParser::makeTree();
        Point3* collision = raymarchCast();
        if (collision->x != INFINITY) {
            scene.voxelize(tree, *collision);
        }
    } else if (e->key() == Qt::Key_9) {
        // make plant
        QVector<LPair_t> tree = LParser::makeBrush();
        Point3* collision = raymarchCast();
        if (collision->x != INFINITY) {
            scene.voxelize(tree, *collision);
        }
    } else if (e->key() == Qt::Key_0) {
        // make plant
        QVector<LPair_t> tree = LParser::makeCarrieTree();
        Point3* collision = raymarchCast();
        if (collision->x != INFINITY) {
            scene.voxelize(tree, *collision);
        }
    }
    gl_camera.RecomputeAttributes();
    Point3 new_pos = getChunkPosition();
    // We moved to a different chunk
    if (!(old_pos == new_pos)) {
        if (new_pos.x > old_pos.x) {
            scene.shift(SHIFT_DISTANCE, 0, 0);
        } else if (new_pos.x < old_pos.x) {
            scene.shift(-SHIFT_DISTANCE, 0, 0);
        }
        if (new_pos.z > old_pos.z) {
            scene.shift(0, 0, SHIFT_DISTANCE);
        } else if (new_pos.z < old_pos.z) {
            scene.shift(0, 0, -SHIFT_DISTANCE);
        }
    }
    update();  // Calls paintGL, among other things
}

void MyGL::keyReleaseEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_W) {
        inz = false;
        qDebug() << "w is release";
    }
    else if (e->key() == Qt::Key_S) {
        outz = false;
    }
    else if (e->key() == Qt::Key_D) {
        rightx = false;
    }
    else if (e->key() == Qt::Key_A) {
        leftx = false;
    }
    else if (e->key() == Qt::Key_Q) {
        downy = false;
    }
    else if (e->key() == Qt::Key_E) {
        upy = false;
    }
//    else if (e->key() == Qt::Key_G) {
//        isGravity = false;
//    }
    gl_camera.RecomputeAttributes();
    update();
}

Point3* MyGL::raymarchCast() {
    Ray ray_from_center = gl_camera.raycast();
    for (float t = 0.1; t < 32.f; t+=0.1) {
        glm::vec3 new_dir = glm::vec3 (t*ray_from_center.direction.x, t*ray_from_center.direction.y,
                                       t*ray_from_center.direction.z);
        glm::vec3 position = ray_from_center.origin + new_dir;
        //floor the position value and check if there is an object in there;
        //if there is: remove it and break out of the loop
        Point3* point_cube = new Point3((glm::floor(position.x)), glm::floor(position.y), glm::floor(position.z));
        Point3 containpoint = Point3(glm::floor(position.x), glm::floor(position.y), glm::floor(position.z));

        //NEW CHUNK STUFF FOR TESTING FILL
        if (scene.isFilled(containpoint)) {
            return point_cube;
        }
    }
    return new Point3(INFINITY, INFINITY, INFINITY);
}

OctNode* MyGL::octreeMarch() {
    gl_camera.RecomputeAttributes();
    Ray ray_from_center = gl_camera.raycast();
    glm::vec3 ray_origin = ray_from_center.origin;

    OctNode* intersection = scene.octree->rayCastOct(ray_from_center);
    if (intersection != nullptr) {
        return intersection;
    }
    return nullptr;
}

Texture MyGL::destroyBlocks() {
    OctNode* node = octreeMarch();
    Point3* cube = raymarchCast();

    if (cube != nullptr && cube->x != INFINITY) {
        Point3 localchunk = scene.worldToChunk(Point3(cube->x, cube->y, cube->z));
        Chunk* chunk = scene.getContainingChunk(Point3(cube->x, cube->y, cube->z));
        if (localchunk.x < chunk->cells.size()) {
            QList<QList<Texture>> ypart = chunk->cells[localchunk.x];
            if (localchunk.y < ypart.size()) {
                QList<Texture> zpart = ypart[localchunk.y];
                if (localchunk.z < zpart.size()) {
                    if (chunk->cells[localchunk.x][localchunk.y][localchunk.z] != EMPTY) {
                        Texture old = chunk->cells[localchunk.x][localchunk.y][localchunk.z];
                        chunk->cells[localchunk.x][localchunk.y][localchunk.z] = EMPTY;
                        chunk->create();
                        update();
                        return old;
                    }
                }
            }
        }
    }
    return EMPTY;
}

//FIX ADDING LATER

bool MyGL::canAddBlock() {
    Point3* cube = raymarchCast();
    if (cube != nullptr && cube->x != INFINITY) {
        Point3 localchunk = scene.worldToChunk(Point3(cube->x, cube->y, cube->z));
        Chunk* chunk = scene.getContainingChunk(Point3(cube->x, cube->y, cube->z));
        if (localchunk.y + 1 > 15) {
            chunk = scene.getContainingChunk(Point3(cube->x, cube->y + 1, cube->z));
        }
        Texture &pt = chunk->cells[localchunk.x][(int) (localchunk.y+1) % 16][localchunk.z];
        if (pt == EMPTY) {
            return true;
        }
    }
    return false;
}


bool MyGL::sachaAddBlock(Texture t) {
    Point3* cube = raymarchCast();
    if (cube != nullptr && cube->x != INFINITY) {
        Point3 localchunk = scene.worldToChunk(Point3(cube->x, cube->y, cube->z));
        Chunk* chunk = scene.getContainingChunk(Point3(cube->x, cube->y, cube->z));
        if (localchunk.y + 1 > 15) {
            chunk = scene.getContainingChunk(Point3(cube->x, cube->y + 1, cube->z));
        }
        Texture &pt = chunk->cells[localchunk.x][(int) (localchunk.y+1) % 16][localchunk.z];
        if (pt == EMPTY) {
            pt = t;
            chunk->create();
            update();
            return true;
        }
    }
    return false;
}

void MyGL::animateTextures() {
    int remainTime = timer.remainingTime();
    //timer is active
    if (remainTime != -1) {
        int moduolo = frame % 5;
        prog_lambert.setTimer(moduolo);
        if (parentView && parentView->scene()) {
           parentView->scene()->update();
        }
        frame++;
    }
}

//COLLISION CODE
//right is true -> move to right
//false-> try to move to left
Point3 MyGL::collisionX(bool right, float time) {
    float position = time * time * acceleration;
    //negative
    if (!right) {
        gl_camera.TranslateAlongRight(-position);
        if (parentView && parentView->scene()) {
           parentView->scene()->update();
        }
    }

    //posive amt
    else if (right) {
        gl_camera.TranslateAlongRight(position);
        if (parentView && parentView->scene()) {
           parentView->scene()->update();
        }
    }
    //feet
    return Point3(gl_camera.eye.x, glm::floor(gl_camera.eye.y-1.5), gl_camera.eye.z);
}

//if up is true we are moving up;
//else we are trying to move down;
Point3 MyGL::collisionY(bool up, float time) {
    //float d_speed = time * acceleration;
    float position = time * time * acceleration;
    //negative
    if (!up) {
        gl_camera.TranslateAlongUp(-position);
        if (parentView && parentView->scene()) {
           parentView->scene()->update();
        }
    }
    //posive amt
    else if(up) {
        gl_camera.TranslateAlongUp(position);
        if (parentView && parentView->scene()) {
           parentView->scene()->update();
        }
    }
    //feet
    return Point3(gl_camera.eye.x, glm::floor(gl_camera.eye.y-1.5), gl_camera.eye.z);
}

//if look is true we are trying to move towards us
//is look is false we are trying to move away
Point3 MyGL::collisionZ(bool look, float time) {
    float position =  time * time * acceleration;
    //negative
    if (!look) {
        gl_camera.TranslateAlongLook(-position);
        if (parentView && parentView->scene()) {
           parentView->scene()->update();
        }
    }

    //posive amt
    else if(look) {
        gl_camera.TranslateAlongLook(position);
        if (parentView && parentView->scene()) {
           parentView->scene()->update();
        }
    }
    //feet
    return Point3(gl_camera.eye.x, glm::floor(gl_camera.eye.y-1.5), gl_camera.eye.z);
}

Point3 MyGL::gravity(float time) {
    float d_speed = time * acceleration;
    character_velocity.y -= d_speed;
    if (character_velocity.y < -terminal_v) {
        character_velocity.y = -terminal_v;
    }
    float position = character_velocity.y * time;
    //qDebug() << position;
    gl_camera.TranslateAlongUp(position);
    if (parentView && parentView->scene()) {
       parentView->scene()->update();
    }
    return Point3(gl_camera.eye.x, glm::floor(gl_camera.eye.y-1.5), gl_camera.eye.z);
}

Point3 MyGL::moveCharacter(Point3 character) {
    float head = character.y + 0.5;
    float feet = character.y - 1.5;
    float left = character.x - 0.5;
    float right = character.x + 0.5;
    float front = character.z + 0.5;
    float back = character.z - 0.5;

    float floor_head = glm::floor(head);
    float floor_feet = glm::floor(feet);
    float floor_left = glm::floor(left);
    float floor_right = glm::floor(right);
    float floor_front = glm::floor(front);
    float floor_back = glm::floor(back);

    float new_x = character.x;
    float new_y = character.y;
    float new_z = character.z;

    //check front
    for (int i = floor_left; i <= floor_right; i++) {
        for (int j = floor_feet; j <= floor_head; j++) {
            if(scene.isFilled(Point3(i, j, floor_front))) {
                new_z = new_z - (front - floor_front);
                front = new_z + 0.5;
                floor_front = glm::floor(front);
                back = new_z - 0.5;
                floor_back = glm::floor(back);
                break;
            }
            if (scene.isFilled(Point3(i, j, floor_back))) {
                new_z = new_z + (glm::ceil(back) - back);
                front = new_z + 0.5;
                floor_front = glm::floor(front);
                back = new_z - 0.5;
                floor_back = glm::floor(back);
                break;
            }
        }
    }


    //check left
    for (int i = floor_feet; i <= floor_head; i++) {
        for (int j = floor_back; j <= floor_front; j++) {
            if (scene.isFilled(Point3(floor_left, i, j))) {
                new_x = new_x + (glm::ceil(left) - left);
                left = new_x - 0.5;
                floor_left = glm::floor(left);
                right = new_z + 0.5;
                floor_right = glm::floor(right);
                break;
            }
            if (scene.isFilled(Point3(floor_right, i, j))) {
                new_x = new_x - (left - floor_right);
                left = new_x - 0.5;
                floor_left = glm::floor(left);
                right = new_z + 0.5;
                floor_right = glm::floor(right);
                break;
            }
        }
    }


    //check head
    for (int i = floor_left; i <= floor_right; i++) {
        for (int j = floor_back; j <= floor_front; j++) {
            if(scene.isFilled(Point3(i, floor_head, j))) {
                new_y = new_y - (head - floor_head);
                head = new_y + 0.5;
                floor_head = glm::floor(head);
                feet = new_y - 1.5;
                floor_feet = glm::floor(feet);
                break;
            }
            if(scene.isFilled(Point3(i, floor_feet, j))) {
                new_y = new_y + (glm::ceil(feet) - feet);
                head = new_y + 0.5;
                floor_head = glm::floor(head);
                feet = new_y - 1.5;
                floor_feet = glm::floor(feet);
                break;
            }
        }
    }

    return Point3(new_x, new_y, new_z);
}

void MyGL::timerUpdate()
{

    // This function is called roughly 60 times per second.
    // Use it to update your scene and then tell it to redraw.
    // (Don't update your scene in paintGL, because it
    // sometimes gets called automatically by Qt.)

    animateTextures();//Animates the textures in the scene

    //1. Read key inputs and move accordingly, ignoring collisions
    //2. Apply gravity, ignoring collisions
    //3. Check if your character is inside the terrain and push him/her out


    glm::vec3 character = gl_camera.eye;
    Point3 feet = Point3(character.x, glm::floor(character.y-1.5), character.z);

    if (isGravity) {
        if (!scene.isFilled(feet)) {
            feet = gravity(0.0167f);
        }
        else {
            character_velocity.y = 0;
        }

    //collision in z
    //+amount
    if (inz) {
        feet = collisionZ(inz, 0.2f);
    }
    //-amt
    if (outz) {
        feet = collisionZ(!outz, 0.2f);
    }

    //collision in x
    if (rightx) {
        feet = collisionX(rightx, 0.2f);
    }
    if (leftx) {
        feet = collisionX(!leftx, 0.2f);
    }

    //collision in y
    if (downy) {
        feet = collisionY(!downy, 0.2f);
    }
    if (upy) {
        feet = collisionY(upy, 0.2f);
    }

    if (inz || outz || upy || downy || leftx || rightx) {
        Point3 new_eye = moveCharacter(Point3(gl_camera.eye.x, gl_camera.eye.y, gl_camera.eye.z));
        gl_camera.eye.x = new_eye.x;
        gl_camera.eye.y = new_eye.y;
        gl_camera.eye.z = new_eye.z;
        gl_camera.RecomputeAttributes();
        inz = false;
        outz = false;
        upy = false;
        downy = false;
        leftx = false;
        rightx = false;
    }
}



    if (parentView && parentView->scene()) {
       parentView->scene()->update();
    }
}
