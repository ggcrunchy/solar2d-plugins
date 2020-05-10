/*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
*/

//
//  LuaLoader.java
//  TemplateApp
//
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

// This corresponds to the name of the Lua library,
// e.g. [Lua] require "plugin.library"
package plugin.Bytemap;

import com.ansca.corona.CoronaActivity;
import com.ansca.corona.CoronaEnvironment;
import com.ansca.corona.CoronaRuntime;
import com.ansca.corona.CoronaRuntimeListener;
import com.naef.jnlua.JavaFunction;
import com.naef.jnlua.LuaState;
import com.naef.jnlua.NamedJavaFunction;
import android.content.res.AssetManager;

/**
 * Implements the Lua interface for a Corona plugin.
 * <p>
 * Only one instance of this class will be created by Corona for the lifetime of the application.
 * This instance will be re-used for every new Corona activity that gets created.
 */
@SuppressWarnings("WeakerAccess")
public class LuaLoader implements JavaFunction, CoronaRuntimeListener {
	private static final ThreadLocal<Integer> BytemapCoreRef = new ThreadLocal<>();

	/**
	 * Creates a new Lua interface to this plugin.
	 * <p>
	 * Note that a new LuaLoader instance will not be created for every CoronaActivity instance.
	 * That is, only one instance of this class will be created for the lifetime of the application process.
	 * This gives a plugin the option to do operations in the background while the CoronaActivity is destroyed.
	 */
	@SuppressWarnings("unused")
	public LuaLoader() {
	}

	private native int OpenLib (LuaState L);
	private native int SetAssets (AssetManager assets);

	/**
	 * Called when this plugin is being loaded via the Lua require() function.
	 * <p>
	 * Note that this method will be called every time a new CoronaActivity has been launched.
	 * This means that you'll need to re-initialize this plugin here.
	 * <p>
	 * Warning! This method is not called on the main UI thread.
	 * @param L Reference to the Lua state that the require() function was called from.
	 * @return Returns the number of values that the require() function will return.
	 *         <p>
	 *         Expected to return 1, the library that the require() function is loading.
	 */
	@Override
	public int invoke(LuaState L) {
		NamedJavaFunction[] luaFunctions = new NamedJavaFunction[] {
				new GetFileContentsWrapper(),
				new LoadTextureWrapper(),
				new NewTextureWrapper(),
		};

		L.register(L.toString( 1 ), luaFunctions);

		return 1;
	}

	/**
	 * Called after the Corona runtime has been created and just before executing the "main.lua" file.
	 * <p>
	 * Warning! This method is not called on the main thread.
	 * @param runtime Reference to the CoronaRuntime object that has just been loaded/initialized.
	 *                Provides a LuaState object that allows the application to extend the Lua API.
	 */
	@Override
	public void onLoaded(CoronaRuntime runtime) {
		// Note that this method will not be called the first time a Corona activity has been launched.
		// This is because this listener cannot be added to the CoronaEnvironment until after
		// this plugin has been required-in by Lua, which occurs after the onLoaded() event.
		// However, this method will be called when a 2nd Corona activity has been created.
		System.loadLibrary("plugin.BytemapCore");
	}

	/**
	 * Called just after the Corona runtime has executed the "main.lua" file.
	 * <p>
	 * Warning! This method is not called on the main thread.
	 * @param runtime Reference to the CoronaRuntime object that has just been started.
	 */
	@Override
	public void onStarted(CoronaRuntime runtime) {
	}

	/**
	 * Called just after the Corona runtime has been suspended which pauses all rendering, audio, timers,
	 * and other Corona related operations. This can happen when another Android activity (ie: window) has
	 * been displayed, when the screen has been powered off, or when the screen lock is shown.
	 * <p>
	 * Warning! This method is not called on the main thread.
	 * @param runtime Reference to the CoronaRuntime object that has just been suspended.
	 */
	@Override
	public void onSuspended(CoronaRuntime runtime) {
	}

	/**
	 * Called just after the Corona runtime has been resumed after a suspend.
	 * <p>
	 * Warning! This method is not called on the main thread.
	 * @param runtime Reference to the CoronaRuntime object that has just been resumed.
	 */
	@Override
	public void onResumed(CoronaRuntime runtime) {
	}

