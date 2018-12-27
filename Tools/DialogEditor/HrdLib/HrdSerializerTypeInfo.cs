using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;

namespace HrdLib
{
    internal class HrdSerializerTypeInfo
    {
        private readonly Type _type, _elementType;
        private readonly bool _isSelfSerializable;
        private readonly Type _collectionInterface;
        private readonly HrdSerializableAttribute _classAttribute;

        /// <summary>
        /// Describing type
        /// </summary>
        public Type Type
        {
            get { return _type; }
        }

        /// <summary>
        /// Type of an element of array or collection.
        /// </summary>
        public Type ElementType
        {
            get { return _elementType; }
        }

        /// <summary>
        /// Collection's interface if <see cref="HrdSerializerTypeInfo.Type"/> is a collection. Otherwise null.
        /// </summary>
        public Type CollectionInterface
        {
            get { return _collectionInterface; }
        }

        /// <summary>
        /// Type of the element which could be added to the collection through <b>Add</b> method.
        /// </summary>
        public Type CollectionInterfaceElement
        {
            get
            {
                if (_collectionInterface == null)
                    return null;

                if (!_collectionInterface.IsGenericType)
                    return typeof (object);

                return _collectionInterface.GetGenericArguments()[0];
            }
        }

        /// <summary>
        /// Describes if <see cref="HrdSerializerTypeInfo.Type"/> can serialize itself (i.e. it implements <see cref="IHrdSerializable"/>).
        /// </summary>
        public bool IsSelfSerializable
        {
            get { return _isSelfSerializable; }
        }

        public HrdSerializableAttribute Attribute
        {
            get { return _classAttribute; }
        }

        public bool IsCollection
        {
            get { return _collectionInterface != null; }
        }

        public bool IsArray
        {
            get { return Type.IsArray; }
        }

        public bool IsNullable
        {
            get
            {
                if (Type.IsValueType)
                    return true;

                if (!Type.IsGenericType)
                    return false;

                var def = Type.GetGenericTypeDefinition();
                return def == typeof (Nullable<>);
            }
        }

        public bool IsGeneric
        {
            get { return Type.IsGenericType; }
        }

        public int ArrayRank
        {
            get { return IsArray ? -1 : Type.GetArrayRank(); }
        }

        public HrdSerializerTypeInfo(Type type)
        {
            if (type == null)
                throw new ArgumentNullException("type");

            _type = type;
            var attribute = type.GetCustomAttributes(typeof (HrdSerializableAttribute), false);
            if (attribute.Length > 0)
                _classAttribute = ((HrdSerializableAttribute) attribute[0]).Clone();
            else
                _classAttribute = new HrdSerializableAttribute();

            if (type.IsArray)
            {
                _elementType = type.GetElementType();
                Debug.Assert(_elementType != null);
            }
            else
            {
                var interfaces = type.GetInterfaces();
                foreach (var itf in interfaces)
                {
                    if (itf == typeof (IHrdSerializable))
                        _isSelfSerializable = true;
                    else if (itf == typeof (IList))
                    {
                        if (_collectionInterface == null)
                            _collectionInterface = itf;
                    }
                    else if (itf.IsGenericType && itf.GetGenericTypeDefinition() == typeof (ICollection<>))
                    {
                        var elementDefinition = itf.GetGenericArguments()[0];
                        if (_classAttribute.CollectionElement == null || elementDefinition.IsAssignableFrom(_classAttribute.CollectionElement))
                        {
                            _collectionInterface = itf;
                            _elementType = _classAttribute.CollectionElement ?? elementDefinition;
                        }
                    }
                }

                if (_collectionInterface != null && _elementType == null && (_elementType = _classAttribute.CollectionElement) == null)
                    _collectionInterface = null;
            }
        }
    }
}
