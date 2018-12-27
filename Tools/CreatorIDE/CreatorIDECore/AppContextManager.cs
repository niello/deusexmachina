using System;

namespace CreatorIDE.Core
{
    public sealed class AppContextManager:IAppContextManager,IDisposable
    {
        private static readonly AppContextManager Instance = new AppContextManager();

        private AppContextManager()
        {}

        private readonly object _registrationLockObject=new object();
        private IAppContextManager _innerContextManager;

        public static void SetContextManager(IAppContextManager contextManager)
        {
            lock (Instance._registrationLockObject)
            {
                if (Instance._innerContextManager != null)
                    throw new Exception("Another context manager is already regisered. You can register a context manager only once.");

                Instance._innerContextManager = contextManager;
            }
        }

        bool IAppContextManager.TryGetContext<T>(out T context)
        {
            if(_innerContextManager==null)
                throw new Exception("Context manager is not registered.");

            return _innerContextManager.TryGetContext(out context);
        }

        T IAppContextManager.GetContext<T>()
        {
            if(_innerContextManager==null)
                throw new Exception("Context manager is not registered.");

            return _innerContextManager.GetContext<T>();
        }

        public static T GetContext<T>()
            where T: class
        {
            return ((IAppContextManager)Instance).GetContext<T>();
        }

        public static bool TryGetContext<T>(out T context)
            where T : class
        {
            return ((IAppContextManager)Instance).TryGetContext(out context);
        }

        void IDisposable.Dispose()
        {
            lock(_registrationLockObject)
            {
                var disposable = _innerContextManager as IDisposable;
                if (disposable != null)
                    disposable.Dispose();
                _innerContextManager = null;
            }
        }

        public static void Dispose()
        {
            ((IDisposable) Instance).Dispose();
        }

        public static IWindowManager WindowManager { get { return GetContext<IWindowManager>(); } }

        public static IAppConfiguration Configuration { get { return GetContext<IAppConfiguration>(); } }
    }
}
