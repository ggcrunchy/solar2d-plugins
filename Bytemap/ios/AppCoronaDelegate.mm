//
//  AppCoronaDelegate.mm
//  TemplateApp
//
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "AppCoronaDelegate.h"

#import "CoronaRuntime.h"
#import "CoronaLua.h"

@implementation AppCoronaDelegate

// Sample custom Lua error handler
// Register it via: Corona::Lua::SetErrorHandler()
static int
MyTraceback( lua_State* L )
{
	if (!lua_isstring(L, 1))  // 'message' not a string?
		return 1;  // keep it intact

	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return 1;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return 1;
	}
	lua_pushvalue(L, 1);  // pass error message
	lua_pushinteger(L, 1);  // skip this function and traceback
	lua_call(L, 2, 1);  // call debug.traceback

	if ( ! lua_gethook( L ) ) // Don't interfere with Lua debugger hook
	{
		// Log result of calling debug.traceback()
		NSLog( @"[LUA ERROR]: %s", lua_tostring( L, -1 ) );
	}

	return 1;
}

static int
throwException( lua_State *L )
{
	NSArray *callStack = [NSThread callStackSymbols];
	NSString *callStackStr = [callStack componentsJoinedByString:@"\n"];

	luaL_error( L,
		"Throwing exception from native C code:\n"
		"\n"
		"%s\n"
		"\n", [callStackStr UTF8String] );

	return 0;
}

- (void)willLoadMain:(id<CoronaRuntime>)runtime
{
	lua_State *L = runtime.L;

	// Add 'Traceback' as custom error handler
	Corona::Lua::SetErrorHandler( MyTraceback );

	const luaL_Reg kFunctions[] =
	{
		"throwException", throwException,

		NULL, NULL
	};

	luaL_register( L, "myTests", kFunctions );
	lua_pop( L, 1 );
}

- (void)didLoadMain:(id<CoronaRuntime>)runtime
{
}

#pragma mark UIApplicationDelegate methods

// The following are stubs for common delegate methods. Uncomment and implement
// the ones you wish to be called. Or add additional delegate methods that
// you wish to be called.

/*
- (void)applicationWillResignActive:(UIApplication *)application
{
	// Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
	// Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}
*/

/*
- (void)applicationDidEnterBackground:(UIApplication *)application
{
	// Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
	// If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}
*/

/*
- (void)applicationWillEnterForeground:(UIApplication *)application
{
	// Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}
*/

/*
- (void)applicationDidBecomeActive:(UIApplication *)application
{
	// Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}
*/

/*
- (void)applicationWillTerminate:(UIApplication *)application
{
	// Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}
*/

@end
