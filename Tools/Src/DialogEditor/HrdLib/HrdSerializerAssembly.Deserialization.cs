using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Reflection;
using System.Text;

namespace HrdLib
{
	partial class HrdSerializerAssembly
	{
        private void BuildTypeDeserializationMethod(HrdIndentWriter writer, HrdGeneratorQueue queue)
        {
            var type = queue.CurrentType;
            Debug.Assert(type != null);

            var typeInfo = new HrdSerializerTypeInfo(type);

            if (!typeInfo.Attribute.Serializable)
                throw new InvalidOperationException(SR.GetFormatString(SR.TypeMarkedNonserializableFormat, type.FullName));

            if (typeInfo.IsSelfSerializable)
            {
                DeserializeArray(writer, typeInfo, queue);
                return;
            }

            if (typeInfo.IsSelfSerializable)
            {
                var ctr = type.GetConstructor(new Type[0]);
                if (ctr == null || !ctr.IsPublic)
                    throw new HrdStructureValidationException(SR.GetFormatString(SR.ParameterlessConstructorRequiredFormat, type.FullName));

                writer.WriteLine("{0} result = new {0}();", ReflectionHelper.GetCsTypeName(type))
                      .WriteLine("(({0}) result).Deserialize(reader);", ReflectionHelper.GetCsTypeName<IHrdSerializable>())
                      .WriteLine("return result;");

                return;
            }

            if (typeInfo.IsCollection)
            {
                DeserializeCollection(writer, typeInfo, queue);
                return;
            }

            DeserializeType(writer, typeInfo, queue);
        }

        private void DeserializeType(HrdIndentWriter writer, HrdSerializerTypeInfo typeInfo, HrdGeneratorQueue queue)
        {
            throw new NotImplementedException();
        }

        private void DeserializeCollection(HrdIndentWriter writer, HrdSerializerTypeInfo typeInfo, HrdGeneratorQueue queue)
        {
            if (typeInfo.ElementType == null)
                throw new HrdStructureValidationException(SR.GetString(SR.CollectionElementTypeNotDefined));

            var ctr = typeInfo.Type.GetConstructor(new Type[0]);
            if (ctr == null || !ctr.IsPublic)
                throw new HrdStructureValidationException(SR.GetFormatString(SR.ParameterlessConstructorRequiredFormat, typeInfo.Type.FullName));

            writer.WriteLine("{0} result = new {0}();", ReflectionHelper.GetCsTypeName(typeInfo.Type));
            writer.WriteBeginElement(null);
            if (typeInfo.Attribute.SerializeAs == HrdSerializeAs.Array)
            {
                writer.WriteLine("while(reader.ReadNextSibling())").WriteLine("{").IncreaseIndent();
                WriteReadValue(writer, typeInfo, ReflectionHelper.GetCsTypeName(typeInfo.ElementType) + " item", queue);

                writer.WriteLine("(({0}) result).Add(({1}) item);", ReflectionHelper.GetCsTypeName(typeInfo.CollectionInterface),
                                 ReflectionHelper.GetCsTypeName(typeInfo.CollectionInterfaceElement))
                      .DecreaseIndent()
                      .WriteLine("}");
            }
            else
            {
                throw new NotImplementedException();
            }

            writer.WriteLine("return result;");
        }

