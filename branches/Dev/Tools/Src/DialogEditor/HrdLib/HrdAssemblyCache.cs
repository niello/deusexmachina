using System;
using System.Collections.Generic;

namespace HrdLib
{
    /// <summary>
    /// Provides access to generated assemblies
    /// </summary>
    internal class HrdAssemblyCache
    {
        private static readonly Dictionary<Type, HrdSerializerAssembly> Assemblies = new Dictionary<Type, HrdSerializerAssembly>();

        public static HrdSerializerAssembly GetOrCreateAssembly(Type type)
        {
            if (type == null)
                throw new ArgumentNullException("type");

            lock (Assemblies)
            {
                HrdSerializerAssembly asm;
                if (!Assemblies.TryGetValue(type, out asm))
                {
                    asm = new HrdSerializerAssembly(type);
                    Assemblies.Add(type, asm);
                }
                return asm;
            }
        }
    }
}
