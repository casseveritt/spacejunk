//
//  star3mapAppDelegate.h
//  star3map
//
//  Created by Cass Everitt on 1/30/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>
#import <FacebookSDK/FacebookSDK.h>

@class EAGLView;

@interface star3mapAppDelegate : NSObject <UIApplicationDelegate, CLLocationManagerDelegate, UIAccelerometerDelegate> {
    UIWindow *window;
    EAGLView *glView;
	CLLocationManager *locationManager;
	FBSession *fbsession;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet EAGLView *glView;
@property (nonatomic, retain) CLLocationManager *locationManager;
@property (nonatomic, retain) FBSession *fbsession;

@end

extern star3mapAppDelegate *myAppDelegate;

