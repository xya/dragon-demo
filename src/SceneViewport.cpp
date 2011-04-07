#include <cmath>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QVector4D>
#include "SceneViewport.h"
#include "Scene.h"
#include "Material.h"
#include "RenderState.h"

SceneViewport::SceneViewport(const QGLFormat &format, QWidget *parent) : QGLWidget(format, parent)
{
    setMinimumSize(640, 480);
    m_scene = 0;
    m_state = new RenderState(this);
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(1000 / 60);
    m_start = 0;
    m_frames = 0;
    m_lastFPS = 0;
    m_fpsTimer = new QTimer(this);
    m_fpsTimer->setInterval(1000 / 6);
    //m_bgColor = palette().color(QPalette::Background);
    setAutoFillBackground(false);
    m_bgColor = QColor::fromRgbF(0.6, 0.6, 1.0, 1.0);
    m_ambient0 = QVector4D(1.0, 1.0, 1.0, 1.0);
    m_diffuse0 = QVector4D(1.0, 1.0, 1.0, 1.0);
    m_specular0 = QVector4D(1.0, 1.0, 1.0, 1.0);
    m_light0_pos = QVector4D(0.0, 1.0, 1.0, 0.0);
    connect(m_fpsTimer, SIGNAL(timeout()), this, SLOT(updateFPS()));
}

SceneViewport::~SceneViewport()
{
    if(m_scene)
        m_scene->freeTextures();
}

RenderState *SceneViewport::state() const
{
    return m_state;
}

Scene* SceneViewport::scene() const
{
    return m_scene;
}

void SceneViewport::setScene(Scene *newScene)
{
    if(m_scene)
    {
        m_scene->freeTextures();
        disconnect(m_scene, SIGNAL(invalidated()), this, SLOT(update()));
        disconnect(m_renderTimer, SIGNAL(timeout()), m_scene, SLOT(animate()));
    }
    m_scene = newScene;
    if(newScene)
    {
        if(context())
        {
            makeCurrent();
            newScene->loadTextures();
        }
        connect(newScene, SIGNAL(invalidated()), this, SLOT(update()));
        connect(m_renderTimer, SIGNAL(timeout()), newScene, SLOT(animate()));
    }
    update();
}

void SceneViewport::initializeGL()
{
    if(m_scene)
        m_scene->loadTextures();
    reset_camera();
}

void SceneViewport::resizeGL(int w, int h)
{
    setupGLViewport(w, h);
}

void SceneViewport::setupGLState()
{
    glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT);
    glEnable(GL_DEPTH_TEST);
    // we do non-uniform scaling and not all normals are one-unit-length
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, (GLfloat *)&m_light0_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, (GLfloat *)&m_ambient0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, (GLfloat *)&m_diffuse0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, (GLfloat *)&m_specular0);
    glPolygonMode(GL_FRONT_AND_BACK, m_wireframe_mode ? GL_LINE : GL_FILL);
    setupGLViewport(width(), height());
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
}

void SceneViewport::restoreGLState()
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
}

void SceneViewport::setupGLViewport(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if(m_projection_mode)
    {
        gluPerspective(45.0f, (GLfloat)w / (GLfloat)h, 0.1f, 100.0f);
    }
    else
    {
        if (w <= h)
            glOrtho(-1.0, 1.0, -1.0 * (GLfloat) h / (GLfloat) w,
                1.0 * (GLfloat) h / (GLfloat) w, -10.0, 10.0);
        else
            glOrtho(-1.0 * (GLfloat) w / (GLfloat) h,
                1.0 * (GLfloat) w / (GLfloat) h, -1.0, 1.0, -10.0, 10.0);
    }
    glMatrixMode(GL_MODELVIEW);
}

void SceneViewport::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    paintGL();
    m_frames++;
    if(m_fpsTimer->isActive())
        paintFPS(&painter, m_lastFPS);
}

