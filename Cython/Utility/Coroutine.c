//////////////////// GeneratorYieldFrom.proto ////////////////////

static CYTHON_INLINE PyObject* __Pyx_Generator_Yield_From(__pyx_CoroutineObject *gen, PyObject *source);

//////////////////// GeneratorYieldFrom ////////////////////
//@requires: Generator

static CYTHON_INLINE PyObject* __Pyx_Generator_Yield_From(__pyx_CoroutineObject *gen, PyObject *source) {
    PyObject *source_gen, *retval;
    source_gen = PyObject_GetIter(source);
    if (unlikely(!source_gen)) {
        #ifdef __Pyx_Coroutine_USED
        #if CYTHON_COMPILING_IN_CPYTHON
        // avoid exception instantiation if possible
        if (PyErr_Occurred() == PyExc_TypeError
        #else
        if (PyErr_ExceptionMatches(PyExc_TypeError)
        #endif
                && __Pyx_Coroutine_CheckExact(source)) {
            PyErr_Clear();
            // TODO: this should only happen for types.coroutine()ed generators, but we can't determine that here
            source_gen = __Pyx_Coroutine_await(source);
        } else
        #endif
        return NULL;
    }
    // source_gen is now the iterator, make the first next() call
    retval = Py_TYPE(source_gen)->tp_iternext(source_gen);
    if (likely(retval)) {
        gen->yieldfrom = source_gen;
        return retval;
    }
    Py_DECREF(source_gen);
    return NULL;
}


//////////////////// CoroutineYieldFrom.proto ////////////////////

static CYTHON_INLINE PyObject* __Pyx_Coroutine_Yield_From(__pyx_CoroutineObject *gen, PyObject *source);

//////////////////// CoroutineYieldFrom ////////////////////
//@requires: Coroutine
//@requires: GetAwaitIter

static CYTHON_INLINE PyObject* __Pyx_Coroutine_Yield_From(__pyx_CoroutineObject *gen, PyObject *source) {
    PyObject *retval;
    if (__Pyx_Coroutine_CheckExact(source)) {
        retval = __Pyx_Generator_Next(source);
        if (retval) {
            Py_INCREF(source);
            gen->yieldfrom = source;
            return retval;
        }
    } else {
        PyObject *source_gen = __Pyx__Coroutine_GetAwaitableIter(source);
        if (unlikely(!source_gen))
            return NULL;
        // source_gen is now the iterator, make the first next() call
        if (__Pyx_Coroutine_CheckExact(source_gen)) {
            retval = __Pyx_Generator_Next(source_gen);
        } else {
            retval = Py_TYPE(source_gen)->tp_iternext(source_gen);
        }
        if (retval) {
            gen->yieldfrom = source_gen;
            return retval;
        }
        Py_DECREF(source_gen);
    }
    return NULL;
}


//////////////////// GetAwaitIter.proto ////////////////////

static CYTHON_INLINE PyObject *__Pyx_Coroutine_GetAwaitableIter(PyObject *o); /*proto*/
static PyObject *__Pyx__Coroutine_GetAwaitableIter(PyObject *o); /*proto*/

//////////////////// GetAwaitIter ////////////////////
//@requires: Coroutine
//@requires: ObjectHandling.c::PyObjectGetAttrStr
//@requires: ObjectHandling.c::PyObjectCallNoArg
//@requires: ObjectHandling.c::PyObjectCallOneArg

static CYTHON_INLINE PyObject *__Pyx_Coroutine_GetAwaitableIter(PyObject *o) {
#ifdef __Pyx_Coroutine_USED
    if (__Pyx_Coroutine_CheckExact(o)) {
        Py_INCREF(o);
        return o;
    }
#endif
    return __Pyx__Coroutine_GetAwaitableIter(o);
}

// adapted from genobject.c in Py3.5
static PyObject *__Pyx__Coroutine_GetAwaitableIter(PyObject *obj) {
    PyObject *res;
#if PY_MAJOR_VERSION >= 3
    __Pyx_PyAsyncMethodsStruct* am = __Pyx_PyType_AsAsync(obj);
    if (likely(am && am->am_await)) {
        res = (*am->am_await)(obj);
    } else
#endif
    {
#if PY_VERSION_HEX >= 0x030500B1
    #if CYTHON_COMPILING_IN_CPYTHON
        if (PyGen_CheckCoroutineExact(obj)) {
            // Python generator marked with "@types.coroutine" decorator
            Py_INCREF(obj);
            return obj;
        }
    #endif
        // no slot => no method
        goto slot_error;
#else
        PyObject *method = __Pyx_PyObject_GetAttrStr(obj, PYIDENT("__await__"));
        if (unlikely(!method)) goto slot_error;
        #if CYTHON_COMPILING_IN_CPYTHON
        if (likely(PyMethod_Check(method))) {
            PyObject *self = PyMethod_GET_SELF(method);
            if (likely(self)) {
                PyObject *function = PyMethod_GET_FUNCTION(method);
                res = __Pyx_PyObject_CallOneArg(function, self);
            } else
                res = __Pyx_PyObject_CallNoArg(method);
        } else
        #endif
            res = __Pyx_PyObject_CallNoArg(method);
        Py_DECREF(method);
#endif
    }
    if (unlikely(!res)) goto bad;
    if (!PyIter_Check(res)) {
        PyErr_Format(PyExc_TypeError,
                     "__await__() returned non-iterator of type '%.100s'",
                     Py_TYPE(res)->tp_name);
        Py_CLEAR(res);
    } else {
        int is_coroutine = 0;
        #ifdef __Pyx_Coroutine_USED
        is_coroutine |= __Pyx_Coroutine_CheckExact(res);
        #endif
        #if PY_VERSION_HEX >= 0x030500B1
        is_coroutine |= PyGen_CheckCoroutineExact(res);
        #endif
        if (unlikely(is_coroutine)) {
            /* __await__ must return an *iterator*, not
               a coroutine or another awaitable (see PEP 492) */
            PyErr_SetString(PyExc_TypeError,
                            "__await__() returned a coroutine");
            Py_CLEAR(res);
        }
    }
    return res;
slot_error:
    PyErr_Format(PyExc_TypeError,
                 "object %.100s can't be used in 'await' expression",
                 Py_TYPE(obj)->tp_name);
bad:
    return NULL;
}


//////////////////// AsyncIter.proto ////////////////////

static CYTHON_INLINE PyObject *__Pyx_Coroutine_GetAsyncIter(PyObject *o); /*proto*/
static CYTHON_INLINE PyObject *__Pyx_Coroutine_AsyncIterNext(PyObject *o); /*proto*/

//////////////////// AsyncIter ////////////////////
//@requires: GetAwaitIter
//@requires: ObjectHandling.c::PyObjectCallMethod0

static CYTHON_INLINE PyObject *__Pyx_Coroutine_GetAsyncIter(PyObject *obj) {
#if PY_MAJOR_VERSION >= 3
    __Pyx_PyAsyncMethodsStruct* am = __Pyx_PyType_AsAsync(obj);
    if (likely(am && am->am_aiter)) {
        return (*am->am_aiter)(obj);
    }
#endif
#if PY_VERSION_HEX < 0x030500B1
    {
        PyObject *iter = __Pyx_PyObject_CallMethod0(obj, PYIDENT("__aiter__"));
        if (likely(iter))
            return iter;
        // FIXME: for the sake of a nicely conforming exception message, assume any AttributeError meant '__aiter__'
        if (!PyErr_ExceptionMatches(PyExc_AttributeError))
            return NULL;
    }
#endif

    PyErr_Format(PyExc_TypeError, "'async for' requires an object with __aiter__ method, got %.100s",
                 Py_TYPE(obj)->tp_name);
    return NULL;
}

static CYTHON_INLINE PyObject *__Pyx_Coroutine_AsyncIterNext(PyObject *obj) {
#if PY_MAJOR_VERSION >= 3
    __Pyx_PyAsyncMethodsStruct* am = __Pyx_PyType_AsAsync(obj);
    if (likely(am && am->am_anext)) {
        return (*am->am_anext)(obj);
    }
#endif
#if PY_VERSION_HEX < 0x030500B1
    {
        PyObject *value = __Pyx_PyObject_CallMethod0(obj, PYIDENT("__anext__"));
        if (likely(value))
            return value;
    }
    // FIXME: for the sake of a nicely conforming exception message, assume any AttributeError meant '__anext__'
    if (PyErr_ExceptionMatches(PyExc_AttributeError))
#endif
        PyErr_Format(PyExc_TypeError, "'async for' requires an object with __anext__ method, got %.100s",
                     Py_TYPE(obj)->tp_name);
    return NULL;
}


//////////////////// pep479.proto ////////////////////

static void __Pyx_Generator_Replace_StopIteration(void); /*proto*/

//////////////////// pep479 ////////////////////
//@requires: Exceptions.c::GetException

static void __Pyx_Generator_Replace_StopIteration(void) {
    PyObject *exc, *val, *tb;
    // Chain exceptions by moving StopIteration to exc_info before creating the RuntimeError.
    // In Py2.x, no chaining happens, but the exception still stays visible in exc_info.
    __Pyx_GetException(&exc, &val, &tb);
    Py_XDECREF(exc);
    Py_XDECREF(val);
    Py_XDECREF(tb);
    PyErr_SetString(PyExc_RuntimeError, "generator raised StopIteration");
}


//////////////////// CoroutineBase.proto ////////////////////

typedef PyObject *(*__pyx_coroutine_body_t)(PyObject *, PyObject *);

typedef struct {
    PyObject_HEAD
    __pyx_coroutine_body_t body;
    PyObject *closure;
    PyObject *exc_type;
    PyObject *exc_value;
    PyObject *exc_traceback;
    PyObject *gi_weakreflist;
    PyObject *classobj;
    PyObject *yieldfrom;
    PyObject *gi_name;
    PyObject *gi_qualname;
    int resume_label;
    // using T_BOOL for property below requires char value
    char is_running;
} __pyx_CoroutineObject;

static __pyx_CoroutineObject *__Pyx__Coroutine_New(PyTypeObject *type, __pyx_coroutine_body_t body,
                                                   PyObject *closure, PyObject *name, PyObject *qualname); /*proto*/
static int __Pyx_Coroutine_clear(PyObject *self); /*proto*/

#if 1 || PY_VERSION_HEX < 0x030300B0
static int __Pyx_PyGen_FetchStopIterationValue(PyObject **pvalue); /*proto*/
#else
#define __Pyx_PyGen_FetchStopIterationValue(pvalue) PyGen_FetchStopIterationValue(pvalue)
#endif


//////////////////// Coroutine.proto ////////////////////

#define __Pyx_Coroutine_USED
static PyTypeObject *__pyx_CoroutineType = 0;
#define __Pyx_Coroutine_CheckExact(obj) (Py_TYPE(obj) == __pyx_CoroutineType)

#define __Pyx_Coroutine_New(body, closure, name, qualname)  \
    __Pyx__Coroutine_New(__pyx_CoroutineType, body, closure, name, qualname)

static int __pyx_Coroutine_init(void); /*proto*/
static PyObject *__Pyx_Coroutine_await(PyObject *coroutine); /*proto*/


//////////////////// Generator.proto ////////////////////

#define __Pyx_Generator_USED
static PyTypeObject *__pyx_GeneratorType = 0;
#define __Pyx_Generator_CheckExact(obj) (Py_TYPE(obj) == __pyx_GeneratorType)

#define __Pyx_Generator_New(body, closure, name, qualname)  \
    __Pyx__Coroutine_New(__pyx_GeneratorType, body, closure, name, qualname)

static int __pyx_Generator_init(void); /*proto*/


//////////////////// CoroutineBase ////////////////////
//@requires: Exceptions.c::PyErrFetchRestore
//@requires: Exceptions.c::SwapException
//@requires: Exceptions.c::RaiseException
//@requires: ObjectHandling.c::PyObjectCallMethod1
//@requires: ObjectHandling.c::PyObjectGetAttrStr
//@requires: CommonTypes.c::FetchCommonType

#include <structmember.h>
#include <frameobject.h>

static PyObject *__Pyx_Generator_Next(PyObject *self);
static PyObject *__Pyx_Coroutine_Send(PyObject *self, PyObject *value);
static PyObject *__Pyx_Coroutine_Close(PyObject *self);
static PyObject *__Pyx_Coroutine_Throw(PyObject *gen, PyObject *args);

#define __Pyx_Coroutine_Undelegate(gen) Py_CLEAR((gen)->yieldfrom)

//   If StopIteration exception is set, fetches its 'value'
//   attribute if any, otherwise sets pvalue to None.
//
//   Returns 0 if no exception or StopIteration is set.
//   If any other exception is set, returns -1 and leaves
//   pvalue unchanged.
#if 1 || PY_VERSION_HEX < 0x030300B0
static int __Pyx_PyGen_FetchStopIterationValue(PyObject **pvalue) {
    PyObject *et, *ev, *tb;
    PyObject *value = NULL;
    int result;

    __Pyx_ErrFetch(&et, &ev, &tb);

    if (!et) {
        Py_XDECREF(tb);
        Py_XDECREF(ev);
        Py_INCREF(Py_None);
        *pvalue = Py_None;
        return 0;
    }

    // most common case: plain StopIteration without or with separate argument
    if (likely(et == PyExc_StopIteration)) {
        int error = 0;
#if PY_VERSION_HEX >= 0x030300A0
        if (ev && Py_TYPE(ev) == (PyTypeObject*)PyExc_StopIteration) {
            value = ((PyStopIterationObject *)ev)->value;
            Py_INCREF(value);
            Py_DECREF(ev);
            Py_XDECREF(tb);
            Py_DECREF(et);
            *pvalue = value;
            return 0;
        }
#endif
        if (!ev || !(error = PyObject_IsInstance(ev, PyExc_StopIteration))) {
            // PyErr_SetObject() and friends put the value directly into ev
            if (!ev) {
                Py_INCREF(Py_None);
                ev = Py_None;
            } else if (PyTuple_Check(ev)) {
                // however, if it's a tuple, it is interpreted as separate constructor arguments (surprise!)
                if (PyTuple_GET_SIZE(ev) >= 1) {
                    PyObject *value;
#if CYTHON_COMPILING_IN_CPYTHON
                    value = PySequence_ITEM(ev, 0);
#else
                    value = PyTuple_GET_ITEM(ev, 0);
                    Py_INCREF(value);
#endif
                    Py_DECREF(ev);
                    ev = value;
                } else {
                    Py_INCREF(Py_None);
                    Py_DECREF(ev);
                    ev = Py_None;
                }
            }
            Py_XDECREF(tb);
            Py_DECREF(et);
            *pvalue = ev;
            return 0;
        }
        if (unlikely(error == -1)) {
            // error during isinstance() check
            return -1;
        }
    } else if (!PyErr_GivenExceptionMatches(et, PyExc_StopIteration)) {
        __Pyx_ErrRestore(et, ev, tb);
        return -1;
    }

    // otherwise: normalise and check what that gives us
    PyErr_NormalizeException(&et, &ev, &tb);
    result = PyObject_IsInstance(ev, PyExc_StopIteration);
    if (unlikely(!result)) {
        // looks like normalisation failed - raise the new exception
        __Pyx_ErrRestore(et, ev, tb);
        return -1;
    }
    Py_XDECREF(tb);
    Py_DECREF(et);
    if (unlikely(result == -1)) {
        // error during isinstance() check
        Py_DECREF(ev);
        return -1;
    }
#if PY_VERSION_HEX >= 0x030300A0
    value = ((PyStopIterationObject *)ev)->value;
    Py_INCREF(value);
    Py_DECREF(ev);
#else
    {
        PyObject* args = __Pyx_PyObject_GetAttrStr(ev, PYIDENT("args"));
        Py_DECREF(ev);
        if (likely(args)) {
            value = PySequence_GetItem(args, 0);
            Py_DECREF(args);
        }
        if (unlikely(!value)) {
            __Pyx_ErrRestore(NULL, NULL, NULL);
            Py_INCREF(Py_None);
            value = Py_None;
        }
    }
#endif
    *pvalue = value;
    return 0;
}
#endif

static CYTHON_INLINE
void __Pyx_Coroutine_ExceptionClear(__pyx_CoroutineObject *self) {
    PyObject *exc_type = self->exc_type;
    PyObject *exc_value = self->exc_value;
    PyObject *exc_traceback = self->exc_traceback;

    self->exc_type = NULL;
    self->exc_value = NULL;
    self->exc_traceback = NULL;

    Py_XDECREF(exc_type);
    Py_XDECREF(exc_value);
    Py_XDECREF(exc_traceback);
}

static CYTHON_INLINE
int __Pyx_Coroutine_CheckRunning(__pyx_CoroutineObject *gen) {
    if (unlikely(gen->is_running)) {
        PyErr_SetString(PyExc_ValueError,
                        "generator already executing");
        return 1;
    }
    return 0;
}

static CYTHON_INLINE
PyObject *__Pyx_Coroutine_SendEx(__pyx_CoroutineObject *self, PyObject *value) {
    PyObject *retval;

    assert(!self->is_running);

    if (unlikely(self->resume_label == 0)) {
        if (unlikely(value && value != Py_None)) {
            PyErr_SetString(PyExc_TypeError,
                            "can't send non-None value to a "
                            "just-started generator");
            return NULL;
        }
    }

    if (unlikely(self->resume_label == -1)) {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }


    if (value) {
#if CYTHON_COMPILING_IN_PYPY
        // FIXME: what to do in PyPy?
#else
        // Generators always return to their most recent caller, not
        // necessarily their creator.
        if (self->exc_traceback) {
            PyThreadState *tstate = PyThreadState_GET();
            PyTracebackObject *tb = (PyTracebackObject *) self->exc_traceback;
            PyFrameObject *f = tb->tb_frame;

            Py_XINCREF(tstate->frame);
            assert(f->f_back == NULL);
            f->f_back = tstate->frame;
        }
#endif
        __Pyx_ExceptionSwap(&self->exc_type, &self->exc_value,
                            &self->exc_traceback);
    } else {
        __Pyx_Coroutine_ExceptionClear(self);
    }

    self->is_running = 1;
    retval = self->body((PyObject *) self, value);
    self->is_running = 0;

    if (retval) {
        __Pyx_ExceptionSwap(&self->exc_type, &self->exc_value,
                            &self->exc_traceback);
#if CYTHON_COMPILING_IN_PYPY
        // FIXME: what to do in PyPy?
#else
        // Don't keep the reference to f_back any longer than necessary.  It
        // may keep a chain of frames alive or it could create a reference
        // cycle.
        if (self->exc_traceback) {
            PyTracebackObject *tb = (PyTracebackObject *) self->exc_traceback;
            PyFrameObject *f = tb->tb_frame;
            Py_CLEAR(f->f_back);
        }
#endif
    } else {
        __Pyx_Coroutine_ExceptionClear(self);
    }

    return retval;
}

static CYTHON_INLINE
PyObject *__Pyx_Coroutine_MethodReturn(PyObject *retval) {
    if (unlikely(!retval && !PyErr_Occurred())) {
        // method call must not terminate with NULL without setting an exception
        PyErr_SetNone(PyExc_StopIteration);
    }
    return retval;
}

static CYTHON_INLINE
PyObject *__Pyx_Coroutine_FinishDelegation(__pyx_CoroutineObject *gen) {
    PyObject *ret;
    PyObject *val = NULL;
    __Pyx_Coroutine_Undelegate(gen);
    __Pyx_PyGen_FetchStopIterationValue(&val);
    // val == NULL on failure => pass on exception
    ret = __Pyx_Coroutine_SendEx(gen, val);
    Py_XDECREF(val);
    return ret;
}

static PyObject *__Pyx_Coroutine_Send(PyObject *self, PyObject *value) {
    PyObject *retval;
    __pyx_CoroutineObject *gen = (__pyx_CoroutineObject*) self;
    PyObject *yf = gen->yieldfrom;
    if (unlikely(__Pyx_Coroutine_CheckRunning(gen)))
        return NULL;
    if (yf) {
        PyObject *ret;
        // FIXME: does this really need an INCREF() ?
        //Py_INCREF(yf);
        gen->is_running = 1;
        #ifdef __Pyx_Generator_USED
        if (__Pyx_Generator_CheckExact(yf)) {
            ret = __Pyx_Coroutine_Send(yf, value);
        } else
        #endif
        #ifdef __Pyx_Coroutine_USED
        if (__Pyx_Coroutine_CheckExact(yf)) {
            ret = __Pyx_Coroutine_Send(yf, value);
        } else
        #endif
        {
            if (value == Py_None)
                ret = PyIter_Next(yf);  // FIXME!
            else
                ret = __Pyx_PyObject_CallMethod1(yf, PYIDENT("send"), value);
        }
        gen->is_running = 0;
        //Py_DECREF(yf);
        if (likely(ret)) {
            return ret;
        }
        retval = __Pyx_Coroutine_FinishDelegation(gen);
    } else {
        retval = __Pyx_Coroutine_SendEx(gen, value);
    }
    return __Pyx_Coroutine_MethodReturn(retval);
}

//   This helper function is used by gen_close and gen_throw to
//   close a subiterator being delegated to by yield-from.
static int __Pyx_Coroutine_CloseIter(__pyx_CoroutineObject *gen, PyObject *yf) {
    PyObject *retval = NULL;
    int err = 0;

    #ifdef __Pyx_Generator_USED
    if (__Pyx_Generator_CheckExact(yf)) {
        retval = __Pyx_Coroutine_Close(yf);
        if (!retval)
            return -1;
    } else
    #endif
    #ifdef __Pyx_Coroutine_USED
    if (__Pyx_Coroutine_CheckExact(yf)) {
        retval = __Pyx_Coroutine_Close(yf);
        if (!retval)
            return -1;
    } else
    #endif
    {
        PyObject *meth;
        gen->is_running = 1;
        meth = __Pyx_PyObject_GetAttrStr(yf, PYIDENT("close"));
        if (unlikely(!meth)) {
            if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
                PyErr_WriteUnraisable(yf);
            }
            PyErr_Clear();
        } else {
            retval = PyObject_CallFunction(meth, NULL);
            Py_DECREF(meth);
            if (!retval)
                err = -1;
        }
        gen->is_running = 0;
    }
    Py_XDECREF(retval);
    return err;
}

