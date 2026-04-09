//
//  main.mm
//  App
//

#import <UIKit/UIKit.h>
#import "CoronaCards/CoronaApplicationMain.h"
#import "AppDelegate.h"

int main(int argc, char * argv[]) {
	@autoreleasepool {
	    return CoronaApplicationMain(argc, argv, [AppDelegate class]);
	}
}
