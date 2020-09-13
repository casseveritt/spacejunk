package us.xyzw.star3map;

import com.android.vending.licensing.AESObfuscator;
import com.android.vending.licensing.LicenseChecker;
import com.android.vending.licensing.LicenseCheckerCallback;
import com.android.vending.licensing.ServerManagedPolicy;

import us.xyzw.s3mActivity;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings.Secure;
import android.util.Log;

public class star3map extends s3mActivity {
	private static final String TAG = "star3map";
	private LicenseChecker licenseChecker;
    private static final String BASE64_PUBLIC_KEY = "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxl9ExvYc+ljotTOVEWSjGr8K9gDeIIJQexGFEmRVtzgP9By3f/pXyngZHwF4MqUcFd3DcRKWq5X+9KLdF13HKaqAe3ejtzzLUzQgtH+7L544ShetqUNAbNGUPX0boHZjzJWfHunAVmu6jtzMZdR4FIuK3qeCByMe/DVpXOiP64b9AI7GgwWrtiYYyrnYsYD4EbNx/KjIvtuZGN0q9zcLZIAhvVOq0z52udJ+Yga9+IZ7lqtXF7f7YVGNuFYDPCPeL9nLDdEwnDZulaTjrh7r3Z3/Mp6MYl1C8Hgb048gnpn/mqzTb1DYvNNdie/YdigyE8xedgMwBTckoD6vvHLAmwIDAQAB";

    // Generate your own 20 random bytes, and put them here.
    private static final byte[] SALT = new byte[] {
        -80,  -38,  126,  -13,  -82,  116,   74,    7,  -68, -100,    1,  -15,   -9,   26,  -67,  -66,  120, -123,   96,  -16
        };
	
	public class Star3mapNativeDispatch extends s3mActivity.NativeDispatch {
		public void Init( s3mActivity activity ) { star3mapNative.Init( activity ); }
	    public void ExecuteCommand( String cmd ) { star3mapNative.ExecuteCommand(cmd); }

	    public void Quit() { star3mapNative.Quit(); }
	    public void Pause() { star3mapNative.Pause(); }
	    public void Resume() { star3mapNative.Resume(); }
	    public void Touches( int count, int[] points ) { star3mapNative.Touches( count, points ); }
	    public void Accel( float x, float y, float z ) { star3mapNative.Accel( x, y, z ); }
	    public void Location( float latitude, float longitude ) { star3mapNative.Location( latitude, longitude ); }
	    public void Heading( float x, float y, float z, float trueDiff ) { star3mapNative.Heading( x, y, z, trueDiff ); }
	   
	    public void Display() { star3mapNative.Display(); }
	    public void Reshape(int width, int height) { star3mapNative.Reshape( width, height ); }    
	    
	    public void SetStatus( String status ) { star3mapNative.SetStatus( status ); }
	}
	
	public Star3mapNativeDispatch nativeDispatch = new Star3mapNativeDispatch();
	
	
    private LicenseCheckerCallback licenseCheckerCallback = new LicenseCheckerCallback() {
        public void allow() {
        	nativeDispatch.ExecuteCommand( "app_license 1" );
        	}
        public void dontAllow() {
        	nativeDispatch.ExecuteCommand( "app_license 0" );
        	}
		@Override
		public void applicationError(ApplicationErrorCode errorCode) {}
    };

	/** Called when the activity is first created. */
	@Override
	protected void onCreate(Bundle icicle) {
		super.nativeDispatch = nativeDispatch;
		Log.i(TAG, "init: star3map.onCreate() called");
		super.onCreate(icicle);
        // Try to use more data here. ANDROID_ID is a single point of attack.
        String deviceId = Secure.getString(getContentResolver(), Secure.ANDROID_ID);
		ServerManagedPolicy p = new ServerManagedPolicy(this, new AESObfuscator(SALT, getPackageName(), deviceId) );
        licenseChecker = new LicenseChecker( this, p, BASE64_PUBLIC_KEY );
        licenseChecker.checkAccess( licenseCheckerCallback );
        startService( new Intent( this, SpaceJunkService.class ) );
	}

	@Override
	protected void onDestroy() {
		Log.i(TAG, "init: star3map.onDestroy() called");
		licenseChecker.onDestroy();
		super.onDestroy();
	}

}