static PyObject *__Pyx_Generator_Next(PyObject *self) {
    __pyx_CoroutineObject *gen = (__pyx_CoroutineObject*) self;
    PyObject *yf = gen->yieldfrom;
    if (unlikely(__Pyx_Coroutine_CheckRunning(gen)))
        return NULL;
    if (yf) {
        PyObject *ret;
        // FIXME: does this really need an INCREF() ?
        //Py_INCREF(yf);
        // YieldFrom code ensures that yf is an iterator
        gen->is_running = 1;
        ret = Py_TYPE(yf)->tp_iternext(yf);
        gen->is_running = 0;
        //Py_DECREF(yf);
        if (likely(ret)) {
            return ret;
        }
        return __Pyx_Coroutine_FinishDelegation(gen);
    }
    return __Pyx_Coroutine_SendEx(gen, Py_None);
}

static PyObject *__Pyx_Coroutine_Close(PyObject *self) {
    __pyx_CoroutineObject *gen = (__pyx_CoroutineObject *) self;
    PyObject *retval, *raised_exception;
    PyObject *yf = gen->yieldfrom;
    int err = 0;

    if (unlikely(__Pyx_Coroutine_CheckRunning(gen)))
        return NULL;

    if (yf) {
        Py_INCREF(yf);
        err = __Pyx_Coroutine_CloseIter(gen, yf);
        __Pyx_Coroutine_Undelegate(gen);
        Py_DECREF(yf);
    }
    if (err == 0)
        PyErr_SetNone(PyExc_GeneratorExit);
    retval = __Pyx_Coroutine_SendEx(gen, NULL);
    if (retval) {
        Py_DECREF(retval);
        PyErr_SetString(PyExc_RuntimeError,
                        "generator ignored GeneratorExit");
        return NULL;
    }
    raised_exception = PyErr_Occurred();
    if (!raised_exception
        || raised_exception == PyExc_StopIteration
        || raised_exception == PyExc_GeneratorExit
        || PyErr_GivenExceptionMatches(raised_exception, PyExc_GeneratorExit)
        || PyErr_GivenExceptionMatches(raised_exception, PyExc_StopIteration))
    {
        // ignore these errors
        if (raised_exception) PyErr_Clear();
        Py_INCREF(Py_None);
        return Py_None;
    }
    return NULL;
}