void SceneViewport::paintGL()
{
    setupGLState();
    glClearColor(m_bgColor.redF(), m_bgColor.greenF(), m_bgColor.blueF(), 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // determine which rotation to apply from both the user and the scene
    QVector3D rot = m_theta;
    if(m_scene)
        rot += m_scene->orientation();
    m_state->setMatrixMode(RenderState::ModelView);
    m_state->pushMatrix();
        glLoadIdentity();
        glTranslatef(m_delta.x(), m_delta.y(), m_delta.z());
        glRotatef(rot.x(), 1.0, 0.0, 0.0);
        glRotatef(rot.y(), 0.0, 1.0, 0.0);
        glRotatef(rot.z(), 0.0, 0.0, 1.0);
        glScalef(m_sigma, m_sigma, m_sigma);
        if(m_scene)
            m_scene->draw();
        if(m_draw_axes)
            draw_axes();
        if(m_draw_grids)
            draw_axis_grids(true, true, true);
    m_state->popMatrix();
    glFlush();
    restoreGLState();
}

void SceneViewport::draw_axis()
{
    glLineWidth(3.0);
    glBegin(GL_LINES);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(0.8, 0.0, 0.0);
    glEnd();
    glLineWidth(1.0);

    glPushMatrix();
        glTranslatef(0.8, 0.0, 0.0);
        glRotatef(90.0, 0.0, 1.0, 0.0);
        //glutSolidCone(0.04, 0.2, 10, 10);
    glPopMatrix();
}

void SceneViewport::draw_axes()
{
    Material m;
    glPushMatrix();
        // X axis
        m.setAmbient(QVector4D(1.0, 0.0, 0.0, 1.0));
        m.beginApply();
        draw_axis();
        m.endApply();
        // Y axis
        m.setAmbient(QVector4D(0.0, 1.0, 0.0, 1.0));
        m.beginApply();
        glRotatef(90.0, 0.0, 0.0, 1.0);
        draw_axis();
        m.endApply();
        // Z axis
        m.setAmbient(QVector4D(0.0, 0.0, 1.0, 1.0));
        m.beginApply();
        glRotatef(-90.0, 0.0, 1.0, 0.0);
        draw_axis();
        m.endApply();
    glPopMatrix();
}

void SceneViewport::draw_axis_grids(bool draw_x, bool draw_y, bool draw_z)
{
    static Material m(QVector4D(0.0, 0.0, 0.0, 1.0), QVector4D(0.0, 0.0, 0.0, 1.0),
                      QVector4D(0.0, 0.0, 0.0, 0.0), 0.0);

    m.beginApply();
    if(draw_x)
    {
        draw_axis_grid();
    }

    if(draw_y)
    {
        glPushMatrix();
            glRotatef(-90.0, 1.0, 0.0, 0.0);
            draw_axis_grid();
        glPopMatrix();
    }

    if(draw_z)
    {
        glPushMatrix();
            glRotatef(90.0, 0.0, 1.0, 0.0);
            glRotatef(180.0, 0.0, 0.0, 1.0);
            draw_axis_grid();
        glPopMatrix();
    }
    m.endApply();
}

void SceneViewport::draw_axis_grid()
{
    glLineStipple(1, 0xAAAA); //line style = fine dots
    glEnable(GL_LINE_STIPPLE);
    glBegin(GL_LINES);
    for(int i = -10; i <= 10; i++)
    {
        if(i != 0)
        {
            glVertex3f(-1.0 * i, -10.0, 0.0);
            glVertex3f(-1.0 * i, 10.0, 0.0);
            glVertex3f(-10.0, -1.0 * i, 0.0);
            glVertex3f(10.0, -1.0 * i, 0.0);
        }
    }
    glEnd();
    glDisable(GL_LINE_STIPPLE);

    glLineWidth(3.0);
    glBegin(GL_LINES);
        glVertex3f(0.0, -10.0, 0.0);
        glVertex3f(0.0, -1.0, 0.0);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(0.0, 10.0, 0.0);
        glVertex3f(-10.0, 0.0, 0.0);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(1.0, 0.0, 0.0);
        glVertex3f(10.0, 0.0, 0.0);
    glEnd();
    glLineWidth(1.0);
}

void SceneViewport::reset_camera()
{
    m_delta = QVector3D(-0.0, -0.5, -5.0);
    m_theta = QVector3D(21.0, -37.0, 0.0);
    m_trans_state.last = QVector3D();
    m_rot_state.last = QVector3D();
    m_trans_state.active = false;
    m_rot_state.active = false;
    m_sigma = 0.40;
    m_animate = true;
    m_draw_axes = false;
    m_draw_grids = false;
    m_wireframe_mode = false;
    m_projection_mode = true;
    m_state->reset();
    if(m_scene)
        m_scene->reset();
    updateAnimationState();
}

void SceneViewport::toggle_animation()
{
    m_animate = !m_animate;
    if(m_animate)
        m_renderTimer->start();
    else
        m_renderTimer->stop();
}

void SceneViewport::top_view()
{
    m_theta = QVector3D(0.0, 0.0, 0.0);
}

void SceneViewport::side_view()
{
    m_theta = QVector3D(-90.0, 0.0, -90.0);
}

void SceneViewport::front_view()
{
    m_theta = QVector3D(-90.0, 0.0, 0.0);
}

void SceneViewport::startFPS()
{
    m_start = clock();
}

void SceneViewport::updateFPS()
{
    clock_t elapsed = clock() - m_start;
    m_lastFPS = m_frames / ((float)elapsed / CLOCKS_PER_SEC);
    m_frames = 0;
    m_start = clock();
}

void SceneViewport::paintFPS(QPainter *p, float fps)
{
    QFont f;
    f.setPointSizeF(16.0);
    f.setWeight(QFont::Bold);
    p->setFont(f);
    QString text = QString("%1 FPS").arg(fps, 0, 'g', 3);
    p->setPen(QPen(Qt::white));
    p->drawText(QRectF(QPointF(10, 5), QSizeF(100, 100)), text);
}

void SceneViewport::updateAnimationState()
{
    if(m_animate)
    {
        startFPS();
        m_renderTimer->start();
        m_fpsTimer->start();
    }
    else
    {
        m_renderTimer->stop();
        m_fpsTimer->stop();
    }
}

void SceneViewport::keyReleaseEvent(QKeyEvent *e)
{
    int key = e->key();
    if(key == Qt::Key_Q)
        m_theta.setY(m_theta.y() + 5.0);
    else if(key == Qt::Key_D)
        m_theta.setY(m_theta.y() - 5.0);
    else if(key == Qt::Key_2)
        m_theta.setX(m_theta.x() + 5.0);
    else if(key == Qt::Key_8)
        m_theta.setX(m_theta.x() - 5.0);
    else if(key == Qt::Key_4)
        m_theta.setZ(m_theta.z() + 5.0);
    else if(key == Qt::Key_6)
        m_theta.setZ(m_theta.z() - 5.0);
    else if(key == Qt::Key_R)
        reset_camera();
    else if(key == Qt::Key_Period)
        m_draw_axes = !m_draw_axes;
    else if(key == Qt::Key_G)
        m_draw_grids = !m_draw_grids;
    else if(key == Qt::Key_Z)
        m_wireframe_mode = !m_wireframe_mode;
    else if(key == Qt::Key_P)
        m_projection_mode = !m_projection_mode;
    else if(key == Qt::Key_Space)
		toggle_animation();
	else if(key == Qt::Key_7)
		top_view();
	else if(key == Qt::Key_3)
		side_view();
	else if(key == Qt::Key_1)
		front_view();
    else if(m_scene)
        m_scene->keyReleaseEvent(e);
    QGLWidget::keyReleaseEvent(e);
    update();
}

void SceneViewport::mouseMoveEvent(QMouseEvent *e)
{
    int x = e->x();
    int y = e->y();
    
    if(m_trans_state.active)
    {
        int dx = m_trans_state.x0 - x;
        int dy = m_trans_state.y0 - y;
        m_delta.setX(m_trans_state.last.x() - (dx / 100.0));
        m_delta.setY(m_trans_state.last.y() + (dy / 100.0));
        update();
    }
    
    if(m_rot_state.active)
    {
        int dx = m_rot_state.x0 - x;
        int dy = m_rot_state.y0 - y;
        m_theta.setX(m_rot_state.last.x() + (dy * 2.0));
        m_theta.setY(m_rot_state.last.y() + (dx * 2.0));
        update();
    }
}

void SceneViewport::mousePressEvent(QMouseEvent *e)
{
    int x = e->x();
    int y = e->y();
    if(e->button() & Qt::MiddleButton)       // middle button pans the scene
    {
        m_trans_state.active = true;
        m_trans_state.x0 = x;
        m_trans_state.y0 = y;
        m_trans_state.last = m_delta;
    }
    else if(e->button() & Qt::LeftButton)   // left button rotates the scene
    {
        m_rot_state.active = true;
        m_rot_state.x0 = x;
        m_rot_state.y0 = y;
        m_rot_state.last = m_theta;
		setFocus();
    }
    else
    {
        e->ignore();
        QGLWidget::mousePressEvent(e);
        return;
    }
    update();
}

void SceneViewport::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() & Qt::MiddleButton)
        m_trans_state.active = false;
    else if(e->button() & Qt::LeftButton)
        m_rot_state.active = false;
    else
    {
        e->ignore();
        QGLWidget::mouseReleaseEvent(e);
        return;
    }
    update();
}

void SceneViewport::wheelEvent(QWheelEvent *e)
{
    // mouse wheel up zooms towards the scene
    // mouse wheel down zooms away from scene
    m_sigma *= pow(1.01, e->delta() / 8);
    update();
}
