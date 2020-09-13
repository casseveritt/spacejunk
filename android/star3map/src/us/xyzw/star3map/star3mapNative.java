package us.xyzw.star3map;

import us.xyzw.s3mActivity;
import android.util.Log;

public class star3mapNative {
    static {
    	try{
    		System.loadLibrary("star3map");
    	} catch ( UnsatisfiedLinkError e ) {
    		Log.e("star3map", "Unsatisifed Link Error " + e.getMessage() );
    	}
    }
    
    public static native void Init( s3mActivity activity ); 
    public static native void ExecuteCommand( String cmd );

    public static native void Quit();
    public static native void Pause();
    public static native void Resume();
    public static native void Touches( int count, int[] points );
    public static native void Accel( float x, float y, float z );
    public static native void Location( float latitude, float longitude );
    public static native void Heading( float x, float y, float z, float trueDiff );
   
    public static native void Display();
    public static native void Reshape(int width, int height);    
    
    public static native void SetStatus( String status );
}
