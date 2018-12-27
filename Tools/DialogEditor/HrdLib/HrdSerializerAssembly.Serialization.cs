using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Xml.Serialization;

namespace HrdLib
{
    partial class HrdSerializerAssembly
    {
        private void BuildTypeSerializationMethod(HrdIndentWriter writer, HrdGeneratorQueue queue)
        {
            var type = queue.CurrentType;
            Debug.Assert(type != null);

            var serizalizableAttribute = GetSerializableAttribute(type);

            if (!serizalizableAttribute.Serializable)
                throw new InvalidOperationException(SR.GetFormatString(SR.TypeMarkedNonserializableFormat, type.FullName));

            if (type.IsArray)
            {
                SerializeArray(writer, type, queue);
                return;
            }

            var interfaces = type.GetInterfaces();
            bool isSelfSerializable = false, isCollection = false;
            foreach (var itf in interfaces)
            {
                if (itf == typeof(IHrdSerializable))
                    isSelfSerializable = true;
                else if (itf == typeof(ICollection))
                    isCollection = true;
                else if (itf.IsGenericType)
                {
                    var itfDef = itf.GetGenericTypeDefinition();
                    if (itfDef == typeof(ICollection<>))
                        isCollection = true;
                }
            }

            if (isSelfSerializable)
            {
                writer.WriteLine("(({0}) value).Serialize(writer);", ReflectionHelper.GetCsTypeName<IHrdSerializable>()).DecreaseIndent();
                return;
            }

            if (isCollection)
            {
                SerializeCollection(writer, type, queue);
                return;
            }

            SerializeType(writer, type, queue);
        }

        private void SerializeType(HrdIndentWriter writer, Type type, HrdGeneratorQueue queue)
        {
            var attrs = type.GetCustomAttributes(typeof(HrdSerializableAttribute), false);
            var attr = attrs.Length == 0 ? new HrdSerializableAttribute() : ((HrdSerializableAttribute)attrs[0]).Clone();

            if (attr.SerializeAs == HrdSerializeAs.Array && !(queue.IsRoot && attr.AnonymousRoot))
            {
                writer.WriteBeginArray(null);
                WriteTypeProperties(writer, type, attr, queue);
                writer.WriteEndArray();
            }
            else if (attr.SerializeAs != HrdSerializeAs.Array && queue.IsRoot && !attr.AnonymousRoot)
            {
                writer.WriteBeginElement(null);
                WriteTypeProperties(writer, type, attr, queue);
                writer.WriteEndElement();
            }
            else
                WriteTypeProperties(writer, type, attr, queue);
        }

        private void SerializeCollection(HrdIndentWriter writer, Type type, HrdGeneratorQueue queue)
        {
            var attrs = type.GetCustomAttributes(typeof(HrdSerializableAttribute), false);
            var attr = attrs.Length == 0 ? new HrdSerializableAttribute() : ((HrdSerializableAttribute)attrs[0]).Clone();
            if (attr.IgnoreProperties == null)
                attr.IgnoreProperties = true;

            Action<HrdIndentWriter> before = null, after = null;
            bool writeValuesAsArray = true;
            if (attr.SerializeAs == HrdSerializeAs.Array)
            {
                if (!(queue.IsRoot && attr.AnonymousRoot))
                {
                    before = w => w.WriteBeginArray(null);
                    after = w => w.WriteEndArray();
                }
                writeValuesAsArray = false;
            }
            else if (queue.IsRoot && !attr.AnonymousRoot)
            {
                before = w => w.WriteBeginElement(null);
                after = w => w.WriteEndElement();
            }

            if (before != null)
                before(writer);

            var properties = WriteTypeProperties(writer, type, attr, queue);
            if (writeValuesAsArray)
            {
                string arrName = null;
                if (properties.Count > 0)
                {
                    const string collecionPropertyName = "Collection";
                    arrName = collecionPropertyName;
                    for (int i = 1; properties.Contains(arrName); i++)
                        arrName = string.Concat(collecionPropertyName, i.ToString(CultureInfo.InvariantCulture));
                }
                writer.WriteBeginArray(arrName);
            }

            WriteCollectionElements(writer, type, attr, queue);

            if (writeValuesAsArray)
                writer.WriteEndArray();

            if (after != null)
                after(writer);
        }

