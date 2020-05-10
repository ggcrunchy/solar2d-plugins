# solar2d-plugins
Source for various [Solar2D](https://solar2d.com) plugins, both in Lua and C / C++.

Some of these are fairly mature, a few are in development, while others are abandoned or on hold.

### The list so far ###

* AssetReader (a thin veneer over Android's asset features)
* Bytemap (original)
* cachestack (original, complements eigen)
* chips (itself)
* clipper (itself)
* eigen (uses Eigen, of course; WIP)
* FreeImage (and this uses FreeImage; abandoned as too difficult to compile on Android)
* impack (uses stb_*, jo_*, projectNe10, Accelerate, urraka's GIF code)
* ipc (luaipc)
* libtess2 (itself)
* luaffifb (itself; usable on desktop, but fails at least one test on Mac)
* luaproc (itself, plus concurrentqueue, libcuckoo, and pevents; and of course pthreads)
* MemoryBlob (uses libcuckoo)
* morphing (some code from a Chinese university)
* mwc (adapts code from a mailing list)
* msquares (from par)
* nudge (itself; on hold?)
* pagecurl (original)
* qu3e (itself; on hold?)
* quaternion (original)
* serialize (uses the three libraries so far)
* StableArray (original)
* streamlines (from par; WIP)
* tinyfiledialogs (itself)
* truetype (uses stb_truetype)

### Some libraries used by more than one of the above ###

* Accelerate
* ConcRT
* DirectXMath
* Hinnant's allocator (although only experimental)
* Another "stacka"-ish allocator (ditto)
* libdispatch
* pthreads-win32
* XMath (might now be superfluous; DirectXMath required a newer C++ version than all targets could honor)

### Others I might use later ###

* boost.simd
* ckfft
* libsimdpp
* simdfloat

### Licenses ###

Everything original to this repository falls under the MIT license.
