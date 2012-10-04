using System;
using System.Collections.Generic;
using CreatorIDE.Core;

namespace CreatorIDE
{
    internal sealed class CreatorIDEContextManager : IAppContextManager
    {
        private readonly TypeDictionary<object> _contexts = new TypeDictionary<object>();

        public T GetContext<T>() where T : class
        {
            T context;
            if (!_contexts.TryGetValue(out context))
                throw new KeyNotFoundException(string.Format("Context of type {0} not found.", typeof (T)));
            return context;
        }

        public bool TryGetContext<T>(out T context)
            where T : class
        {
            return _contexts.TryGetValue(out context);
        }

        public void RegisterContext<T>(T context)
        {
            RegisterContext(typeof (T), context);
        }

        public void RegisterContext(Type contextType, object context)
        {
            _contexts.Add(contextType, context);
        }
    }
}
