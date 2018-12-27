using System;

namespace HrdLib
{
    /// <summary>
    /// Attribute describes how to serialize an object of the specified type with HRD serializer
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = false)]
    public sealed class HrdSerializableAttribute:Attribute,ICloneable
    {
        /// <summary>
        /// Instruction for the serizlizer to use XML serialization attributes.
        /// Xml attributes has less priority than HRD attributes. Default value is <b>true</b>.
        /// </summary>
        public bool UseXmlAttributes { get; set; }

        /// <summary>
        /// Indicates if the type is serializable. Default value is <b>true</b>.
        /// </summary>
        public bool Serializable { get; set; }

        /// <summary>
        /// Indicates how the element should be serialized.
        /// Default value is <see cref="HrdSerializeAs.Auto">Auto</see>.
        /// </summary>
        public HrdSerializeAs SerializeAs { get; set; }

        /// <summary>
        /// Indicates if type's properties must be serialized in the defined order. If an order is not defined, will be used an alphabetic order.
        /// Default value is <b>false</b>.
        /// </summary>
        /// <remarks>
        /// If <see cref="HrdSerializableAttribute.SerializeAs">SerializeAs</see> property's value is <see cref="HrdSerializeAs.Array">Array</see>
        /// or <see cref="HrdSerializableAttribute.AnonymousRoot">AnonymousRoot</see> property's value is <b>true</b>, this property's value will be ignored.
        /// </remarks>
        public bool KeepOrder { get; set; }

        /// <summary>
        /// This property affects only root element. It indicates that each property of current type must be serialized as anonymous propery.
        /// Default value is <b>false</b>.
        /// </summary>
        /// <remarks>
        /// If <see cref="HrdSerializableAttribute.SerializeAs">SerializeAs</see> property's value is <see cref="HrdSerializeAs.Item">Item</see>,
        /// no one property of current type can be <b>null</b>.
        /// </remarks>
        public bool AnonymousRoot { get; set; }

        /// <summary>
        /// Defines if the properties are ignored by default. If class implements <see cref="System.Collections.ICollection">ICollection</see> or 
        /// <see cref="System.Collections.Generic.ICollection{T}">ICollection&lt;T&gt;</see> the value for this property will be <b>true</b>, otherwise <b>false</b>.
        /// </summary>
        public bool? IgnoreProperties { get; set; }

        /// <summary>
        /// If the serializable class is <see cref="System.Collections.ICollection">ICollection</see>, use this property to define the type of it's element.
        /// </summary>
        public Type CollectionElement { get; set; }

        public HrdSerializableAttribute()
        {
            SerializeAs = HrdSerializeAs.Auto;
            UseXmlAttributes = true;
            Serializable = true;
        }

        public HrdSerializableAttribute Clone()
        {
            return new HrdSerializableAttribute
                {
                    AnonymousRoot = AnonymousRoot,
                    IgnoreProperties = IgnoreProperties,
                    KeepOrder = KeepOrder,
                    Serializable = Serializable,
                    SerializeAs = SerializeAs,
                    UseXmlAttributes = UseXmlAttributes
                };
        }

        object ICloneable.Clone()
        {
            return Clone();
        }
    }

    /// <summary>
    /// Instruction for an hrd serializer how to interpret the item
    /// </summary>
    public enum HrdSerializeAs
    {
        /// <summary>
        /// Serialize the item as a node
        /// </summary>
        Item = 1,

        /// <summary>
        /// Serialize the item as an array
        /// </summary>
        Array = 2,

        /// <summary>
        /// Let the serializer to deside how to serialize the item
        /// </summary>
        Auto = 3
    }
}
