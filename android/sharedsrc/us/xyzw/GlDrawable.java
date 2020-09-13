/*
 * Copyright (C) 2008 The Android Open Source Project
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

package us.xyzw; 

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGL11;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class GlDrawable extends SurfaceView implements SurfaceHolder.Callback {
    public GlDrawable(Context context) {
        super(context);
        init();
    }

    public GlDrawable(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
    	mEGLConfigChooser = new ComponentSizeChooser( 5, 6, 5, 0, 16, 0 );
    	eh = new EglHelper();
        SurfaceHolder holder = getHolder();
        holder.addCallback(this);
        holder.setFormat(PixelFormat.RGB_565);
    }

    public void setSingleThread( boolean inSingleThread ) {
    	singleThread = inSingleThread;
    }
    
    public void surfaceCreated(SurfaceHolder holder) {
    	eh.createSurface( holder );
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
    	eh.destroySurface();
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
    	if ( w != width || h != height ) {
    		sizeChanged = true;
    		width = w;
    		height = h;
    	}
    }

    public synchronized boolean isReady() {
    	return ready;
    }

    private synchronized void setReady( boolean rdy ) {
    	ready = rdy;
    }

    public boolean needsReshape() {
    	return sizeChanged;
    }
    
    public int getDrawableWidth() {
    	return width;
    }
    
    public int getDrawableHeight() {
    	return height;
    }
    
    public void clearNeedsReshape() {
    	sizeChanged = false;
    }

    public boolean swapBuffers() {
    	return eh.swap();
    }
    
    public int getCurrentGlContext() {
    	return eh.getCurrent();
    }
    
    public void makeGlContextCurrent( int context ) {
    	eh.makeCurrent( context );
    }
    
    public int createGlContext( int shareContext ) {
    	int context = 0;
    	try {
    		context = eh.createContext( shareContext );
    	} catch ( Exception e ) {
    		Log.e( TAG, "Context creation failed." );
    	}
    	return context;
    }
    
    public void destroyGlContext( int context ) {
    	eh.destroyContext( context );
    }

    public interface EGLContextFactory {
        EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig eglConfig, EGLContext share);
        void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context);
    }

    public interface EGLConfigChooser {
        EGLConfig chooseConfig(EGL10 egl, EGLDisplay display);
    }

    private abstract class BaseConfigChooser
            implements EGLConfigChooser {
        public BaseConfigChooser(int[] configSpec) {
            mConfigSpec = configSpec;
        }

        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
            int[] num_config = new int[1];
            if (!egl.eglChooseConfig(display, mConfigSpec, null, 0,
                    num_config)) {
                throw new IllegalArgumentException("eglChooseConfig failed");
            }

            int numConfigs = num_config[0];

            if (numConfigs <= 0) {
                throw new IllegalArgumentException(
                        "No configs match configSpec");
            }

            EGLConfig[] configs = new EGLConfig[numConfigs];
            if (!egl.eglChooseConfig(display, mConfigSpec, configs, numConfigs,
                    num_config)) {
                throw new IllegalArgumentException("eglChooseConfig#2 failed");
            }
            EGLConfig config = chooseConfig(egl, display, configs);
            if (config == null) {
                throw new IllegalArgumentException("No config chosen");
            }
            return config;
        }

        abstract EGLConfig chooseConfig(EGL10 egl, EGLDisplay display,
                EGLConfig[] configs);

        protected int[] mConfigSpec;
    }

    private class ComponentSizeChooser extends BaseConfigChooser {
        private static final int EGL_OPENGL_ES2_BIT = 4;
               public ComponentSizeChooser(int redSize, int greenSize, int blueSize,
                int alphaSize, int depthSize, int stencilSize) {
            super(new int[] {
                    EGL10.EGL_RED_SIZE, redSize,
                    EGL10.EGL_GREEN_SIZE, greenSize,
                    EGL10.EGL_BLUE_SIZE, blueSize,
                    EGL10.EGL_ALPHA_SIZE, alphaSize,
                    EGL10.EGL_DEPTH_SIZE, depthSize,
                    EGL10.EGL_STENCIL_SIZE, stencilSize,
                    EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL10.EGL_NONE});
            mValue = new int[1];
            mRedSize = redSize;
            mGreenSize = greenSize;
            mBlueSize = blueSize;
            mAlphaSize = alphaSize;
            mDepthSize = depthSize;
            mStencilSize = stencilSize;
       }

        @Override
        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display,
                EGLConfig[] configs) {
            for (EGLConfig config : configs) {
                int d = findConfigAttrib(egl, display, config,
                        EGL10.EGL_DEPTH_SIZE, 0);
                int s = findConfigAttrib(egl, display, config,
                        EGL10.EGL_STENCIL_SIZE, 0);
                if ((d >= mDepthSize) && (s >= mStencilSize)) {
                    int r = findConfigAttrib(egl, display, config,
                            EGL10.EGL_RED_SIZE, 0);
                    int g = findConfigAttrib(egl, display, config,
                             EGL10.EGL_GREEN_SIZE, 0);
                    int b = findConfigAttrib(egl, display, config,
                              EGL10.EGL_BLUE_SIZE, 0);
                    int a = findConfigAttrib(egl, display, config,
                            EGL10.EGL_ALPHA_SIZE, 0);
                    if ((r == mRedSize) && (g == mGreenSize)
                            && (b == mBlueSize) && (a == mAlphaSize)) {
                        return config;
                    }
                }
            }
            return null;
        }

        private int findConfigAttrib(EGL10 egl, EGLDisplay display,
                EGLConfig config, int attribute, int defaultValue) {

            if (egl.eglGetConfigAttrib(display, config, attribute, mValue)) {
                return mValue[0];
            }
            return defaultValue;
        }

        private int[] mValue;
        // Subclasses can adjust these values:
        protected int mRedSize;
        protected int mGreenSize;
        protected int mBlueSize;
        protected int mAlphaSize;
        protected int mDepthSize;
        protected int mStencilSize;
        }

    private class EglHelper {
        public EglHelper() {
        	mCurrentContext = -1;
        	srf = null;
        	init();
        }

        synchronized public void init() {
            egl = (EGL10) EGLContext.getEGL();
            dpy = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
            if (dpy == EGL10.EGL_NO_DISPLAY) {
                throw new RuntimeException("eglGetDisplay failed");
            }
            int[] version = new int[2];
            if(!egl.eglInitialize(dpy, version)) {
                throw new RuntimeException("eglInitialize failed");
            }
            cfg = mEGLConfigChooser.chooseConfig(egl, dpy);
            final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
            int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE };
            try {
            ctx[0] = egl.eglCreateContext( dpy, cfg, EGL10.EGL_NO_CONTEXT, attrib_list );
            } catch ( Exception e ) {
            	throwEglException( "createContext" );
            }
            if (ctx[0] == null || ctx[0] == EGL10.EGL_NO_CONTEXT) {
                ctx[0] = null;
                throwEglException("createContext");
            }
            srf = null;
        }

        synchronized public void createSurface(SurfaceHolder holder) {
            Log.e("GlDrawableEglHelper", "createSurface called.");

            if (egl == null) {
                throw new RuntimeException("egl not initialized");
            }
            if (dpy == null) {
                throw new RuntimeException("eglDisplay not initialized");
            }
            if (cfg == null) {
                throw new RuntimeException("mEglConfig not initialized");
            }
            if (srf != null && srf != EGL10.EGL_NO_SURFACE) {
                egl.eglMakeCurrent(dpy, EGL10.EGL_NO_SURFACE,
                        EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
                egl.eglDestroySurface( dpy, srf );
            }
            srf = egl.eglCreateWindowSurface( dpy, cfg, holder, null);

            if (srf == null || srf == EGL10.EGL_NO_SURFACE) {
                int error = egl.eglGetError();
                if (error == EGL10.EGL_BAD_NATIVE_WINDOW) {
                    Log.e("EglHelper", "eglCreateWindowSurface returned EGL_BAD_NATIVE_WINDOW.");
                    return;
                }
                throwEglException("eglCreateWindowSurface", error);
            } 
            mCurrentContext = -1;
            setReady( srf != null );
        }

        synchronized public boolean swap() {
        	if ( mCurrentContext != 0 || dpy == null || srf == null ) {
        		return false; // can only swap on the window context
        	}

        	try {
        		egl.eglSwapBuffers(dpy, srf);
        	} catch ( Exception e ) {
        		Log.e( "EglHelper", e.getMessage() );
                if ( dpy == null || srf == null || egl.eglSwapBuffers(dpy, srf) == false ) {
                    Log.e("EglHelper", "eglSwapBuffers failed. tid=" + Thread.currentThread().getId());
                    int error = egl.eglGetError();
                    switch(error) {
                    case EGL11.EGL_CONTEXT_LOST:
                        Log.e("EglHelper", "eglSwapBuffers returned EGL_CONTEXT_LOST. tid=" + Thread.currentThread().getId());
                        return false;
                    case EGL10.EGL_BAD_NATIVE_WINDOW:
                        // The native window is bad, probably because the
                        // window manager has closed it. Ignore this error,
                        // on the expectation that the application will be closed soon.
                        Log.e("EglHelper", "eglSwapBuffers returned EGL_BAD_NATIVE_WINDOW. tid=" + Thread.currentThread().getId());
                        break;
                    default:
                        Log.e("EglHelper", "eglSwapBuffers returned an unknown error. tid=" + Thread.currentThread().getId());
                        throwEglException("eglSwapBuffers", error);
                    }
                    return false;
                }
        	}
            return true;
        }

        synchronized public void destroySurface() {
            Log.e("GlDrawableEglHelper", "destroySurface called.");
            if (srf != null && srf != EGL10.EGL_NO_SURFACE) {
                mCurrentContext = -1;
                egl.eglMakeCurrent(dpy, EGL10.EGL_NO_SURFACE,
                        EGL10.EGL_NO_SURFACE,
                        EGL10.EGL_NO_CONTEXT);
                egl.eglDestroySurface(dpy, srf);
                srf = null;
            }
            ready = false;
        }

        synchronized public boolean makeCurrent( int context ) {
        	if ( srf == null ) {
        		return false;
        	}
        	if ( context == -1 && singleThread == true ) {
        		context = 0;
        	}
        	boolean ret = true;
        	if ( 0 <= context && context < MAX_CONTEXTS ) {
        		if ( mCurrentContext != context ) {
                    if (!egl.eglMakeCurrent(dpy, srf, srf, ctx[ context ])) {
                        throwEglException("eglMakeCurrent");
                    }        		
                    mCurrentContext = context;
        		}
                ret = true;
        	} else {
                mCurrentContext = -1;
                ret = egl.eglMakeCurrent(dpy, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
        	}
        	return ret;
        }
        
        synchronized public int getCurrent() {
        	return mCurrentContext;
        }
        
        synchronized public int createContext( int shareContext ) {
        	for ( int i = 1; i < MAX_CONTEXTS; i++ ) {
        		if ( ctx[i] == null ) {
        			EGLContext share = EGL10.EGL_NO_CONTEXT;
        			if ( 0 <= shareContext && shareContext < MAX_CONTEXTS && ctx[ shareContext ] != null ) {
        				share = ctx[ shareContext ];
        			}
                    EGLContext newContext = ctx[i] = egl.eglCreateContext( dpy, cfg, share, null );
                    if ( newContext == null || newContext == EGL10.EGL_NO_CONTEXT) {
                        ctx[i] = null;
                        throwEglException("createContext");
                    }
                    return i;
        		}
        	}
        	return -1;
        }
        synchronized public void destroyContext( int context ) {
        	throwEglException( "Damnit." );
        	if ( 0 < context && context < MAX_CONTEXTS && context != mCurrentContext && ctx[ context ] != null ) {
                egl.eglDestroyContext( dpy, ctx[ context ] );
                ctx[ context ] = null;
        	}
        }

        private void throwEglException(String function) {
            throwEglException(function, egl.eglGetError());
        }

        private void throwEglException(String function, int error) {
            String message = function + " failed";
            throw new RuntimeException(message);
        }

        EGL10 egl;
        EGLDisplay dpy;
        EGLSurface srf;
        EGLConfig cfg;
        private final int MAX_CONTEXTS = 10;
        EGLContext[] ctx = new EGLContext[ MAX_CONTEXTS ];
        int mCurrentContext;

    }

    private static final String TAG = "GlDrawable";
    private boolean ready = false;
    private boolean sizeChanged = false;
    private int width;
    private int height;
    private boolean singleThread = true;

    private EGLConfigChooser mEGLConfigChooser;    
    private EglHelper eh;
}