        private void WriteCollectionElements(HrdIndentWriter writer, Type type, HrdSerializableAttribute attr, HrdGeneratorQueue queue)
        {
            bool isGenericCollection = false;
            Type genericCollectionElement = null;
            foreach (var itf in type.GetInterfaces().Where(i => i.IsGenericType))
            {
                var def = itf.GetGenericTypeDefinition();
                if (def != typeof(ICollection<>))
                    continue;

                isGenericCollection = true;
                var eltType = itf.GetGenericArguments()[0];
                if (attr.CollectionElement != null)
                {
                    if (eltType == attr.CollectionElement)
                    {
                        genericCollectionElement = eltType;
                        break;
                    }
                    continue;
                }

                if (genericCollectionElement == null || eltType.IsSubclassOf(genericCollectionElement))
                    genericCollectionElement = eltType;
                else if (!genericCollectionElement.IsSubclassOf(eltType) && genericCollectionElement != eltType)
                {
                    isGenericCollection = false;
                    break;
                }
            }

            if (genericCollectionElement == null)
                genericCollectionElement = attr.CollectionElement ?? typeof(object);

            var collectionType = isGenericCollection ? typeof(ICollection<>).MakeGenericType(genericCollectionElement) : typeof(ICollection);
            writer.WriteLine("foreach({0} collectionElement in ({1}) value)",
                             ReflectionHelper.GetCsTypeName(genericCollectionElement),
                             ReflectionHelper.GetCsTypeName(collectionType))
                  .WriteLine("{").IncreaseIndent();
            WriteWriteValue(writer, genericCollectionElement, "collectionElement", queue, true, false);
            writer.DecreaseIndent();
            writer.WriteLine("}");
        }