static PyObject *__Pyx_Coroutine_Throw(PyObject *self, PyObject *args) {
    __pyx_CoroutineObject *gen = (__pyx_CoroutineObject *) self;
    PyObject *typ;
    PyObject *tb = NULL;
    PyObject *val = NULL;
    PyObject *yf = gen->yieldfrom;

    if (!PyArg_UnpackTuple(args, (char *)"throw", 1, 3, &typ, &val, &tb))
        return NULL;

    if (unlikely(__Pyx_Coroutine_CheckRunning(gen)))
        return NULL;

    if (yf) {
        PyObject *ret;
        Py_INCREF(yf);
        if (PyErr_GivenExceptionMatches(typ, PyExc_GeneratorExit)) {
            int err = __Pyx_Coroutine_CloseIter(gen, yf);
            Py_DECREF(yf);
            __Pyx_Coroutine_Undelegate(gen);
            if (err < 0)
                return __Pyx_Coroutine_MethodReturn(__Pyx_Coroutine_SendEx(gen, NULL));
            goto throw_here;
        }
        gen->is_running = 1;
        #ifdef __Pyx_Generator_USED
        if (__Pyx_Generator_CheckExact(yf)) {
            ret = __Pyx_Coroutine_Throw(yf, args);
        } else
        #endif
        #ifdef __Pyx_Coroutine_USED
        if (__Pyx_Coroutine_CheckExact(yf)) {
            ret = __Pyx_Coroutine_Throw(yf, args);
        } else
        #endif
        {
            PyObject *meth = __Pyx_PyObject_GetAttrStr(yf, PYIDENT("throw"));
            if (unlikely(!meth)) {
                Py_DECREF(yf);
                if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
                    gen->is_running = 0;
                    return NULL;
                }
                PyErr_Clear();
                __Pyx_Coroutine_Undelegate(gen);
                gen->is_running = 0;
                goto throw_here;
            }
            ret = PyObject_CallObject(meth, args);
            Py_DECREF(meth);
        }
        gen->is_running = 0;
        Py_DECREF(yf);
        if (!ret) {
            ret = __Pyx_Coroutine_FinishDelegation(gen);
        }
        return __Pyx_Coroutine_MethodReturn(ret);
    }
