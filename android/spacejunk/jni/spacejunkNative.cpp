/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// OpenGL ES 2.0 code

#include <jni.h>
#include <android/log.h>

#include <GL/Regal.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>

#include "app.h"

#include "r3/command.h"
#include "r3/init.h"
#include "r3/output.h"
#include "r3/var.h"

using namespace std;
using namespace r3;
extern VarInteger r_windowWidth;
extern VarInteger r_windowHeight;

#define  LOG_TAG    "star3map"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static void printGLString(const char *name, GLenum s) {
  const char *v = (const char *) glGetString(s);
  LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
  for (GLint error = glGetError(); error; error
      = glGetError()) {
    LOGI("after %s() glError (0x%x)\n", op, error);
  }
}

namespace {
  JavaVM * jvm = NULL;
  JNIEnv * appEnv = NULL;
  jobject appActivity = 0;
}

bool setupGraphics(int w, int h) {
  RegalMakeCurrent( eglGetCurrentContext() );
  printGLString("Version", GL_VERSION);
  printGLString("Vendor", GL_VENDOR);
  printGLString("Renderer", GL_RENDERER);
  printGLString("Extensions", GL_EXTENSIONS);

  Output( "in setupGraphics" );
  r_windowWidth.SetVal( w );
  r_windowWidth.SetVal( h );

  LOGI("setupGraphics(%d, %d)", w, h);
  glViewport(0, 0, w, h);
  checkGlError("glViewport");
  return true;
}

const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f,
  0.5f, -0.5f };

void renderFrame() {
  static float grey;
  grey += 0.01f;
  if (grey > 1.0f) {
    grey = 0.0f;
  }
  glClearColor(grey, grey, grey, 1.0f);
  checkGlError("glClearColor");
  glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  checkGlError("glClear");

  glColor4f( 0, 1, 0, 1 );
  glEnableClientState( GL_VERTEX_ARRAY );
  glVertexPointer( 2, GL_FLOAT, 0, gTriangleVertices);
  checkGlError("glVertexAttribPointer");
  glDrawArrays(GL_TRIANGLES, 0, 3);
  checkGlError("glDrawArrays");
}

jstring appNewJavaString( JNIEnv *env, const string & s ) {
  return env->NewStringUTF( s.c_str() );
}

static JNIEnv *attachThread() {
  if( ! appActivity ) {
    return NULL;
  }
  JNIEnv *env = NULL;
  if( jvm == NULL || jvm->AttachCurrentThread( &env, NULL) || env == NULL ) {
    LOGE( "Unable to attach the current thread!" );
  }
  return env;
}

void platformDetachThread() {
    if ( jvm ) {
      jvm->DetachCurrentThread();
    }
}

static jmethodID getMethod( JNIEnv *env, jobject obj, const char *methodName, const char *methodSignature ) {

  jclass objClass = env->GetObjectClass( obj );

  jmethodID m = env->GetMethodID( objClass, methodName, methodSignature );
  if ( m == NULL ) {
    LOGE( "failed to find %s method (sig %s)\n", methodName, methodSignature );
  }
}


// this tries to copy the file from the apk, network, etc and put it into
// the local cache
bool appMaterializeFile( const char * filename ) {
  JNIEnv *env = attachThread();
  if ( env == NULL ) { return false; }

  LOGI( "Materialize file %s\n", filename );

  jmethodID m = getMethod( env, appActivity, "copyFromApk", "(Ljava/lang/String;)Z" );
  if ( m == NULL ) { return false; }

  jstring s = appNewJavaString( env, filename );

  jboolean r = env->CallBooleanMethod( appActivity, m, s );
  LOGI( "Materialize file %s (%s)\n", filename, r ? "succeeded" : "failed" );
  return r;
}