        private HashSet<string> WriteTypeProperties(HrdIndentWriter writer, Type type, HrdSerializableAttribute attr, HrdGeneratorQueue queue)
        {
            var properties = new HashSet<string>();

            Dictionary<string, HrdPropertySetterAttribute> ctrPropSetters = null;
            foreach (var ctr in type.GetConstructors(BindingFlags.Public | BindingFlags.Instance))
            {
                var ctrParameters = ctr.GetParameters();
                if (ctrParameters.Length == 0 && ctrPropSetters == null)
                {
                    ctrPropSetters = new Dictionary<string, HrdPropertySetterAttribute>();
                    continue;
                }

                var currentCtrPropSetters = new Dictionary<string, HrdPropertySetterAttribute>(ctrParameters.Length);
                foreach (var ctrParam in ctrParameters)
                {
                    var attrs = ctrParam.GetCustomAttributes(typeof(HrdPropertySetterAttribute), false);
                    if (attrs.Length == 0)
                        break;

                    var paramAttr = new HrdPropertySetterAttribute(((HrdPropertySetterAttribute)attrs[0]).PropertyName) { Parameter = ctrParam };
                    try
                    {
                        currentCtrPropSetters.Add(ctrParam.Name, paramAttr);
                    }
                    catch (ArgumentException ex)
                    {
                        throw new HrdContractException(SR.GetFormatString(SR.ParameterSetterAlreadyDeclaredFormat, ctrParam.Name), ex);
                    }
                }

                if (currentCtrPropSetters.Count != ctrParameters.Length)
                    continue;

                if (ctrPropSetters != null && ctrPropSetters.Count > 0)
                    throw new HrdContractException(SR.GetFormatString(SR.AmbigousConstructorDeclaredFormat, type.FullName));
                ctrPropSetters = currentCtrPropSetters;
            }
            if (ctrPropSetters == null)
                throw new HrdContractException(string.Format(SR.GetFormatString(SR.NoApproppriateConstructorFormat, type.FullName)));

            var props = new List<HrdSerializationAttribute>();

            var undefinedCtrParams = new HashSet<string>(ctrPropSetters.Keys);

            foreach (var prop in type.GetProperties(BindingFlags.Public | BindingFlags.Instance))
            {
                var getter = prop.GetGetMethod();
                if (getter == null)
                    continue;

                var setter = prop.GetSetMethod();
                if (setter == null && !ctrPropSetters.ContainsKey(prop.Name))
                    continue;

                undefinedCtrParams.Remove(prop.Name);

                HrdSerializationAttribute propSrzAttr = null;
                bool? ignore = null;
                foreach (var propAttr in prop.GetCustomAttributes(true))
                {
                    if (propAttr is HrdSerializationAttribute)
                        propSrzAttr = ((HrdSerializationAttribute)propAttr).Clone();
                    else if (propAttr is HrdIgnoreAttribute)
                        ignore = ((HrdIgnoreAttribute)propAttr).Ignore;
                    else if (ignore == null && attr.UseXmlAttributes && propAttr is XmlIgnoreAttribute)
                        ignore = true;
                }

                if (propSrzAttr == null)
                    propSrzAttr = new HrdSerializationAttribute(ignore ?? (attr.IgnoreProperties ?? false));

                if (propSrzAttr.Ignore)
                    continue;

                propSrzAttr.PropertyInfo = prop;
                props.Add(propSrzAttr);
            }

            if (undefinedCtrParams.Count > 0)
                throw new HrdContractException(SR.GetFormatString(SR.NoPublicPropertyFormat, type.FullName, undefinedCtrParams.First()));

            props.Sort(
                (a, b) =>
                {
                    if (ctrPropSetters.ContainsKey(a.PropertyInfo.Name))
                    {
                        if (!ctrPropSetters.ContainsKey(b.PropertyInfo.Name))
                            return -1;
                        var aProp = ctrPropSetters[a.PropertyInfo.Name];
                        var bProp = ctrPropSetters[b.PropertyInfo.Name];
                        return aProp.Parameter.Position.CompareTo(bProp.Parameter.Position);
                    }
                    int cmpRes = a.Order.CompareTo(b.Order);
                    if (cmpRes != 0)
                        return cmpRes;
                    return string.CompareOrdinal(a.PropertyInfo.Name, b.PropertyInfo.Name);
                });

            for (int i = 0; i < ctrPropSetters.Count; i++)
            {
                var prop = props[i];
                var ctrProp = ctrPropSetters[prop.PropertyInfo.Name];
                if (prop.PropertyInfo.PropertyType != ctrProp.Parameter.ParameterType)
                    throw new HrdContractException(SR.GetFormatString(SR.ParameterTypeMismatchFormat, prop.PropertyInfo.Name,
                                                                      prop.PropertyInfo.PropertyType.FullName, ctrProp.Parameter.ParameterType.FullName));
            }

            if (attr.KeepOrder)
            {
                for (int i = props.Count; i > ctrPropSetters.Count; i--)
                {
                    if (props[i].Order == props[i - 1].Order)
                        throw new HrdContractException(SR.GetFormatString(SR.IncorrectOrderFormat, props[i].PropertyInfo.Name, props[i - 1].PropertyInfo.Name));
                }
            }

            bool allowNull = true, ignoreNull = true, writeElementName = true;
            if (attr.SerializeAs != HrdSerializeAs.Array)
            {
                if (queue.IsRoot && attr.AnonymousRoot)
                {
                    allowNull = false;
                    writeElementName = false;
                }
            }
            else
                ignoreNull = false;

            foreach (var prop in props)
            {
                if (writeElementName)
                    writer.WriteBeginElement(prop.PropertyInfo.Name);
                WriteWriteValue(writer, prop.PropertyInfo.PropertyType, "value." + prop.PropertyInfo.Name, queue, allowNull, ignoreNull);
                if (writeElementName)
                    writer.WriteEndElement();

                properties.Add(prop.PropertyInfo.Name);
            }

            return properties;
        }

