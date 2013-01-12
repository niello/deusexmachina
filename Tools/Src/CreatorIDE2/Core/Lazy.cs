using System;
using System.Threading;

namespace CreatorIDE.Core
{
    public sealed class Lazy<T> : IDisposable
        where T:class
    {
        private readonly object _syncObject = new object();
        private readonly bool _autoDispose;
        private readonly Func<T> _factoryMethod;

        private volatile int _disposed;
        private volatile T _object;

        public T Value
        {
            get
            {
                var obj = _object;
                if (obj == null)
                {
                    lock (_syncObject)
                    {
                        if (_disposed != 0)
                            throw new ObjectDisposedException("this");

                        obj = _object;
                        if (obj == null)
                            _object = obj = _factoryMethod();
                    }
                }
                else if (_disposed != 0)
                    throw new ObjectDisposedException("this");
                return obj;
            }
        }

        public Lazy(Func<T> factoryMethod):
            this (true, factoryMethod)
        {}

        public Lazy(bool disposeObject, Func<T> factoryMethod)
        {
            if (factoryMethod == null)
                throw new ArgumentNullException("factoryMethod");

            _autoDispose = disposeObject;
            _factoryMethod = factoryMethod;
        }

        public void Reset()
        {
            Reset(_autoDispose);
        }

        public void Reset(bool dispose)
        {
            if (_disposed != 0)
                return;

            lock(_syncObject)
            {
                var disposable = _object as IDisposable;
                _object = null;
                if (dispose && disposable != null)
                    disposable.Dispose();
            }
        }

        public void Dispose()
        {
            Dispose(true);
        }

        private void Dispose(bool disposing)
        {
            if (Interlocked.Exchange(ref _disposed, 1) != 0 || !_autoDispose)
                return;

            if (disposing)
                GC.SuppressFinalize(this);

            var disposable = _object as IDisposable;
            if (disposable != null)
                disposable.Dispose();
        }

        ~Lazy()
        {
            Dispose(false);
        }
    }
}