throw_here:
    __Pyx_Raise(typ, val, tb, NULL);
    return __Pyx_Coroutine_MethodReturn(__Pyx_Coroutine_SendEx(gen, NULL));
}

static int __Pyx_Coroutine_traverse(PyObject *self, visitproc visit, void *arg) {
    __pyx_CoroutineObject *gen = (__pyx_CoroutineObject *) self;

    Py_VISIT(gen->closure);
    Py_VISIT(gen->classobj);
    Py_VISIT(gen->yieldfrom);
    Py_VISIT(gen->exc_type);
    Py_VISIT(gen->exc_value);
    Py_VISIT(gen->exc_traceback);
    return 0;
}

static int __Pyx_Coroutine_clear(PyObject *self) {
    __pyx_CoroutineObject *gen = (__pyx_CoroutineObject *) self;

    Py_CLEAR(gen->closure);
    Py_CLEAR(gen->classobj);
    Py_CLEAR(gen->yieldfrom);
    Py_CLEAR(gen->exc_type);
    Py_CLEAR(gen->exc_value);
    Py_CLEAR(gen->exc_traceback);
    Py_CLEAR(gen->gi_name);
    Py_CLEAR(gen->gi_qualname);
    return 0;
}

static void __Pyx_Coroutine_dealloc(PyObject *self) {
    __pyx_CoroutineObject *gen = (__pyx_CoroutineObject *) self;

    PyObject_GC_UnTrack(gen);
    if (gen->gi_weakreflist != NULL)
        PyObject_ClearWeakRefs(self);

    if (gen->resume_label > 0) {
        // Generator is paused, so we need to close
        PyObject_GC_Track(self);
#if PY_VERSION_HEX >= 0x030400a1
        if (PyObject_CallFinalizerFromDealloc(self))
#else
        Py_TYPE(gen)->tp_del(self);
        if (self->ob_refcnt > 0)
#endif
        {
            // resurrected.  :(
            return;
        }
        PyObject_GC_UnTrack(self);
    }

    __Pyx_Coroutine_clear(self);
    PyObject_GC_Del(gen);
}

static void __Pyx_Coroutine_del(PyObject *self) {
    PyObject *res;
    PyObject *error_type, *error_value, *error_traceback;
    __pyx_CoroutineObject *gen = (__pyx_CoroutineObject *) self;

    if (gen->resume_label <= 0)
        return ;

#if PY_VERSION_HEX < 0x030400a1
    // Temporarily resurrect the object.
    assert(self->ob_refcnt == 0);
    self->ob_refcnt = 1;
#endif

    // Save the current exception, if any.
    __Pyx_ErrFetch(&error_type, &error_value, &error_traceback);

    res = __Pyx_Coroutine_Close(self);

    if (res == NULL)
        PyErr_WriteUnraisable(self);
    else
        Py_DECREF(res);

    // Restore the saved exception.
    __Pyx_ErrRestore(error_type, error_value, error_traceback);

#if PY_VERSION_HEX < 0x030400a1
    // Undo the temporary resurrection; can't use DECREF here, it would
    // cause a recursive call.
    assert(self->ob_refcnt > 0);
    if (--self->ob_refcnt == 0) {
        // this is the normal path out
        return;
    }

    // close() resurrected it!  Make it look like the original Py_DECREF
    // never happened.
    {
        Py_ssize_t refcnt = self->ob_refcnt;
        _Py_NewReference(self);
        self->ob_refcnt = refcnt;
    }
#if CYTHON_COMPILING_IN_CPYTHON
    assert(PyType_IS_GC(self->ob_type) &&
           _Py_AS_GC(self)->gc.gc_refs != _PyGC_REFS_UNTRACKED);

    // If Py_REF_DEBUG, _Py_NewReference bumped _Py_RefTotal, so
    // we need to undo that.
    _Py_DEC_REFTOTAL;
#endif
    // If Py_TRACE_REFS, _Py_NewReference re-added self to the object
    // chain, so no more to do there.
    // If COUNT_ALLOCS, the original decref bumped tp_frees, and
    // _Py_NewReference bumped tp_allocs:  both of those need to be
    // undone.
#ifdef COUNT_ALLOCS
    --Py_TYPE(self)->tp_frees;
    --Py_TYPE(self)->tp_allocs;
#endif
#endif
}

static PyObject *
__Pyx_Coroutine_get_name(__pyx_CoroutineObject *self)
{
    Py_INCREF(self->gi_name);
    return self->gi_name;
}

