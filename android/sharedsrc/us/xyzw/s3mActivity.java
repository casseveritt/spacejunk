package us.xyzw;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Timer;
import java.util.TimerTask;

import us.xyzw.GlDrawable;

import android.app.Activity;
import android.app.Application;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.GeomagneticField;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;

public class s3mActivity extends Activity {
	private static final String TAG = "s3mActivity";
	private GlDrawable drawable;
	private PowerManager.WakeLock wakeLock;
	private float lat, lon;
	private GeomagneticField gmf = null;
	
	public abstract class NativeDispatch {
		public abstract void Init( s3mActivity activity );
	    public abstract void ExecuteCommand( String cmd );

	    public abstract void Quit();
	    public abstract void Pause();
	    public abstract void Resume();
	    public abstract void Touches( int count, int[] points );
	    public abstract void Accel( float x, float y, float z );
	    public abstract void Location( float latitude, float longitude );
	    public abstract void Heading( float x, float y, float z, float trueDiff );
	   
	    public abstract void Display();
	    public abstract void Reshape(int width, int height);    
	    
	    public abstract void SetStatus( String status );
	}
	
	public NativeDispatch nativeDispatch = null;

	private OnTouchListener touchListener = new OnTouchListener() {
		public boolean onTouch(View v, MotionEvent me) {
			if( nativeDispatch == null ) {
				return false;
			}
			int[] points = new int[10];
			if (me.getAction() == MotionEvent.ACTION_UP) {
				nativeDispatch.Touches(0, points);
				// Log.i( TAG, "Touch event ACTION UP..." );
			} else {
				for (int i = 0; i < me.getPointerCount(); i++) {
					points[2 * i + 0] = (int) me.getX(i);
					points[2 * i + 1] = (int) me.getY(i);
				}
				nativeDispatch.Touches(me.getPointerCount(), points);
				// Log.i( TAG, "Touch event regular " + me.getX() + " " +
				// me.getY() );
			}
			return true;
		}
	};

	// Define a listener that responds to location updates
	LocationListener locationListener = new LocationListener() {
		int gotUpdate = 0;

		public void onLocationChanged(Location location) {
			if( nativeDispatch == null ) {
				return;
			}
			// Called when a new location is found by the network location
			// provider.
			lat = (float) location.getLatitude();
			lon = (float) location.getLongitude();
			gmf = new GeomagneticField(lat, lat, 0, System.currentTimeMillis());
			String provider = location.getProvider();
			if (gotUpdate < 3) {
				gotUpdate++;
				if (provider == LocationManager.GPS_PROVIDER) {
					nativeDispatch.SetStatus("Got GPS position update.");
				} else {
					nativeDispatch.SetStatus("Got NETWORK position update.");
				}
			}
			// Log.i( TAG, "Sending new lat/lon " + lat + ", " + lon );
			nativeDispatch.Location(lat, lon);
		}

		public void onStatusChanged(String provider, int status, Bundle extras) {
		}

		public void onProviderEnabled(String provider) {
		}

		public void onProviderDisabled(String provider) {
		}
	};

	// Define a listener that responds to location updates
	SensorEventListener sensorEventListener = new SensorEventListener() {
		public void onAccuracyChanged(Sensor sensor, int accuracy) {
		}

		public void onSensorChanged(SensorEvent event) {
			if( nativeDispatch == null ) {
				return;
			}
			int type = event.sensor.getType();
			if (type == Sensor.TYPE_ACCELEROMETER) {
				// Log.i( TAG, "received accelerometer event");
				nativeDispatch.Accel(-event.values[0], -event.values[1],
						-event.values[2]);
			} else if (type == Sensor.TYPE_MAGNETIC_FIELD) {
				// Log.i( TAG, "received magnetometer event");
				float decl = 0.0f;
				if (gmf != null) {
					decl = gmf.getDeclination();
				} else {
					//Log.i( TAG, "no GeomagneticField object!" );
				}
				nativeDispatch.Heading(event.values[0], event.values[1],
						event.values[2], decl);
			}
		}
	};

