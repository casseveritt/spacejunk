package us.xyzw.spacejunk;

import us.xyzw.s3mActivity;
import us.xyzw.spacejunk.spacejunkNative;
import android.os.Bundle;
import android.util.Log;

public class spacejunk extends s3mActivity {
	private static final String TAG = "spacejunk";
	

	public class Star3mapNativeDispatch extends s3mActivity.NativeDispatch {
		public void Init( s3mActivity activity ) { spacejunkNative.Init( activity ); }
	    public void ExecuteCommand( String cmd ) { spacejunkNative.ExecuteCommand(cmd); }

	    public void Quit() { spacejunkNative.Quit(); }
	    public void Pause() { spacejunkNative.Pause(); }
	    public void Resume() { spacejunkNative.Resume(); }
	    public void Touches( int count, int[] points ) { spacejunkNative.Touches( count, points ); }
	    public void Accel( float x, float y, float z ) { spacejunkNative.Accel( x, y, z ); }
	    public void Location( float latitude, float longitude ) { spacejunkNative.Location( latitude, longitude ); }
	    public void Heading( float x, float y, float z, float trueDiff ) { spacejunkNative.Heading( x, y, z, trueDiff ); }
	   
	    public void Display() { spacejunkNative.Display(); }
	    public void Reshape(int width, int height) { spacejunkNative.Reshape( width, height ); }    
	    
	    public void SetStatus( String status ) { spacejunkNative.SetStatus( status ); }
	}
	
	public Star3mapNativeDispatch nativeDispatch = new Star3mapNativeDispatch();
	

	/** Called when the activity is first created. */
	@Override
	protected void onCreate(Bundle icicle) {
		super.nativeDispatch = nativeDispatch;
		
		Log.i(TAG, "init: spacejunk.onCreate() called");

		super.onCreate(icicle);

	}

}

