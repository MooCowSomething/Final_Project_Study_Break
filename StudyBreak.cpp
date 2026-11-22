// Study Break - Interactive Student Desk
// C++11 and FreeGLUT only. Requires desktop OpenGL 2.1 (legacy compatibility).
// No external images or fonts are required.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstddef>


// Load the four required VBO functions after creating the OpenGL context.
// APIENTRY preserves the Windows calling convention (including 32-bit builds).
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
using GenBuffersProc = void (APIENTRY *)(GLsizei, GLuint*);
using BindBufferProc = void (APIENTRY *)(GLenum, GLuint);
using BufferDataProc = void (APIENTRY *)(GLenum, std::ptrdiff_t, const void*, GLenum);
using DeleteBuffersProc = void (APIENTRY *)(GLsizei, const GLuint*);
GenBuffersProc bufferGen = nullptr;
BindBufferProc bufferBind = nullptr;
BufferDataProc bufferUpload = nullptr;
DeleteBuffersProc bufferDelete = nullptr;

bool loadBufferFunctions() {
    bufferGen = reinterpret_cast<GenBuffersProc>(glutGetProcAddress("glGenBuffers"));
    bufferBind = reinterpret_cast<BindBufferProc>(glutGetProcAddress("glBindBuffer"));
    bufferUpload = reinterpret_cast<BufferDataProc>(glutGetProcAddress("glBufferData"));
    bufferDelete = reinterpret_cast<DeleteBuffersProc>(glutGetProcAddress("glDeleteBuffers"));
    return bufferGen && bufferBind && bufferUpload && bufferDelete;
}

const float PI = 3.14159265359f;
const int WIDTH = 1280, HEIGHT = 720;
float carX = 160, fanAngle = 0, noteScale = 1;
bool paused = false;
int lastTime = 0;
GLuint carVBO = 0;

void color(float r, float g, float b) { glColor3f(r,g,b); }
void rect(float x, float y, float w, float h) {
    glBegin(GL_TRIANGLES);
    glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h);
    glVertex2f(x,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}
void circle(float x, float y, float r) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x,y);
    for (int i=0; i<=64; ++i) {
        float a=2*PI*i/64;
        glVertex2f(x+r*std::cos(a),y+r*std::sin(a));
    }
    glEnd();
}
void line(float x1,float y1,float x2,float y2) {
    glBegin(GL_LINES); glVertex2f(x1,y1); glVertex2f(x2,y2); glEnd();
}
// Stroke text follows the same projection and transformations as the objects.
void text(float x,float y,const std::string& s,float size=16) {
    glPushMatrix();
    glTranslatef(x,y,0); glScalef(size/119.05f,size/119.05f,1);
    for (unsigned char c : s) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();
}
void ring(float x,float y,float r) {
    // Separate lines keep this drawing within the three selected primitive types.
    for (int i=0;i<64;++i) {
        float a=2*PI*i/64, b=2*PI*(i+1)/64;
        line(x+r*std::cos(a),y+r*std::sin(a),
             x+r*std::cos(b),y+r*std::sin(b));
    }
}

void background() {
    color(.95f,.92f,.85f); rect(0,0,1280,720);
    color(.89f,.85f,.76f); rect(0,290,1280,14);
    // Window and a simple outdoor view.
    color(.72f,.57f,.43f); rect(75,365,360,225);
    color(.67f,.84f,.88f); rect(87,377,336,201);
    color(1,.88f,.52f); circle(365,533,26);
    color(.96f,.97f,.92f);
    circle(155,524,21); circle(180,535,28); circle(210,524,22);
    color(.53f,.68f,.48f); rect(87,377,336,33);
    color(.97f,.95f,.88f); rect(248,377,10,201); rect(87,468,336,10);
    // Wall poster.
    color(.85f,.81f,.72f); rect(495,425,185,155);
    color(.99f,.98f,.93f); rect(488,432,185,155);
    color(.26f,.40f,.39f); text(515,545,"ONE TASK",21);
    text(521,510,"AT A TIME",19);
    color(.80f,.52f,.33f); rect(522,479,114,4);
    color(.39f,.43f,.40f); text(518,452,"You got this.",15);
    // Desk: top, front edge, legs, and floor.
    color(.76f,.73f,.65f); rect(0,0,1280,160);
    color(.40f,.29f,.23f); rect(105,105,40,100); rect(1135,105,40,100);
    color(.76f,.54f,.36f); rect(40,205,1200,110);
    color(.55f,.36f,.25f); rect(40,185,1200,23);
    color(.85f,.64f,.44f); line(60,295,1220,295);
    line(60,225,1220,225);
    // Books and a pencil cup on the back of the desk.
    color(.34f,.49f,.48f); rect(470,315,155,22);
    color(.97f,.93f,.83f); rect(477,320,141,10);
    color(.74f,.40f,.32f); rect(485,337,130,20);
    color(.97f,.93f,.83f); rect(491,342,118,9);
    color(.93f,.69f,.28f); rect(665,350,7,78);
    color(.39f,.55f,.58f); rect(685,350,7,87);
    color(.72f,.39f,.34f); rect(705,350,7,69);
    color(.48f,.57f,.49f); rect(652,315,73,65);
    color(.37f,.45f,.39f); rect(652,370,73,10);
}

