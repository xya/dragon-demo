#include <GL/gl.h>
#include <ctime>
#include <cmath>
#include <iostream>
#include <QVector3D>
#include <QMatrix4x4>
#include <QKeyEvent>
#include "DragonScene.h"
#include "Dragon.h"
#include "Mesh.h"
#include "Letters.h"
#include "Images.h"

using namespace std;

static Material metal_material(QVector4D(0.2, 0.2, 0.2, 1.0),
    QVector4D(0.4, 0.4, 0.4, 1.0), QVector4D(0.2, 0.2, 0.2, 1.0), 20.0);

static Material red_material(QVector4D(0.2, 0.0, 0.0, 1.0),
    QVector4D(1.0, 0.0, 0.0, 1.0), QVector4D(0.0, 0.0, 0.0, 1.0), 20.0);

static Material green_material(QVector4D(0.0, 0.2, 0.0, 1.0),
    QVector4D(0.0, 1.0, 0.0, 1.0), QVector4D(0.0, 0.2, 0.0, 1.0), 20.0);

static Material blue_material(QVector4D(0.0, 0.0, 0.2, 1.0),
    QVector4D(0.0, 0.0, 0.1, 1.0), QVector4D(0.0, 0.0, 0.2, 1.0), 20.0);

static Material yellow_material(QVector4D(0.2, 0.2, 0.2, 1.0),
    QVector4D(1.0, 4.0/6.0, 0.0, 1.0), QVector4D(0.2, 0.2, 0.2, 1.0), 20.0);

static Material glass_material(QVector4D(0.2, 0.2, 0.2, 0.2),
    QVector4D(1.0, 1.0, 1.0, 0.2), QVector4D(0.2, 0.2, 0.2, 0.2), 20.0);

static Material debug_material(QVector4D(0.2, 0.2, 0.2, 1.0),
    QVector4D(1.0, 4.0/6.0, 0.0, 1.0), QVector4D(0.2, 0.2, 0.2, 1.0), 20.0);

static Material floor_material(QVector4D(0.5, 0.5, 0.5, 1.0),
    QVector4D(1.0, 1.0, 1.0, 1.0), QVector4D(1.0, 1.0, 1.0, 1.0), 00.0, true);

static Material tongue_material(QVector4D(0.1, 0.0, 0.0, 1.0),
    QVector4D(0.6, 0.0, 0.0, 1.0), QVector4D(1.0, 1.0, 1.0, 1.0), 50.0);

static Material scales_material(QVector4D(0.2, 0.2, 0.2, 1.0),
    QVector4D(0.8, 0.8, 0.8, 1.0), QVector4D(1.0, 1.0, 1.0, 1.0), 20.0);

static Material wing_material(QVector4D(0.2, 0.2, 0.2, 1.0),
    QVector4D(1.0, 1.0, 1.0, 1.0), QVector4D(1.0, 1.0, 1.0, 1.0), 20.0);

static Material wing_membrane_material(QVector4D(0.1, 0.0, 0.0, 1.0),
    QVector4D(0.6, 0.0, 0.0, 1.0), QVector4D(0.2, 0.2, 0.2, 1.0), 20.0);

DragonScene::DragonScene(QObject *parent) : Scene(parent)
{
    m_loadedMeshes = 0;
    m_missingMeshes = 0;

    //s->floor = create_cube();
    //floor_material.texture = textureFromTIFFImage("lava_green.tiff", 1);

    for(int i = 0; i < 3; i++)
    {
        Dragon *d = new Dragon(this);
        d->default_material = scales_material;
        d->tongue_material = tongue_material;
        d->wing_material = wing_material;
        d->wing_membrane_material = wing_membrane_material;
        m_dragons.append(d);
    }
    m_dragons.value(0)->default_material.setTexture(textureFromTIFFImage("scale_green.tiff", 0));
    m_dragons.value(0)->wing_material.setTexture(textureFromTIFFImage("wing_green.tiff", 0));
    m_dragons.value(1)->default_material.setTexture(textureFromTIFFImage("scale_black.tiff", 0));
    m_dragons.value(1)->wing_material.setTexture(textureFromTIFFImage("wing_black.tiff", 0));
    m_dragons.value(2)->default_material.setTexture(textureFromTIFFImage("scale_bronze.tiff", 0));
    m_dragons.value(2)->wing_material.setTexture(textureFromTIFFImage("wing_bronze.tiff", 0));
}

DragonScene::~DragonScene()
{
    while(m_dragons.count() > 0)
        delete m_dragons.takeFirst();
}

