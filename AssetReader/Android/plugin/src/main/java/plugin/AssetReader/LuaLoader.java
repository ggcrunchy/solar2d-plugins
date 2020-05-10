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
package plugin.AssetReader;

import com.ansca.corona.CoronaActivity;
import com.ansca.corona.CoronaEnvironment;
import com.ansca.corona.CoronaRuntime;
import com.ansca.corona.CoronaRuntimeListener;
import com.naef.jnlua.JavaFunction;
import com.naef.jnlua.LuaState;
import com.naef.jnlua.NamedJavaFunction;
import android.content.res.AssetManager;
import java.io.IOException;

/**
 * Implements the Lua interface for a Corona plugin.
 * <p>
 * Only one instance of this class will be created by Corona for the lifetime of the application.
 * This instance will be re-used for every new Corona activity that gets created.
 */
@SuppressWarnings("WeakerAccess")
public class LuaLoader implements JavaFunction, CoronaRuntimeListener {
	/**
	 * Creates a new Lua interface to this plugin.
	 * <p>
	 * Note that a new LuaLoader instance will not be created for every CoronaActivity instance.
	 * That is, only one instance of this class will be created for the lifetime of the application process.
	 * This gives a plugin the option to do operations in the background while the CoronaActivity is destroyed.
	 */
	@SuppressWarnings("unused")
	public LuaLoader() {}

	private native void BindAssetManager (AssetManager manager);
	private native int NewProxy (LuaState L);
	private native int Read (LuaState L);

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
	public int invoke (LuaState L)
	{
		NamedJavaFunction[] luaFunctions = new NamedJavaFunction[] {
			new EnumerateDirectoryWrapper(),
			new NewProxyWrapper(),
			new ReadWrapper()
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
	public void onLoaded (CoronaRuntime runtime)
	{
		// Note that this method will not be called the first time a Corona activity has been launched.
		// This is because this listener cannot be added to the CoronaEnvironment until after
		// this plugin has been required-in by Lua, which occurs after the onLoaded() event.
		// However, this method will be called when a 2nd Corona activity has been created.
		System.loadLibrary("plugin.AssetReaderCore");

		CoronaActivity activity = CoronaEnvironment.getCoronaActivity();

		BindAssetManager(activity.getAssets());
	}

	/**
	 * Called just after the Corona runtime has executed the "main.lua" file.
	 * <p>
	 * Warning! This method is not called on the main thread.
	 * @param runtime Reference to the CoronaRuntime object that has just been started.
	 */
	@Override
	public void onStarted (CoronaRuntime runtime) {}

	/**
	 * Called just after the Corona runtime has been suspended which pauses all rendering, audio, timers,
	 * and other Corona related operations. This can happen when another Android activity (ie: window) has
	 * been displayed, when the screen has been powered off, or when the screen lock is shown.
	 * <p>
	 * Warning! This method is not called on the main thread.
	 * @param runtime Reference to the CoronaRuntime object that has just been suspended.
	 */
	@Override
	public void onSuspended (CoronaRuntime runtime) {}

	/**
	 * Called just after the Corona runtime has been resumed after a suspend.
	 * <p>
	 * Warning! This method is not called on the main thread.
	 * @param runtime Reference to the CoronaRuntime object that has just been resumed.
	 */
	@Override
	public void onResumed (CoronaRuntime runtime) {}

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
	public void onExiting (CoronaRuntime runtime) {}

	@SuppressWarnings("unused")
	private class EnumerateDirectoryWrapper implements NamedJavaFunction {
		@Override
		public String getName () {
			return "EnumerateDirectory";
		}

		// N.B. the NDK seems to omit directories
		@Override
		public int invoke (LuaState L)
		{
			String path = L.checkString(1);
			CoronaActivity activity = CoronaEnvironment.getCoronaActivity();

			try {
				String[] list = activity.getAssets().list(path);

				L.setTop(2);// path, into

				if (!L.isTable(2)) L.newTable();	// path, into / nil[, out]

				int n = 0;

				for (String name : list)
				{
					L.pushString(name);	// path, into / nil[, out], name
					L.rawSet(-2, ++n);	// path, into / nil[, out] = { ..., name }
				}

				if (n > 0)
				{
					L.pushInteger(n);	// path, into / nil[, out], n

					return 2;
				}
			} catch (IOException e) {}

			L.pushNil();

			return 1;
		}
	}


	@SuppressWarnings("unused")
	private class NewProxyWrapper implements NamedJavaFunction {
		@Override
		public String getName () {
			return "NewProxy";
		}

		@Override
		public int invoke (LuaState L) {
			return NewProxy(L);
		}
	}

	@SuppressWarnings("unused")
	private class ReadWrapper implements NamedJavaFunction {
		@Override
		public String getName () {
			return "Read";
		}

		@Override
		public int invoke (LuaState L) {
			return Read(L);
		}
	}
}
