using System;
using System.IO;
using System.Threading;

namespace HrdLib
{
    public class HrdSerializer
    {
        private readonly Type _baseType;

        private HrdSerializerAssembly _assembly;

        private HrdSerializerAssembly Assembly
        {
            get
            {
                if (_assembly != null)
                    return _assembly;

                var asm = HrdAssemblyCache.GetOrCreateAssembly(_baseType);

                // An assembly class may be created twice or more, but each thread shall get the same instance of the assembly class
                return Interlocked.CompareExchange(ref _assembly, asm, null) ?? asm;
            }
        }

        public HrdSerializer(Type type)
        {
            if (type == null)
                throw new ArgumentNullException("type");
            if (type.IsInterface)
                throw new ArgumentException(SR.GetFormatString(SR.TypeCantBeInterfaceFormat, type.FullName), "type");
            if (type.IsGenericTypeDefinition)
                throw new ArgumentException(SR.GetFormatString(SR.TypeCantBeGenericDefinitionFormat, type.FullName), "type");

            _baseType = type;
        }

        public void Serialize(Stream stream, object obj)
        {
            SerializeInternal(stream, obj);
        }

        private void SerializeInternal<T>(Stream stream, T obj)
        {
            var innerSerializer = Assembly.CreateSerializer();
            HrdWriter writer = null;
            try
            {
                writer = new HrdWriter(stream);
                var exactSerializer = innerSerializer as IHrdSerializer<T>;
                if (exactSerializer != null)
                    exactSerializer.Serialize(writer, obj);
                else
                    innerSerializer.Serialize(writer, obj);
            }
            finally
            {
                if (writer != null)
                    writer.Close();
            }
        }

        protected virtual void Serialize(HrdWriter writer, object obj)
        {
            throw new NotSupportedException();
        }

        public static void Serialize<T>(Stream stream, T obj)
        {
            var serializer = new HrdSerializer(typeof (T));
            serializer.SerializeInternal(stream, obj);
        }

        public object Deserialize(Stream stream)
        {
            return DeserializeInternal<object>(stream);
        }

        private T DeserializeInternal<T>(Stream stream)
        {
            var innerSerializer = Assembly.CreateSerializer();
            HrdReader reader = null;
            try
            {
                reader = new HrdReader(stream);
                var exactSerializer = innerSerializer as IHrdSerializer<T>;
                if (exactSerializer != null)
                    return exactSerializer.Deserialize(reader);
                return (T) innerSerializer.Deserialize(reader);
            }
            finally
            {
                if (reader != null)
                    reader.Close();
            }
        }

        protected virtual object Deserialize(HrdReader reader)
        {
            throw new NotSupportedException();
        }

        public static T Deserialize<T>(Stream stream)
        {
            var serializer = new HrdSerializer(typeof (T));
            var result = serializer.DeserializeInternal<T>(stream);
            return result;
        }
    }
}