        private void SerializeArray(HrdIndentWriter writer, Type arrayType, HrdGeneratorQueue queue)
        {
            var elementType = arrayType.GetElementType();
            Debug.Assert(elementType != null, "The type must be an array.");

            var rank = arrayType.GetArrayRank();
            Debug.Assert(rank >= 1, "Array must have at least one dimension.");

            if (rank > 1)
            {
                writer.WriteBeginElement(null);
                writer.WriteBeginElement("Size");
                writer.WriteBeginArray(null);
                for (int i = 0; i < rank; i++)
                {
                    writer.WriteLine("int length{0} = value.GetLength({0});", i)
                          .WriteLine("writer.WriteValue(length{0});", i);
                }

                writer.WriteEndArray();
                writer.WriteEndElement();

                writer.WriteBeginElement("Values");
            }
            else
                writer.WriteLine("int length{0} = value.Length;", rank - 1);

            writer.WriteBeginArray(null);

            for (int i = 0; i < rank - 1; i++)
            {
                writer.WriteLine("for (int i{0} = 0; i{0} < length{0}; i{0}++)", i).WriteLine("{").IncreaseIndent();
                writer.WriteBeginArray(null);
            }

            writer.WriteLine("for (int i{0} = 0; i{0} < length{0}; i{0}++)", rank - 1).WriteLine("{").IncreaseIndent();
            const string delimiter = @", ";
            var builder = Enumerable.Range(0, rank)
                                    .Aggregate(new StringBuilder("value["),
                                               (sb, i) => sb.Append("i").Append(i.ToString(CultureInfo.InvariantCulture)).Append(delimiter));
            builder.Length -= delimiter.Length;
            builder.Append("]");
            WriteWriteValue(writer, elementType, builder.ToString(), queue);
            writer.DecreaseIndent();
            writer.WriteLine("}");

            for (int i = 0; i < rank - 1; i++)
            {
                writer.WriteEndArray();
                writer.DecreaseIndent();
                writer.WriteLine("}");
            }

            writer.WriteEndArray();

            if (rank > 1)
            {
                writer.WriteEndElement();
                writer.WriteEndElement();
            }
        }

        private void WriteWriteValue(HrdIndentWriter writer, Type valueType, string valueCodeString, HrdGeneratorQueue queue)
        {
            WriteWriteValue(writer, valueType, valueCodeString, queue, true, true);
        }

        private void WriteWriteValue(HrdIndentWriter writer, Type valueType, string valueCodeString, HrdGeneratorQueue queue, bool allowNull, bool ignoreNull)
        {
            bool isNullable = !valueType.IsValueType;
            if (!isNullable && valueType.IsGenericType)
            {
                var genericDefinition = valueType.GetGenericTypeDefinition();
                if (genericDefinition == typeof(Nullable<>))
                {
                    isNullable = true;
                    valueType = valueType.GetGenericArguments()[0];
                }
            }

            if (isNullable)
            {
                writer.WriteLine("{").IncreaseIndent();
                writer.Write(ReflectionHelper.GetCsTypeName(valueType));
                if (valueType.IsValueType)
                    writer.Write("?");

                writer.WriteLine(" tmpNullableValue = {0};", valueCodeString)
                      .WriteLine("if(!{0}.ReferenceEquals(tmpNullableValue, null))", ReflectionHelper.GetCsTypeName<object>()).WriteLine("{").IncreaseIndent();
                valueCodeString = "tmpNullableValue";
            }

            switch (Type.GetTypeCode(valueType))
            {
                case TypeCode.Boolean:
                case TypeCode.Byte:
                case TypeCode.Char:
                case TypeCode.Double:
                case TypeCode.Int16:
                case TypeCode.Int32:
                case TypeCode.Int64:
                case TypeCode.SByte:
                case TypeCode.Single:
                case TypeCode.String:
                case TypeCode.UInt16:
                case TypeCode.UInt32:
                case TypeCode.UInt64:
                    writer.WriteLine("writer.WriteValue(({0}) ({1}));", ReflectionHelper.GetCsTypeName(valueType), valueCodeString);
                    return;

                case TypeCode.DateTime:
                    writer.WriteLine("writer.WriteValue(({0}) ({1}), true);", ReflectionHelper.GetCsTypeName<DateTime>(), valueCodeString);
                    return;
            }

            if (valueType != typeof(object))
            {
                writer.WriteLine("Serialize(writer, ({0}) ({1}));", ReflectionHelper.GetCsTypeName(valueType), valueCodeString);
                queue.AddType(valueType);
            }
            else
                writer.WriteLine("writer.WriteEmptyValue();");

            if (isNullable)
            {
                writer.DecreaseIndent();
                writer.WriteLine("}");
                if (!allowNull || !ignoreNull)
                {
                    writer.WriteLine("else").WriteLine("{").IncreaseIndent();
                    if (!allowNull)
                        writer.WriteLine("throw new {0}({1});", ReflectionHelper.GetCsTypeName<HrdContractException>(),
                                         MakeVerbatimResourceString(SR.PropertyCantBeNullFormat, valueCodeString));
                    else
                        writer.WriteLine("writer.WriteEmptyValue();");
                    writer.DecreaseIndent();
                    writer.WriteLine("}");
                }

                writer.DecreaseIndent();
                writer.WriteLine("}");
            }
        }
    }
}