bool appOpenUrl( const char * url ) {
  JNIEnv *env = attachThread();
  if ( env == NULL ) { return false; }

  jmethodID m = getMethod( env, appActivity, "openUrl", "(Ljava/lang/String;)Z" );
  if ( m == NULL ) { return false; }

  jstring s = appNewJavaString( env, url );

  jboolean r = env->CallBooleanMethod( appActivity, m, s );
  LOGI( "openUrl %s (%s)\n", url, r ? "succeeded" : "failed" );
  return r;
}

void GfxSwapBuffers() {
	JNIEnv *env = attachThread();
	if ( env == NULL ) { return; }
	
	jmethodID m = getMethod( env, appActivity, "swapBuffers", "()V" );
	if ( m == NULL ) { return; }
	
	env->CallVoidMethod( appActivity, m );
}

typedef int GlContext;
GlContext GfxGetCurrentContext() {
  JNIEnv *env = attachThread();
  if ( env == NULL ) { return false; }

  jmethodID m = getMethod( env, appActivity, "getCurrentGlContext", "()I" );
  if ( m == NULL ) { return false; }

  jint r = env->CallIntMethod( appActivity, m );
  LOGI( "GfxGetCurrentContext returned %d\n", r );
  return r;
}

void GfxSetCurrentContext( GlContext context ) {
  JNIEnv *env = attachThread();
  if ( env == NULL ) { return; }

  jmethodID m = getMethod( env, appActivity, "makeGlContextCurrent", "(I)V" );
  if ( m == NULL ) { return; }

  env->CallVoidMethod( appActivity, m, context );
  RegalMakeCurrent( eglGetCurrentContext() );
  //LOGI( "GfxSetCurrentContext %d\n", context );
}

GlContext GfxCreateContext( GlContext context ) {
  JNIEnv *env = attachThread();
  if ( env == NULL ) { return false; }

  jmethodID m = getMethod( env, appActivity, "createGlContext", "(I)I" );
  if ( m == NULL ) { return false; }

  jint r = env->CallIntMethod( appActivity, m, context );
  LOGI( "GfxCreateContext returned %d\n", r );
  return r;
}

void GfxDestroyContext( GlContext context ) {
  JNIEnv *env = attachThread();
  if ( env == NULL ) { return; }

  jmethodID m = getMethod( env, appActivity, "destroyGlContext", "(I)V" );
  if ( m == NULL ) { return; }

  env->CallVoidMethod( appActivity, m, context );
  LOGI( "GfxDestroyContext %d\n", context );
}


void platformOutput( const char * msg ) {
  if( ! appActivity ) {
    return;
  }
  LOGI( "%s\n", msg );
}


void platformFacebookLogin() {
  if( ! appActivity ) {
    return;
  }
  JNIEnv *env = NULL;
  if( jvm == NULL || jvm->AttachCurrentThread( &env, NULL) || env == NULL ) {
    LOGI( "Unable to attach the current thread!" );
    return;
  }

  jmethodID login = NULL;

  jclass appActivityClass = env->GetObjectClass( appActivity );

  login = env->GetMethodID( appActivityClass, "login", "()V" );
  if ( login == NULL ) {
    LOGI( "failed to find login method\n" );
  }

  env->CallVoidMethod( appActivity, login );
}

void platformFacebookLogout() {
  if( ! appActivity ) {
    return;
  }
  JNIEnv *env = NULL;
  if( jvm == NULL || jvm->AttachCurrentThread( &env, NULL) || env == NULL ) {
    LOGI( "Unable to attach the current thread!" );
    return;
  }

  jmethodID logout = NULL;

  jclass appActivityClass = env->GetObjectClass( appActivity );

  logout = env->GetMethodID( appActivityClass, "logout", "()V" );
  if ( logout == NULL ) {
    LOGI( "failed to find logout method\n" );
  }

  env->CallVoidMethod( appActivity, logout );
}