	/**
	 * Called just before the Corona runtime terminates.
	 * <p>
	 * This happens when the Corona activity is being destroyed which happens when the user presses the Back button
	 * on the activity, when the native.requestExit() method is called in Lua, or when the activity's finish()
	 * method is called. This does not mean that the application is exiting.
	 * <p>
	 * Warning! This method is not called on the main thread.
	 * @param runtime Reference to the CoronaRuntime object that is being terminated.
	 */
	@Override
	public void onExiting(CoronaRuntime runtime) {
	}

	/**
	 * Warning! This method is not called on the main thread.
	 * @param L Reference to the Lua state that the Lua function was called from.
	 * @return Returns the number of values to be returned by the library.init() function.
	 */
	@SuppressWarnings({"WeakerAccess", "SameReturnValue"})
	public int CallViaRef (LuaState L, String what) {
		Integer bc_ref = BytemapCoreRef.get();

		if (bc_ref == null)
		{
			OpenLib(L);	// ..., bytemap_core

			BytemapCoreRef.set(L.ref(LuaState.REGISTRYINDEX));	// ...

			bc_ref = BytemapCoreRef.get();
		}

		L.rawGet(LuaState.REGISTRYINDEX, bc_ref);	// ..., core
		L.getField(-1, what);	// ..., core, method
		L.insert(1);// method, ..., core
		L.pop(1);	// method, ...
		L.call(L.getTop() - 1, LuaState.MULTRET);	// ...

		return L.getTop();
	}

	/** Implements the getFileContents() Lua function. */
	@SuppressWarnings("unused")
	private class GetFileContentsWrapper implements NamedJavaFunction {
		/**
		 * Gets the name of the Lua function as it would appear in the Lua script.
		 * @return Returns the name of the custom Lua function.
		 */
		@Override
		public String getName() {
			return "getFileContents";
		}

		/**
		 * This method is called when the Lua function is called.
		 * <p>
		 * Warning! This method is not called on the main UI thread.
		 * @param L Reference to the Lua state.
		 *                 Needed to retrieve the Lua function's parameters and to return values back to Lua.
		 * @return Returns the number of values to be returned by the Lua function.
		 */
		@Override
		public int invoke (LuaState L) {
			CoronaActivity activity = CoronaEnvironment.getCoronaActivity();

			SetAssets(activity.getAssets());

			return CallViaRef(L, "getFileContents");
		}
	}

	/** Implements the loadTexture() Lua function. */
	@SuppressWarnings("unused")
	private class LoadTextureWrapper implements NamedJavaFunction {
		/**
		 * Gets the name of the Lua function as it would appear in the Lua script.
		 * @return Returns the name of the custom Lua function.
		 */
		@Override
		public String getName() {
			return "loadTexture";
		}

		/**
		 * This method is called when the Lua function is called.
		 * <p>
		 * Warning! This method is not called on the main UI thread.
		 * @param L Reference to the Lua state.
		 *                 Needed to retrieve the Lua function's parameters and to return values back to Lua.
		 * @return Returns the number of values to be returned by the Lua function.
		 */
		@Override
		public int invoke(LuaState L) {
			CoronaActivity activity = CoronaEnvironment.getCoronaActivity();

			SetAssets(activity.getAssets());

			return CallViaRef(L, "loadTexture");
		}
	}

	/** Implements the newTexture() Lua function. */
	@SuppressWarnings("unused")
	private class NewTextureWrapper implements NamedJavaFunction {
		/**
		 * Gets the name of the Lua function as it would appear in the Lua script.
		 * @return Returns the name of the custom Lua function.
		 */
		@Override
		public String getName() {
			return "newTexture";
		}

		/**
		 * This method is called when the Lua function is called.
		 * <p>
		 * Warning! This method is not called on the main UI thread.
		 * @param L Reference to the Lua state.
		 *                 Needed to retrieve the Lua function's parameters and to return values back to Lua.
		 * @return Returns the number of values to be returned by the Lua function.
		 */
		@Override
		public int invoke(LuaState L) {
			return CallViaRef(L, "newTexture");
		}
	}
}
