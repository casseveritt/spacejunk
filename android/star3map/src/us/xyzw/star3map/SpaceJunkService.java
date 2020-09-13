package us.xyzw.star3map;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

public class SpaceJunkService extends Service {

	private static final int FLYOVER_ID = 1;

	public void createNotification( ) {
		String ns = Context.NOTIFICATION_SERVICE;
		NotificationManager mNotificationManager = (NotificationManager) getSystemService(ns);
		CharSequence tickerText = "International Space Station flying over soon!";
		long when = System.currentTimeMillis();

		Notification notification = new Notification(R.drawable.icon_flyover_notification, tickerText, when);
		Context context = getApplicationContext();
		CharSequence contentTitle = "Space Junk";
		CharSequence contentText = "International Space Station flying over soon!";
		Intent intent = new Intent( this, star3map.class );
		PendingIntent contentIntent = PendingIntent.getActivity(this, 0, intent, 0);

		notification.flags |= Notification.FLAG_AUTO_CANCEL;
		notification.setLatestEventInfo(context, contentTitle, contentText, contentIntent);

		mNotificationManager.notify( FLYOVER_ID, notification);
		Log.i("SpaceJunkService", "Created Notification!");

	}

	@Override
	public void onCreate() {
		Log.i("SpaceJunkService", "onCreate called");
	}

	/*
	Runnable r = new Runnable() {
		@Override
		public void run() {
			try {
				for (int i = 0; i < 10; i++) {
					Thread.sleep(6000);
					Log.i( "SpaceJunkService", "SpaceJunkServiceRunnable run loop - " + i  );
				}
			} catch (Exception e) {
			}
			createNotification();			
		}
	};
    */
	
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		Log.i("SpaceJunkService", "onStartCommand called");
		//new Thread( r );
		try {
			for (int i = 0; i < 5; i++) {
				Thread.sleep(3000);
				Log.i( "SpaceJunkService", "SpaceJunkServiceRunnable run loop - " + i  );
			}
		} catch (Exception e) {
		}
		Log.i( "SpaceJunkService", "About to create notification."  );
		//createNotification();			
		return START_STICKY;
	}	

    @Override
    public void onDestroy() {
		Log.i("SpaceJunkService", "onDestroy called");
    }
	
	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}
};