static int
__Pyx_Coroutine_set_name(__pyx_CoroutineObject *self, PyObject *value)
{
    PyObject *tmp;

#if PY_MAJOR_VERSION >= 3
    if (unlikely(value == NULL || !PyUnicode_Check(value))) {
#else
    if (unlikely(value == NULL || !PyString_Check(value))) {
#endif
        PyErr_SetString(PyExc_TypeError,
                        "__name__ must be set to a string object");
        return -1;
    }
    tmp = self->gi_name;
    Py_INCREF(value);
    self->gi_name = value;
    Py_XDECREF(tmp);
    return 0;
}

static PyObject *
__Pyx_Coroutine_get_qualname(__pyx_CoroutineObject *self)
{
    Py_INCREF(self->gi_qualname);
    return self->gi_qualname;
}

static int
__Pyx_Coroutine_set_qualname(__pyx_CoroutineObject *self, PyObject *value)
{
    PyObject *tmp;

#if PY_MAJOR_VERSION >= 3
    if (unlikely(value == NULL || !PyUnicode_Check(value))) {
#else
    if (unlikely(value == NULL || !PyString_Check(value))) {
#endif
        PyErr_SetString(PyExc_TypeError,
                        "__qualname__ must be set to a string object");
        return -1;
    }
    tmp = self->gi_qualname;
    Py_INCREF(value);
    self->gi_qualname = value;
    Py_XDECREF(tmp);
    return 0;
}

static PyGetSetDef __pyx_Coroutine_getsets[] = {
    {(char *) "__name__", (getter)__Pyx_Coroutine_get_name, (setter)__Pyx_Coroutine_set_name,
     (char*) PyDoc_STR("name of the generator"), 0},
    {(char *) "__qualname__", (getter)__Pyx_Coroutine_get_qualname, (setter)__Pyx_Coroutine_set_qualname,
     (char*) PyDoc_STR("qualified name of the generator"), 0},
    {0, 0, 0, 0, 0}
};

static PyMemberDef __pyx_Coroutine_memberlist[] = {
    {(char *) "gi_running", T_BOOL, offsetof(__pyx_CoroutineObject, is_running), READONLY, NULL},
    {0, 0, 0, 0, 0}
};

static __pyx_CoroutineObject *__Pyx__Coroutine_New(PyTypeObject* type, __pyx_coroutine_body_t body,
                                                   PyObject *closure, PyObject *name, PyObject *qualname) {
    __pyx_CoroutineObject *gen = PyObject_GC_New(__pyx_CoroutineObject, type);

    if (gen == NULL)
        return NULL;

    gen->body = body;
    gen->closure = closure;
    Py_XINCREF(closure);
    gen->is_running = 0;
    gen->resume_label = 0;
    gen->classobj = NULL;
    gen->yieldfrom = NULL;
    gen->exc_type = NULL;
    gen->exc_value = NULL;
    gen->exc_traceback = NULL;
    gen->gi_weakreflist = NULL;
    Py_XINCREF(qualname);
    gen->gi_qualname = qualname;
    Py_XINCREF(name);
    gen->gi_name = name;

    PyObject_GC_Track(gen);
    return gen;
}


//////////////////// Coroutine ////////////////////
//@requires: CoroutineBase
//@requires: PatchGeneratorABC

static void __Pyx_Coroutine_check_and_dealloc(PyObject *self) {
    __pyx_CoroutineObject *gen = (__pyx_CoroutineObject *) self;

    if (gen->resume_label == 0 && !PyErr_Occurred()) {
#if PY_VERSION_HEX >= 0x03030000 || defined(PyErr_WarnFormat)
        PyErr_WarnFormat(PyExc_RuntimeWarning, 1, "coroutine '%.50S' was never awaited", gen->gi_qualname);
#else
        PyObject *msg, *qualname;
        char *cname, *cmsg;
        #if PY_MAJOR_VERSION >= 3
        qualname = PyUnicode_AsUTF8String(gen->gi_qualname);
        if (likely(qualname)) {
            cname = PyBytes_AS_STRING(qualname);
        } else {
            PyErr_Clear();
            cname = (char*) "?";
        }
        msg = PyBytes_FromFormat(
        #else
        qualname = gen->gi_qualname;
        cname = PyString_AS_STRING(qualname);
        msg = PyString_FromFormat(
        #endif
            "coroutine '%.50s' was never awaited", cname);

        #if PY_MAJOR_VERSION >= 3
        Py_XDECREF(qualname);
        #endif

        if (unlikely(!msg)) {
            PyErr_Clear();
            cmsg = (char*) "coroutine was never awaited";
        } else {
            #if PY_MAJOR_VERSION >= 3
            cmsg = PyBytes_AS_STRING(msg);
            #else
            cmsg = PyString_AS_STRING(msg);
            #endif
        }
        if (unlikely(PyErr_WarnEx(PyExc_RuntimeWarning, cmsg, 1) < 0))
            PyErr_WriteUnraisable(self);
        Py_XDECREF(msg);
#endif
    }

    __Pyx_Coroutine_dealloc(self);
}

#if CYTHON_COMPILING_IN_CPYTHON && PY_MAJOR_VERSION >= 3 && PY_VERSION_HEX < 0x030500B1
static PyObject *__Pyx_Coroutine_compare(PyObject *obj, PyObject *other, int op) {
    PyObject* result;
    switch (op) {
        case Py_EQ: result = (other == obj) ? Py_True : Py_False; break;
        case Py_NE: result = (other != obj) ? Py_True : Py_False; break;
        default:
            result = Py_NotImplemented;
    }
    Py_INCREF(result);
    return result;
}
#endif

static PyObject *__Pyx_Coroutine_await(PyObject *self) {
    Py_INCREF(self);
    return self;
}

static PyMethodDef __pyx_Coroutine_methods[] = {
    {"send", (PyCFunction) __Pyx_Coroutine_Send, METH_O, 0},
    {"throw", (PyCFunction) __Pyx_Coroutine_Throw, METH_VARARGS, 0},
    {"close", (PyCFunction) __Pyx_Coroutine_Close, METH_NOARGS, 0},
#if PY_VERSION_HEX < 0x030500B1
    {"__await__", (PyCFunction) __Pyx_Coroutine_await, METH_NOARGS, 0},
#endif
    {0, 0, 0, 0}
};

#if PY_MAJOR_VERSION >= 3
static __Pyx_PyAsyncMethodsStruct __pyx_Coroutine_as_async = {
    __Pyx_Coroutine_await, /*am_await*/
    0, /*am_aiter*/
    0, /*am_anext*/
};
#endif

static PyTypeObject __pyx_CoroutineType_type = {
    PyVarObject_HEAD_INIT(0, 0)
    "coroutine",                        /*tp_name*/
    sizeof(__pyx_CoroutineObject),      /*tp_basicsize*/
    0,                                  /*tp_itemsize*/
    (destructor) __Pyx_Coroutine_check_and_dealloc,/*tp_dealloc*/
    0,                                  /*tp_print*/
    0,                                  /*tp_getattr*/
    0,                                  /*tp_setattr*/
#if PY_MAJOR_VERSION >= 3
    &__pyx_Coroutine_as_async,          /*tp_as_async (tp_reserved)*/
#else
    0,                                  /*tp_reserved*/
#endif
    0,                                  /*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    0,                                  /*tp_hash*/
    0,                                  /*tp_call*/
    0,                                  /*tp_str*/
    0,                                  /*tp_getattro*/
    0,                                  /*tp_setattro*/
    0,                                  /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_HAVE_FINALIZE, /*tp_flags*/
    0,                                  /*tp_doc*/
    (traverseproc) __Pyx_Coroutine_traverse,   /*tp_traverse*/
    0,                                  /*tp_clear*/
#if CYTHON_COMPILING_IN_CPYTHON && PY_MAJOR_VERSION >= 3 && PY_VERSION_HEX < 0x030500B1
    // in order to (mis-)use tp_reserved above, we must also implement tp_richcompare
    __Pyx_Coroutine_compare,            /*tp_richcompare*/
#else
    0,                                  /*tp_richcompare*/
#endif
    offsetof(__pyx_CoroutineObject, gi_weakreflist), /*tp_weaklistoffset*/
// no tp_iter() as iterator is only available through __await__()
    0,                                  /*tp_iter*/
    (iternextfunc) __Pyx_Generator_Next, /*tp_iternext*/
    __pyx_Coroutine_methods,            /*tp_methods*/
    __pyx_Coroutine_memberlist,         /*tp_members*/
    __pyx_Coroutine_getsets,            /*tp_getset*/
    0,                                  /*tp_base*/
    0,                                  /*tp_dict*/
    0,                                  /*tp_descr_get*/
    0,                                  /*tp_descr_set*/
    0,                                  /*tp_dictoffset*/
    0,                                  /*tp_init*/
    0,                                  /*tp_alloc*/
    0,                                  /*tp_new*/
    0,                                  /*tp_free*/
    0,                                  /*tp_is_gc*/
    0,                                  /*tp_bases*/
    0,                                  /*tp_mro*/
    0,                                  /*tp_cache*/
    0,                                  /*tp_subclasses*/
    0,                                  /*tp_weaklist*/
#if PY_VERSION_HEX >= 0x030400a1
    0,                                  /*tp_del*/
#else
    __Pyx_Coroutine_del,                /*tp_del*/
#endif
    0,                                  /*tp_version_tag*/
#if PY_VERSION_HEX >= 0x030400a1
    __Pyx_Coroutine_del,                /*tp_finalize*/
#endif
};

static int __pyx_Coroutine_init(void) {
    // on Windows, C-API functions can't be used in slots statically
    __pyx_CoroutineType_type.tp_getattro = PyObject_GenericGetAttr;

    __pyx_CoroutineType = __Pyx_FetchCommonType(&__pyx_CoroutineType_type);
    if (unlikely(!__pyx_CoroutineType)) {
        return -1;
    }
    return 0;
}

//////////////////// Generator ////////////////////
//@requires: CoroutineBase
//@requires: PatchGeneratorABC

static PyMethodDef __pyx_Generator_methods[] = {
    {"send", (PyCFunction) __Pyx_Coroutine_Send, METH_O, 0},
    {"throw", (PyCFunction) __Pyx_Coroutine_Throw, METH_VARARGS, 0},
    {"close", (PyCFunction) __Pyx_Coroutine_Close, METH_NOARGS, 0},
    {0, 0, 0, 0}
};

static PyTypeObject __pyx_GeneratorType_type = {
    PyVarObject_HEAD_INIT(0, 0)
    "generator",                        /*tp_name*/
    sizeof(__pyx_CoroutineObject),      /*tp_basicsize*/
    0,                                  /*tp_itemsize*/
    (destructor) __Pyx_Coroutine_dealloc,/*tp_dealloc*/
    0,                                  /*tp_print*/
    0,                                  /*tp_getattr*/
    0,                                  /*tp_setattr*/
    0,                                  /*tp_compare / tp_as_async*/
    0,                                  /*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    0,                                  /*tp_hash*/
    0,                                  /*tp_call*/
    0,                                  /*tp_str*/
    0,                                  /*tp_getattro*/
    0,                                  /*tp_setattro*/
    0,                                  /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_HAVE_FINALIZE, /*tp_flags*/
    0,                                  /*tp_doc*/
    (traverseproc) __Pyx_Coroutine_traverse,   /*tp_traverse*/
    0,                                  /*tp_clear*/
    0,                                  /*tp_richcompare*/
    offsetof(__pyx_CoroutineObject, gi_weakreflist), /*tp_weaklistoffset*/
    0,                                  /*tp_iter*/
    (iternextfunc) __Pyx_Generator_Next, /*tp_iternext*/
    __pyx_Generator_methods,            /*tp_methods*/
    __pyx_Coroutine_memberlist,         /*tp_members*/
    __pyx_Coroutine_getsets,            /*tp_getset*/
    0,                                  /*tp_base*/
    0,                                  /*tp_dict*/
    0,                                  /*tp_descr_get*/
    0,                                  /*tp_descr_set*/
    0,                                  /*tp_dictoffset*/
    0,                                  /*tp_init*/
    0,                                  /*tp_alloc*/
    0,                                  /*tp_new*/
    0,                                  /*tp_free*/
    0,                                  /*tp_is_gc*/
    0,                                  /*tp_bases*/
    0,                                  /*tp_mro*/
    0,                                  /*tp_cache*/
    0,                                  /*tp_subclasses*/
    0,                                  /*tp_weaklist*/
#if PY_VERSION_HEX >= 0x030400a1
    0,                                  /*tp_del*/
#else
    __Pyx_Coroutine_del,                /*tp_del*/
#endif
    0,                                  /*tp_version_tag*/
#if PY_VERSION_HEX >= 0x030400a1
    __Pyx_Coroutine_del,                /*tp_finalize*/
#endif
};

static int __pyx_Generator_init(void) {
    // on Windows, C-API functions can't be used in slots statically
    __pyx_GeneratorType_type.tp_getattro = PyObject_GenericGetAttr;
    __pyx_GeneratorType_type.tp_iter = PyObject_SelfIter;

    __pyx_GeneratorType = __Pyx_FetchCommonType(&__pyx_GeneratorType_type);
    if (unlikely(!__pyx_GeneratorType)) {
        return -1;
    }
    return 0;
}


/////////////// ReturnWithStopIteration.proto ///////////////

#define __Pyx_ReturnWithStopIteration(value)  \
    if (value == Py_None) PyErr_SetNone(PyExc_StopIteration); else __Pyx__ReturnWithStopIteration(value)
static void __Pyx__ReturnWithStopIteration(PyObject* value); /*proto*/

/////////////// ReturnWithStopIteration ///////////////
//@requires: Exceptions.c::PyErrFetchRestore
//@substitute: naming

// 1) Instantiating an exception just to pass back a value is costly.
// 2) CPython 3.3 <= x < 3.5b1 crash in yield-from when the StopIteration is not instantiated.
// 3) Passing a tuple as value into PyErr_SetObject() passes its items on as arguments.
// 4) If there is currently an exception being handled, we need to chain it.

static void __Pyx__ReturnWithStopIteration(PyObject* value) {
    PyObject *exc, *args;
#if CYTHON_COMPILING_IN_CPYTHON
    if ((PY_VERSION_HEX >= 0x03030000 && PY_VERSION_HEX < 0x030500B1) || PyTuple_Check(value)) {
        args = PyTuple_New(1);
        if (unlikely(!args)) return;
        Py_INCREF(value);
        PyTuple_SET_ITEM(args, 0, value);
        exc = PyType_Type.tp_call(PyExc_StopIteration, args, NULL);
        Py_DECREF(args);
        if (!exc) return;
    } else {
        // it's safe to avoid instantiating the exception
        Py_INCREF(value);
        exc = value;
    }
    if (!PyThreadState_GET()->exc_type) {
        // no chaining needed => avoid the overhead in PyErr_SetObject()
        Py_INCREF(PyExc_StopIteration);
        __Pyx_ErrRestore(PyExc_StopIteration, exc, NULL);
        return;
    }
#else
    args = PyTuple_Pack(1, value);
    if (unlikely(!args)) return;
    exc = PyObject_Call(PyExc_StopIteration, args, NULL);
    Py_DECREF(args);
    if (unlikely(!exc)) return;
#endif
    PyErr_SetObject(PyExc_StopIteration, exc);
    Py_DECREF(exc);
}


//////////////////// PatchModuleWithCoroutine.proto ////////////////////

static PyObject* __Pyx_Coroutine_patch_module(PyObject* module, const char* py_code); /*proto*/

//////////////////// PatchModuleWithCoroutine ////////////////////
//@substitute: naming

static PyObject* __Pyx_Coroutine_patch_module(PyObject* module, const char* py_code) {
#if defined(__Pyx_Generator_USED) || defined(__Pyx_Coroutine_USED)
    PyObject *globals, *result_obj;
    globals = PyDict_New();  if (unlikely(!globals)) goto ignore;
    #ifdef __Pyx_Coroutine_USED
    if (unlikely(PyDict_SetItemString(globals, "_cython_coroutine_type", (PyObject*)__pyx_CoroutineType) < 0)) goto ignore;
    #endif
    #ifdef __Pyx_Generator_USED
    if (unlikely(PyDict_SetItemString(globals, "_cython_generator_type", (PyObject*)__pyx_GeneratorType) < 0)) goto ignore;
    #endif
    if (unlikely(PyDict_SetItemString(globals, "_module", module) < 0)) goto ignore;
    if (unlikely(PyDict_SetItemString(globals, "__builtins__", $builtins_cname) < 0)) goto ignore;
    result_obj = PyRun_String(py_code, Py_file_input, globals, globals);
    if (unlikely(!result_obj)) goto ignore;
    Py_DECREF(result_obj);
    Py_DECREF(globals);
    return module;

ignore:
    Py_XDECREF(globals);
    PyErr_WriteUnraisable(module);
    if (unlikely(PyErr_WarnEx(PyExc_RuntimeWarning, "Cython module failed to patch module with custom type", 1) < 0)) {
        Py_DECREF(module);
        module = NULL;
    }
#else
    // avoid "unused" warning
    py_code++;
#endif
    return module;
}


//////////////////// PatchGeneratorABC.proto ////////////////////

// patch 'collections.abc' if it lacks generator support
// see https://bugs.python.org/issue24018
static int __Pyx_patch_abc(void); /*proto*/

//////////////////// PatchGeneratorABC ////////////////////
//@requires: PatchModuleWithCoroutine

static int __Pyx_patch_abc(void) {
#if (defined(__Pyx_Generator_USED) || defined(__Pyx_Coroutine_USED)) && \
        (!defined(CYTHON_PATCH_ABC) || CYTHON_PATCH_ABC)
    static int abc_patched = 0;
    if (!abc_patched) {
        PyObject *module;
        module = PyImport_ImportModule((PY_VERSION_HEX >= 0x03030000) ? "collections.abc" : "collections");
        if (!module) {
            PyErr_WriteUnraisable(NULL);
            if (unlikely(PyErr_WarnEx(PyExc_RuntimeWarning,
                    ((PY_VERSION_HEX >= 0x03030000) ?
                        "Cython module failed to patch collections.abc module" :
                        "Cython module failed to patch collections module"), 1) < 0)) {
                return -1;
            }
        } else {
            module = __Pyx_Coroutine_patch_module(
                module,
#ifdef __Pyx_Generator_USED
                CSTRING("""\
def mk_gen():
    from abc import abstractmethod

    required_methods = (
        '__iter__', '__next__' if hasattr(iter(()), '__next__') else 'next',
         'send', 'throw', 'close')

    class Generator(_module.Iterator):
        __slots__ = ()

        if '__next__' in required_methods:
            def __next__(self):
                return self.send(None)
        else:
            def next(self):
                return self.send(None)

        @abstractmethod
        def send(self, value):
            raise StopIteration

        @abstractmethod
        def throw(self, typ, val=None, tb=None):
            if val is None:
                if tb is None:
                    raise typ
                val = typ()
            if tb is not None:
                val = val.with_traceback(tb)
            raise val

        def close(self):
            try:
                self.throw(GeneratorExit)
            except (GeneratorExit, StopIteration):
                pass
            else:
                raise RuntimeError('generator ignored GeneratorExit')

        @classmethod
        def __subclasshook__(cls, C):
            if cls is Generator:
                mro = C.__mro__
                for method in required_methods:
                    for base in mro:
                        if method in base.__dict__:
                            break
                    else:
                        return NotImplemented
                return True
            return NotImplemented

    generator = type((lambda: (yield))())
    Generator.register(generator)
    return Generator

try:
    Generator = _module.Generator
except AttributeError:
    Generator = _module.Generator = mk_gen()
Generator.register(_cython_generator_type)
""")
#endif
#ifdef __Pyx_Coroutine_USED
                CSTRING("""\
def mk_awaitable():
    from abc import abstractmethod, ABCMeta
""")
#if PY_MAJOR_VERSION >= 3
                CSTRING("""\
    class Awaitable(metaclass=ABCMeta):
""")
#else
                CSTRING("""\
    class Awaitable(object):
        __metaclass__ = ABCMeta
""")
#endif
                CSTRING("""\
        __slots__ = ()

        @abstractmethod
        def __await__(self):
            yield

        @classmethod
        def __subclasshook__(cls, C):
            if cls is Awaitable:
                for B in C.__mro__:
                    if '__await__' in B.__dict__:
                        if B.__dict__['__await__']:
                            return True
                        break
            return NotImplemented

    return Awaitable

try:
    Awaitable = _module.Awaitable
except AttributeError:
    Awaitable = _module.Awaitable = mk_awaitable()

def mk_coroutine():
    from abc import abstractmethod, ABCMeta

    class Coroutine(Awaitable):
        __slots__ = ()

        @abstractmethod
        def send(self, value):
            '''Send a value into the coroutine.
            Return next yielded value or raise StopIteration.
            '''
            raise StopIteration

        @abstractmethod
        def throw(self, typ, val=None, tb=None):
            '''Raise an exception in the coroutine.
            Return next yielded value or raise StopIteration.
            '''
            if val is None:
                if tb is None:
                    raise typ
                val = typ()
            if tb is not None:
                val = val.with_traceback(tb)
            raise val

        def close(self):
            '''Raise GeneratorExit inside coroutine.
            '''
            try:
                self.throw(GeneratorExit)
            except (GeneratorExit, StopIteration):
                pass
            else:
                raise RuntimeError('coroutine ignored GeneratorExit')

        @classmethod
        def __subclasshook__(cls, C):
            if cls is Coroutine:
                mro = C.__mro__
                for method in ('__await__', 'send', 'throw', 'close'):
                    for base in mro:
                        if method in base.__dict__:
                            break
                    else:
                        return NotImplemented
                return True
            return NotImplemented

    return Coroutine

try:
    Coroutine = _module.Coroutine
except AttributeError:
    Coroutine = _module.Coroutine = mk_coroutine()
Coroutine.register(_cython_coroutine_type)
""")
#endif
            );
            abc_patched = 1;
            if (unlikely(!module))
                return -1;
            Py_DECREF(module);
        }
    }
#else
    // avoid "unused" warning for __Pyx_Coroutine_patch_module()
    if (0) __Pyx_Coroutine_patch_module(NULL, NULL);
#endif
    return 0;
}


//////////////////// PatchAsyncIO.proto ////////////////////

// run after importing "asyncio" to patch Cython generator support into it
static PyObject* __Pyx_patch_asyncio(PyObject* module); /*proto*/

//////////////////// PatchAsyncIO ////////////////////
//@requires: ImportExport.c::Import
//@requires: PatchModuleWithCoroutine
//@requires: PatchInspect

static PyObject* __Pyx_patch_asyncio(PyObject* module) {
#if PY_VERSION_HEX < 0x030500B1 && \
        (defined(__Pyx_Coroutine_USED) || defined(__Pyx_Generator_USED)) && \
        (!defined(CYTHON_PATCH_ASYNCIO) || CYTHON_PATCH_ASYNCIO)
    PyObject *patch_module = NULL;
    static int asyncio_patched = 0;
    if (unlikely((!asyncio_patched) && module)) {
        PyObject *package;
        package = __Pyx_Import(PYIDENT("asyncio.coroutines"), NULL, 0);
        if (package) {
            patch_module = __Pyx_Coroutine_patch_module(
                PyObject_GetAttrString(package, "coroutines"), CSTRING("""\
coro_types = getattr(_module, '_COROUTINE_TYPES', None)
""")
#ifdef __Pyx_Coroutine_USED
CSTRING("""\
if coro_types is not None and _cython_coroutine_type not in coro_types:
    coro_types = type(coro_types) (tuple(coro_types) + (_cython_coroutine_type,))
""")
#endif
#ifdef __Pyx_Generator_USED
CSTRING("""\
if coro_types is not None and _cython_generator_type not in coro_types:
    coro_types = type(coro_types) (tuple(coro_types) + (_cython_generator_type,))
""")
#endif
CSTRING("""
_module._COROUTINE_TYPES = coro_types
""")
            );
#if PY_VERSION_HEX < 0x03050000
        } else {
            // Py3.4 used to have asyncio.tasks instead of asyncio.coroutines
            PyErr_Clear();
            package = __Pyx_Import(PYIDENT("asyncio.tasks"), NULL, 0);
            if (unlikely(!package)) goto asyncio_done;
            patch_module = __Pyx_Coroutine_patch_module(
                PyObject_GetAttrString(package, "tasks"), CSTRING("""\
if hasattr(_module, 'iscoroutine'):
    old_coroutine_types = getattr(_module.iscoroutine, '_cython_coroutine_types', None)
    if old_coroutine_types is None or not isinstance(old_coroutine_types, list):
        old_coroutine_types = []
        def cy_wrap(orig_func, type=type, cython_coroutine_types=old_coroutine_types):
            def cy_iscoroutine(obj): return type(obj) in cython_coroutine_types or orig_func(obj)
            cy_iscoroutine._cython_coroutine_types = cython_coroutine_types
            return cy_iscoroutine
        _module.iscoroutine = cy_wrap(_module.iscoroutine)
""")
#ifdef __Pyx_Coroutine_USED
CSTRING("""\
    if _cython_coroutine_type not in old_coroutine_types: old_coroutine_types.append(_cython_coroutine_type)
""")
#endif
#ifdef __Pyx_Generator_USED
CSTRING("""\
    if _cython_generator_type not in old_coroutine_types: old_coroutine_types.append(_cython_generator_type)
""")
#endif
            );
        #endif
        }
        Py_DECREF(package);
        if (unlikely(!patch_module)) goto ignore;
#if PY_VERSION_HEX < 0x03050000
asyncio_done:
        PyErr_Clear();
#endif
        asyncio_patched = 1;
        // now patch inspect.isgenerator() by looking up the imported module in the patched asyncio module
        {
            PyObject *inspect_module;
            if (patch_module) {
                inspect_module = PyObject_GetAttrString(patch_module, "inspect");
                Py_DECREF(patch_module);
            } else {
                inspect_module = __Pyx_Import(PYIDENT("inspect"), NULL, 0);
            }
            if (unlikely(!inspect_module)) goto ignore;
            inspect_module = __Pyx_patch_inspect(inspect_module);
            if (unlikely(!inspect_module)) {
                Py_DECREF(module);
                module = NULL;
            }
            Py_DECREF(inspect_module);
        }
    }
    return module;
ignore:
    PyErr_WriteUnraisable(module);
    if (unlikely(PyErr_WarnEx(PyExc_RuntimeWarning, "Cython module failed to patch asyncio package with custom generator type", 1) < 0)) {
        Py_DECREF(module);
        module = NULL;
    }
#else
    // avoid "unused" warning for __Pyx_Coroutine_patch_module()
    if (0) return __Pyx_Coroutine_patch_module(module, NULL);
#endif
    return module;
}


//////////////////// PatchInspect.proto ////////////////////

// run after importing "inspect" to patch Cython generator support into it
static PyObject* __Pyx_patch_inspect(PyObject* module); /*proto*/

//////////////////// PatchInspect ////////////////////
//@requires: PatchModuleWithCoroutine

static PyObject* __Pyx_patch_inspect(PyObject* module) {
#if (defined(__Pyx_Generator_USED) || defined(__Pyx_Coroutine_USED)) && (!defined(CYTHON_PATCH_INSPECT) || CYTHON_PATCH_INSPECT)
    static int inspect_patched = 0;
    if (unlikely((!inspect_patched) && module)) {
        module = __Pyx_Coroutine_patch_module(
            module,
#ifdef __Pyx_Generator_USED
            CSTRING("""\
if getattr(_module.isgenerator, '_cython_generator_type', None) is not _cython_generator_type:
    def cy_wrap(orig_func, cython_generator_type=_cython_generator_type, type=type):
        def cy_isgenerator(obj): return type(obj) is cython_generator_type or orig_func(obj)
        cy_isgenerator._cython_generator_type = cython_generator_type
        return cy_isgenerator
    _module.isgenerator = cy_wrap(_module.isgenerator)
""")
#endif
#ifdef __Pyx_Coroutine_USED
            CSTRING("""\
try:
    _module.iscoroutine
except AttributeError:
    def cy_wrap(cython_coroutine_type=_cython_coroutine_type, type=type):
        try:
            from collections.abc import Coroutine
        except ImportError:
            from collections import Coroutine
        def cy_iscoroutine(obj): return isinstance(obj, Coroutine)
        return cy_iscoroutine

    try:
        _module.iscoroutine = cy_wrap()
    except ImportError:
        pass

try:
    _module.isawaitable
except AttributeError:
    def cy_wrap(cython_coroutine_type=_cython_coroutine_type, type=type):
        try:
            from collections.abc import Awaitable
        except ImportError:
            from collections import Awaitable
        def cy_isawaitable(obj): return isinstance(obj, Awaitable)
        return cy_isawaitable

    try:
        _module.isawaitable = cy_wrap()
    except ImportError:
        pass
""")
#endif
        );
        inspect_patched = 1;
    }
#else
    // avoid "unused" warning for __Pyx_Coroutine_patch_module()
    if (0) return __Pyx_Coroutine_patch_module(module, NULL);
#endif
    return module;
}


//////////////////// StopAsyncIteration.proto ////////////////////

#define __Pyx_StopAsyncIteration_USED
static PyObject *__Pyx_PyExc_StopAsyncIteration;
static int __pyx_StopAsyncIteration_init(void); /*proto*/

//////////////////// StopAsyncIteration ////////////////////

#if PY_VERSION_HEX < 0x030500B1
static PyTypeObject __Pyx__PyExc_StopAsyncIteration_type = {
    PyVarObject_HEAD_INIT(0, 0)
    "StopAsyncIteration",               /*tp_name*/
    sizeof(PyBaseExceptionObject),      /*tp_basicsize*/
    0,                                  /*tp_itemsize*/
    0,                                  /*tp_dealloc*/
    0,                                  /*tp_print*/
    0,                                  /*tp_getattr*/
    0,                                  /*tp_setattr*/
#if PY_MAJOR_VERSION < 3
    0,                                  /*tp_compare*/
#else
    0,                                  /*reserved*/
#endif
    0,                                  /*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    0,                                  /*tp_hash*/
    0,                                  /*tp_call*/
    0,                                  /*tp_str*/
    0,                                  /*tp_getattro*/
    0,                                  /*tp_setattro*/
    0,                                  /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
    PyDoc_STR("Signal the end from iterator.__anext__()."),  /*tp_doc*/
    0,                                  /*tp_traverse*/
    0,                                  /*tp_clear*/
    0,                                  /*tp_richcompare*/
    0,                                  /*tp_weaklistoffset*/
    0,                                  /*tp_iter*/
    0,                                  /*tp_iternext*/
    0,                                  /*tp_methods*/
    0,                                  /*tp_members*/
    0,                                  /*tp_getset*/
    0,                                  /*tp_base*/
    0,                                  /*tp_dict*/
    0,                                  /*tp_descr_get*/
    0,                                  /*tp_descr_set*/
    0,                                  /*tp_dictoffset*/
    0,                                  /*tp_init*/
    0,                                  /*tp_alloc*/
    0,                                  /*tp_new*/
    0,                                  /*tp_free*/
    0,                                  /*tp_is_gc*/
    0,                                  /*tp_bases*/
    0,                                  /*tp_mro*/
    0,                                  /*tp_cache*/
    0,                                  /*tp_subclasses*/
    0,                                  /*tp_weaklist*/
    0,                                  /*tp_del*/
    0,                                  /*tp_version_tag*/
#if PY_VERSION_HEX >= 0x030400a1
    0,                                  /*tp_finalize*/
#endif
};
#endif

static int __pyx_StopAsyncIteration_init(void) {
#if PY_VERSION_HEX >= 0x030500B1
    __Pyx_PyExc_StopAsyncIteration = PyExc_StopAsyncIteration;
#else
    PyObject *builtins = PyEval_GetBuiltins();
    if (likely(builtins)) {
        PyObject *exc = PyMapping_GetItemString(builtins, "StopAsyncIteration");
        if (exc) {
            __Pyx_PyExc_StopAsyncIteration = exc;
            return 0;
        }
    }
    PyErr_Clear();

    __Pyx__PyExc_StopAsyncIteration_type.tp_traverse = ((PyTypeObject*)PyExc_BaseException)->tp_traverse;
    __Pyx__PyExc_StopAsyncIteration_type.tp_clear = ((PyTypeObject*)PyExc_BaseException)->tp_clear;
    __Pyx__PyExc_StopAsyncIteration_type.tp_dictoffset = ((PyTypeObject*)PyExc_BaseException)->tp_dictoffset;
    __Pyx__PyExc_StopAsyncIteration_type.tp_base = (PyTypeObject*)PyExc_Exception;

    __Pyx_PyExc_StopAsyncIteration = (PyObject*) __Pyx_FetchCommonType(&__Pyx__PyExc_StopAsyncIteration_type);
    if (unlikely(!__Pyx_PyExc_StopAsyncIteration))
        return -1;
    if (builtins && unlikely(PyMapping_SetItemString(builtins, "StopAsyncIteration", __Pyx_PyExc_StopAsyncIteration) < 0))
        return -1;
#endif
    return 0;
}