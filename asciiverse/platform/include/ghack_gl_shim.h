/*
 * Asciiverse terminal edition: no OpenGL. The calls Goblin Hack makes are
 * no-ops here; the terminal draws the game from its own state instead
 * (asciiverse/platform/terminal.cpp).
 *
 * Copyright (C) 2026 David Sandquist. GNU Library GPL v2 or later, as the game.
 */
#ifndef GHACK_GL_SHIM_H
#define GHACK_GL_SHIM_H

typedef unsigned int   GLenum;
typedef unsigned int   GLuint;
typedef int            GLint;
typedef int            GLsizei;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef unsigned char  GLubyte;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef void           GLvoid;

#define GL_NO_ERROR                     0
#define GL_LINES                        0x0001
#define GL_TRIANGLE_STRIP               0x0005
#define GL_TRIANGLE_FAN                 0x0006
#define GL_QUADS                        0x0007
#define GL_COMPILE                      0x1300
#define GL_DEPTH_BUFFER_BIT             0x00000100
#define GL_COLOR_BUFFER_BIT             0x00004000
#define GL_BACK_LEFT                    0x0402
#define GL_DEPTH_TEST                   0x0B71
#define GL_BLEND                        0x0BE2
#define GL_SCISSOR_TEST                 0x0C11
#define GL_PERSPECTIVE_CORRECTION_HINT  0x0C50
#define GL_PACK_ALIGNMENT               0x0D05
#define GL_TEXTURE_2D                   0x0DE1
#define GL_FASTEST                      0x1101
#define GL_UNSIGNED_BYTE                0x1401
#define GL_MODELVIEW                    0x1700
#define GL_PROJECTION                   0x1701
#define GL_RGB                          0x1907
#define GL_RGBA                         0x1908
#define GL_VENDOR                       0x1F00
#define GL_RENDERER                     0x1F01
#define GL_VERSION                      0x1F02
#define GL_EXTENSIONS                   0x1F03
#define GL_NEAREST                      0x2600
#define GL_TEXTURE_MAG_FILTER           0x2800
#define GL_TEXTURE_MIN_FILTER           0x2801
#define GL_TEXTURE_WRAP_S               0x2802
#define GL_TEXTURE_WRAP_T               0x2803
#define GL_REPEAT                       0x2901
#define GL_TEXTURE_PRIORITY             0x8066
#define GL_ONE                          1
#define GL_SRC_ALPHA                    0x0302
#define GL_DST_COLOR                    0x0306
#define GL_ONE_MINUS_DST_COLOR          0x0307

static inline GLuint ghack_gl_next_id(void) { static GLuint n; return ++n; }

static inline void glBegin(GLenum) {}
static inline void glEnd(void) {}
static inline void glBindTexture(GLenum, GLuint) {}
static inline void glBlendFunc(GLenum, GLenum) {}
static inline void glCallList(GLuint) {}
static inline void glNewList(GLuint, GLenum) {}
static inline void glEndList(void) {}
static inline GLuint glGenLists(GLsizei n) { GLuint f = ghack_gl_next_id(); for (GLsizei i = 1; i < n; i++) ghack_gl_next_id(); return f; }
static inline void glClear(GLbitfield) {}
static inline void glClearColor(GLclampf, GLclampf, GLclampf, GLclampf) {}
static inline void glColor4fv(const GLfloat *) {}
static inline void glColor4ub(GLubyte, GLubyte, GLubyte, GLubyte) {}
static inline void glColor4ubv(const GLubyte *) {}
static inline void glCopyTexImage2D(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint) {}
static inline void glGenBuffers(GLsizei n, GLuint *b) { for (GLsizei i = 0; i < n; i++) b[i] = ghack_gl_next_id(); }
static inline void glDeleteBuffers(GLsizei, const GLuint *) {}
static inline void glGenTextures(GLsizei n, GLuint *t) { for (GLsizei i = 0; i < n; i++) t[i] = ghack_gl_next_id(); }
static inline void glDeleteTextures(GLsizei, const GLuint *) {}
static inline void glDisable(GLenum) {}
static inline void glEnable(GLenum) {}
static inline GLenum glGetError(void) { return GL_NO_ERROR; }
static inline const GLubyte *glGetString(GLenum) { return (const GLubyte *) "terminal"; }
static inline void glHint(GLenum, GLenum) {}
static inline void glLineWidth(GLfloat) {}
static inline void glLoadIdentity(void) {}
static inline void glMatrixMode(GLenum) {}
static inline void glMultMatrixf(const GLfloat *) {}
static inline void glOrtho(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble) {}
static inline void glPixelStorei(GLenum, GLint) {}
static inline void glPopMatrix(void) {}
static inline void glPushMatrix(void) {}
static inline void glReadBuffer(GLenum) {}
static inline void glReadPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *) {}
static inline void glScalef(GLfloat, GLfloat, GLfloat) {}
static inline void glScissor(GLint, GLint, GLsizei, GLsizei) {}
static inline void glTexCoord2f(GLfloat, GLfloat) {}
static inline void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *) {}
static inline void glTexParameteri(GLenum, GLenum, GLint) {}
static inline void glTranslatef(GLfloat, GLfloat, GLfloat) {}
static inline void glVertex2d(GLdouble, GLdouble) {}
static inline void glVertex2f(GLfloat, GLfloat) {}
static inline void glVertex3f(GLfloat, GLfloat, GLfloat) {}
static inline void glViewport(GLint, GLint, GLsizei, GLsizei) {}
static inline int  gluBuild2DMipmaps(GLenum, GLint, GLsizei, GLsizei, GLenum, GLenum, const void *) { return 0; }
static inline const GLubyte *gluErrorString(GLenum) { return (const GLubyte *) ""; }

#endif
