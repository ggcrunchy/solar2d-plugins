// Adapted from https://github.com/saghul/txiki.js

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

#include "common.h"

//
//
//

#if 1
    #define MULTILINE(...) #__VA_ARGS__ /* cf. https://stackoverflow.com/a/14293615 */
#else
    #define MULTILINE(body) R"(body)"
    // TODO: ^^ literal seems to resolve before the macro does
    // would preserve newlines, but Intellisense makes it too annoying to use during dev
#endif

//
//
//

static const char kWebAssemblyPolyfill[] = MULTILINE(
    const { wasm } = globalThis.__bootstrap;

    const kArray = Symbol('kArray');
    const kBuffer = Symbol('kBuffer');
    const kMemory = Symbol('kMemory');
    const kTableSize = Symbol('kTableSize');
    const kInternalMemory = Symbol('kInternalMemory');
    const kWasmModule = Symbol('kWasmModule');
    const kWasmModuleRef = Symbol('kWasmModuleRef');
    const kWasmExports = Symbol('kWasmExports');
    const kWasmInstance = Symbol('kWasmInstance');
    const kWasmInstances = Symbol('kWasmInstances');
    const kWasiLinked = Symbol('kWasiLinked');
    const kWasiStarted = Symbol('kWasiStarted');
    const kWasiOptions = Symbol('kWasiOptions');

    // Emscripten wants this... n.b. NOT cryptographic 
    globalThis.crypto = {
        getRandomValues(arr) {
            var len = arr.length;
            for (var i = 0; i < len; i++) {
                arr[i] = Math.floor(Math.random() * 255 + 0.5);
            }
            return arr;
        }
    };

    class CompileError extends Error {
        constructor(...args) {
            super(...args);
            this.name = 'CompileError';
        }
    }

    class LinkError extends Error {
        constructor(...args) {
            super(...args);
            this.name = 'LinkError';
        }
    }

    class RuntimeError extends Error {
        constructor(...args) {
            super(...args);
            this.name = 'RuntimeError';
        }
    }


    function getWasmError(e) {
        switch (e.wasmError) {
            case 'CompileError':
                return new CompileError(e.message);
            case 'LinkError':
                return new LinkError(e.message);
            case 'RuntimeError':
                return new RuntimeError(e.message);
            default:
                return new TypeError(`Invalid WASM error: ${e.wasmError}`);
        }
    }

    function callIndexedFunction(index, ...args) {
        const instance = this;

        try {
            return instance.callIndexedFunction(index, ...args);
        } catch (e) {
            if (e.wasmError) {
                throw getWasmError(e);
            } else {
                throw e;
            }
        }
    }

    function callWasmFunction(name, ...args) {
        const instance = this;

        try {
            return instance.callFunction(name, ...args);
        } catch (e) {
            if (e.wasmError) {
                throw getWasmError(e);
            } else {
                throw e;
            }
        }
    }

    function buildInstance(mod) {
        try {
            return wasm.buildInstance(mod);
        } catch (e) {
            if (e.wasmError) {
                throw getWasmError(e);
            } else {
                throw e;
            }
        }
    }

    function linkWasi(instance) {
        try {
            instance.linkWasi();
        } catch (e) {
            if (e.wasmError) {
                throw getWasmError(e);
            } else {
                throw e;
            }
        }
    }

    function parseModule(buf) {
        try {
            return wasm.parseModule(buf);
        } catch (e) {
            if (e.wasmError) {
                throw getWasmError(e);
            } else {
                throw e;
            }
        }
    }

    class Module {
        constructor(buf) {
            this[kWasmModule] =  parseModule(buf);
        }

        static exports(module) {
            return wasm.moduleExports(module[kWasmModule]);
        }

        static imports(module) {
            return wasm.moduleImports(module[kWasmModule]);
        }
    }

    class Memory {
        constructor(descriptor) {
            if (!descriptor[kInternalMemory]) {
                if (!descriptor.initial) {
                    throw new TypeError('initial not specified');
                } else if (descriptor.maximum && descriptor.maximum < descriptor.initial) {
                    throw new RangeError('maximum is smaller than initial');
                } else if (descriptor.initial > 65536) {
                    throw new RangeError('initial exceeds 2^16');
                }
            }
                
            // TODO `shared` seems to need resize()
            if (descriptor.shared && !descriptor.maximum) {
                throw new TypeError('shared is true, yet maximum is not specified');
            }

            if (typeof descriptor != 'object') {
                throw new TypeError('descriptor must be an object');
            } else if (descriptor.shared) {
                throw new TypeError('shared memories NYI');
            } else if (!descriptor[kInternalMemory]) {
                const arr = new Uint8Array(descriptor.initial * 65536);

                arr.fill(0);

                this[kArray] = arr;
            }
        }

        grow(delta) {
            if (typeof delta != 'number' || delta < 0) {
                throw new TypeError('invalid delta');
            } else {
                const arr = this[kArray];
                var oldPageCount;

                if (arr) {
                    oldPageCount = arr.length / 65536;
                } else {
                    oldPageCount = wasm.requestMemoryPages(delta);
                }

                if (!this[kBuffer].detached) {
                    const _ = this[kBuffer].transfer();
                }

                if (arr && delta > 0) {
                    const added = new Uint8Array(delta * 65536);

                    added.fill(0);

                    this[kArray] = [...arr].concat(added);

                    wasm.bindMemory(this[kArray]);
                }

                return oldPageCount;
            }
        }

        get buffer() {
            const old = this[kBuffer];
            if (!old || old.detached) {
                try {
                    if (this[kArray]) {
                        this[kBuffer] = wasm.arrayBackedBuffer(this[kArray]);
                    } else {
                        this[kBuffer] = wasm.memoryBuffer;
                    }
                } catch (_) {
                    return old; // ??? (some docs seem to suggest this can fail gracefully?)
                }
            }

            return this[kBuffer];
        }
    }

    class Table {
        constructor(descriptor) {
            this[kTableSize] = descriptor.initial;
        }

        get(index) {
            return callIndexedFunction.bind(this[kWasmInstance], index);
        }
    }

    class Instance {
        constructor(module, importObject = {}) {
            const instance = buildInstance(module[kWasmModule]);
            const _imports = Module.imports(module);

            Instance.visit(_imports, importObject, (item, jobj) => {
                if (item.kind === 'memory') {
                    wasm.bindMemory(jobj[kArray]);

                    instance[kMemory] = jobj;
                }
            });

            instance.loadModule();

            if (importObject.wasi_unstable) {
                linkWasi(instance);
                this[kWasiLinked] = true;
            }

            const _exports = Module.exports(module);
            const exports = Object.create(null);

            for (const item of _exports) {
                if (item.kind === 'function') {
                    exports[item.name] = callWasmFunction.bind(instance, item.name);
                } else if (item.kind === 'memory') {
                    const descriptor = {};

                    descriptor[kInternalMemory] = true;

                    exports[item.name] = new Memory({ [kInternalMemory]: true });//descriptor);
                } else if (item.kind === 'table') {
                    exports[item.name] = new Table({ element: "anyfunc", initial: item.size });
                    exports[item.name][kWasmInstance] = instance;
                }
            }

            Instance.visit(_imports, importObject, (item, jobj) => {
                if (item.kind === 'function') {
                    instance.linkImport(item.module, item.name, jobj);
                } else if (item.kind === 'global') {
                    const _ = instance.setGlobal(item.name, jobj);
                } else if (item.kind === 'table') {
                    // TODO?
                }
            });

            this[kWasmInstance] = instance;
            this[kWasmExports] = Object.freeze(exports);
            this[kWasmModuleRef] = module;

            globalThis.WebAssembly[kWasmInstances].push(this);
        }

        static visit(list, ref, func) {
            for (const item of list) {
                const mod = ref[item.module];
                if (!mod) {
                    throw new LinkError('Unmatched link module');
                }

                const jobj = mod[item.name];
                if (!jobj) {
                    throw new LinkError('No object to link');
                }

                func(item, jobj);
            }
        }

        get exports() {
            return this[kWasmExports];
        }
    }

    class WASI {
        wasiImport = 'w4s1';  // Doesn't matter right now.

        constructor(options = { args: [], env: {}, preopens: {} }) {
            this[kWasiStarted] = false;

            if (options === null || typeof options !== 'object') {
                throw new TypeError('options must be an object');
            }

            this[kWasiOptions] = JSON.parse(JSON.stringify(options));
        }

        start(instance) {
            if (this[kWasiStarted]) {
                throw new Error('WASI instance has already started');
            }

            if (!instance[kWasiLinked]) {
                throw new Error('WASM instance doesn\'t have WASI linked');
            }

            if (!instance.exports._start) {
                throw new TypeError('WASI entrypoint not found');
            }

            this[kWasiStarted] = true;
            instance.exports._start(...(this[kWasiOptions].args ?? []));
        }
    }

    class WebAssembly {
        Module = Module;
        Instance = Instance;
        Memory = Memory;
        CompileError = CompileError;
        LinkError = LinkError;
        RuntimeError = RuntimeError;
        WASI = WASI;      

        constructor() {
            this[kWasmInstances] = [];
        }

        async compile(src) {
            return new Module(src);
        }

        async instantiate(src, importObject) {
            const module = await this.compile(src);
            const instance = new Instance(module, importObject);

            return { module, instance };
        }
    }


    Object.defineProperty(globalThis, 'WebAssembly', {
        enumerable: true,
        configurable: true,
        writable: true,
        value: new WebAssembly()
    });
);

//
//
//

const char * GetWebAssemblyPolyfill (size_t * len)
{
    *len = sizeof(kWebAssemblyPolyfill) - 1;

    return kWebAssemblyPolyfill;
}