using System;
using System.Collections;
using System.Collections.Generic;

namespace CreatorIDE.Core
{
    public class TypeDictionary<T> : IDictionary<Type, T>
    {
        private readonly object _writeLock = new object();
        private readonly Dictionary<Type, T> _innerDictionary = new Dictionary<Type, T>();

        #region IDictionary<Type, T> implementation

        public IEnumerator<KeyValuePair<Type, T>> GetEnumerator()
        {
            return _innerDictionary.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        void ICollection<KeyValuePair<Type, T>>.Add(KeyValuePair<Type, T> item)
        {
            Add(item.Key, item.Value);
        }

        public void Clear()
        {
            _innerDictionary.Clear();
        }

        bool ICollection<KeyValuePair<Type, T>>.Contains(KeyValuePair<Type, T> item)
        {
            KeyValuePair<Type, T> pair;
            if (!TryGetValue(item.Key, out pair))
                return false;

            return Equals(pair.Value, item.Value);
        }

        void ICollection<KeyValuePair<Type, T>>.CopyTo(KeyValuePair<Type, T>[] array, int arrayIndex)
        {
            ((ICollection<KeyValuePair<Type, T>>)_innerDictionary).CopyTo(array, arrayIndex);
        }

        bool ICollection<KeyValuePair<Type, T>>.Remove(KeyValuePair<Type, T> item)
        {
            lock (_writeLock)
            {
                KeyValuePair<Type, T> pair;
                if (!TryGetValue(item.Key, out pair))
                    return false;

                if (!Equals(pair.Value, item))
                    return false;

                return _innerDictionary.Remove(pair.Key);
            }
        }

        public int Count
        {
            get { return _innerDictionary.Count; }
        }

        bool ICollection<KeyValuePair<Type, T>>.IsReadOnly
        {
            get { return false; }
        }

        public bool ContainsKey(Type key)
        {
            var type = key;
            while (type != null)
            {
                if (_innerDictionary.ContainsKey(type))
                    return true;
                type = type.BaseType;
            }
            return false;
        }

        public void Add(Type key, T value)
        {
            lock (_writeLock)
            {
                KeyValuePair<Type, T> pair;
                if (!TryGetValue(key, out pair))
                    _innerDictionary.Add(key, value);
                else
                    throw new ArgumentException(string.Format("An element for the same type ({0}) already exists.",
                                                          pair.Key.Name));
            }
        }

        public bool Remove(Type key)
        {
            lock (_writeLock)
            {
                KeyValuePair<Type, T> pair;
                if (TryGetValue(key, out pair))
                    return _innerDictionary.Remove(pair.Key);
                return false;
            }
        }

        public bool TryGetValue(Type key, out T value)
        {
            KeyValuePair<Type, T> result;
            if (!TryGetValue(key, out result))
            {
                value = default(T);
                return false;
            }
            value = result.Value;
            return true;
        }

        T IDictionary<Type, T>.this[Type key]
        {
            get
            {
                KeyValuePair<Type, T> pair;
                if (!TryGetValue(key, out pair))
                    throw new KeyNotFoundException(string.Format("There is no value for the type {0} in the dictionary.", key.Name));
                return pair.Value;
            }
            set
            {
                lock (_writeLock)
                {
                    KeyValuePair<Type, T> pair;
                    if (!TryGetValue(key, out pair))
                        _innerDictionary[key] = value;
                    _innerDictionary[pair.Key] = value;
                }
            }
        }

        ICollection<Type> IDictionary<Type, T>.Keys
        {
            get { return _innerDictionary.Keys; }
        }

        ICollection<T> IDictionary<Type, T>.Values
        {
            get { return _innerDictionary.Values; }
        }

        #endregion

        public void Add<TType>(T value)
        {
            Add(typeof(TType), value);
        }

        public bool TryGetRealKey(Type key, out Type realKey)
        {
            KeyValuePair<Type, T> pair;
            if (!TryGetValue(key, out pair))
            {
                realKey = null;
                return false;
            }

            realKey = pair.Key;
            return true;
        }

        public bool TryGetValue<TValue>(out TValue value)
            where TValue : T
        {
            KeyValuePair<Type, T> pair;
            if (!TryGetValue(typeof(TValue), out pair) || !(pair.Value is TValue))
            {
                value = default(TValue);
                return false;
            }

            value = (TValue)pair.Value;
            return true;
        }

        private bool TryGetValue(Type key, out KeyValuePair<Type, T> pair)
        {
            var type = key;
            while (type != null)
            {
                T value;
                if (_innerDictionary.TryGetValue(type, out value))
                {
                    pair = new KeyValuePair<Type, T>(type, value);
                    return true;
                }
                type = type.BaseType;
            }

            foreach (var intf in key.GetInterfaces())
            {
                T value;
                if (_innerDictionary.TryGetValue(intf, out value))
                {
                    pair = new KeyValuePair<Type, T>(intf, value);
                    return true;
                }
            }

            pair = default(KeyValuePair<Type, T>);
            return false;
        }
    }
}