void DragonScene::reset()
{
    m_selected = SCENE;
    m_drawNormals = false;
    m_detailLevel = 3;
    emit invalidated();
}

bool DragonScene::load()
{
    /*if(meshLoaded())
        return true;
    reset();
    return meshLoaded();*/
    return Letters::initMeshes();
}

Mesh * DragonScene::loadMesh(QString path)
{
    Mesh *m = Mesh::load_stl(path.toUtf8().constData(), this);
    if(m)
        m_loadedMeshes++;
    else
        m_missingMeshes++;
    return m;
}

bool DragonScene::meshLoaded() const
{
    return Letters::meshLoaded();
}

void DragonScene::draw()
{
    if(!meshLoaded())
        return;
    int item = m_selected;
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    if(item == SCENE)
    {
        Dragon *da = m_dragons[0];
        glPushMatrix();
            glTranslatef(0.0, 2.0 + 0.6 * da->alpha, 0.0);
            glScalef(3.0, 3.0, 3.0);
            da->setDetailLevel(m_detailLevel);
            drawDragonHoldingA(da);
        glPopMatrix();

        Dragon *dp = m_dragons[1];
        glPushMatrix();
            glTranslatef(-dp->beta, dp->beta, dp->beta);
            glRotatef(dp->alpha, 0.0, 1.0, 0.0);
            glTranslatef(4.0, 0.0, 4.0);
            glRotatef(60.0, 0.0, 1.0, 0.0);
            glScalef(1.5, 1.5, 1.5);
            dp->setDetailLevel(m_detailLevel);
            drawDragonHoldingP(dp);
        glPopMatrix();

        Dragon *ds = m_dragons[2];
        glPushMatrix();
            glTranslatef(0.0, ds->beta, 0.0);
            glRotatef(-ds->alpha, 0.0, 1.0, 0.0);
            glTranslatef(3.0, 0.0, 3.0);
            glRotatef(-120.0, 0.0, 1.0, 0.0);
            glScalef(1.5, 1.5, 1.5);
            ds->setDetailLevel(m_detailLevel);
            drawDragonHoldingS(ds);
        glPopMatrix();
    }
    else
    {
        debug_material.beginApply();
        switch(item)
        {
        case LETTER_P:
            Letters::drawP(this);
            break;
        case LETTER_A:
            Letters::drawA(this);
            break;
        case LETTER_S:
            Letters::drawS(this);
            break;
        }

        debug_material.endApply();
    }
    glPopMatrix();
}

void DragonScene::drawDragonHoldingA(Dragon *d)
{
    glPushMatrix();
        glPushMatrix();
            glRotatef(45.0, 0.0, 0.0, 1.0);
            d->draw();
        glPopMatrix();
        glPushMatrix();
            glTranslatef(1.0/3.0, 0.2/3.0, 0.0);
            glRotatef(15.0, 0.0, 1.0, 0.0);
            glRotatef(-d->theta_front_legs, 0.0, 0.0, 1.0);
            glScalef(2.0/3.0, 2.0/3.0, 1.0/3.0);
            tongue_material.beginApply();
            Letters::drawA(this);
            tongue_material.endApply();
        glPopMatrix();
    glPopMatrix();
}

void DragonScene::drawDragonHoldingP(Dragon *d)
{
    glPushMatrix();
        d->draw();
        glPushMatrix();
            glTranslatef(0.08, -0.13, 0.0);
            glRotatef(-d->theta_front_legs + 90.0, 0.0, 0.0, 1.0);
            glTranslatef(0.2, -0.1, 0.0);
            glRotatef(-170, 0.0, 0.0, 1.0);
            glScalef(1.0, 1.0, 0.5);
            tongue_material.beginApply();
            Letters::drawP(this);
            tongue_material.endApply();
        glPopMatrix();
    glPopMatrix();
}

void DragonScene::drawDragonHoldingS(Dragon *d)
{
    glPushMatrix();
        d->draw();
        glPushMatrix();
            glTranslatef(0.26, -0.25, 0.0);
            glRotatef(180.0 - d->theta_front_legs, 0.0, 0.0, 1.0);
            // need to change the center of the rotation
            glTranslatef(-0.4, 0.1, 0.0);
            glScalef(1.0, 1.0, 0.5);
            tongue_material.beginApply();
            Letters::drawS(this);
            tongue_material.endApply();
        glPopMatrix();
    glPopMatrix();
}

void DragonScene::select_next()
{
    if(m_selected < LAST)
    {
        m_selected++;
        emit invalidated();
    }
}

