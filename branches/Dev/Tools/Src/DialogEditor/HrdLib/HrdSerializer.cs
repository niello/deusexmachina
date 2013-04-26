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
            var innerSerializer = Assembly.CreateSerializer();
            var writer = new HrdWriter(stream);

            innerSerializer.Serialize(writer, obj);

            var document = writer.EndWrite();
            document.WriteDocument(stream);
        }

        protected virtual void Serialize(HrdWriter writer, object obj)
        {
            throw new NotSupportedException();
        }

        public static void Serialize<T>(Stream stream, T obj)
        {
            var serializer = new HrdSerializer(typeof (T));
            serializer.Serialize(stream, (object) obj);
        }

        public object Deserialize(Stream stream)
        {
            throw new NotImplementedException();
        }

        public static T Deserialize<T>(Stream stream)
        {
            var serializer = new HrdSerializer(typeof (T));
            var result = serializer.Deserialize(stream);
            return (T) result;
        }
    }
}