	/** Called when the activity is first created. */
	//@Override
	protected void onCreate(Bundle icicle) {
		Log.i(TAG, "init: s3mActivity.onCreate() called");

		super.onCreate(icicle);

		Log.i(TAG, "Build.BOARD: " + Build.BOARD);
		Log.i(TAG, "Build.BRAND: " + Build.BRAND);
		Log.i(TAG, "Build.DEVICE: " + Build.DEVICE);
		Log.i(TAG, "Build.MANUFACTURER: " + Build.MANUFACTURER);
		Log.i(TAG, "Build.MODEL: " + Build.MODEL);
		Log.i(TAG, "Build.PRODUCT: " + Build.PRODUCT);

		File f = new java.io.File(getFilesDir().getAbsolutePath() + "/base");
		if (f.exists() == false) {
			Log.i(TAG, "Creating local base dir " + f.getAbsolutePath());
			f.mkdirs();
		}
		nativeDispatch.Init( this );
		nativeDispatch.ExecuteCommand("set f_basePath "
				+ getFilesDir().getAbsolutePath() + "/base/");
		try {
			nativeDispatch.ExecuteCommand("set app_apkPath "
					+ getPackageManager().getApplicationInfo(
							"us.xyzw.star3map", 0).sourceDir);
		} catch (PackageManager.NameNotFoundException e) {
		}
		String locale = getResources().getConfiguration().locale.toString();
		nativeDispatch.ExecuteCommand("set app_locale " + locale);
		// nativeDispatch.ExecuteCommand( "set app_locale iw_IL" );
		Application a = getApplication();
		drawable = new GlDrawable( a );
		setContentView(drawable);
		drawable.setOnTouchListener(touchListener);
		// Register the listener with the Location Manager to receive location
		// updates
		SensorManager sensorManager = (SensorManager) this
				.getSystemService(Context.SENSOR_SERVICE);
		Sensor accelerometer = sensorManager
				.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
		Sensor magnetometer = sensorManager
				.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
		sensorManager.registerListener(sensorEventListener, accelerometer,
				SensorManager.SENSOR_DELAY_GAME);
		sensorManager.registerListener(sensorEventListener, magnetometer,
				SensorManager.SENSOR_DELAY_GAME);

		DisplayMetrics metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(metrics);
		nativeDispatch.ExecuteCommand("set r_windowDpi " + metrics.densityDpi);


		PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		wakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK,
				"star3map wakeLock");
		
	}

	@Override
	protected void onStart() {
		Log.i(TAG, "init: s3mActivity.onStart() called");
		super.onStart();
	}

	private class DisplayTimerTask extends TimerTask {
		@Override
		public void run() {
			if (drawable.isReady()) {
				if (drawable.needsReshape()) {
					if (drawable.getCurrentGlContext() != 0) {
						drawable.makeGlContextCurrent(0);
					}
					nativeDispatch.Reshape(drawable.getDrawableWidth(),
							drawable.getDrawableHeight());
					drawable.clearNeedsReshape();
				}
				nativeDispatch.Display();
			}
		}
	};

	private DisplayTimerTask task = new DisplayTimerTask();
	private Timer timer = new Timer();

	@Override
	protected void onResume() {
		Log.i(TAG, "init: s3mActivity.onResume() called");
		super.onResume();
		nativeDispatch.Resume();
		wakeLock.acquire();
		if (drawable.isReady()) {
			Log.i(TAG, "drawable is ready");
		}
		LocationManager locationManager = (LocationManager) this
				.getSystemService(Context.LOCATION_SERVICE);
		try {
			Location netLoc = locationManager
					.getLastKnownLocation(LocationManager.NETWORK_PROVIDER);
			if (netLoc != null) {
				locationListener.onLocationChanged(netLoc);
			}
			Location gpsLoc = locationManager
					.getLastKnownLocation(LocationManager.GPS_PROVIDER);
			if (gpsLoc != null) {
				locationListener.onLocationChanged(gpsLoc);
			}
			locationManager.requestLocationUpdates(
						LocationManager.NETWORK_PROVIDER, 60000, 1000,
						locationListener);
			locationManager
						.requestLocationUpdates(LocationManager.GPS_PROVIDER,
								60000, 1000, locationListener);
		} catch (Exception e) {
			Log.e(TAG,
					"Error trying to get last known location or request updates. "
							+ e.getMessage());
		}
		try {
			//task.cancel();
			task = new DisplayTimerTask();
			timer.schedule(task, 100, 32);
		} catch (Exception e) {
			Log.e(TAG, "Caught exception in onResume(): " + e.getMessage());
		}
	}

	@Override
	protected void onPause() {
		Log.i(TAG, "init: s3mActivity.onPause() called");
		super.onPause();
		nativeDispatch.Pause();
		wakeLock.release();
		//// timer.cancel();
		task.cancel();
		//task = new DisplayTimerTask();		
		//timer.schedule(task, 100, 1000);
		LocationManager locationManager = (LocationManager) this
		.getSystemService(Context.LOCATION_SERVICE);
		locationManager.removeUpdates( locationListener );
	}

	@Override
	protected void onStop() {
		Log.i(TAG, "init: s3mActivity.onStop() called");
		super.onStop();
	}

	@Override
	protected void onDestroy() {
		Log.i(TAG, "init: s3mActivity.onDestroy() called");
		super.onDestroy();
		nativeDispatch.ExecuteCommand("quit");
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
	}

    
    public boolean openUrl( String url ) {
    	Intent i = new Intent(Intent.ACTION_VIEW);
    	i.setData(Uri.parse(url));
    	startActivity(i);
    	return true;
    }
	
	public boolean copyFromApk(String filename) {
		// implement me!
		Log.i(TAG, "in Java s3mActivity.copyFromApk( " + filename + " ) called");
		InputStream input = null;
		try {
			input = getAssets().open("base/" + filename);
			// Log.i( TAG, "apk file " + filename + " opened!" );

			File f = new java.io.File(getFilesDir().getAbsolutePath()
					+ "/base/" + filename);
			if (f.exists() == false) {
				// Log.i(TAG, "Creating local file " + f.getAbsolutePath() );
				f.getParentFile().mkdirs();
				f.createNewFile();
			}

			FileOutputStream output = new FileOutputStream(f);

			byte[] buf = new byte[4096];
			int len = 0;
			while ((len = input.read(buf, 0, 4096)) > 0) {
				output.write(buf, 0, len);
			}
			input.close();
			output.close();

		} catch (IOException e) {
			Log.e(TAG, "copyFromApk failure of  " + e.getMessage());
			return false;
		}

		return true;
	}

	public void login() {
	}

	public void logout() {
	}

	public void publish(String caption, String description) {
	}

	public void swapBuffers() {
		drawable.swapBuffers();
	}

	public int createGlContext(int shareContext) {
		return drawable.createGlContext(shareContext);
	}

	public void destroyGlContext(int context) {
		drawable.destroyGlContext(context);
	}

	public void makeGlContextCurrent(int context) {
		drawable.makeGlContextCurrent(context);
	}

	public int getCurrentGlContext() {
		return drawable.getCurrentGlContext();
	}

}
