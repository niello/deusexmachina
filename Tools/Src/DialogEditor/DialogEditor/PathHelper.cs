using System;
using System.Text;

namespace DialogDesigner
{
    public static class PathHelper
    {
        public static string GetRelativePath(string baseDirectory, string fullPath)
        {
            if(baseDirectory==null)
                throw new ArgumentNullException("baseDirectory");
            if (fullPath == null)
                throw new ArgumentNullException("fullPath");

            var targetPath = fullPath.Split('\\');
            var basePath = baseDirectory.Split('\\');

            int minLen = Math.Min(targetPath.Length, basePath.Length);
            int i;
            for (i = 0; i < minLen; i++)
            {
                if (!string.Equals(targetPath[i], basePath[i], StringComparison.OrdinalIgnoreCase))
                {
                    if(i==0)
                        return fullPath;

                    break;
                }
            }

            StringBuilder relPath = new StringBuilder();
            for (int j = i; j < basePath.Length; j++)
            {
                if (j != i)
                    relPath.Append('\\');
                relPath.Append("..");
            }

            for (int j = i; j < targetPath.Length; j++)
            {
                if (relPath.Length > 0)
                    relPath.Append('\\');
                relPath.Append(targetPath[j]);
            }

            if (relPath.Length == 0)
                relPath.Append('.');

            return relPath.ToString();
        }
    }
}
