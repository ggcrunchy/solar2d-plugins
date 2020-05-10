# solar2d-plugins
Source for various [Solar2D](https://solar2d.com) plugins, both in Lua and C / C++.

Some of these are fairly mature, a few are in development, while others are abandoned or on hold.

### The list so far ###

* AssetReader (a thin veneer over [Android's asset management](https://developer.android.com/reference/android/content/res/AssetManager))
* Bytemap (original, but uses a bit of [stb](https://github.com/nothings/stb))
* cachestack (original, complements **eigen**)
* chips ([eponymous](https://github.com/floooh/chips))
* clipper ([eponymous](http://angusj.com/delphi/clipper.php))
* eigen ([eponymous](http://eigen.tuxfamily.org/index.php?title=Main_Page); WIP)
* FreeImage ([eponymous](http://freeimage.sourceforge.net); abandoned due to difficulty compiling on Android)
* impack (uses several bits of **stb**, many libraries from [Jon Olick](https://www.jonolick.com), [Spot](https://github.com/r-lyeh-archived/spot), [projectNe10](https://projectne10.github.io/Ne10/), [Accelerate](https://developer.apple.com/documentation/accelerate?language=objc), [SDF](https://github.com/memononen/SDF), [urraka's GIF code](https://gist.github.com/urraka/685d9a6340b26b830d49))
* ipc ([luaipc](https://github.com/siffiejoe/lua-luaipc), very slightly modified for use as a plugin)
* libtess2 ([eponymous](https://github.com/memononen/libtess2))
* luaffifb ([eponymous](https://github.com/facebookarchive/luaffifb); mostly usable on desktop, but fails at least one rather esoteric test on Mac)
* luaproc ([eponymous](https://github.com/askyrme/luaproc), plus [concurrentqueue](https://github.com/cameron314/concurrentqueue), [libcuckoo](https://github.com/efficient/libcuckoo), and [pevents](https://github.com/NeoSmart/PEvents); and of course **pthreads**)
* MemoryBlob (uses **libcuckoo**)
* morphing (some code from [here](https://cg.cs.tsinghua.edu.cn/people/~xianying/Papers/PoissonMVCs/index.html))
* mwc (adapts [code](https://www.math.uni-bielefeld.de/~sillke/ALGORITHMS/random/marsaglia-c) from George Marsaglia)
* msquares (from [par](https://github.com/prideout/par))
* nudge ([eponymous](https://github.com/rasmusbarr/nudge); on hold?)
* pagecurl (original)
* qu3e ([eponymous](https://github.com/RandyGaul/qu3e); on hold?)
* quaternion (original)
* serialize (uses [lpack](http://webserver2.tecgraf.puc-rio.br/~lhf/ftp/lua/#lpack), [lua-marshal](https://github.com/richardhundt/lua-marshal), and [struct](http://www.inf.puc-rio.br/~roberto/struct/))
* StableArray (original)
* streamlines (from **par**; WIP)
* tinyfiledialogs (eponymous)
* truetype (uses **stb**'s `truetype` module)

A few larger ones might be divvied up into more manageable sub-plugins.

### Some libraries used by more than one of the above ###

* Accelerate (beyond that used by **impack**)
* [ConcRT](docs.microsoft.com/en-us/cpp/parallel/concrt/concurrency-runtime)
* [DirectXMath](https://github.com/Microsoft/DirectXMath)
* [libdispatch](https://github.com/apple/swift-corelibs-libdispatch)
* [pthreads-win32](https://locklessinc.com/articles/pthreads_on_windows/)
* [XMath](https://github.com/Napoleon314/XMath) (might now be superfluous; **DirectXMath** required a newer C++ version than all targets could honor at the time)

### Licenses ###

Everything original to this repository falls under the MIT license.