void initializeVBO() {
    // VBO: car body uploaded ONCE. Translation changes its model matrix.
    const GLfloat vertices[] = {
        -65,0, 65,0, 65,35, -65,0, 65,35, -65,35,
        -40,35, 42,35, 20,63, -40,35, 20,63, -23,63
    };
    bufferGen(1,&carVBO);
    bufferBind(GL_ARRAY_BUFFER,carVBO);
    bufferUpload(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    bufferBind(GL_ARRAY_BUFFER,0);
}
void drawCar() {
    glPushMatrix(); // Isolate translation from the fan, note, and background.
    glTranslatef(carX,249,0);
    color(.79f,.24f,.20f);
    bufferBind(GL_ARRAY_BUFFER,carVBO);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2,GL_FLOAT,0,nullptr); // Offset zero into the bound VBO.
    glDrawArrays(GL_TRIANGLES,0,12);
    glDisableClientState(GL_VERTEX_ARRAY);
    bufferBind(GL_ARRAY_BUFFER,0);
    color(.75f,.89f,.90f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-30,38); glVertex2f(-5,38); glVertex2f(-5,57);
    glVertex2f(-30,38); glVertex2f(-5,57); glVertex2f(-20,57);
    glVertex2f(2,38); glVertex2f(31,38); glVertex2f(16,57);
    glVertex2f(2,38); glVertex2f(16,57); glVertex2f(2,57);
    glEnd();
    color(1,.86f,.53f); rect(54,15,11,10);
    color(.17f,.20f,.22f); circle(-40,0,16); circle(40,0,16);
    color(.80f,.82f,.79f); circle(-40,0,7); circle(40,0,7);
    glPopMatrix();
}
void drawFan() {
    glPushMatrix();
    glTranslatef(860,438,0);
    color(.24f,.39f,.44f); rect(-11,-110,22,110); rect(-61,-123,122,18);
    color(.85f,.91f,.89f); circle(0,0,87);
    // Only the blades rotate; the stand and cage remain stationary.
    glPushMatrix();
    glRotatef(fanAngle,0,0,1);
    color(.25f,.57f,.64f);
    for (int i=0;i<3;++i) {
        glPushMatrix(); glRotatef(i*120.0f,0,0,1);
        glBegin(GL_TRIANGLES);
        glVertex2f(-8,0); glVertex2f(64,30); glVertex2f(43,62);
        glEnd(); glPopMatrix();
    }
    glPopMatrix();
    color(.35f,.47f,.49f); ring(0,0,87); ring(0,0,76);
    for(int i=0;i<8;++i) {
        float a=i*PI/4;
        line(0,0,87*std::cos(a),87*std::sin(a));
    }
    color(.23f,.37f,.41f); circle(0,0,16);
    color(.86f,.88f,.81f); circle(0,0,7);
    glPopMatrix();
}
void drawNote() {
    // CPU-side Vertex Array (distinct from the car's GPU-side VBO).
    const GLfloat noteVertices[] = {
        -60,-60, 60,-60, 60,60, -60,-60, 60,60, -60,60
    };
    glPushMatrix();
    glTranslatef(1090,440,0);
    glScalef(noteScale,noteScale,1); // Scale about the note's center.
    color(.82f,.76f,.59f); rect(-54,-66,120,120);
    color(1,.88f,.43f);
    bufferBind(GL_ARRAY_BUFFER,0); // Pointer below is CPU memory.
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2,GL_FLOAT,0,noteVertices);
    glDrawArrays(GL_TRIANGLES,0,6);
    glDisableClientState(GL_VERTEX_ARRAY);
    color(.94f,.77f,.32f); rect(-60,40,120,20);
    color(.36f,.32f,.22f); text(-43,17,"TO DO",17);
    text(-43,-9,"Study",13); text(-43,-31,"Take a break",10);
    color(.67f,.55f,.29f); line(-43,-16,43,-16); line(-43,-39,43,-39);
    glPopMatrix();
}
void interfacePanel() {
    color(.17f,.26f,.28f); rect(0,620,1280,100);
    color(.99f,.95f,.84f); text(45,670,"STUDY BREAK",34);
    color(.69f,.81f,.79f); text(46,640,"Interactive Student Desk",17);
    color(.98f,.87f,.59f); text(765,668,"MOVE. ROTATE. RESIZE.",21);
    color(.80f,.85f,.80f); text(765,640,"A little pause between lessons.",16);
    color(.17f,.26f,.28f); rect(40,20,1200,105);
    color(.98f,.87f,.59f); text(62,94,"CONTROLS",17);
    color(.94f,.94f,.87f);
    text(62,65,"Left click: rotate fan",16);
    text(405,65,"Scroll: resize sticky note",16);
    text(820,65,"Space: pause car",16);
    text(62,38,"R: reset",14); text(240,38,"F: full screen / window",14);
    text(570,38,"Esc: exit",14);
    char status[100];
    std::snprintf(status,sizeof(status),"Car: %s  |  Note: %.0f%%",paused?"PAUSED":"MOVING",noteScale*100);
    text(820,38,status,14);
}
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    background(); drawFan(); drawNote(); drawCar(); interfacePanel();
    glutSwapBuffers();
}
void reshape(int w,int h) {
    // Letterboxing preserves circles and object proportions on any monitor.
    w=std::max(w,1); h=std::max(h,1);
    float scale=std::min(w/float(WIDTH),h/float(HEIGHT));
    int vw=int(WIDTH*scale), vh=int(HEIGHT*scale);
    glViewport((w-vw)/2,(h-vh)/2,vw,vh);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(0,WIDTH,0,HEIGHT,-1,1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}
void timer(int) {
    int now=glutGet(GLUT_ELAPSED_TIME);
    float dt=std::min((now-lastTime)/1000.0f,0.1f);
    lastTime=now;
    if(!paused) {
        carX+=110*dt;
        if(carX>1170) carX=110;
    }
    glutPostRedisplay();
    glutTimerFunc(16,timer,0); // Request next update in approximately 16 ms.
}
void mouse(int button,int state,int,int) {
    if(button==GLUT_LEFT_BUTTON && state==GLUT_DOWN) {
        fanAngle=std::fmod(fanAngle+30,360.0f);
        glutPostRedisplay();
    }
}
void wheel(int wheelNumber,int direction,int,int) {
    if(wheelNumber!=0) return;
    noteScale=std::max(.6f,std::min(1.6f,noteScale+(direction>0?.1f:-.1f)));
    glutPostRedisplay();
}
void keyboard(unsigned char key,int,int) {
    switch(key) {
        case 27: glutLeaveMainLoop(); return;
        case ' ': paused=!paused; break;
        case 'r': case 'R': carX=160; fanAngle=0; noteScale=1; paused=false; break;
        case 'f': case 'F': glutFullScreenToggle(); break;
    }
    glutPostRedisplay();
}
void cleanup() {
    if(carVBO) { bufferDelete(1,&carVBO); carVBO=0; }
}
int main(int argc,char** argv) {
    glutInit(&argc,argv);
    glutInitContextVersion(2,1); // Legacy APIs require a compatibility context.
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(WIDTH,HEIGHT);
    glutCreateWindow("Study Break - Interactive Student Desk");
    if(!loadBufferFunctions()) {
        std::fprintf(stderr,"Required OpenGL VBO functions are unavailable.\n"
                            "Use a graphics driver supporting desktop OpenGL 2.1.\n");
        glutDestroyWindow(glutGetWindow());
        return EXIT_FAILURE;
    }
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glClearColor(.12f,.17f,.18f,1);
    initializeVBO();
    glutDisplayFunc(display); glutReshapeFunc(reshape);
    glutMouseFunc(mouse); glutMouseWheelFunc(wheel); glutKeyboardFunc(keyboard);
    glutCloseFunc(cleanup);
    lastTime=glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16,timer,0);
    glutFullScreen();
    glutMainLoop();
    return EXIT_SUCCESS;
}
