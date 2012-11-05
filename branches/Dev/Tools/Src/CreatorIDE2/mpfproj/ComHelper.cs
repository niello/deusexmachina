using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Microsoft.VisualStudio.Project
{
    public delegate TResult Func2<T1, T2, T3, T4, T5, TResult>(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5);
    public delegate TResult Func2<T1, T2, T3, T4, T5, T6, TResult>(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6);

    public delegate void Action2<T1, T2, T3, T4, T5>(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5);
    public delegate void Action2<T1, T2, T3, T4, T5, T6>(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6);
    public delegate void Action2<T1, T2, T3, T4, T5, T6, T7>(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7);
    public delegate void Action2<T1, T2, T3, T4, T5, T6, T7, T8>(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8);
    public delegate void Action2<T1, T2, T3, T4, T5, T6, T7, T8, T9>(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9);
    public delegate void Action2<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10);
    public delegate void Action2<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11>(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11);
    public delegate void Action2<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12>(T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12);

    public static class ComHelper
    {
        public static HResult WrapFunction<TResult>(bool trace, Func<TResult> function, out TResult result)
        {
            if(trace)
                CCITracing.TraceCall();
            try
            {
                if (function == null)
                    throw new ArgumentNullException("function");
                result = function();
                return HResult.Ok;
            }
            catch(ComSpecificException ex)
            {
                result = default(TResult);
                return (HResult) ex.HResult;
            }
            catch (Exception ex)
            {
                Logger.LogException(LogMessageLevel.Warning, ex);
                result = default(TResult);
                return (HResult) Marshal.GetHRForException(ex);
            }
        }

        public static HResult WrapFunction<TImmediate, TResult>(bool trace, Func<TImmediate> function, out TResult result, Func<TImmediate, TResult> cast)
        {
            return WrapFunction(trace, function != null && cast != null ? () => cast(function()) : (Func<TResult>)null, out result);
        }

        public static HResult WrapFunction<T, TResult>(bool trace, Func<T, TResult> function, T arg, out TResult result)
        {
            return WrapFunction(trace, function != null ? () => function(arg) : (Func<TResult>)null, out result);
        }

        public static HResult WrapFunction<T, TImmediate, TResult>(bool trace, Func<T, TImmediate> function, T arg, out TResult result, Func<TImmediate, TResult> cast)
        {
            return WrapFunction(trace, function != null ? () => function(arg) : (Func<TImmediate>)null, out result, cast);
        }

        public static HResult WrapFunction<T1, T2, TResult>(bool trace, Func<T1, T2, TResult> function, T1 arg1, T2 arg2, out TResult result)
        {
            return WrapFunction(trace, function != null ? () => function(arg1, arg2) : (Func<TResult>)null, out result);
        }

        public static HResult WrapFunction<T1, T2, TImmediate, TResult>(bool trace, Func<T1, T2, TImmediate> function, T1 arg1, T2 arg2, out TResult result, Func<TImmediate, TResult> cast)
        {
            return WrapFunction(trace, function != null ? () => function(arg1, arg2) : (Func<TImmediate>)null, out result, cast);
        }

        public static HResult WrapFunction<T1, T2, T3, TResult>(bool trace, Func<T1, T2, T3, TResult> function, T1 arg1, T2 arg2, T3 arg3, out TResult result)
        {
            return WrapFunction(trace, function != null ? () => function(arg1, arg2, arg3) : (Func<TResult>)null, out result);
        }

        public static HResult WrapFunction<T1, T2, T3, TImmediate, TResult>(bool trace, Func<T1, T2, T3, TImmediate> function, T1 arg1, T2 arg2, T3 arg3, out TResult result, Func<TImmediate, TResult> cast)
        {
            return WrapFunction(trace, function != null ? () => function(arg1, arg2, arg3) : (Func<TImmediate>)null, out result, cast);
        }

        public static HResult WrapFunction<T1, T2, T3, T4, TResult>(bool trace, Func<T1, T2, T3, T4, TResult> function, T1 arg1, T2 arg2, T3 arg3, T4 arg4, out TResult result)
        {
            return WrapFunction(trace, function != null ? () => function(arg1, arg2, arg3, arg4) : (Func<TResult>)null, out result);
        }

        public static HResult WrapFunction<T1, T2, T3, T4, TImmediate, TResult>(bool trace, Func<T1, T2, T3, T4, TImmediate> function, T1 arg1, T2 arg2, T3 arg3, T4 arg4, out TResult result, Func<TImmediate, TResult> cast)
        {
            return WrapFunction(trace, function != null ? () => function(arg1, arg2, arg3, arg4) : (Func<TImmediate>)null, out result, cast);
        }

        public static HResult WrapFunction<T1, T2, T3, T4, T5, TResult>(bool trace, Func2<T1, T2, T3, T4, T5, TResult> function, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, out TResult result)
        {
            return WrapFunction(trace, function != null ? () => function(arg1, arg2, arg3, arg4, arg5) : (Func<TResult>)null, out result);
        }

        public static HResult WrapFunction<T1, T2, T3, T4, T5, TImmediate, TResult>(bool trace, Func2<T1, T2, T3, T4, T5, TImmediate> function, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, out TResult result, Func<TImmediate, TResult> cast)
        {
            return WrapFunction(trace, function != null ? () => function(arg1, arg2, arg3, arg4, arg5) : (Func<TImmediate>)null, out result, cast);
        }

        public static HResult WrapFunction<T1, T2, T3, T4, T5, T6, TResult>(bool trace, Func2<T1, T2, T3, T4, T5, T6, TResult> function, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, out TResult result)
        {
            return WrapFunction(trace, function != null ? () => function(arg1, arg2, arg3, arg4, arg5, arg6) : (Func<TResult>)null, out result);
        }

        public static HResult WrapFunction<T1, T2, T3, T4, T5, T6, TImmediate, TResult>(bool trace, Func2<T1, T2, T3, T4, T5, T6, TImmediate> function, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, out TResult result, Func<TImmediate, TResult> cast)
        {
            return WrapFunction(trace, function != null ? () => function(arg1, arg2, arg3, arg4, arg5, arg6) : (Func<TImmediate>)null, out result, cast);
        }

        public static HResult WrapAction(bool trace, Action action)
        {
            if(trace)
                CCITracing.TraceCall();
            try
            {
                if (action == null)
                    throw new ArgumentNullException("action");
                action();
                return HResult.Ok;
            }
            catch (ComSpecificException ex)
            {
                return (HResult)ex.HResult;
            }
            catch (Exception ex)
            {
                Logger.LogException(LogMessageLevel.Warning, ex);
                return (HResult) Marshal.GetHRForException(ex);
            }
        }
        public static HResult WrapAction<T>(bool trace, Action<T> action, T arg)
        {
            return WrapAction(trace, action != null ? () => action(arg) : (Action)null);
        }

        public static HResult WrapAction<T1, T2>(bool trace, Action<T1, T2> action, T1 arg1, T2 arg2)
        {
            return WrapAction(trace, action != null ? () => action(arg1, arg2) : (Action)null);
        }

        public static HResult WrapAction<T1, T2, T3>(bool trace, Action<T1, T2, T3> action, T1 arg1, T2 arg2, T3 arg3)
        {
            return WrapAction(trace, action != null ? () => action(arg1, arg2, arg3) : (Action)null);
        }

        public static HResult WrapAction<T1, T2, T3, T4>(bool trace, Action<T1, T2, T3, T4> action, T1 arg1, T2 arg2, T3 arg3, T4 arg4)
        {
            return WrapAction(trace, action != null ? () => action(arg1, arg2, arg3, arg4) : (Action)null);
        }

        public static HResult WrapAction<T1, T2, T3, T4, T5>(bool trace, Action2<T1, T2, T3, T4, T5> action, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
        {
            return WrapAction(trace, action != null ? () => action(arg1, arg2, arg3, arg4, arg5) : (Action)null);
        }

        public static HResult WrapAction<T1, T2, T3, T4, T5, T6>(bool trace, Action2<T1, T2, T3, T4, T5, T6> action, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6)
        {
            return WrapAction(trace, action != null ? () => action(arg1, arg2, arg3, arg4, arg5, arg6) : (Action)null);
        }

        public static HResult WrapAction<T1, T2, T3, T4, T5, T6, T7>(bool trace, Action2<T1, T2, T3, T4, T5, T6, T7> action, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7)
        {
            return WrapAction(trace, action != null ? () => action(arg1, arg2, arg3, arg4, arg5, arg6, arg7) : (Action)null);
        }

        public static HResult WrapAction<T1, T2, T3, T4, T5, T6, T7, T8>(bool trace, Action2<T1, T2, T3, T4, T5, T6, T7, T8> action, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8)
        {
            return WrapAction(trace, action != null ? () => action(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) : (Action)null);
        }

        public static HResult WrapAction<T1, T2, T3, T4, T5, T6, T7, T8, T9>(bool trace, Action2<T1, T2, T3, T4, T5, T6, T7, T8, T9> action, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9)
        {
            return WrapAction(trace, action != null ? () => action(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) : (Action)null);
        }

        public static HResult WrapAction<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>(bool trace, Action2<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> action, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10)
        {
            return WrapAction(trace, action != null ? () => action(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) : (Action)null);
        }

        public static HResult WrapAction<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11>(bool trace, Action2<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11> action, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11)
        {
            return WrapAction(trace, action != null ? () => action(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) : (Action)null);
        }

        public static HResult WrapAction<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12>(bool trace, Action2<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12> action, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10, T11 arg11, T12 arg12)
        {
            return WrapAction(trace, action != null ? () => action(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) : (Action)null);
        }

        /// <summary>
        /// Returns an interface pointer that represents the specified interface for an object.
        /// </summary>
        /// <typeparam name="TInterface">Interface</typeparam>
        /// <param name="o">Object</param>
        public static IntPtr GetComInterface<TInterface>(object o)
        {
            var result = Marshal.GetComInterfaceForObject(o, typeof (TInterface));
            if (result == IntPtr.Zero)
                throw new InvalidCastException();
            return result;
        }

        /// <summary>
        /// Check if pointer has a non-zero value
        /// </summary>
        /// <param name="pointer">Pointer to check</param>
        /// <param name="prevHRes">HResult from previous function</param>
        public static HResult CheckIfNonZero(this HResult prevHRes, IntPtr pointer)
        {
            return (prevHRes == HResult.Ok || prevHRes == HResult.False) && pointer == IntPtr.Zero
                       ? (HResult) VSConstants.E_NOTIMPL
                       : prevHRes;
        }

        public static HResult Check(this HResult prevHRes, Func<HResult> check)
        {
            if (prevHRes != HResult.Ok && prevHRes != HResult.False)
                return prevHRes;

            HResult result;
            var funcRes = WrapFunction(false, check, out result);
            Debug.Assert(funcRes != HResult.False);
            if (funcRes != HResult.Ok)
                result = funcRes;
            else if (result == HResult.Ok)
                result = prevHRes; // Prompting S_FALSE result

            return result;
        }
    }
}