void platformFacebookPublish( const string & caption, const string & description ) {
  if( ! appActivity ) {
    return;
  }
  JNIEnv *env = NULL;
  if( jvm == NULL || jvm->AttachCurrentThread( &env, NULL) || env == NULL ) {
    LOGI( "Unable to attach the current thread!" );
    return;
  }

  jmethodID publish = NULL;

  jclass appActivityClass = env->GetObjectClass( appActivity );

  publish = env->GetMethodID( appActivityClass, "publish", "(Ljava/lang/String;Ljava/lang/String;)V" );
  if ( publish == NULL ) {
    LOGI( "failed to find publish method\n" );
  }

  jstring cap = appNewJavaString( env, caption );
  jstring desc = appNewJavaString( env, description );

  env->CallVoidMethod( appActivity, publish, cap, desc );
}




extern "C" {
  JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Init( JNIEnv * env, jobject obj, jobject activity );
  JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_ExecuteCommand(JNIEnv * env, jobject obj, jstring cmd);
  JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Quit( JNIEnv * env, jobject obj );
  JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Pause( JNIEnv * env, jobject obj );
  JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Resume( JNIEnv * env, jobject obj );
  JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Touches( JNIEnv * env, jobject obj, jint count, jintArray points );
  JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Accel( JNIEnv * env, jobject obj, jfloat x, jfloat y, jfloat z );
  JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Location( JNIEnv * env, jobject obj, jfloat latitude, jfloat longitude );
  JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Heading( JNIEnv * env, jobject obj, jfloat x, jfloat y, jfloat z, jfloat trueDiff );
  JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Reshape(JNIEnv * env, jobject obj,  jint width, jint height);
  JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Display(JNIEnv * env, jobject obj);

  JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_SetStatus( JNIEnv * env, jobject obj, jstring status );
};

JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Init( JNIEnv * env, jobject obj, jobject activity ) {
  appEnv = env;
  appActivity = env->NewGlobalRef( activity );
  //JNI_GetCreatedJavaVMs( &jvm, jint(1), &numjvms);
  //LOGI( "Found %d JavaVMs\n", numjvms);
  env->GetJavaVM( &jvm );
}

JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_ExecuteCommand(JNIEnv * env, jobject obj, jstring cmd )
{
  const char * str = appEnv->GetStringUTFChars( cmd, NULL );
  r3::ExecuteCommand( str );	
  appEnv->ReleaseStringUTFChars( cmd, str );
}

JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Quit( JNIEnv * env, jobject obj ) {
  platformQuit();
}

JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Pause( JNIEnv * env, jobject obj ) {
  platformResignActive();
}

JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Resume( JNIEnv * env, jobject obj ) {
}

JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Touches( JNIEnv * env, jobject obj, jint count, jintArray points ) {
  jint p[10];
  // FIXME: move points into p
  jsize len = env->GetArrayLength( points );
  env->GetIntArrayRegion( points, 0, count * 2, p );
  touches( count, p );
}

JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Accel( JNIEnv * env, jobject obj, jfloat x, jfloat y, jfloat z ) {
  setAccel( x, y, z );
}

JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Location( JNIEnv * env, jobject obj, jfloat latitude, jfloat longitude ) {
  setLocation( latitude, longitude );
}

JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Heading( JNIEnv * env, jobject obj, jfloat x, jfloat y, jfloat z, jfloat trueDiff ) {
  setHeading( x, y, z, trueDiff );
}

JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Reshape(JNIEnv * env, jobject obj,  jint width, jint height)
{
  static bool realInit = false;
  if ( realInit == false ) {
    appEnv = env; // the render thread owns the env now...
    //r3::Init( 0, NULL );
    LOGI("calling platformMain!");
    RegalMakeCurrent( eglGetCurrentContext() );
    platformMain();
  }
  //setupGraphics(width, height);
  reshape( width, height );
}

JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_Display(JNIEnv * env, jobject obj)
{
  //renderFrame();
  display();
}

namespace star3map {
  void SetStatus( const char *status );
}

JNIEXPORT void JNICALL Java_us_xyzw_spacejunk_spacejunkNative_SetStatus(JNIEnv * env, jobject obj, jstring status )
{
  const char * str = env->GetStringUTFChars( status, NULL );
  star3map::SetStatus( str );
  env->ReleaseStringUTFChars( status, str );
}



