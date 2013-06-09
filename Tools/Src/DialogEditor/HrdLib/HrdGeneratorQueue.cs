using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;

namespace HrdLib
{
    internal class HrdGeneratorQueue
    {
        private readonly Queue<Type> _typeQueue = new Queue<Type>();
        private readonly HashSet<Type> _types = new HashSet<Type>();
        private readonly HashSet<Type> _rootTypes = new HashSet<Type>(); 
        private readonly HashSet<string> _assemblies = new HashSet<string>(StringComparer.InvariantCulture);
        private readonly Dictionary<Type, int> _typeIDMap = new Dictionary<Type, int>();

        private bool _isFirst;
        private int _typeIdentity = 1;

        public bool IsRoot
        {
            get
            {
                var type = CurrentType;
                return type != null && _rootTypes.Contains(type);
            }
        }

        public Type CurrentType { get { return _typeQueue.Count == 0 ? null : _typeQueue.Peek(); } }

        public int CurrentTypeID
        {
            get
            {
                var type = CurrentType;
                int typeID;
                if (type == null || !_typeIDMap.TryGetValue(type, out typeID))
                    typeID = -1;
                return typeID;
            }
        }

        public HrdGeneratorQueue(Type rootType, params Type[] additionalRootTypes)
        {
            if (rootType == null)
                throw new ArgumentNullException("rootType");

            _isFirst = true;

            AddType(rootType, true);
            _rootTypes.Add(rootType);
            
            if (additionalRootTypes == null || additionalRootTypes.Length == 0)
                return;

            foreach (var additionalType in additionalRootTypes.Where(additionalType => additionalType != null))
            {
                AddType(additionalType, true);

                if (!_rootTypes.Contains(additionalType))
                    _rootTypes.Add(additionalType);
            }
        }

        public int AddType(Type type, bool enqueue)
        {
            if (type == null)
                throw new ArgumentNullException("type");

            int typeID;
            if (!_typeIDMap.TryGetValue(type, out typeID))
            {
                typeID = _typeIdentity++;
                _typeIDMap.Add(type, typeID);
            }

            if (enqueue)
                _typeQueue.Enqueue(type);
            else
                RegisterType(type);

            return typeID;
        }

        public int AddType(Type type)
        {
            return AddType(type, true);
        }

        private void RegisterType(Type type)
        {
            if (_types.Contains(type))
                return;

            _types.Add(type);
            var asmName = type.Assembly.FullName;
            Debug.Assert(asmName != null);
            if (!_assemblies.Contains(asmName))
                _assemblies.Add(asmName);
        }

        public bool MoveNext()
        {
            if (_isFirst)
                _isFirst = false;
            else if (_typeQueue.Count > 0)
                _typeQueue.Dequeue();

            var isMoved = _typeQueue.Count > 0;
            if (isMoved)
            {
                var type = _typeQueue.Peek();
                Debug.Assert(type != null);
                if (_types.Contains(type))
                    return MoveNext();

                RegisterType(type);
            }
            return isMoved;
        }

        public int GetTypeID(Type type)
        {
            if (type == null)
                throw new ArgumentNullException("type");

            return AddType(type, false);
        }

        public Type[] GetRootTypes()
        {
            return _rootTypes.ToArray();
        }

        public HashSet<string> GetReferencedAssemblies()
        {
            return new HashSet<string>(_assemblies);
        }
    }
}
