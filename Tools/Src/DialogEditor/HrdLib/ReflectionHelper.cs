using System;
using System.Linq;
using System.Collections.Generic;
using System.Diagnostics;

namespace HrdLib
{
    internal static class ReflectionHelper
    {
        /// <summary>
        /// A special character which is used to separate a generic type's name from it's generic arguments
        /// </summary>
        private const char GenericNameSeparator = '`';

        public static string GetCsTypeName(Type type)
        {
            if (type == null)
                throw new ArgumentNullException("type");

            if (type.IsArray)
            {
                var rankList = new List<string>();

                Type t;
                for (t = type; t != null && t.IsArray; t = t.GetElementType())
                {
                    var rank = t.GetArrayRank();
                    string rankStr;
                    if (rank > 1)
                        rankStr = string.Concat("[", new string(',', rank - 1), "]");
                    else
                    {
                        Debug.Assert(rank == 1);
                        rankStr = "[]";
                    }
                    rankList.Add(rankStr);
                }
                Debug.Assert(t != null);

                var fullRank = string.Concat(rankList.ToArray());
                return GetCsTypeName(t) + fullRank;
            }

            if (!type.IsGenericType)
                return type.FullName;

            var genericDefinition = type.GetGenericTypeDefinition();
            Debug.Assert(genericDefinition != null);

            var idx = genericDefinition.FullName.IndexOf(GenericNameSeparator);
            Debug.Assert(idx > 0);
            var name = genericDefinition.FullName.Substring(0, idx);

            var genericNames = type.GetGenericArguments().Select<Type, string>(GetCsTypeName).ToArray();

            var result = string.Concat(name, "<", string.Join(", ", genericNames), ">");
            return result;
        }

        public static string GetCsTypeName<T>()
        {
            return GetCsTypeName(typeof (T));
        }
    }
}