void DragonScene::select_previous()
{
    if(m_selected > SCENE)
    {
        m_selected--;
        emit invalidated();
    }
}

void DragonScene::keyReleaseEvent(QKeyEvent *e)
{
    int key = e->key();
    if((key == Qt::Key_Plus) || (key == Qt::Key_Right))
        select_next();
    else if((key == Qt::Key_Minus)  || (key == Qt::Key_Left))
        select_previous();
    else if(key == Qt::Key_N)
        m_drawNormals = !m_drawNormals;
}

void DragonScene::drawMesh(Mesh *m)
{
    if(!m)
        return;
    m->draw();
    if(m_drawNormals)
        m->draw_normals();
}

void DragonScene::animate()
{
    clock_t ticks = clock();
    float t = (float)ticks / (float)CLOCKS_PER_SEC;

    double angle = fmod(t * 45.0, 360.0);

    // hovering dragon
    animateDragon(m_dragons[0], t);
    m_dragons[0]->alpha = cos(t * 3.5 + M_PI);
    m_dragons[0]->theta_head_z = -45.0;
    m_dragons[0]->theta_paw = 60.0;

    // drunk dragon trying to fly clockwise
    animateDragon(m_dragons[1], t);
    m_dragons[1]->alpha = angle;
    m_dragons[1]->beta = cos(t * 3.5) * cos(t) * cos(t);
    m_dragons[1]->theta_head_z = -30.0;
    m_dragons[1]->theta_neck = 30.0;
    m_dragons[1]->theta_paw = 60.0;

    // dragon jumping anticlockwise
    animateDragon(m_dragons[2], t);
    m_dragons[2]->alpha = angle;
    m_dragons[2]->beta = 1.20 * sqrt(fabs(cos(5.0 * t) - cos(6.0 * t) + cos(7.0 * t)));
    m_dragons[2]->theta_wing = 0.0;
    m_dragons[2]->theta_wing_joint = 20.0;
    m_dragons[2]->theta_neck = 30.0;
    m_dragons[2]->theta_paw = 60.0;
    m_dragons[2]->theta_neck = 30.0;
    m_dragons[2]->theta_paw = 60.0;
    // this one is definitely having the time of its life
    m_dragons[2]->theta_head_z = 60.0 * spaced_cos(t, 1.0, 2.0) - 30.0;
    m_dragons[2]->theta_jaw = 10.0 * spaced_cos(t, 1.0, 2.0) + 10.0;

    /*switch(s->camera)
    {
        default:
        case CAMERA_STATIC:
            s->theta[1] = 0.0;			// static camera
            break;
        case CAMERA_JUMPING:
            s->theta[1] = angle;		// following jumping dragon
            break;
        case CAMERA_FLYING:
            s->theta[1] = -angle;		// following drunk dragon
    }*/

    emit invalidated();
}

// Periodic function linearly going from 0 to 1
float DragonScene::sawtooth(float t)
{
    return t - (float)floor(t);
}

// Periodic function which returns 0 for w seconds then returns 1 for a seconds
float DragonScene::spaced_rect(float t, float w, float a)
{
    return (sawtooth(t / (w + a)) > w / (w + a)) ? 1.0 : 0.0;
}

// Periodic function which returns 0 for w seconds then is the sawtooth function for a seconds
float DragonScene::spaced_sawtooth(float x, float w, float a)
{
    return spaced_rect(x, w, a) * sawtooth((x - w) / (w + a)) * ((w + a) / a);
}

// Periodic function which returns 0 for w seconds then is the cosine function for a seconds
float DragonScene::spaced_cos(float x, float w, float a)
{
    return cos(2.0 * M_PI * spaced_sawtooth(x, w, a) + M_PI / 2.0);
}

void DragonScene::animateDragon(Dragon *d, float t)
{
    d->theta_jaw = 10.0 * spaced_cos(t, 5.0, 2.0) + 10.0;
    d->theta_head_y = 45.0 * spaced_cos(t, 5.0, 2.0);
    d->theta_neck = 5.0 * cos(t * 3.0);
    d->theta_wing = 45.0 * cos(t * 3.5);
    d->theta_wing_joint = 60.0 - 30.0 * fabs(cos(t * 3.5) * cos(t));
    d->theta_front_legs = 10.0 * cos(t * 3.0) + 40.0 + 45.0;
    d->theta_back_legs = 10.0 * cos(t * 3.0) + 80.0 + 45.0;
    d->theta_tail = 15.0 * cos(pow(t * 0.3, 2.0)) * cos(6.0 * t * 0.3);
}
