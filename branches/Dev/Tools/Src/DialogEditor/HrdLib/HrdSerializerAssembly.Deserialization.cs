using System;
using System.Diagnostics;
using System.Globalization;
using System.Text;

namespace HrdLib
{
	partial class HrdSerializerAssembly
	{
        private void BuildTypeDeserializationMethod(HrdIndentWriter writer, HrdGeneratorQueue queue)
        {
            var type = queue.CurrentType;
            Debug.Assert(type != null);

            var serizalizableAttribute = GetSerializableAttribute(type);

            if (!serizalizableAttribute.Serializable)
                throw new InvalidOperationException(SR.GetFormatString(SR.TypeMarkedNonserializableFormat, type.FullName));

            if (type.IsArray)
            {
                DeserializeArray(writer, type, queue);
                return;
            }

            //TODO: deserialize any other types
            writer.WriteLine("return default({0});", ReflectionHelper.GetCsTypeName(queue.CurrentType));
        }

        private void DeserializeArray(HrdIndentWriter writer, Type arrayType, HrdGeneratorQueue queue)
        {
            var elementType = arrayType.GetElementType();
            Debug.Assert(elementType != null, "The type must be an array.");

            var rank = arrayType.GetArrayRank();
            Debug.Assert(rank >= 1, "Array must have at least one dimension.");

            if (rank > 1)
            {
                writer.ReadBeginElement("Size");
                writer.ReadBeginElement();

                var sizeBuilder = new StringBuilder("[");
                for (int i = 0; i < rank; i++)
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
                var subElementRank = ReflectionHelper.GetArrayRankString(elementType, out realElementType);
                if (subElementRank != null)
                    sizeBuilder.Append(subElementRank);

                writer.WriteLine("{0} result = new {1}{2};", ReflectionHelper.GetCsTypeName(arrayType),
                                 ReflectionHelper.GetCsTypeName(realElementType), sizeBuilder);

                var resultBuilder = new StringBuilder("result[");
                for (int i = 0; i < rank; i++)
                {
                    writer.WriteLine("for(int i{0} = 0; i{0} < length{0}; i{0}++)", i).WriteLine("{").IncreaseIndent();
                    if (i > 0)
                        resultBuilder.Append(", ");
                    resultBuilder.AppendFormat(CultureInfo.InvariantCulture, "i{0}", i);
                }
                resultBuilder.Append("]");

                WriteReadValue(writer, elementType, resultBuilder.ToString(), queue);

                for (int i = 0; i < rank; i++)
                {
                    writer.ReadNextSibling();
                    writer.DecreaseIndent().WriteLine("}");
                }
            }
            else
            {
                Type realElementType;
                var subElementRank = ReflectionHelper.GetArrayRankString(elementType, out realElementType);

                writer.WriteLine("int length = reader.ChildrenCount;")
                      .Write("{0} result = new {1}[length]", ReflectionHelper.GetCsTypeName(arrayType), ReflectionHelper.GetCsTypeName(realElementType));

                if (subElementRank != null)
                    writer.Write(subElementRank);

                writer.WriteLine(";");

                writer.WriteLine("for(int i = 0; i< length; i++)").WriteLine("{").IncreaseIndent();
                WriteReadValue(writer, elementType, "result[i]", queue);

                writer.DecreaseIndent().WriteLine("}");
            }

            writer.WriteLine("return result;");
        }

        private void WriteReadValue(HrdIndentWriter writer, Type valueType, string valueCodeString, HrdGeneratorQueue queue)
        {
            WriteReadValue(writer, valueType, valueCodeString, queue, true);
        }

        private void WriteReadValue(HrdIndentWriter writer, Type valueType, string valueCodeString, HrdGeneratorQueue queue, bool allowNull)
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