        private void DeserializeArray(HrdIndentWriter writer, HrdSerializerTypeInfo arrayType, HrdGeneratorQueue queue)
        {
            Debug.Assert(arrayType.IsArray, "The type must be an array.");
            Debug.Assert(arrayType.ArrayRank >= 1, "Array must have at least one dimension.");

            if (arrayType.ArrayRank > 1)
            {
                writer.ReadBeginElement("Size");
                writer.ReadBeginElement();

                var sizeBuilder = new StringBuilder("[");
                for (int i = 0; i < arrayType.ArrayRank; i++)
                {
                    if (i > 0)
                    {
                        writer.ReadNextSibling();
                        sizeBuilder.Append(", ");
                    }
                    writer.WriteLine("int length{0} = reader.ReadInt32();", i);
                    sizeBuilder.AppendFormat(CultureInfo.InvariantCulture, "length{0}", i);
                }
                sizeBuilder.Append("]");

                writer.ReadNextSibling();
                writer.ReadBeginElement("Value");

                Type realElementType;
                var subElementRank = ReflectionHelper.GetArrayRankString(arrayType.ElementType, out realElementType);
                if (subElementRank != null)
                    sizeBuilder.Append(subElementRank);

                writer.WriteLine("{0} result = new {1}{2};", ReflectionHelper.GetCsTypeName(arrayType.Type),
                                 ReflectionHelper.GetCsTypeName(realElementType), sizeBuilder);

                var resultBuilder = new StringBuilder("result[");
                for (int i = 0; i < arrayType.ArrayRank; i++)
                {
                    writer.WriteLine("for(int i{0} = 0; i{0} < length{0}; i{0}++)", i).WriteLine("{").IncreaseIndent();
                    if (i > 0)
                        resultBuilder.Append(", ");
                    resultBuilder.AppendFormat(CultureInfo.InvariantCulture, "i{0}", i);
                }
                resultBuilder.Append("]");

                WriteReadValue(writer, arrayType, resultBuilder.ToString(), queue);

                for (int i = 0; i < arrayType.ArrayRank; i++)
                {
                    writer.ReadNextSibling();
                    writer.DecreaseIndent().WriteLine("}");
                }
            }
            else
            {
                Type realElementType;
                var subElementRank = ReflectionHelper.GetArrayRankString(arrayType.ElementType, out realElementType);

                writer.WriteLine("int length = reader.ChildrenCount;")
                      .Write("{0} result = new {1}[length]", ReflectionHelper.GetCsTypeName(arrayType.ElementType), ReflectionHelper.GetCsTypeName(realElementType));

                if (subElementRank != null)
                    writer.Write(subElementRank);

                writer.WriteLine(";");

                writer.WriteLine("for(int i = 0; i< length; i++)").WriteLine("{").IncreaseIndent();
                WriteReadValue(writer, arrayType, "result[i]", queue);

                writer.DecreaseIndent().WriteLine("}");
            }

            writer.WriteLine("return result;");
        }

        private void WriteReadValue(HrdIndentWriter writer, HrdSerializerTypeInfo valueType, string valueCodeString, HrdGeneratorQueue queue)
        {
            WriteReadValue(writer, valueType, valueCodeString, queue, true);
        }

        private void WriteReadValue(HrdIndentWriter writer, HrdSerializerTypeInfo valueType, string valueCodeString, HrdGeneratorQueue queue, bool allowNull)
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

            writer.WriteLine("if(!reader.HasValue)").WriteLine("{").IncreaseIndent();
            if (isNullable && allowNull)
            {
                writer.WriteLine("{0} = ({1}) null;", valueCodeString,
                                 ReflectionHelper.GetCsTypeName(valueType) + (valueType.IsValueType ? "?" : string.Empty));
            }
            else
            {
                writer.WriteLine("throw new {0}({1});", ReflectionHelper.GetCsTypeName<HrdStructureValidationException>(),
                                     MakeVerbatimString(SR.GetString(SR.NullValueNotAllowed)));
            }
            writer.DecreaseIndent().WriteLine("}").WriteLine("else").WriteLine("{").IncreaseIndent();

            var typeCode = Type.GetTypeCode(valueType);
            switch (typeCode)
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
                    writer.WriteLine("{0} = reader.Read{1}();", valueCodeString, typeCode);
                    break;

                case TypeCode.DateTime:
                    writer.WriteLine("{0} = reader.ReadDateTime(true);", ReflectionHelper.GetCsTypeName<DateTime>(), valueCodeString);
                    break;

                default:
                    if (valueType == typeof(object))
                        writer.WriteLine("{0} = new {1}();", valueCodeString, ReflectionHelper.GetCsTypeName<object>());
                    else
                        writer.WriteLine("{0} = Deserialize{1}(reader);", valueCodeString, queue.AddType(valueType));
                    break;
            }

            writer.DecreaseIndent().WriteLine("}");
        }
	}
}
